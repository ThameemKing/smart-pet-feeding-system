#include <WiFi.h>

#include <WebServer.h>

#include <time.h>

#include <vector>

using namespace std;

// ===== WiFi Credentials =====

const char *ssid = "";       // <-- Change this

const char *password = "";    // <-- Change this

// ===== Time Zone (IST +5:30) =====

const char *ntpServer = "pool.ntp.org";

const long gmtOffset_sec = 19800;

const int daylightOffset_sec = 0;

// ===== Stepper Motor Pins =====

#define IN1 14

#define IN2 16

#define IN3 27

#define IN4 26

int stepDelay = 3;  // Motor speed (ms between steps)

int totalGramsFed = 0;

int feedCount = 0;

// ===== Feed Request Control =====

bool feedRequestPending = false;

int requestedGrams = 0;

// ===== Feed Scheduling =====

struct FeedSchedule {

  int hour;

  int minute;

  int grams;

};

vector<FeedSchedule> schedules;

// ===== Web Server =====

WebServer server(80);

// ===== Function Prototypes =====

void feedPet(int grams);

void stepMotor(int steps, bool clockwise);

void checkSchedules();

// ===== HTML Page (served by ESP32) =====

const char webpage[] PROGMEM = R"=====(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>🐾 Smart Pet Feeder</title>
<style>

body {

  font-family: Arial, sans-serif;

  background: linear-gradient(120deg,#a2d9ff,#d1f4a5);

  text-align:center; padding:20px; color:#333;

}

h1{color:#0066cc;}

button{

  background-color:#007bff;color:white;border:none;

  padding:12px 24px;border-radius:8px;margin:10px;

  cursor:pointer;font-size:16px;

}

button:hover{background-color:#0056b3;}

input{padding:10px;border-radius:6px;border:1px solid #ccc;

  margin:8px;width:100px;}

#status{background:#fff;padding:10px;border-radius:8px;

  margin:20px auto;width:80%;text-align:left;}

img{max-width:200px;border-radius:12px;margin:10px;}
</style>
</head>
<body>
<h1>🐶 Smart Pet Feeder</h1>
<!-- Image Section -->
<input type="file" id="imageUpload" accept="image/*"><br>
<img id="preview" src="" alt="Pet Preview"/><br>
<button id="detectBtn">Detect Breed</button>
<p><b>Detected Pet:</b> <span id="detectedPet">None</span></p>
<p><b>Suggested Feed:</b> <span id="suggestedAmount">0g</span></p>
<hr>
<h3>Manual Feeding</h3>
<input type="number" id="feedAmount" placeholder="grams" />
<button id="feedBtn">Feed Now</button>
<hr>
<h3>Schedule Feeding</h3>
<input type="number" id="hour" placeholder="Hour(0-23)" />
<input type="number" id="minute" placeholder="Minute(0-59)" />
<input type="number" id="scheduleAmount" placeholder="grams" />
<button id="scheduleBtn">Add Schedule</button>
<hr>
<h3>System Status</h3>
<div id="status">Loading...</div>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs@3.18.0"></script>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow-models/mobilenet"></script>
<script>

let model;

async function loadModel(){

  model = await mobilenet.load();

  console.log("✅ Model loaded");

}

loadModel();

const imageUpload=document.getElementById("imageUpload");

const preview=document.getElementById("preview");

imageUpload.addEventListener("change",e=>{

  const file=e.target.files[0];

  if(file) preview.src=URL.createObjectURL(file);

});

// ===== Detect Pet =====

document.getElementById("detectBtn").addEventListener("click",async()=>{

  if(!model || !preview.src) return alert("Upload an image first!");

  const img=document.createElement("img");

  img.src=preview.src;

  const preds=await model.classify(img);

  const best=preds[0];

  document.getElementById("detectedPet").textContent=

    `${best.className.toUpperCase()} (${(best.probability*100).toFixed(1)}%)`;

  let grams=100;

  if(best.className.toLowerCase().includes("dog")) grams=120;

  else if(best.className.toLowerCase().includes("cat")) grams=60;

  document.getElementById("suggestedAmount").textContent=`${grams}g`;

});

// ===== Manual Feed =====

document.getElementById("feedBtn").addEventListener("click",async()=>{

  const g=parseInt(document.getElementById("feedAmount").value);

  if(isNaN(g)||g<=0) return alert("Enter valid grams!");

  if(!confirm(`Feed ${g}g now?`)) return;

  const res=await fetch(`/feed?amount=${g}`);

  alert(await res.text());

  loadStatus();

});

// ===== Add Schedule =====

document.getElementById("scheduleBtn").addEventListener("click",async()=>{

  const h=parseInt(document.getElementById("hour").value);

  const m=parseInt(document.getElementById("minute").value);

  const g=parseInt(document.getElementById("scheduleAmount").value);

  if(isNaN(h)||isNaN(m)||isNaN(g)) return alert("Enter all values!");

  const res=await fetch(`/addSchedule?hour=${h}&minute=${m}&grams=${g}`);

  alert(await res.text());

  loadStatus();

});

// ===== Load Status =====

async function loadStatus(){

  const res=await fetch("/status");

  document.getElementById("status").innerText=await res.text();

}

setInterval(loadStatus,5000);

loadStatus();
</script>
</body>
</html>)=====";

// ===== Stepper Motor Function =====

void stepMotor(int steps, bool clockwise) {

  int seq[4][4] = {

    {1, 0, 0, 1},

    {1, 0, 1, 0},

    {0, 1, 1, 0},

    {0, 1, 0, 1}};

  for (int s = 0; s < steps; s++) {

    int i = clockwise ? s % 4 : 3 - (s % 4);

    digitalWrite(IN1, seq[i][0]);

    digitalWrite(IN2, seq[i][1]);

    digitalWrite(IN3, seq[i][2]);

    digitalWrite(IN4, seq[i][3]);

    delay(stepDelay);

  }

  digitalWrite(IN1, LOW);

  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);

  digitalWrite(IN4, LOW);

}

