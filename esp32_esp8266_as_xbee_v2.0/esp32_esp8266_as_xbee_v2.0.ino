/*
ESP32/ESP8266 XBEE sketch v2.0 - Multi-Client with Command Queue
Date: 2025/01/17

www.printed-droid.com

NEW IN v2.0:
- Multi-client support (up to 4 simultaneous TCP connections)
- Command queue to prevent simultaneous serial transmissions
- ESP32 and ESP8266 compatibility
- Improved status messages for each client

This sketch makes it possible to use an ESP32 or ESP8266 instead of the XBEE.
Optimized for bidirectional communication with multiple clients.

Use is at your own risk and without further support.
Please test the range and reliability before use.

More information is available at: https://www.printed-droid.com/kb/esp-as-xbee-replacement/

Change the SSID and password in the sketch!!!
The ESP IP is visible via the Arduino IDE serial monitor (default: 192.168.4.1)

Connection to Astrocomms, Benduino or Marcduino like the XBEE:
5V (or 3.3V)
GND
RX to TX
TX to RX
*/

// ESP32 or ESP8266 detection
#if defined(ESP32)
  #include <WiFi.h>
  #define DEVICE_TYPE "ESP32"
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #define DEVICE_TYPE "ESP8266"
#else
  #error "This sketch only works with ESP32 or ESP8266"
#endif

// ============================================================================
// Configuration - Change these values according to your needs
// ============================================================================
const char* ssid = "R2D2 Astromech";       // Name of the WiFi network
const char* pass = "C3POsucks";            // WiFi password
const uint16_t serverPort = 9750;          // TCP server port
const uint32_t serialBaud = 9600;          // Serial communication baud rate

// Network configuration
IPAddress apIP(192, 168, 4, 1);            // Access Point IP address
IPAddress apGateway(192, 168, 4, 1);       // Gateway IP address
IPAddress apSubnet(255, 255, 255, 0);      // Subnet mask

// Multi-client configuration
#define MAX_CLIENTS 4                      // Maximum simultaneous TCP clients

// ============================================================================
// Command Queue Configuration
// ============================================================================
#define QUEUE_SIZE 32                      // Maximum queued commands
#define MAX_COMMAND_LENGTH 64              // Maximum command length

struct Command {
  char data[MAX_COMMAND_LENGTH];
  uint8_t length;
};

// ============================================================================
// Global Variables
// ============================================================================
WiFiServer server(serverPort);
WiFiClient clients[MAX_CLIENTS];
bool clientConnected[MAX_CLIENTS] = {false};

// Command Queue
Command commandQueue[QUEUE_SIZE];
volatile uint8_t queueHead = 0;
volatile uint8_t queueTail = 0;
volatile uint8_t queueCount = 0;

// Status tracking
unsigned long lastWaitingCheck = 0;
const unsigned long waitingCheckInterval = 10000; // Check every 10 seconds
unsigned long lastSerialSend = 0;
const unsigned long serialSendDelay = 50;         // Minimum 50ms between commands

// Statistics
uint32_t totalCommandsSent = 0;
uint32_t queueOverflows = 0;

// ============================================================================
// Command Queue Functions
// ============================================================================

bool enqueueCommand(const char* cmd, uint8_t len) {
  // Atomic check of queue count
  noInterrupts();
  bool isFull = (queueCount >= QUEUE_SIZE);
  if (isFull) {
    queueOverflows++;
  }
  interrupts();

  if (isFull) {
    Serial.println(F("WARNING: Command queue full!"));
    return false;
  }

  if (len >= MAX_COMMAND_LENGTH) {
    len = MAX_COMMAND_LENGTH - 1;
  }

  memcpy(commandQueue[queueHead].data, cmd, len);
  commandQueue[queueHead].data[len] = '\0';
  commandQueue[queueHead].length = len;

  // Atomic update of queue pointers
  noInterrupts();
  queueHead = (queueHead + 1) % QUEUE_SIZE;
  queueCount++;
  interrupts();

  return true;
}

