#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS 53
#define APPS1 A0
// #define APPS2 A1

MCP_CAN CAN(CAN_CS);

unsigned long lastSendTime = 0;
bool enabled = false;

void setup() {
  Serial.begin(115200);

  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
    Serial.println("CAN Initialized");
  else {
    Serial.println("CAN Init Failed");
    while (1);
  }

  CAN.setMode(MCP_NORMAL);
  delay(100);

  // ----------- SEND ENABLE ONCE -----------
  byte enableData[3] = {0x30, 0x01, 0x00};   // REGID=0x30, ENABLE=1
  CAN.sendMsgBuf(0x201, 0, 3, enableData);
  enabled = true;
  Serial.println("Motor ENABLED");
}

void loop() {
  // ----------- READ APPS -----------
  int apps1 = analogRead(APPS1);   
  int apps2 = analogRead(APPS2);

  float v1 = (apps1 * 5.0) / 1023.0;
  float v2 = (apps2 * 5.0) / 1023.0;

  float x1 = (1.0 - (v1 / 4.28));
  float x2 = ((4.09 - v2) / 2.15);
  float x =v1/5;
  float x = min(x1, x2);

  float percentage = (fabs(x2 - x1) / x) * 100.0;

  int torqueValue;
  torqueValue=x*32767;

  if (percentage < 10.0 && x > 0) {
    torqueValue = (1-x) * 32767.0;
  } else {
    torqueValue = 0;             
  }

  if (torqueValue < 0) torqueValue = 0;
  if (torqueValue > 32767) torqueValue = 32767;

  // ----------- CONSTRUCT TORQUE FRAME -----------
  byte torqueMsg[3];
  torqueMsg[0] = 0x90;                   // REGID for TORQUE_SOLL
  torqueMsg[1] = torqueValue & 0xFF;     // Low byte
  torqueMsg[2] = (torqueValue >> 8) & 0xFF; // High byte

  // ----------- SEND EVERY 20 ms -----------
  if (millis() - lastSendTime >= 20) {
    lastSendTime = millis();

    byte status = CAN.sendMsgBuf(0x201, 0, 3, torqueMsg);

    if (status == CAN_OK) {
      Serial.println("Torque CMD Sent");
      Serial.println(torqueValue);
    } else {
      Serial.println("CAN Send ERROR");
    }
  }
}
controlling using potentiometer