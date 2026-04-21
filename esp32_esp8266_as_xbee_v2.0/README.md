# ESP32/ESP8266 as XBee Replacement v2.0

WiFi-to-Serial bridge for R2D2 Astromech droids. Replaces the classic XBee module with an ESP32 or ESP8266 WiFi Access Point and TCP server.

**Version:** 2.0 (Multi-Client Edition)
**Source:** www.printed-droid.com

## NEW in v2.0

- ✅ **Multi-Client Support**: Up to 4 simultaneous TCP connections (Button Board + App + more)
- ✅ **Command Queue**: Prevents command collisions with FIFO queue
- ✅ **ESP32 & ESP8266 Compatible**: One sketch for both platforms
- ✅ **Improved Status**: Per-client connection monitoring and statistics

## Overview

This sketch transforms an ESP32 or ESP8266 into an XBee replacement for wireless communication between multiple controllers (e.g., Button Board, smartphone app) and the R2D2 Control System (Shadow MD, Benduino, MarcDuino).

### Key Features

**Multi-Client Support:**
- Up to 4 TCP clients can connect simultaneously
- Button Board + App + additional devices all work at the same time
- Automatic client slot management
- Per-client connection monitoring

**Command Queue:**
- FIFO (First-In-First-Out) queue with 32 command buffer
- Prevents command collisions when multiple clients send simultaneously
- 50ms minimum delay between serial transmissions
- Queue overflow protection with statistics

**Platform Compatibility:**
- Works with ESP32 (all variants)
- Works with ESP8266 (NodeMCU, Wemos D1, etc.)
- Automatic platform detection at compile time
- Same configuration for both platforms

### How It Works

```
[WiFi Client]           [ESP32 as XBee]         [Arduino Mega / Shadow MD]
Button Board / App      Access Point            MarcDuino Controller
     |                       |                           |
     |-- WiFi -->            |-- WiFi AP                 |
     |   (Client)            |   "R2D2 Astromech"        |
     |                       |                           |
     |-- TCP -->             |-- TCP Server              |
     |   Port 9750           |   Port 9750               |
     |                       |                           |
     |   Sends: ":SE01"      |-- Serial -->              |-- Serial (9600 baud)
     |                       |   RX/TX                   |
     |                       |                           |
     |                       |   Receives data <--       |
     |   <-- TCP Reply       |                           |
```

**Bidirectional Communication:**
- WiFi → Serial (Commands from client to droid)
- Serial → WiFi (Replies from droid to client)

## Hardware

### Supported ESP32 Modules

- ESP32 DevKit
- ESP32 WROOM-32
- ESP32 Mini
- Other ESP32 variants with at least 1 UART

### Pin Assignment

| ESP32 Pin | Function | Connection to Arduino |
|-----------|----------|----------------------|
| TX (GPIO1) | Serial TX | → Arduino RX |
| RX (GPIO3) | Serial RX | ← Arduino TX |
| 5V / 3.3V | Power Supply | 5V or 3.3V |
| GND | Ground | GND |

**Important:**
- ESP32 RX/TX must be **crossed** with Arduino TX/RX!
- ESP32 RX is 3.3V tolerant, Arduino TX (5V) should use voltage divider (optional, usually not needed)

### Power Supply

- **5V**: Directly from Arduino 5V pin (recommended)
- **3.3V**: From Arduino 3.3V pin (less stable, not recommended under high WiFi load)
- **External Supply**: Separate 5V power supply (best option for stability)

**Power Consumption:**
- Idle: ~80mA
- WiFi active: 120-180mA
- Peaks: up to 250mA

## Software Configuration

### Installation

1. **Prepare Arduino IDE:**
   - Install ESP32 Board Support (see below)
   - Open sketch: `esp32_as_xbee_v1.7.ino`

2. **Adjust Configuration** (lines 28-32):
   ```cpp
   const char* ssid = "R2D2 Astromech";       // WiFi Name
   const char* pass = "C3POsucks";           // WiFi Password
   const uint16_t serverPort = 9750;        // TCP Port
   const uint32_t serialBaud = 9600;        // Serial Baud Rate
   ```

3. **Network Settings** (lines 34-37):
   ```cpp
   IPAddress apIP(192, 168, 4, 1);         // Access Point IP
   IPAddress apGateway(192, 168, 4, 1);    // Gateway IP
   IPAddress apSubnet(255, 255, 255, 0);   // Subnet Mask
   ```

4. **Upload:**
   - Select board (e.g., "ESP32 Dev Module")
   - Upload Speed: 115200 or 921600
   - Start upload

### ESP32 Board Support Installation

#### Arduino IDE 1.x

1. File → Preferences
2. Add to "Additional Boards Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Tools → Board → Boards Manager
4. Search "esp32" and install

