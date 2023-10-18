#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "esp32b01";
const char* password = "0000000000";
unsigned int localPort = 2980;
char packetBuffer[255];
char ReplyBuffer[] = "ACK";

WiFiUDP udp;

void setup() {
  Serial.begin(921600);
  delay(10);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("starting WIFI");
  udp.begin(localPort);
  Serial.println("wifi address:");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // put your main code here, to run repeatedly:
  int packetSize = udp.parsePacket();
  if (packetSize){
    Serial.print("Received from: ");
    Serial.println(udp.remoteIP());
    int len = udp.read(packetBuffer, 255);
    if (len > 0){
      packetBuffer[len] = 0;
    }
    Serial.println("Contents: ");
    Serial.println(packetBuffer);

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t*)ReplyBuffer, sizeof(ReplyBuffer));
    udp.endPacket();
  }
}
