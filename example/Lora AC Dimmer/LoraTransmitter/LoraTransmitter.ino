//Example of sending control code to AC Dimmer
//Author :Vincent
//Create :2021/7/31
//Use Maduino Lora Radio 
//Complier arduino pro mini, 328p 3.3v 8MHz

#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 433.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

char command_text[5][32] = {"ID001ACT001PARAM000000#",
                            "ID001ACT000PARAM000000#",
                            "ID001ACT003PARAM000060#",
                            "ID001ACT004PARAM005060#",
                            "ID001ACT002PARAM005150#"};

// char command_text[5][32] = {"ID001ACT002PARAM000000#",
//                             "ID001ACT002PARAM000050#",
//                             "ID001ACT002PARAM000100#",
//                             "ID001ACT002PARAM000150#",
//                             "ID001ACT002PARAM000255#"};

void setup()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  //while (!Serial);
  Serial.begin(115200);
  delay(100);

  Serial.println("Arduino LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init())
  {
    Serial.println("LoRa radio init failed");
    while (1)
      ;
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ))
  {
    Serial.println("setFrequency failed");
    while (1)
      ;
  }
  Serial.print("Set Freq to: ");
  Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

int16_t packetnum = 0; // packet counter, we increment per xmission

void loop()
{
  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server

  // char radiopacket[20] = "Hello World #      ";
  // itoa(packetnum++, radiopacket + 13, 10);
  // Serial.print("Sending ");
  // Serial.println(radiopacket);
  // radiopacket[19] = 0;

  Serial.println("Sending...");
  delay(10);
  rf95.send((uint8_t *)command_text[packetnum++ % 5], 32);

  Serial.println("Waiting for packet to complete...");
  delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply...");
  delay(10);
  if (rf95.waitAvailableTimeout(1000))
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len))
    {
      Serial.print("Got reply: ");
      Serial.println((char *)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
  }
  delay(20000);
}