#### Arduino IDE 2.x

1. File → Preferences → Additional URLs (see above)
2. Open Boards Manager (left sidebar)
3. Search "esp32" and install

### Board Settings

**For ESP32:**
```
Board: ESP32 Dev Module (or your specific model)
Upload Speed: 921600
CPU Frequency: 240MHz (default)
Flash Frequency: 80MHz
Flash Mode: QIO
Flash Size: 4MB (default)
Partition Scheme: Default
Core Debug Level: None
```

**For ESP8266:**
```
Board: NodeMCU 1.0 (ESP-12E Module) or Generic ESP8266
Upload Speed: 921600
CPU Frequency: 80MHz
Flash Size: 4MB (FS:2MB OTA:~1019KB)
Flash Mode: DIO
Flash Frequency: 40MHz
Reset Method: nodemcu
```

## Usage

### First Startup

1. **Upload ESP32:**
   - Configure sketch (SSID, password)
   - Upload to ESP32
   - Open Serial Monitor (9600 baud)

2. **Check Connection:**
   ```
   ESP XBEE Replacement - Multi-Client Edition
   Device: ESP32
   Version: 2.0 - www.printed-droid.com

   Features:
   - Multi-client support (up to 4 simultaneous)
   - Command queue to prevent collisions
   - ESP32 and ESP8266 compatible

   Access Point started successfully
   --- Network Configuration ---
   Device: ESP32
   SSID: R2D2 Astromech
   Password: C3POsucks
   AP IP address: 192.168.4.1
   Server Port: 9750
   MAC Address: XX:XX:XX:XX:XX:XX
   Max Clients: 4
   --- End Configuration ---
   TCP server started
   Astromech ready

   Waiting for connection... WiFi clients: 0 (no WiFi connection)
   ```

3. **Connect to WiFi:**
   - Find WiFi "R2D2 Astromech" on client device
   - Enter password: "C3POsucks"
   - Client should receive IP from range 192.168.4.x

4. **Establish TCP Connection:**
   - Connect TCP client to port 9750
   - Serial Monitor shows:
     ```
     New client #0 connected: 192.168.4.2, Channel: 1
     Total connected clients: 1/4
     ```

5. **Connect Additional Clients:**
   - Second device connects:
     ```
     New client #1 connected: 192.168.4.3, Channel: 1
     Total connected clients: 2/4
     ```

### Status Messages

**No WiFi Connection:**
```
Waiting for connection... WiFi clients: 0 (no WiFi connection)
Stats: Commands sent: 0, Queue: 0/32
```

**WiFi Connected, TCP Waiting:**
```
Waiting for connection... WiFi clients: 2 (WiFi connected, waiting for TCP)
Stats: Commands sent: 0, Queue: 0/32
```

**First Client Connected:**
```
New client #0 connected: 192.168.4.2, Channel: 1
Total connected clients: 1/4
```

**Additional Clients:**
```
New client #1 connected: 192.168.4.3, Channel: 1
Total connected clients: 2/4
```

**Client Disconnected:**
```
Client #0 disconnected (192.168.4.2)
Total connected clients: 1/4
```

**Queue Statistics (every 10 seconds):**
```
Stats: Commands sent: 42, Queue: 2/32
```

**Queue Overflow Warning:**
```
WARNING: Command queue full!
Stats: Commands sent: 156, Queue: 32/32, Overflows: 3
```

### Data Transfer

**WiFi → Serial (Receive commands):**
```
Client sends:  ":SE01\r"
ESP32 forwards via Serial
Arduino receives: ":SE01\r"
MarcDuino executes: Close All Panels
```

**Serial → WiFi (Send replies):**
```
Arduino sends via Serial: "OK\r\n"
ESP32 forwards via TCP
Client receives: "OK\r\n"
```

## Integration

### With Shadow MD / AstroCan / AstroComs

1. **Hardware Connection:**
   ```
   ESP32 TX  →  Arduino Mega RX1 (Pin 19)
   ESP32 RX  ←  Arduino Mega TX1 (Pin 18)
   ESP32 GND -  Arduino Mega GND
   ESP32 5V  -  Arduino Mega 5V
   ```

2. **Shadow MD Sketch:**
   - Serial1 used for XBee/ESP32
   - Baud rate: 9600 (already configured)
   - No changes needed

### With Benduino / Betterduino

Similar to Shadow MD, uses Serial1 or Serial2 depending on configuration.

### With Smartphone App

Apps supporting TCP communication (e.g., custom apps for R2D2):

1. Connect to WiFi "R2D2 Astromech"
2. Configure TCP connection in app:
   - Host: 192.168.4.1
   - Port: 9750
3. Send commands in format: `:SEXX\r`

## Configuration

### Change WiFi Settings

**Change SSID** (line 29):
```cpp
const char* ssid = "My-R2D2";
```

