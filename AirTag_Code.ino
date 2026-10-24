#include <WiFi.h> // Lets the ESP32 connect to a WiFi network
#include <TinyGPS++.h> // Parses the NEO-6M GPS data 
#include <HardwareSerial.h> // Allows use of the ESP32's extra UART ports
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include "discord_root_ca.h"
#if __has_include("AirTagConfig.h")
#include "AirTagConfig.h"
#else
#include "AirTagConfig.example.h"
#endif

// Local credentials live in ignored AirTagConfig.h, never in tracked source.
const char* ssid = AIRTAG_WIFI_SSID;
const char* password = AIRTAG_WIFI_PASSWORD;
const char* discordHost = "discord.com";
const int httpsPort = 443;
const char* webhookPath = AIRTAG_DISCORD_WEBHOOK_PATH;
const char* gpsKey = AIRTAG_GPS_KEY;
bool configured = false;
unsigned long lastPhoneUpdate = 0;
constexpr unsigned long PHONE_LOCATION_TTL_MS = 120000;

// ==== Static IP (Optional) ====
IPAddress local_IP(172, 20, 10, 2);
IPAddress gateway(172, 20, 10, 1);
IPAddress subnet(255, 255, 255, 240);

// ==== GPS UART Setup ====
#define GPS_RX 2
#define GPS_TX 3
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

// ==== Buzzer ====
#define BUZZER_PIN 27

// ==== Web Server ====
WebServer server(80);

// Coordinates
const int AVERAGE_SIZE = 1;
double phoneLat = 0.0, phoneLon = 0.0;

// Coordinate buffers (uninplemented atm)
double latBuffer[AVERAGE_SIZE];
double lonBuffer[AVERAGE_SIZE];
int bufferIndex = 0;


bool alarmTriggered = false;
bool phoneLocationReceived = false;

bool validGpsKey(const char* key) {
  if (strlen(key) != 64) return false;
  for (size_t i = 0; i < 64; ++i) {
    if (!isxdigit(static_cast<unsigned char>(key[i]))) return false;
  }
  return true;
}

bool authorizedGpsRequest() {
  const String supplied = server.header("X-AirTag-Key");
  if (supplied.length() != 64) return false;
  unsigned char difference = 0;
  for (size_t i = 0; i < 64; ++i) difference |= supplied[i] ^ gpsKey[i];
  return difference == 0;
}

bool parseCoordinate(const String& value, double minimum, double maximum, double& result) {
  if (value.isEmpty() || value.length() > 24) return false;
  char* end = nullptr;
  result = strtod(value.c_str(), &end);
  return end != value.c_str() && *end == '\0' && isfinite(result) && result >= minimum && result <= maximum;
}

void handleGPS() {
  server.sendHeader("Cache-Control", "no-store");
  if (!authorizedGpsRequest()) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }
  // No browser website, including localhost, may drive this device endpoint.
  if (!server.header("Origin").isEmpty()) {
    server.send(403, "text/plain", "Browser requests are not allowed");
    return;
  }
  double latitude, longitude;
  if (!server.hasArg("lat") || !server.hasArg("lon") || server.args() != 2 ||
      !parseCoordinate(server.arg("lat"), -90.0, 90.0, latitude) ||
      !parseCoordinate(server.arg("lon"), -180.0, 180.0, longitude)) {
    server.send(400, "text/plain", "Valid lat and lon fields are required");
    return;
  }
  phoneLat = latitude;
  phoneLon = longitude;
  lastPhoneUpdate = millis();
  phoneLocationReceived = true;
  server.send(200, "text/plain", "GPS received");
}

void notifyDiscord(double lat, double lon) {

  // uses HTTPS to connect to discord
  WiFiClientSecure client;
  if (time(nullptr) < 1767225600) {
    Serial.println("Clock not synchronized; refusing unverified HTTPS.");
    return;
  }
  client.setCACert(discordRootCA);
  client.setHandshakeTimeout(10);
  client.setTimeout(5000);

  // Connect by DNS name so SNI and certificate hostname verification apply.
  if (!client.connect(discordHost, httpsPort)) {
    Serial.println("Verified Discord HTTPS connection failed.");
    return;
  }

  // Constructs the JSON payload with the ESP32's last known coordinates
  String latStr = String(lat, 6);
  String lonStr = String(lon, 6);
  String message = "{\"content\":\" ESP32 ALERT:\\nLat: " + latStr + "\\nLon: " + lonStr + "\"}";


  // Sends a POST request with headers and JSON body to the Discord webhook
  client.println("POST " + String(webhookPath) + " HTTP/1.1");
  client.println("Host: discord.com");
  client.println("User-Agent: ESP32WebhookClient");
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(message.length()));
  client.println("Connection: close");
  client.println();
  client.println(message);

  // Bound response waiting and do not log webhook response bodies or coordinates.
  const unsigned long responseStart = millis();
  while (!client.available() && client.connected() && millis() - responseStart < 5000) delay(10);
  if (client.available()) {
    const String status = client.readStringUntil('\n');
    if (status.startsWith("HTTP/1.1 204") || status.startsWith("HTTP/1.1 200"))
      Serial.println("Discord notification accepted.");
    else Serial.println("Discord notification failed.");
  } else Serial.println("Discord response timed out.");
  client.stop();
}

