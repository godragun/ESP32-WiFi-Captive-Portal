# ESP32 WiFi Captive Portal — Google Sign-In Phishing

> **Disclaimer:** This project is strictly for educational and authorized penetration testing purposes only. Use it only on your own networks and devices. Unauthorized use against others is illegal. The author takes no responsibility for misuse.

---

## What This Is

This is a **WiFi captive portal** running on an **ESP32 Dev Module** that impersonates a Google sign-in page. When a victim connects to the rogue access point and opens any website, they are intercepted and shown a fake Google login form that captures their email and password.

No internet connection is required. The ESP32 handles everything on-device using DNS spoofing and a built-in web server.

---

## Screenshots

### 1. Google Sign-In Page
> What the victim sees when they connect to "Free WiFi" and open any website.

![Google Sign-In Page](images/google-signin.png)

---

### 2. After Submit
> Shown after the victim enters their email and password and hits Next.

![After Submit](images/after-submit.png)

---

### 3. Captured Credentials
> Attacker visits `172.0.0.1/pass` to view all stolen email and password pairs.

![Captured Credentials](images/captured-credentials.png)

---

### 4. Change SSID
> Attacker visits `172.0.0.1/ssid` to rename the access point on the fly.

![Change SSID](images/change-ssid.png)

---

### 5. Cleared
> Confirmation page shown after wiping all saved credentials at `172.0.0.1/clear`.

![Cleared](images/cleared.png)

---

## How It Works

1. The ESP32 broadcasts a WiFi hotspot named **"Free WiFi"**
2. When a victim connects and opens any URL (e.g. `google.com`), the ESP32's DNS server intercepts the request and redirects it to itself (`172.0.0.1`)
3. The victim sees a **convincing Google Sign-In page** asking for their email and password
4. On submission, credentials are saved to **EEPROM** (persists after power off)
5. The built-in LED blinks 5 times to alert the attacker that credentials were captured
6. The victim is shown a fake "Signing in... please wait" message

---

## Modifications Made

This project was originally written for the **ESP8266** by [125K](https://github.com/125K). The following changes were made:

| Change | Details |
|---|---|
| **Ported to ESP32** | Replaced `ESP8266WiFi.h` + `ESP8266WebServer.h` with `WiFi.h` + `WebServer.h` |
| **Fixed BUILTIN_LED** | Explicitly defined LED pin as `2` for ESP32 DevKit |
| **New Google UI** | Replaced the old basic HTML with a pixel-accurate Google Sign-In page |
| **Captures email + password** | Previously only captured WiFi password; now captures Google email and password |
| **Better post-submit page** | Victim sees "Signing in... You will be connected to Free WiFi shortly" instead of a suspicious message |
| **Updated EEPROM storage** | Stores credentials as `Email: x \| Pass: y` entries |

---

## Attacker Control Panel

While connected to the ESP32's WiFi, open a browser and go to:

| URL | Function |
|---|---|
| `172.0.0.1/pass` | View all captured email + password credentials |
| `172.0.0.1/clear` | Wipe all saved credentials from EEPROM |
| `172.0.0.1/ssid` | Change the AP broadcast name on the fly |

---

## Hardware Required

- ESP32 Dev Module (any standard 38-pin or 30-pin DevKit)

---

## Setup (Arduino IDE)

1. Open Arduino IDE and go to **File → Preferences**
2. Add this to "Additional boards manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools → Board → Boards Manager**, search `esp32`, install **"esp32 by Espressif Systems"**
4. Select **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
5. Open `WiFi_Captive_Portal/WiFi_Captive_Portal.ino`
6. Upload to your board
7. Connect to the **"Free WiFi"** network and open any browser
