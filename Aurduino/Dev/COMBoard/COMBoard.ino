#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Arduino.h>
#include "HX711.h"
int angle = 90;
float pressTime = 0;
const int buttonpin1 = 15;
const int buttonpin2 = 14;
const int buttonpin3 = 10;//SET PIN NUMBER BASED ON SOLDERING//
const int LEDpin = 27;
const int servo1Open = 11;//SET PIN NUMBER BASED ON SOLDERING//
const int servo2Open = 12;//SET PIN NUMBER BASED ON SOLDERING//
const int DAQIndicator = 13;//SET PIN NUMBER BASED ON SOLDERING//
const int COMIndicator = 18;//SET PIN NUMBER BASED ON SOLDERING//
String success;
int servo1_curr = 90;
int servo2_curr = 90;
int incomingS1 = 0;
float incomingPT1 = 0;
float incomingPT2 = 0;
float incomingPT3 = 0;
float incomingPT4 = 0;
float incomingPT5 = 0;
float incomingFM = 0;
float incomingLC1 = 0;
float incomingLC2 = 0;
float incomingLC3 = 0;
esp_now_peer_info_t peerInfo;
bool pressed1 = false;
bool pressed2 = false;
bool pressed3 = false;
bool prevPressed = false;
bool valveOpened = false;
bool armed = false;
float button1Time = 0;
float currentTime = 0;
float currTime = 0;
float loopTime = 0;
int closedAngle1 = 0;//SET ANGLE
int closedAngle2 = 0; //Set ANGLE
float receiveTimeDAQ = 0;
float receiveTimeCOM = 0;


//SET IF PLOTTING WITH MATLAB OR NOT. SERVO MANUAL CONTROL AND
//MATLAB PLOTTING ARE NOT COMPATIBLE DUE TO USING THE SAME SERIAL
//INPUT. IF TRUE, PLOTTING ENABLED. IF FALSE, MANUAL CONTROL ENABLED
bool MatlabPlot = true;
int state = 0;
//

void shutdownISR() {
  state = 3;
}


//DAQ Breadboard {0x24, 0x62, 0xAB, 0xD2, 0x85, 0xDC}
//DAQ Protoboard {0x0C, 0xDC, 0x7E, 0xCB, 0x05, 0xC4}
uint8_t broadcastAddress[] = {0x0C, 0xDC, 0x7E, 0xCB, 0x05, 0xC4}; //change to new Mac Address

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    float pt1;
    float pt2;
    float pt3;
    float pt4;
    float pt5;
    float lc1;
    float lc2;
    float lc3;
    float fm;
    int S1;
    int S2;
    int S1S2;
    boolean I;
} struct_message;

// Create a struct_message called Readings to recieve sensor readings remotely
struct_message incomingReadings;

// Create a struct_message to send commands
struct_message Commands;

//
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
 // Serial.print("\r\nLast Packet Send Status:\t");
 // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0){
    success = "Delivery Success :)";
    digitalWrite(DAQIndicator, HIGH);
    receiveTimeDAQ = millis();
  }
  else{
    success = "Delivery Fail :(";
    digitalWrite(DAQIndicator, LOW);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
 // Serial.print("Bytes received: ");
  //Serial.println(len);
  incomingPT1 = incomingReadings.pt1;
  incomingPT2 = incomingReadings.pt2;
  incomingPT3 = incomingReadings.pt3;
  incomingPT4 = incomingReadings.pt4;
  incomingPT5 = incomingReadings.pt5;
  incomingFM = incomingReadings.fm;
  incomingLC1 = incomingReadings.lc1;
  incomingLC2 = incomingReadings.lc2;
  incomingLC3 = incomingReadings.lc3;
  incomingS1 = incomingReadings.S1;
  digitalWrite(COMIndicator, HIGH);
  receiveTimeCOM = millis();

}

void setup() {
  Commands.S1 = 90;
  Commands.S2 = 90;
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(buttonpin1,INPUT);
  pinMode(buttonpin2, INPUT);
  pinMode(buttonpin3, INPUT);
  digitalWrite(LEDpin,LOW);
  digitalWrite(servo1Open, LOW);
  digitalWrite(servo2Open, LOW);
  digitalWrite(DAQIndicator, LOW);
  digitalWrite(COMIndicator, LOW);

  //set device as WiFi station
  WiFi.mode(WIFI_STA);

  //initialize ESP32
   if (esp_now_init() != ESP_OK) {
    //Serial.println("Error initializing ESP-NOW");
    return;
  }
   // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    //Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  attachInterrupt(digitalPinToInterrupt(buttonpin3), shutdownISR, RISING);

}