void setup() {

  // setup of pins
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  configured = validGpsKey(gpsKey) && strlen(ssid) > 0 && strlen(password) >= 8 &&
    String(webhookPath).startsWith("/api/webhooks/") &&
    String(webhookPath).indexOf('\r') < 0 && String(webhookPath).indexOf('\n') < 0 &&
    String(webhookPath).indexOf(' ') < 0;
  if (!configured) {
    Serial.println("Configure Wi-Fi, webhook path and a random 64-hex GPS key in AirTagConfig.h first.");
    return;
  }

  // attempting to connect to wifi, if failed, repeat 20 times
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts++ < 20) {
    delay(500); 
    Serial.print(".");
  }

  // reporting on wifi connectivity
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Wi-Fi");
  }

  // Certificate validation requires real time. Failure stays closed in notifyDiscord.
  configTime(0, 0, "time.google.com", "pool.ntp.org");
  const char* collectedHeaders[] = {"X-AirTag-Key", "Origin"};
  server.collectHeaders(collectedHeaders, 2);
  server.on("/gps", HTTP_POST, handleGPS);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.println("Web server started at /gps");
}

void loop() {
  if (!configured) { delay(1000); return; }
  if (phoneLocationReceived && millis() - lastPhoneUpdate > PHONE_LOCATION_TTL_MS)
    phoneLocationReceived = false;

  // handles any new HTTP requests from phone
  server.handleClient();

  // reads GPS data byte-by-byte and feeds it into TinyGPS++ to update position
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }

  // logging GPS fix only upon change
  static bool hadFix = false;
  if (!gps.location.isValid()) {
    if (hadFix) {
      Serial.println("Lost GPS fix...");
      hadFix = false;
    }
    return;
  } else if (!hadFix) {
    Serial.println("GPS fix acquired!");
    hadFix = true;
  }

  // if phone has yet to send location, continue to next iteration of loop
  if (!phoneLocationReceived) {
    Serial.println("Waiting for valid phone GPS coordinates...");
    return;
  }

  //
  if (gps.location.isUpdated() && gps.hdop.isValid() && gps.hdop.hdop() < 2.5) { 
  /* Only run logic if:
     New GPS data is available
     Horizontal dilution of precision is valid and small (i.e. good signal)
  */

  // Averages GPS samples (currently just 1, but you can increase AVERAGE_SIZE later to smooth out noise).
    latBuffer[bufferIndex] = gps.location.lat();
    lonBuffer[bufferIndex] = gps.location.lng();
    bufferIndex = (bufferIndex + 1) % AVERAGE_SIZE;

    double sumLat = 0, sumLon = 0;
    for (int i = 0; i < AVERAGE_SIZE; i++) {
      sumLat += latBuffer[i];
      sumLon += lonBuffer[i];
    }

    double avgLat = sumLat / AVERAGE_SIZE;
    double avgLon = sumLon / AVERAGE_SIZE;

    // Calculates distance between ESP32’s average GPS and the phone's coordinates
    double dist = TinyGPSPlus::distanceBetween(avgLat, avgLon, phoneLat, phoneLon);
    Serial.print("Distance to phone: ");
    Serial.print(dist); Serial.println(" meters");

    if (dist > 25.0 && !alarmTriggered) { // if coordinate distance is greater than 25 and alarm has yet to trigger
      alarmTriggered = true;

      // activating buzzer
      Serial.println("Triggering buzzer...");
      digitalWrite(BUZZER_PIN, HIGH);

      // calling upon norifyDiscord function with last coordinates of device
      Serial.println("Sending Discord notification...");
      notifyDiscord(avgLat, avgLon);
    }
  }
}
