# Smart Attendance System using RFID, ESP8266 & Google Sheets

An IoT attendance system that identifies people by their RFID tag's unique ID (UID), looks up their name in a Google Sheet, and logs attendance automatically — with LCD feedback and duplicate-scan protection.

## Features

- RFID UID-based identification (no writing to tags required)
- Wi-Fi with connection timeout and automatic reconnect
- Debounced scanning (prevents double-logging from a lingering tap)
- API-key-authenticated requests to the backend
- Server-side duplicate-attendance prevention (once per day per person)
- 16x2 I2C LCD feedback: recognized / unknown / duplicate / network error states
- Buzzer feedback patterns per outcome

## Hardware Required

| Component | Notes |
|---|---|
| ESP8266 (NodeMCU) | Main microcontroller |
| MFRC522 RFID Reader Module | Reads tag UID only — no key auth needed |
| Any MIFARE RFID Card/Tag | UID is already unique from the factory |
| 16x2 I2C LCD Display | Address `0x3F` or `0x27` |
| Active Buzzer | Scan confirmation |
| Jumper wires, breadboard | Connections |

### Wiring (ESP8266 to MFRC522)

| MFRC522 Pin | ESP8266 Pin |
|---|---|
| RST | D3 |
| SDA/SS | D4 |
| BUZZER | D8 |

*(LCD connects via I2C — SDA/SCL to the ESP8266's I2C pins)*

## Project Structure

```
smart_attendance_system.ino   Main sketch: reads UID, checks in with backend, shows LCD status
google_apps_script.gs         Backend: auth, UID to name lookup, duplicate check, logging
config.h.example              Template for credentials — copy to config.h and fill in
.gitignore                    Excludes config.h from version control
```

## Setup Instructions

### 1. Set Up the Google Sheet
Create a spreadsheet with two tabs:

**"Students"** (pre-fill with your people):
| A: UID | B: Name |
|---|---|
| 04A3B2C1 | Alex |
| 1122AABB | Priya |

To find a tag's UID, scan it once — the sketch prints it to Serial Monitor (`Card UID: ...`) even before you've added it to the sheet, so you can copy it in.

**"Attendance"** (leave empty — headers optional):
| A: Date | B: Time | C: UID | D: Name |
|---|---|---|---|

### 2. Deploy the Backend
1. In the Sheet, go to Extensions, then Apps Script, and paste `google_apps_script.gs`.
2. Set `SPREADSHEET_ID` to your sheet's ID (from its URL).
3. Set `API_KEY` to a long random string — reuse the same value in `config.h` on the device.
4. Deploy as Web App: Execute as Me, Access: Anyone (the API key protects it).
5. Copy the deployment URL.

### 3. Configure the Device
```bash
cp config.h.example config.h
```
Edit `config.h`:
```cpp
#define WIFI_SSID     "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"
#define SHEET_URL     "https://script.google.com/macros/s/XXXX/exec"
#define API_KEY       "same-random-string-as-apps-script"
```
`config.h` is gitignored — it will never be committed.

### 4. Flash and Run
Upload `smart_attendance_system.ino`. On boot it connects to Wi-Fi (or continues offline and retries on the next scan) and shows "Scan your Card." Scanning a known tag logs attendance and greets the person by name; an unrecognized tag or a repeat scan the same day is handled gracefully instead of silently failing or double-logging.

## Security Notes (read before deploying for real use)

- SSL is still not fully verified (`client->setInsecure()`). This is common for hobbyist ESP8266 projects because pinning Google's certificate reliably across renewals is nontrivial. For anything beyond a classroom demo, consider `BearSSL::WiFiClientSecure` with a properly maintained certificate/fingerprint, or route through a small proxy server you control instead of hitting Google directly.
- The API key is a shared secret, not per-device auth. It stops casual abuse of the public URL but isn't a substitute for OAuth if this project ever needs to scale to multiple untrusted devices.
- UID spoofing: MIFARE UIDs can be spoofed by specialized "magic" cards. This is a much smaller attack surface than a writable-name approach, but not unbreakable — for high-stakes attendance (e.g., exams), pair with a second factor.

## Known Scaling Limit

Google Sheets/Apps Script is fine for classroom or small-team scale (up to a few hundred lookups/day comfortably). If you outgrow the Apps Script daily execution quota, migrate the backend to a lightweight database (Firebase, Airtable, or a small Flask/Node API) — the ESP8266 sketch doesn't need to change beyond the URL.

## Credits

Original concept by Viral Science — [website](http://www.viralsciencecreativity.com) . Our project restructures the identification method and adds the fixes described above.