void loop() {
  // put your main code here, to run repeatedly:
  // STATES:
  // 0 = RESTING
  // 1 = ARMED
  // 2 = ACTUATE VALVES ON PRE_SET PARAMETERS (IF USING MATLAB)
  //     READY FOR SERVO VALVE ANGLE INPUTS (IN FORM angle1,angle2)
  switch (state) {
    case 0:
      //Serial.println("IN CASE 0");
      digitalWrite(LEDpin, LOW);
      pressed1 = digitalRead(buttonpin1);
      currTime = millis();
      if (pressed1 && ((currTime - button1Time) > 1000)) {
        state = 1;
        button1Time = currTime;
        Serial.println("State 1");
        //Serial.println("BUTTON 1 GOOD");
      }
      if ((millis() - receiveTimeDAQ) > 500) {
        digitalWrite(DAQIndicator, LOW);
      }
      if ((millis() - receiveTimeCOM) > 500) {
        digitalWrite(COMIndicator, LOW);
      }
      break;
    case 1:
      //Serial.println("IN CASE 1");
      digitalWrite(LEDpin, HIGH);
      pressed2 = digitalRead(buttonpin2);
      pressed3 = digitalRead(buttonpin1);
      currTime = millis();
      if (pressed2) {
        state = 2;
        Serial.println("State 2");
      }
      if (pressed3 && ((currTime - button1Time) > 1000)) {
        button1Time = currTime;
        state = 0;
        Serial.println("State 0");
      }
      if ((millis() - receiveTimeDAQ) > 500) {
        digitalWrite(DAQIndicator, LOW);
      }
      if ((millis() - receiveTimeCOM) > 500) {
        digitalWrite(COMIndicator, LOW);
      }
      break;
    case 2:
      //Serial.println("IN CASE 2");
      if (MatlabPlot) {
        Commands.S1 = 90 - servo1_curr;
        Commands.S2 = 90 - servo2_curr;
        servo1_curr = 90 - servo1_curr;
        servo2_curr = 90 - servo2_curr;
        pressTime = millis();
        while (millis() - pressTime <= 5000) {
          pressed3 = digitalRead(buttonpin1);
          if (pressed3) {
            state = 0;
            break;
          }
          if (state == 3) {
            break;
          }
          if ((millis() - receiveTimeDAQ) > 500) {
            digitalWrite(DAQIndicator, LOW);
          }
          if ((millis() - receiveTimeCOM) > 500) {
            digitalWrite(COMIndicator, LOW);
          }

          if ((millis() - loopTime) >= 50) {
            esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &Commands, sizeof(Commands));
            if (result != ESP_OK) {
              break;
            // Serial.println("Sent with success");
            }
            //else {
              //Serial.println("Error sending the data");
            //}
            loopTime = millis();
            Serial.print(loopTime);
            Serial.print(" ");
            // Print the flow rate for this second in litres / minute
            // Serial.print("Flow rate: ");
            //Serial.print(incomingFM);  // Print the integer part of the variable
            //Serial.print("L/min");
            //Serial.print(" ");       // Print tab space




           //PT TEST
            //Serial.print("Output Pressures: ");
            Serial.print(incomingPT1);
            Serial.print(" ");
            Serial.print(incomingPT2);
            Serial.print(" ");
            Serial.print(incomingPT3);
            Serial.print(" ");
            Serial.print(incomingPT4);
            Serial.print(" ");
            Serial.print(incomingPT5);
            Serial.print(" ");
            //Serial.println("psi / ");

          //  Serial.print(" ");       // Print tab space

          // FM Test

            Serial.print(incomingFM);
            Serial.print(" ");

            //Serial.println("Commands Follow");
            Serial.print(Commands.S1);
            Serial.print(" ");
            Serial.println(Commands.S2);
             // Print the cumulative total of litres flowed since starting
            //Serial.print("Output Liquid Quantity: ");
             //Serial.print(totalMilliLitres);
             //Serial.print("mL / ");
             //Serial.print(totalMilliLitres / 1000);
             //Serial.print("L");
             //Serial.print("\t");       // Print tab space

           //LC TEST

           //Serial.print("Load Cell Stuff:");
           //Serial.print(incomingLC1);
           //Serial.print(incomingLC2);
           //Serial.print(incomingLC3);

           //delay(50); //delay of 50 optimal for recieving and transmitting
         }
        }

      } else {
        while (!Serial.available()) {
          pressed3 = digitalRead(buttonpin1);
          delay(5);
          if ((millis() - receiveTimeDAQ) > 500) {
            digitalWrite(DAQIndicator, LOW);
          }
          if ((millis() - receiveTimeCOM) > 500) {
            digitalWrite(COMIndicator, LOW);
          }
          esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &Commands, sizeof(Commands));
            if (result != ESP_OK) {
              break;
            // Serial.println("Sent with success");
            }
            //else {
              //Serial.println("Error sending the data");
            //}
          if (pressed3) {
            state = 0;
            Serial.println("State 0");
            break;}  //waiting for inputs
          }
        String angles = Serial.readString();
        int index = angles.indexOf(",");
        String readAngle1 = angles.substring(0, index);
        String readAngle2 = angles.substring(index+1, angles.length());
        if (angles.length() >= 3) {
          Serial.println(readAngle1);
          Serial.println(readAngle2);
          Commands.S1 = readAngle1.toInt();
          Commands.S2 = readAngle2.toInt();
          if (Commands.S1 == closedAngle1) {
            digitalWrite(servo1Open, LOW);
          } else {
            digitalWrite(servo1Open, HIGH);
          }

          if (Commands.S2 == closedAngle2) {
            digitalWrite(servo2Open, LOW);
          } else {
            digitalWrite(servo2Open, HIGH);
          }
          esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &Commands, sizeof(Commands));
          if (result != ESP_OK) {
              break;
            // Serial.println("Sent with success");
            }
            //else {
              //Serial.println("Error sending the data");
            //}

        }

      }
      break;
     case 3:
      Commands.S1 = closedAngle1;
      Commands.S2 = closedAngle2;
      digitalWrite(DAQIndicator, LOW);
      digitalWrite(COMIndicator, LOW);
      digitalWrite(servo1Open, LOW);
      digitalWrite(servo2Open, LOW);
      break;
  }


}