#include "wifi_server.h"
#include "config.h"
#include "security.h"
#include "sensors.h"
#include "alarm.h"
#include "gps.h"
#include "battery.h"
#include "events.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

static ESP8266WebServer server(HTTP_SERVER_PORT);
static unsigned long lastWifiCheckMs = 0;
static uint8_t connectedStationCount = 0;

// Embedded HTML/JS/CSS Web Dashboard for instant browser demonstration
static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no">
<title>SecurePack Dashboard</title>
<style>
:root{--bg:#0b1329;--card:#16223f;--card-border:#253761;--text:#f8fafc;--muted:#94a3b8;--blue:#3b82f6;--green:#10b981;--red:#ef4444;--orange:#f59e0b}
*{box-sizing:border-box;margin:0;padding:0;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif}
body{background:var(--bg);color:var(--text);padding:16px;min-height:100vh;display:flex;flex-direction:column;align-items:center}
.container{width:100%;max-width:480px}
header{display:flex;align-items:center;justify-content:space-between;padding:12px 0 20px}
.brand{display:flex;align-items:center;gap:10px}
.shield-icon{font-size:28px}
h1{font-size:20px;font-weight:700;letter-spacing:0.5px}
.subtitle{font-size:11px;color:var(--muted);text-transform:uppercase;letter-spacing:1px}
.wifi-pill{font-size:12px;padding:4px 10px;border-radius:20px;background:rgba(16,185,129,0.15);color:var(--green);display:flex;align-items:center;gap:6px;font-weight:640}
.wifi-dot{width:8px;height:8px;border-radius:50%;background:var(--green);box-shadow:0 0 8px var(--green)}
.alert-banner{background:rgba(239,68,68,0.2);border:1px solid var(--red);border-radius:14px;padding:14px;margin-bottom:16px;display:none;animation:pulse 1s infinite alternate}
@keyframes pulse{from{opacity:0.85;transform:scale(0.99)}to{opacity:1;transform:scale(1)}}
.alert-title{color:var(--red);font-weight:700;font-size:15px;display:flex;align-items:center;gap:8px}
.alert-desc{color:#fca5a5;font-size:13px;margin:4px 0 10px}
.status-card{background:var(--card);border:1px solid var(--card-border);border-radius:22px;padding:26px 20px;text-align:center;box-shadow:0 12px 30px rgba(0,0,0,0.35);margin-bottom:16px;position:relative;overflow:hidden}
.status-icon{font-size:68px;margin-bottom:10px;transition:all .3s}
.status-title{font-size:24px;font-weight:800;letter-spacing:1px;margin-bottom:4px}
.status-sub{font-size:13px;color:var(--muted)}
.btn-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px}
.btn{border:none;border-radius:14px;padding:15px 12px;font-size:14px;font-weight:700;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:8px;transition:all .2s;text-transform:uppercase;letter-spacing:0.5px}
.btn:active{transform:scale(0.97)}
.btn-arm{background:linear-gradient(135deg,#2563eb,#1d4ed8);color:#fff}
.btn-disarm{background:linear-gradient(135deg,#059669,#047857);color:#fff}
.btn-find{grid-column:span 2;background:#1e293b;border:1px solid var(--card-border);color:var(--text)}
.btn-stop{background:var(--red);color:#fff;width:100%;margin-top:6px}
.cards-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px}
.mini-card{background:var(--card);border:1px solid var(--card-border);border-radius:16px;padding:14px;display:flex;flex-direction:column;gap:4px}
.card-label{font-size:12px;color:var(--muted);text-transform:uppercase;letter-spacing:0.5px}
.card-val{font-size:17px;font-weight:700}
.gps-card{grid-column:span 2}
.gps-actions{margin-top:8px;display:flex;gap:8px}
.btn-map{background:rgba(59,130,246,0.15);color:var(--blue);border:1px solid rgba(59,130,246,0.3);padding:8px 14px;border-radius:10px;font-size:12px;font-weight:700;text-decoration:none;display:inline-flex;align-items:center;gap:6px}
.sens-select{background:var(--bg);color:var(--text);border:1px solid var(--card-border);border-radius:8px;padding:6px 10px;font-size:13px;width:100%;margin-top:4px}
.events-section{background:var(--card);border:1px solid var(--card-border);border-radius:18px;padding:16px;margin-bottom:24px}
.events-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px}
.events-title{font-size:14px;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;color:var(--muted)}
.events-clear{background:none;border:none;color:var(--blue);font-size:12px;cursor:pointer;font-weight:600}
.event-list{max-height:220px;overflow-y:auto;display:flex;flex-direction:column;gap:8px}
.event-item{display:flex;justify-content:space-between;align-items:center;padding:10px 12px;background:rgba(255,255,255,0.03);border-radius:10px;font-size:13px}
.event-msg{font-weight:500}
.event-time{font-size:11px;color:var(--muted)}
.modal-overlay{position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,0.75);display:none;align-items:center;justify-content:center;z-index:99;padding:16px}
.modal{background:#1e293b;border:1px solid var(--card-border);border-radius:20px;padding:24px;width:100%;max-width:340px;text-align:center}
.modal h3{margin-bottom:8px;font-size:18px}
.modal p{font-size:13px;color:var(--muted);margin-bottom:16px}
.pin-input{width:100%;background:var(--bg);border:2px solid var(--card-border);border-radius:12px;padding:14px;color:#fff;font-size:24px;text-align:center;letter-spacing:8px;font-weight:700;outline:none;margin-bottom:16px}
.pin-input:focus{border-color:var(--blue)}
.modal-btns{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.btn-cancel{background:rgba(255,255,255,0.1);color:#fff}
.btn-confirm{background:var(--blue);color:#fff}
</style>
</head>
<body>
<div class="container">
  <header>
    <div class="brand">
      <div class="shield-icon">🛡️</div>
      <div>
        <h1>SECUREPACK</h1>
        <div class="subtitle">Smart Anti-Theft Bag System</div>
      </div>
    </div>
    <div class="wifi-pill"><div class="wifi-dot"></div><span>Connected</span></div>
  </header>

  <div id="alertBanner" class="alert-banner">
    <div class="alert-title"><span>⚠️</span><span id="alertTitleText">SECURITY ALERT</span></div>
    <div id="alertDescText" class="alert-desc">Unauthorized movement detected!</div>
    <div style="display:flex;gap:8px">
      <button class="btn btn-stop" onclick="openPinModal('stop-alarm')">STOP ALARM</button>
      <a id="alertMapBtn" class="btn-map" href="#" target="_blank" style="display:none;background:#fff;color:var(--red);align-items:center;justify-content:center">📍 LOCATION</a>
    </div>
  </div>

  <div class="status-card">
    <div id="statusIcon" class="status-icon">🔓</div>
    <div id="statusTitle" class="status-title" style="color:var(--green)">DISARMED</div>
    <div id="statusDesc" class="status-sub">Your bag is safe and unmonitored</div>
  </div>

  <div class="btn-grid">
    <button class="btn btn-arm" onclick="openPinModal('arm')">🔒 ARM SYSTEM</button>
    <button class="btn btn-disarm" onclick="openPinModal('disarm')">🔓 DISARM</button>
    <button class="btn btn-find" onclick="openPinModal('find')">🔊 FIND MY BAG</button>
  </div>

  <div class="cards-grid">
    <div class="mini-card">
      <div class="card-label">Battery Level</div>
      <div id="cardBattery" class="card-val">85%</div>
      <div id="cardBatteryState" style="font-size:11px;color:var(--green)">HIGH</div>
    </div>
    <div class="mini-card">
      <div class="card-label">Sensitivity</div>
      <select id="sensSelect" class="sens-select" onchange="changeSensitivity(this.value)">
        <option value="LOW">LOW</option>
        <option value="MEDIUM" selected>MEDIUM</option>
        <option value="HIGH">HIGH</option>
      </select>
    </div>
    <div class="mini-card">
      <div class="card-label">Zipper (Reed)</div>
      <div id="cardReed" class="card-val" style="color:var(--green)">CLOSED</div>
    </div>
    <div class="mini-card">
      <div class="card-label">Wi-Fi Device</div>
      <div class="card-val">ESP8266</div>
      <div style="font-size:11px;color:var(--muted)">192.168.4.1</div>
    </div>
    <div class="mini-card gps-card">
      <div class="card-label">GPS Status</div>
      <div id="cardGpsStatus" class="card-val" style="font-size:15px;margin-top:2px">CHECKING...</div>
      <div id="cardGpsCoords" style="font-size:12px;color:var(--muted);margin-top:4px">Lat: -- , Lng: --</div>
      <div class="gps-actions">
        <a id="btnGpsMap" class="btn-map" href="#" target="_blank" style="display:none">🗺️ OPEN IN GOOGLE MAPS</a>
      </div>
    </div>
  </div>

  <div class="events-section">
    <div class="events-header">
      <div class="events-title">Recent Event History</div>
      <button class="events-clear" onclick="clearEvents()">CLEAR</button>
    </div>
    <div id="eventList" class="event-list">
      <div class="event-item"><div>System booting...</div><div class="event-time">--:--</div></div>
    </div>
  </div>
</div>

<div id="pinModal" class="modal-overlay">
  <div class="modal">
    <h3 id="modalTitle">Enter 4-Digit PIN</h3>
    <p>Authentication required for bag security</p>
    <input type="password" id="pinInput" class="pin-input" maxlength="4" placeholder="••••" inputmode="numeric">
    <div class="modal-btns">
      <button class="btn btn-cancel" onclick="closePinModal()">Cancel</button>
      <button class="btn btn-confirm" onclick="submitPin()">Authorize</button>
    </div>
  </div>
</div>

<script>
let currentAction = '';
let activeGpsUrl = '';

function openPinModal(action) {
  currentAction = action;
  const titles = {'arm':'Arm SecurePack','disarm':'Disarm SecurePack','find':'Find My Bag','stop-alarm':'Silence Alarm'};
  document.getElementById('modalTitle').innerText = titles[action] || 'Security PIN';
  document.getElementById('pinInput').value = '';
  document.getElementById('pinModal').style.display = 'flex';
  document.getElementById('pinInput').focus();
}

function closePinModal() {
  document.getElementById('pinModal').style.display = 'none';
  currentAction = '';
}

function submitPin() {
  const pin = document.getElementById('pinInput').value;
  if (!pin || pin.length < 4) {
    alert('Please enter a 4-digit PIN');
    return;
  }
  const action = currentAction;
  closePinModal();
  fetch('/' + action, {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({pin: pin})
  })
  .then(r => r.json())
  .then(data => {
    if (!data.success) {
      alert('Authentication Failed: ' + (data.error || 'Wrong PIN'));
    }
    fetchStatus();
    fetchEvents();
  })
  .catch(e => {
    alert('Network error connecting to ESP8266: ' + e);
  });
}

function changeSensitivity(level) {
  fetch('/sensitivity', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({sensitivity: level})
  }).then(r => r.json()).then(() => fetchStatus());
}

function clearEvents() {
  if (confirm('Clear event history?')) {
    fetch('/clear-events', {method:'POST'}).then(() => fetchEvents());
  }
}

function fetchStatus() {
  fetch('/status')
    .then(r => r.json())
    .then(data => {
      const isAlarm = data.alarm;
      const state = data.state;
      const cause = data.alarmCause;

      const titleEl = document.getElementById('statusTitle');
      const iconEl = document.getElementById('statusIcon');
      const descEl = document.getElementById('statusDesc');
      const alertEl = document.getElementById('alertBanner');

      if (isAlarm) {
        titleEl.innerText = '🔴 ALARM TRIGGERED';
        titleEl.style.color = 'var(--red)';
        iconEl.innerText = '🚨';
        descEl.innerText = 'Unauthorized tampering or movement detected!';
        alertEl.style.display = 'block';
        if (cause === 'ZIPPER') {
          document.getElementById('alertTitleText').innerText = 'ZIPPER OPENED!';
          document.getElementById('alertDescText').innerText = 'Your bag zipper has been opened while armed!';
        } else {
          document.getElementById('alertTitleText').innerText = 'SECURITY ALERT';
          document.getElementById('alertDescText').innerText = 'Unauthorized bag movement detected!';
        }
      } else if (state === 'WARNING') {
        titleEl.innerText = '⚠️ WARNING';
        titleEl.style.color = 'var(--orange)';
        iconEl.innerText = '⚠️';
        descEl.innerText = 'Bag motion detected. Confirming security state...';
        alertEl.style.display = 'block';
        document.getElementById('alertTitleText').innerText = 'MOTION DETECTED';
        document.getElementById('alertDescText').innerText = 'Bag moved. Confirming authorization...';
      } else if (state === 'FIND_MY_BAG') {
        titleEl.innerText = '🔊 FIND MY BAG';
        titleEl.style.color = 'var(--blue)';
        iconEl.innerText = '📡';
        descEl.innerText = 'Buzzer sounding nearby search pattern...';
        alertEl.style.display = 'none';
      } else if (state === 'ARMED') {
        titleEl.innerText = '🔒 ARMED';
        titleEl.style.color = 'var(--blue)';
        iconEl.innerText = '🛡️';
        descEl.innerText = 'Sensors active. Zipper & motion monitored.';
        alertEl.style.display = 'none';
      } else {
        titleEl.innerText = '🔓 DISARMED';
        titleEl.style.color = 'var(--green)';
        iconEl.innerText = '🔓';
        descEl.innerText = 'Bag is safe and unmonitored.';
        alertEl.style.display = 'none';
      }

      document.getElementById('cardBattery').innerText = data.battery + '%';
      document.getElementById('cardBatteryState').innerText = data.batteryState;
      
      const reedEl = document.getElementById('cardReed');
      if (data.reedOpen) {
        reedEl.innerText = 'OPENED ⚠️';
        reedEl.style.color = 'var(--red)';
      } else {
        reedEl.innerText = 'CLOSED ✅';
        reedEl.style.color = 'var(--green)';
      }

      document.getElementById('sensSelect').value = data.sensitivity;
    })
    .catch(() => {});
}

function fetchGps() {
  fetch('/gps')
    .then(r => r.json())
    .then(data => {
      const statusEl = document.getElementById('cardGpsStatus');
      const coordsEl = document.getElementById('cardGpsCoords');
      const mapBtn = document.getElementById('btnGpsMap');
      const alertMapBtn = document.getElementById('alertMapBtn');

      if (data.fix) {
        statusEl.innerText = 'FIX AVAILABLE ✅';
        statusEl.style.color = 'var(--green)';
        coordsEl.innerText = 'Lat: ' + data.lat.toFixed(5) + ', Lng: ' + data.lng.toFixed(5) + ' (' + data.satellites + ' sats)';
        mapBtn.href = data.mapUrl;
        mapBtn.style.display = 'inline-flex';
        alertMapBtn.href = data.mapUrl;
        alertMapBtn.style.display = 'inline-flex';
      } else {
        statusEl.innerText = 'NO FIX (Searching...)';
        statusEl.style.color = 'var(--orange)';
        coordsEl.innerText = 'Waiting for satellite signal...';
        mapBtn.style.display = 'none';
        alertMapBtn.style.display = 'none';
      }
    })
    .catch(() => {});
}

function fetchEvents() {
  fetch('/events')
    .then(r => r.json())
    .then(events => {
      const listEl = document.getElementById('eventList');
      if (!events || events.length === 0) {
        listEl.innerHTML = '<div class="event-item"><div>No events recorded</div></div>';
        return;
      }
      let html = '';
      events.forEach(e => {
        let icon = '⚪';
        if (e.type.includes('ARMED')) icon = '🟢';
        else if (e.type.includes('DISARM')) icon = '🔓';
        else if (e.type.includes('ALERT') || e.type.includes('ALARM')) icon = '🔴';
        else if (e.type.includes('WARNING') || e.type.includes('MOTION')) icon = '⚠️';
        else if (e.type.includes('FIND')) icon = '🔊';

        html += '<div class="event-item">';
        html += '<div class="event-msg">' + icon + ' ' + e.message + '</div>';
        html += '<div class="event-time">' + e.timestamp + '</div>';
        html += '</div>';
      });
      listEl.innerHTML = html;
    })
    .catch(() => {});
}

// Initial fetch and poll intervals
fetchStatus();
fetchGps();
fetchEvents();
setInterval(fetchStatus, 1500);
setInterval(fetchGps, 4000);
setInterval(fetchEvents, 3000);
</script>
</body>
</html>
)rawliteral";

// Helper to set CORS headers on all HTTP responses
static void setCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
}

static void handleOptions() {
  setCorsHeaders();
  server.send(204);
}

// Helper to extract PIN from JSON body or URL parameters
static String extractPinFromRequest() {
  if (server.hasArg("pin")) {
    return server.arg("pin");
  }
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int pinIdx = body.indexOf("\"pin\"");
    if (pinIdx != -1) {
      int colonIdx = body.indexOf(':', pinIdx);
      if (colonIdx != -1) {
        int quoteStart = body.indexOf('\"', colonIdx);
        if (quoteStart != -1) {
          int quoteEnd = body.indexOf('\"', quoteStart + 1);
          if (quoteEnd != -1) {
            return body.substring(quoteStart + 1, quoteEnd);
          }
        } else {
          int commaIdx = body.indexOf(',', colonIdx + 1);
          int braceIdx = body.indexOf('}', colonIdx + 1);
          int endIdx = -1;
          if (commaIdx != -1 && braceIdx != -1) {
            endIdx = (commaIdx < braceIdx) ? commaIdx : braceIdx;
          } else if (commaIdx != -1) {
            endIdx = commaIdx;
          } else {
            endIdx = braceIdx;
          }
          if (endIdx == -1) endIdx = body.length();
          String p = body.substring(colonIdx + 1, endIdx);
          p.trim();
          return p;
        }
      }
    }
  }
  return "";
}

// Endpoint Handlers
static void handleRoot() {
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

static void handleGetStatus() {
  setCorsHeaders();
  server.send(200, "application/json", securityGetStatusJson());
}

static void handleGetGps() {
  setCorsHeaders();
  server.send(200, "application/json", gpsGetJson());
}

static void handleGetBattery() {
  setCorsHeaders();
  server.send(200, "application/json", batteryGetJson());
}

static void handleGetEvents() {
  setCorsHeaders();
  server.send(200, "application/json", eventsGetJson());
}

static void handlePostArm() {
  setCorsHeaders();
  String pin = extractPinFromRequest();
  if (securityArm(pin)) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"System Armed\"}");
  } else {
    server.send(401, "application/json", "{\"success\":false,\"error\":\"Invalid PIN\"}");
  }
}

static void handlePostDisarm() {
  setCorsHeaders();
  String pin = extractPinFromRequest();
  if (securityDisarm(pin)) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"System Disarmed\"}");
  } else {
    server.send(401, "application/json", "{\"success\":false,\"error\":\"Invalid PIN\"}");
  }
}

static void handlePostFind() {
  setCorsHeaders();
  String pin = extractPinFromRequest();
  if (securityFindMyBag(pin)) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Find My Bag Activated\"}");
  } else {
    server.send(401, "application/json", "{\"success\":false,\"error\":\"Invalid PIN\"}");
  }
}