bool dequeueCommand(Command* cmd) {
  // Atomic check of queue count
  noInterrupts();
  bool isEmpty = (queueCount == 0);
  interrupts();

  if (isEmpty) {
    return false;
  }

  memcpy(cmd, &commandQueue[queueTail], sizeof(Command));

  // Atomic update of queue pointers
  noInterrupts();
  queueTail = (queueTail + 1) % QUEUE_SIZE;
  queueCount--;
  interrupts();

  return true;
}

// ============================================================================
// Serial Command Processing
// ============================================================================

void processSerialQueue() {
  unsigned long currentTime = millis();

  // Rate limiting: minimum delay between serial sends
  if (currentTime - lastSerialSend < serialSendDelay) {
    return;
  }

  Command cmd;
  if (dequeueCommand(&cmd)) {
    // Send to serial
    Serial.write((uint8_t*)cmd.data, cmd.length);

    lastSerialSend = currentTime;
    totalCommandsSent++;

    #ifdef DEBUG_QUEUE
    Serial.print(F("Sent command ("));
    Serial.print(cmd.length);
    Serial.print(F(" bytes): "));
    Serial.println(cmd.data);
    #endif
  }
}

// ============================================================================
// WiFi Setup
// ============================================================================

void setupWiFi() {
  // Configure Access Point
  WiFi.mode(WIFI_AP);

  #if defined(ESP32)
    WiFi.softAPConfig(apIP, apGateway, apSubnet);
    if (WiFi.softAP(ssid, pass)) {
      Serial.println(F("Access Point started successfully"));
    } else {
      Serial.println(F("Failed to start Access Point"));
      return;
    }
  #elif defined(ESP8266)
    WiFi.softAPConfig(apIP, apGateway, apSubnet);
    WiFi.softAP(ssid, pass);
    Serial.println(F("Access Point started successfully"));
  #endif

  printNetworkInfo();
}

void printNetworkInfo() {
  Serial.println(F("--- Network Configuration ---"));
  Serial.print(F("Device: "));
  Serial.println(DEVICE_TYPE);
  Serial.print(F("SSID: "));
  Serial.println(ssid);
  Serial.print(F("Password: "));
  Serial.println(pass);
  Serial.print(F("AP IP address: "));
  Serial.println(WiFi.softAPIP());
  Serial.print(F("Server Port: "));
  Serial.println(serverPort);
  Serial.print(F("MAC Address: "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("Max Clients: "));
  Serial.println(MAX_CLIENTS);
  Serial.println(F("--- End Configuration ---"));
}

// ============================================================================
// Multi-Client Management
// ============================================================================

void handleClientConnections() {
  // Check for disconnected clients
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientConnected[i] && !clients[i].connected()) {
      Serial.print(F("Client #"));
      Serial.print(i);
      Serial.print(F(" disconnected ("));
      Serial.print(clients[i].remoteIP());
      Serial.println(F(")"));

      clients[i].stop();
      clientConnected[i] = false;
    }
  }

  // Check for new client connections
  #if defined(ESP32)
    WiFiClient newClient = server.available();
  #elif defined(ESP8266)
    WiFiClient newClient = server.accept();
  #endif

  if (newClient) {
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!clientConnected[i]) {
        slot = i;
        break;
      }
    }

    if (slot != -1) {
      clients[slot] = newClient;
      clientConnected[slot] = true;

      Serial.println();
      Serial.print(F("New client #"));
      Serial.print(slot);
      Serial.print(F(" connected: "));
      Serial.print(newClient.remoteIP());

      #if defined(ESP32)
        Serial.print(F(", Channel: "));
        Serial.println(WiFi.channel());
      #else
        Serial.println();
      #endif

      printClientStatus();
    } else {
      Serial.println(F("Client rejected: Maximum clients reached"));
      newClient.stop();
    }
  }
}

void printClientStatus() {
  int connected = 0;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientConnected[i]) {
      connected++;
    }
  }

  Serial.print(F("Total connected clients: "));
  Serial.print(connected);
  Serial.print(F("/"));
  Serial.println(MAX_CLIENTS);
}

// ============================================================================
// Data Transfer
// ============================================================================

