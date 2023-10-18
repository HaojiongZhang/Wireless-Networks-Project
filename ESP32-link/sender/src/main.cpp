#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "esp32b01";
const char* password = "0000000000";
byte targetIP[4] = {192, 168, 4, 1};
unsigned int localPort = 2980;

char ackBuffer[10];
char greeting[] = "Greetings from esp32";

WiFiUDP udp;

void setup() {
  Serial.begin(921600);
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("connecting to WIFI");
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("Not yet connected");
  }
  udp.begin(localPort);
}

void loop() {
  // put your main code here, to run repeatedly:
  udp.beginPacket(targetIP, localPort);
  udp.write((const uint8_t*)greeting, sizeof(greeting));
  udp.endPacket();
  Serial.println("packet Sent");

  int packetSize = udp.parsePacket();
  if (packetSize){
    Serial.print("Received from: ");
    Serial.println(udp.remoteIP());
    udp.read(ackBuffer, 255);
    Serial.println("Contents: ");
    Serial.println(ackBuffer);
  }
  delay(1000);
}