static void handlePostStopAlarm() {
  setCorsHeaders();
  String pin = extractPinFromRequest();
  if (securityStopAlarm(pin)) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Alarm Stopped\"}");
  } else {
    server.send(401, "application/json", "{\"success\":false,\"error\":\"Invalid PIN\"}");
  }
}

static void handlePostSensitivity() {
  setCorsHeaders();
  String level = "";
  if (server.hasArg("sensitivity")) {
    level = server.arg("sensitivity");
  } else if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int idx = body.indexOf("\"sensitivity\"");
    if (idx != -1) {
      int colon = body.indexOf(':', idx);
      if (colon != -1) {
        int q1 = body.indexOf('\"', colon);
        if (q1 != -1) {
          int q2 = body.indexOf('\"', q1 + 1);
          if (q2 != -1) level = body.substring(q1 + 1, q2);
        }
      }
    }
  }

  level.toUpperCase();
  if (level == "LOW") {
    sensorsSetSensitivity(SENSITIVITY_LOW);
  } else if (level == "HIGH") {
    sensorsSetSensitivity(SENSITIVITY_HIGH);
  } else {
    sensorsSetSensitivity(SENSITIVITY_MEDIUM);
  }

  server.send(200, "application/json", "{\"success\":true,\"sensitivity\":\"" + String(sensorsGetSensitivityString()) + "\"}");
}

