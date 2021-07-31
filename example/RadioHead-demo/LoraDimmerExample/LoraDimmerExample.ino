//Lora AC Dimmer default demo, can get different command to control ac.
//Author :Vincent
//Create :2021/7/31
//Use Makerfabs Lora AC Dimmer
//Complier arduino pro mini, 328p 3.3v 8MHz

/*

Command example:

  ACT = 0 Close
    ID001ACT000PARAM000000#

  ACT = 1 All Open
    ID001ACT001PARAM000000#

  ACT = 2 PWM 
    PARAM = 0-255 Dimmer PWM 
    ID001ACT002PARAM005150

  ACT = 3 PWM DELAY ON  (delay and then close)
    PARAM % 1000 = 0-255 Dimmer PWM
    PARAM % 1000 = 0-999 Second Delay
    ID001ACT003PARAM000060

  ACT = 4 PWM DELAY OFF  (delay and then all on)
    PARAM % 1000 = 0-255 Dimmer PWM
    PARAM % 1000 = 0-999 Second Delay
    ID001ACT004PARAM005060

*/

#include <SPI.h>
#include <RH_RF95.h>
#include <arduino.h>

String node_id = "ID001";

#define LED_PIN A3  // Blinky on receipt
#define RELAY_PIN 5 // the pin that the Relay is attached to
#define TRIAC_PIN 4 // the pin that the TRIAC is attached to
#define ZCD_PIN 3   //Zero Crossing Detector

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 433.0

#define TRIAC_CTRL_OFF digitalWrite(TRIAC_PIN, LOW)
#define TRIAC_CTRL_ON digitalWrite(TRIAC_PIN, HIGH)
#define RELAY_OFF digitalWrite(RELAY_PIN, LOW)
#define RELAY_ON digitalWrite(RELAY_PIN, HIGH)

int dim = 0; //Initial brightness level from 0 to 255, change as you like!

int delay_flag = 0;
long command_time = 0;
long delay_second = 0;

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void pin_init()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  //RELAY_OFF;
  RELAY_ON;

  pinMode(TRIAC_PIN, OUTPUT);
  TRIAC_CTRL_OFF;

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  pinMode(ZCD_PIN, INPUT);
  pinMode(ZCD_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), zero_cross_int, FALLING); //CHANGE FALLING
}

void lora_init()
{
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

void zero_cross_int() // function to be fired at the zero crossing to dim the light
{
  // if (Serial.available())
  // {
  //   dim = Serial.read(); //get 0x00~0xff
  // }
  if (dim < 1)
  {
    RELAY_OFF;
    delay(10);
    //Turn TRIAC completely OFF if dim is 0
    TRIAC_CTRL_OFF; // digitalWrite(AC_pin, LOW);
  }
  else if (dim > 254)
  { //Turn TRIAC completely ON if dim is 255
    RELAY_ON;
    delay(10);
    TRIAC_CTRL_ON; //digitalWrite(AC_pin, HIGH);
  }
  else if (dim > 0 && dim < 255)
  {
    RELAY_ON;
    //Dimming part, if dim is not 0 and not 255
    delayMicroseconds(34 * (255 - dim));
    TRIAC_CTRL_ON;
    delayMicroseconds(500); //delayMicroseconds(500);
    TRIAC_CTRL_OFF;
  }
  else
    RELAY_OFF;
}

//example : ID001 ACT00 4PARA M0051 80#

int command_explain(char *buf)
{
  //string spilt
  String txt = String(buf);
  if (txt.startsWith(node_id))
  {
    //int node_id = (txt.substring(2, 5)).toInt();
    long node_act = txt.substring(8, 11).toInt();
    int node_param = txt.substring(17, 23).toInt();

    Serial.println("ACT:  " + String(node_act));
    Serial.println("PARAM: " + String(node_param));

    switch (node_act)
    {
    case 0:
      Serial.println("ALL CLOSE");
      dim = 0;
      delay_flag = 0;
      break;

    case 1:
      Serial.println("ALL OPEN");
      dim = 255;
      delay_flag = 0;
      break;

    case 2:
      Serial.println("PWM");
      dim = node_param % 1000;
      delay_flag = 0;
      break;

    case 3:
      Serial.println("PWM DELAY ON");
      dim = node_param % 1000;
      delay_second = node_param / 1000;
      command_time = millis();
      delay_flag = 1;
      break;

    case 4:
      Serial.println("PWM DELAY OFF");
      dim = node_param % 1000;
      delay_second = node_param / 1000;
      command_time = millis();
      delay_flag = -1;
      break;

    case 114:
      Serial.println("CHECK DIM");
      break;

    default:
      Serial.println("UNKNOWN ACT!");
      return 0;
    }
    return 1;
  }

  return 0;
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  pin_init();

  Serial.println("LoRa Dimmer RX Test v1.0!");

  lora_init();

  Serial.println("Lora dimmer Lora RX test begin v1.3");
}

void loop()
{
  //Lora
  if (rf95.available())
  {

    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED_PIN, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char *)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      // Send a reply when ID same
      if (command_explain(buf))
      {
        String back_str = node_id + " REPLY : DIM " + String(dim);
        char data[32];
        back_str.toCharArray(data, sizeof(data));
        rf95.send((uint8_t *)data, sizeof(data));
        rf95.waitPacketSent();
        Serial.println("Sent a reply");
      }
      digitalWrite(LED_PIN, LOW);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }

  //delay

  if (delay_flag != 0)
  {
    if (millis() - command_time > delay_second * 1000)
    {
      if (delay_flag == 1)
      {
        dim = 255;
        Serial.println("Delay Over Turn ON");
      }

      else
      {
        dim = 0;
        Serial.println("Delay Over Turn OFF");
      }

      delay_flag = 0;
    }
  }
}