**Change Password** (line 30):
```cpp
const char* pass = "SecurePassword123";
```

⚠️ **Security:** Use a strong password in production environments!

### Change TCP Port

**Change Port** (line 31):
```cpp
const uint16_t serverPort = 8080;  // Example: Port 8080
```

**Important:** Also adjust port on client side!

### Change Serial Baud Rate

**Change Baud Rate** (line 32):
```cpp
const uint32_t serialBaud = 19200;  // Example: 19200 baud
```

**Important:**
- Baud rate must match Arduino sketch!
- Shadow MD uses 9600 baud
- Other systems may use 115200 baud

### Change IP Address

**Change IP** (line 35):
```cpp
IPAddress apIP(192, 168, 10, 1);  // New IP: 192.168.10.1
```

**Important:** Adjust gateway and client configuration accordingly!

## Troubleshooting

### ESP32 Doesn't Start

**Symptom:** No serial output, no WiFi

**Solutions:**
- Check USB cable (data cable, not just charging cable)
- Hold Boot button during upload
- Check baud rate in Serial Monitor (9600)
- Check power supply (min. 500mA)

### WiFi Access Point Doesn't Appear

**Symptom:** WiFi "R2D2 Astromech" not visible

**Solutions:**
- Check Serial Monitor: "Access Point started successfully"?
- Reset ESP32 (Reset button)
- Turn off other WiFi devices nearby (interference)
- WiFi channel may be congested (automatic selection by ESP32)

**Manually Set WiFi Channel:**
```cpp
WiFi.softAP(ssid, pass, 6);  // Use channel 6
```

### Client Can't Connect to WiFi

**Symptom:** WiFi visible but connection fails

**Solutions:**
- Password entered correctly?
- Client device supports 2.4 GHz? (ESP32 has no 5 GHz!)
- Too many clients? (Default: max 4 clients simultaneously)
- Restart Access Point

**Increase Max Clients:**
```cpp
WiFi.softAP(ssid, pass, 1, 0, 8);  // max 8 clients
```

### TCP Connection Fails

**Symptom:** WiFi connected but TCP doesn't work

**Solutions:**
- Check IP address: `ping 192.168.4.1`
- Port correct? (9750)
- Disable firewall on client device
- Check Serial Monitor: Does ESP32 show "TCP client connected"?

**Test Port with Telnet:**
```bash
telnet 192.168.4.1 9750
```

### Data Transfer Doesn't Work

**Symptom:** TCP connected but no data

**Solutions:**

**Case 1: WiFi → Serial not working:**
- Check serial connection to Arduino (RX/TX swapped?)
- Baud rate correct? (ESP32 and Arduino must match)
- Arduino running and listening on Serial1?
- Serial Monitor shows received data?

**Case 2: Serial → WiFi not working:**
- Arduino actually sending data?
- TCP client receiving data?
- In code bidirectional is active (line 146-149)

**Enable Debug Output:**
```cpp
void handleDataTransfer() {
  while (client.available() > 0) {
    char c = client.read();
    Serial.write(c);
    Serial.print(" [0x");  // Debug: Show hex value
    Serial.print(c, HEX);
    Serial.print("]");
  }
  // ...
}
```

### ESP32 Crashes / Watchdog Reset

**Symptom:** ESP32 restarts repeatedly

**Solutions:**
- Power supply too weak (min. 500mA)
- Use external power supply (not from Arduino)
- Flash size set correctly?
- Increase Core Debug Level for more info

### Serial Communication with Errors

**Symptom:** Characters transmitted incorrectly

**Solutions:**
- Baud rate exactly the same on both sides
- Common ground connection (GND)
- Cables not too long (max. 30cm for clean signals)
- Use level shifter for 5V Arduino

### WiFi Range Too Short

**Symptom:** WiFi connection drops

**Solutions:**
- Use external antenna (if ESP32 module has antenna connector)
- Position ESP32 exposed (not in metal housing)
- Increase TX power:
  ```cpp
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Max power
  ```
- Avoid obstacles between ESP32 and client

## Optimizations

### Performance Tuning

**Increase CPU Frequency:**
```cpp
setCpuFrequencyMhz(240);  // Max 240 MHz
```

**Adjust Buffer Size:**
```cpp
WiFiClient client;
client.setNoDelay(true);  // Enable TCP_NODELAY (less latency)
```

**Disable WiFi Sleep:**
```cpp
WiFi.setSleep(false);  // Better latency, more power consumption
```

### Multi-Client Support

Currently: 1 TCP client at a time
Extension to multiple clients possible:

```cpp
WiFiClient clients[4];  // Max 4 clients
// Loop through all clients in handleClientConnection()
```

### Advanced Features