static void handlePostClearEvents() {
  setCorsHeaders();
  eventsClear();
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Event history cleared\"}");
}

static void handlePostChangePin() {
  setCorsHeaders();
  String oldPin = "";
  String newPin = "";

  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int idx1 = body.indexOf("\"oldPin\"");
    if (idx1 != -1) {
      int c1 = body.indexOf(':', idx1);
      int q1 = body.indexOf('\"', c1);
      int q2 = body.indexOf('\"', q1 + 1);
      if (q1 != -1 && q2 != -1) oldPin = body.substring(q1 + 1, q2);
    }
    int idx2 = body.indexOf("\"newPin\"");
    if (idx2 != -1) {
      int c2 = body.indexOf(':', idx2);
      int q3 = body.indexOf('\"', c2);
      int q4 = body.indexOf('\"', q3 + 1);
      if (q3 != -1 && q4 != -1) newPin = body.substring(q3 + 1, q4);
    }
  }

  if (securityChangePin(oldPin, newPin)) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"PIN changed successfully\"}");
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Failed to change PIN. Verify old PIN and ensure new PIN is 4 digits.\"}");
  }
}

void wifiServerInit() {
  WiFi.mode(WIFI_AP);
  
  IPAddress localIp(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(localIp, gateway, subnet);
  
  bool apStarted = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, WIFI_AP_HIDDEN, WIFI_AP_MAX_CONN);

  if (apStarted) {
    Serial.println(F("[WIFI] Access Point started successfully!"));
    Serial.print(F("[WIFI] SSID: "));
    Serial.println(WIFI_AP_SSID);
    Serial.print(F("[WIFI] Password: "));
    Serial.println(WIFI_AP_PASSWORD);
    Serial.print(F("[WIFI] IP Address: "));
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println(F("[WIFI] ERROR: Failed to start Access Point."));
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/gps", HTTP_GET, handleGetGps);
  server.on("/battery", HTTP_GET, handleGetBattery);
  server.on("/events", HTTP_GET, handleGetEvents);

  server.on("/arm", HTTP_POST, handlePostArm);
  server.on("/disarm", HTTP_POST, handlePostDisarm);
  server.on("/find", HTTP_POST, handlePostFind);
  server.on("/stop-alarm", HTTP_POST, handlePostStopAlarm);
  server.on("/sensitivity", HTTP_POST, handlePostSensitivity);
  server.on("/clear-events", HTTP_POST, handlePostClearEvents);
  server.on("/change-pin", HTTP_POST, handlePostChangePin);

  server.on("/status", HTTP_OPTIONS, handleOptions);
  server.on("/gps", HTTP_OPTIONS, handleOptions);
  server.on("/battery", HTTP_OPTIONS, handleOptions);
  server.on("/events", HTTP_OPTIONS, handleOptions);
  server.on("/arm", HTTP_OPTIONS, handleOptions);
  server.on("/disarm", HTTP_OPTIONS, handleOptions);
  server.on("/find", HTTP_OPTIONS, handleOptions);
  server.on("/stop-alarm", HTTP_OPTIONS, handleOptions);
  server.on("/sensitivity", HTTP_OPTIONS, handleOptions);
  server.on("/clear-events", HTTP_OPTIONS, handleOptions);
  server.on("/change-pin", HTTP_OPTIONS, handleOptions);

  server.begin();
  Serial.println(F("[WIFI] HTTP REST Server started on port 80."));

  alarmSetBlueLed(true);
}

void wifiServerUpdate() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastWifiCheckMs >= 1000) {
    lastWifiCheckMs = now;
    connectedStationCount = WiFi.softAPgetStationNum();
    alarmSetBlueLed(true);
  }
}

uint8_t wifiGetConnectedClients() {
  return connectedStationCount;
}

String wifiGetIpAddress() {
  return WiFi.softAPIP().toString();
}