// ===== Feed Function =====

void feedPet(int grams) {

  int steps = grams * 10;  // Calibrate for your motor

  Serial.printf("🐾 Feeding %dg...\n", grams);

  stepMotor(steps, true);

  totalGramsFed += grams;

  feedCount++;

  Serial.printf("✅ Done! Total fed: %dg | Feeds today: %d\n", totalGramsFed, feedCount);

}

// ===== Schedule Check =====

void checkSchedules() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) return;

  int h = timeinfo.tm_hour;

  int m = timeinfo.tm_min;

  static int lastMinute = -1;

  if (m == lastMinute) return;

  lastMinute = m;

  for (auto &s : schedules) {

    if (s.hour == h && s.minute == m) {

      Serial.printf("⏰ Scheduled feed triggered: %dg at %02d:%02d\n", s.grams, s.hour, s.minute);

      requestedGrams = s.grams;

      feedRequestPending = true;

    }

  }

}

// ===== Web Handlers =====

void handleRoot() { server.send(200, "text/html", webpage); }

void handleFeed() {

  if (!server.hasArg("amount")) {

    server.send(400, "text/plain", "Missing amount parameter");

    return;

  }

  requestedGrams = server.arg("amount").toInt();

  feedRequestPending = true;

  Serial.printf("🌐 Feed command received: %d grams\n", requestedGrams);

  server.send(200, "text/plain", "Feeding command received! Dispensing now...");

}

void handleAddSchedule() {

  if (!server.hasArg("hour") || !server.hasArg("minute") || !server.hasArg("grams")) {

    server.send(400, "text/plain", "Missing parameters");

    return;

  }

  FeedSchedule fs;

  fs.hour = server.arg("hour").toInt();

  fs.minute = server.arg("minute").toInt();

  fs.grams = server.arg("grams").toInt();

  schedules.push_back(fs);

  server.send(200, "text/plain", "✅ Schedule added successfully!");

  Serial.printf("📅 Schedule: %02d:%02d for %dg\n", fs.hour, fs.minute, fs.grams);

}

void handleStatus() {

  String status = "📋 SYSTEM STATUS\n";

  status += "Feeds today: " + String(feedCount) + "\n";

  status += "Total grams fed: " + String(totalGramsFed) + "g\n";

  status += "Schedules:\n";

  for (auto &s : schedules)

    status += " - " + String(s.hour) + ":" + String(s.minute) + " -> " + String(s.grams) + "g\n";

  server.send(200, "text/plain", status);

}

// ===== Setup =====

void setup() {

  Serial.begin(115200);

  pinMode(IN1, OUTPUT);

  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);

  pinMode(IN4, OUTPUT);

  WiFi.begin(ssid, password);

  Serial.print("🌐 Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");

  }

  Serial.println("\n✅ Connected!");

  Serial.print("IP Address: ");

  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println("🕒 Time sync complete");

  server.on("/", handleRoot);

  server.on("/feed", handleFeed);

  server.on("/addSchedule", handleAddSchedule);

  server.on("/status", handleStatus);

  server.begin();

  Serial.println("🚀 Web server started");

}

// ===== Loop =====

void loop() {

  server.handleClient();

  checkSchedules();

  if (feedRequestPending) {

    feedPet(requestedGrams);

    feedRequestPending = false;

  }

}
 