**1. Watchdog Timer:**
```cpp
#include "esp_system.h"

void setup() {
  // ...
  esp_task_wdt_init(30, true);  // 30 seconds watchdog
  esp_task_wdt_add(NULL);
}

void loop() {
  // ...
  esp_task_wdt_reset();  // Reset watchdog
}
```

**2. WiFi Connection LED:**
```cpp
#define LED_PIN 2

void setup() {
  pinMode(LED_PIN, OUTPUT);
  // ...
}

void loop() {
  digitalWrite(LED_PIN, clientConnected ? HIGH : LOW);
  // ...
}
```

**3. OTA (Over-The-Air) Updates:**
```cpp
#include <ArduinoOTA.h>

void setup() {
  // ...
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  // ...
}
```

## Differences from XBee

### ESP32 Advantages

| Feature | XBee | ESP32 |
|---------|------|-------|
| **Price** | $25-50 | $3-8 |
| **Speed** | 250 kbps | up to 150 Mbps |
| **Range** | 100m (line of sight) | 50-100m (depending on antenna) |
| **Configuration** | X-CTU software | Arduino code |
| **Power Consumption** | ~50mA | 120-180mA |
| **Additional Features** | Limited | OTA, Web server, etc. |

### ESP32 Disadvantages

- **Higher power consumption** (approx. 2-3x)
- **No true peer-to-peer** communication (only client-server)
- **WiFi 2.4 GHz only** (XBee can also use 900 MHz)

### Compatibility

✅ **Works with:**
- Shadow MD Control System
- Benduino
- MarcDuino Master
- PADAWAN 360 (with modifications)

❌ **Not compatible with:**
- Systems using XBee API Mode (only Transparent Mode)
- XBee Mesh Networks
- ZigBee Protocol

## Security

### Recommendations

⚠️ **Important for public events:**

1. **Use strong password:**
   ```cpp
   const char* pass = "MyR2D2SecurePass#2024";
   ```

2. **Hide SSID (optional):**
   ```cpp
   WiFi.softAP(ssid, pass, 1, 1);  // hidden=true
   ```

3. **Enable MAC filter:**
   ```cpp
   // In setup() after WiFi.softAP()
   // Only allow defined MACs
   ```

4. **Command validation:**
   Extension to only forward valid commands

### Data Security

- **Unencrypted:** Data transmitted in clear text
- **For hobby projects:** Sufficient
- **For critical applications:** Implement encryption (e.g., SSL/TLS)

## Technical Details

### Memory Usage

```
Sketch uses:   XXXXX bytes (XX%) of program storage
Global vars:   XXXXX bytes (XX%) of dynamic memory
```

Typical:
- Flash: ~250 KB
- RAM: ~25 KB

### Latency

- **WiFi → Serial:** < 10ms
- **Serial → WiFi:** < 10ms
- **Round-Trip:** < 20ms

Depends on:
- WiFi signal strength
- Number of simultaneous clients
- CPU load

### Protocol

**TCP/IP Stack:**
- Protocol: TCP
- Port: 9750 (configurable)
- Format: Raw Data (transparent)
- Terminator: `\r` or `\r\n` (depending on client)

## License & Credits

### Source

- **Original:** www.printed-droid.com
- **Version:** 1.7 (Optimized)
- **Date:** 2025-09-01

### Community

- **Printed-Droid Forum:** https://www.printed-droid.com
- **R2D2 Builders Club:** https://astromech.net
- **Astromech.net Forum:** https://astromech.net/forums

### Usage

**Use at your own risk. No warranty.**

Free use for private, non-commercial purposes.
Please test range and reliability before deploying in your R2D2!

## FAQ

**Q: Can I connect multiple devices simultaneously?**
A: Yes! v2.0 supports up to 4 simultaneous TCP connections. Button Board + App + additional devices can all work at the same time.

**Q: What happens if multiple clients send commands at the same time?**
A: The command queue handles this automatically. Commands are queued in FIFO order and sent with a 50ms delay between each to prevent collisions.

**Q: Does this work with ESP8266?**
A: Yes! v2.0 supports both ESP32 and ESP8266. Same sketch, automatic platform detection.

**Q: Does this work with 5 GHz WiFi?**
A: No, both ESP32 and ESP8266 only support 2.4 GHz WiFi.

**Q: Can I increase the range?**
A: Yes, via external antenna or repeater. Or multiple ESP32 as mesh network (requires code modification).

**Q: Is this more secure than XBee?**
A: When using WPA2 encryption (WiFi) similarly secure. Data itself is unencrypted though.

**Q: Can I use this for other robot projects?**
A: Yes! Any project using serial communication can benefit.

**Q: Do I need to install X-CTU like with XBee?**
A: No, configuration is done directly in Arduino code.

---

**Good luck with your ESP32 WiFi setup!** 📡🤖