void handleDataTransfer() {
  // Buffer for incoming data
  static char incomingBuffer[MAX_COMMAND_LENGTH];
  static uint8_t bufferIndex = 0;

  // Read from all connected clients and enqueue commands
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientConnected[i] && clients[i].available() > 0) {
      while (clients[i].available() > 0) {
        char c = clients[i].read();

        // Add to buffer
        if (bufferIndex < MAX_COMMAND_LENGTH - 1) {
          incomingBuffer[bufferIndex++] = c;
        }

        // Check for command terminator (carriage return)
        if (c == '\r' || c == '\n' || bufferIndex >= MAX_COMMAND_LENGTH - 1) {
          if (bufferIndex > 0) {
            incomingBuffer[bufferIndex] = '\0';

            // Enqueue complete command
            if (!enqueueCommand(incomingBuffer, bufferIndex)) {
              Serial.print(F("Failed to enqueue from client #"));
              Serial.println(i);
            }

            bufferIndex = 0;
          }
        }
      }
    }
  }

  // Process serial queue (send to hardware)
  processSerialQueue();

  // Read from Serial and broadcast to all connected clients
  if (Serial.available() > 0) {
    char serialData[64];
    uint8_t len = 0;

    // Read available serial data
    while (Serial.available() > 0 && len < 63) {
      serialData[len++] = Serial.read();
    }

    // Broadcast to all connected clients
    if (len > 0) {
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientConnected[i]) {
          clients[i].write((uint8_t*)serialData, len);
        }
      }
    }
  }
}

// ============================================================================
// Status Monitoring
// ============================================================================

void checkWaitingStatus() {
  unsigned long currentTime = millis();

  // Handle millis() overflow
  if (currentTime < lastWaitingCheck) {
    lastWaitingCheck = currentTime;
  }

  // Show waiting status every 10 seconds
  if (currentTime - lastWaitingCheck >= waitingCheckInterval) {
    #if defined(ESP32)
      uint8_t wifiClients = WiFi.softAPgetStationNum();
    #elif defined(ESP8266)
      uint8_t wifiClients = WiFi.softAPgetStationNum();
    #endif

    int tcpConnected = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i]) tcpConnected++;
    }

    if (tcpConnected == 0) {
      Serial.print(F("Waiting for connection... WiFi clients: "));
      Serial.print(wifiClients);
      if (wifiClients > 0) {
        Serial.println(F(" (WiFi connected, waiting for TCP)"));
      } else {
        Serial.println(F(" (no WiFi connection)"));
      }
    }

    // Show statistics
    Serial.print(F("Stats: Commands sent: "));
    Serial.print(totalCommandsSent);
    Serial.print(F(", Queue: "));
    Serial.print(queueCount);
    Serial.print(F("/"));
    Serial.print(QUEUE_SIZE);
    if (queueOverflows > 0) {
      Serial.print(F(", Overflows: "));
      Serial.print(queueOverflows);
    }
    Serial.println();

    lastWaitingCheck = currentTime;
  }
}

// ============================================================================
// Arduino Setup & Loop
// ============================================================================

void setup() {
  // Initialize serial communication
  Serial.begin(serialBaud);
  delay(100);

  Serial.println();
  Serial.println(F("ESP XBEE Replacement - Multi-Client Edition"));
  Serial.print(F("Device: "));
  Serial.println(DEVICE_TYPE);
  Serial.println(F("Version: 2.0 - www.printed-droid.com"));
  Serial.println();
  Serial.println(F("Features:"));
  Serial.println(F("- Multi-client support (up to 4 simultaneous)"));
  Serial.println(F("- Command queue to prevent collisions"));
  Serial.println(F("- ESP32 and ESP8266 compatible"));
  Serial.println();

  // Setup WiFi Access Point
  setupWiFi();

  // Start TCP server
  server.begin();
  Serial.println(F("TCP server started"));
  Serial.println(F("Astromech ready"));
  Serial.println();
}

void loop() {
  // Handle client connections/disconnections
  handleClientConnections();

  // Handle bidirectional data transfer
  handleDataTransfer();

  // Show waiting status if no clients connected
  bool anyConnected = false;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientConnected[i]) {
      anyConnected = true;
      break;
    }
  }

  if (!anyConnected) {
    checkWaitingStatus();
  }

  // Small delay to prevent excessive CPU usage
  delay(1);
}