//This code is based on code found on the Adafruit website (http://www.adafruit.com/products/3595), Written by Dean Miller for Adafruit Industries
//This code has been modified by group CW2B3, second year Civil Engineering students @ KU Leuven, for a P&0 project.

//The proximity sensor uses I2C to communicate. The device's I2C address is 0x39

#include "Adafruit_APDS9960.h"

//create the APDS9960 object
Adafruit_APDS9960 apds;

void setup() {
  Serial.begin(9600);

  if(!apds.begin()){
    Serial.println("failed to initialize the proximity sensor! Please check your wiring.");
  }
  else Serial.println("Proximity sensor initialized!");

  //enable proximity mode
  apds.enableProximity(true);
}

void loop() {

    //print the proximity readings in the serial monitor
    //Serial.println(apds.readProximity());
    //delay(200);

    //action is taken if proximity goes over a certain value
    if ((apds.readProximity()) > 2) {
      Serial.println("SLEEPMODE OFF");
    }
    

    //clear the interrupt
    apds.clearInterrupt();
  
}
