#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFiClientSecure.h>

// ==== Wi-Fi Credentials ====
const char* ssid = "YOUR_HOTSPOT_NAME_HERE";
const char* password = "YOUR_HOTSPOT_PASSWORD_HERE";

// ==== Discord Webhook Info ====
const char* discordHost = "discord.com";
const int httpsPort = 443;
const char* webhookPath = "YOUR_DISCORD_WEBHOOKS_LINK_HERE";

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
double latBuffer[AVERAGE_SIZE];
double lonBuffer[AVERAGE_SIZE];
int bufferIndex = 0;
bool alarmTriggered = false;
bool phoneLocationReceived = false;

void handleGPS() {
  String latRaw = server.arg("lat");
  String lonRaw = server.arg("lon");

  latRaw.replace("[", ""); latRaw.replace("]", "");
  lonRaw.replace("[", ""); lonRaw.replace("]", "");

  phoneLat = latRaw.toFloat();
  phoneLon = lonRaw.toFloat();

  if (phoneLat != 0.0 || phoneLon != 0.0) {
    phoneLocationReceived = true;
  }

  Serial.println("Phone GPS Received:");
  Serial.print("Lat: "); Serial.println(phoneLat, 6);
  Serial.print("Lon: "); Serial.println(phoneLon, 6);

  server.send(200, "text/plain", "GPS received");
}

void notifyDiscord(double lat, double lon) {
  WiFiClientSecure client;
  client.setInsecure();

  // Hardcoded Discord IP to bypass DNS on hotspot
  IPAddress discordIP(162, 159, 135, 232);

  Serial.print("Connecting to Discord IP...");
  if (!client.connect(discordIP, httpsPort)) {
    Serial.println("HTTPS IP connect failed.");
    return;
  }
  Serial.println("Connected!");

  String latStr = String(lat, 6);
  String lonStr = String(lon, 6);
  String message = "{\"content\":\" ESP32 ALERT:\\nLat: " + latStr + "\\nLon: " + lonStr + "\"}";

  Serial.println("Payload:");
  Serial.println(message);

  client.println("POST " + String(webhookPath) + " HTTP/1.1");
  client.println("Host: discord.com");
  client.println("User-Agent: ESP32WebhookClient");
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(message.length()));
  client.println("Connection: close");
  client.println();
  client.println(message);

  while (client.connected() || client.available()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }

  client.stop();
  Serial.println("Discord notification sent.");
}

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts++ < 20) {
    delay(500); 
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Wi-Fi");
  }

  server.on("/gps", handleGPS);
  server.begin();
  Serial.println("Web server started at /gps");
}

void loop() {
  server.handleClient();

  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }

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

  if (!phoneLocationReceived) {
    Serial.println("Waiting for valid phone GPS coordinates...");
    return;
  }

  if (gps.location.isUpdated() && gps.hdop.isValid() && gps.hdop.hdop() < 2.5) {
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

    Serial.println("GPS Coordinates Comparison:");
    Serial.print("Phone Lat: "); Serial.println(phoneLat, 6);
    Serial.print("Phone Lon: "); Serial.println(phoneLon, 6);
    Serial.print("ESP32 Lat: "); Serial.println(avgLat, 6);
    Serial.print("ESP32 Lon: "); Serial.println(avgLon, 6);

    double dist = TinyGPSPlus::distanceBetween(avgLat, avgLon, phoneLat, phoneLon);
    Serial.print("Distance to phone: ");
    Serial.print(dist); Serial.println(" meters");

    if (dist > 25.0 && !alarmTriggered) {
      alarmTriggered = true;

      Serial.println("Triggering buzzer...");
      digitalWrite(BUZZER_PIN, HIGH);

      Serial.println("Sending Discord notification...");
      notifyDiscord(avgLat, avgLon);
    }
  }
}