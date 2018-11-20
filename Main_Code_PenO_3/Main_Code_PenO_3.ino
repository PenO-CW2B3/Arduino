
//Serial communication Arduino Raspberry Pi ==> https://www.instructables.com/id/Raspberry-Pi-Arduino-Serial-Communication/ 


//Global variable integers
int procedure = 1;                                              //1 = authentication procedure, 2 = keypad pincode procedure, 3 = new user procedure
int facial_recognition = 0;                                     //0 = no correct face detected, 1 = correct face detected
int state = 0;                                                  //0=no motion, 1=motion, 2=motion & correct fingerprint, 3=failed attempt, 4=correct pincode after failed attempt

//Fingerprint scanner
#include <Adafruit_Fingerprint.h>                               //Library for the fingerprint scanner
#include <SoftwareSerial.h>                                     //Library for Software Serial
SoftwareSerial mySerial(2, 3);                                  //Connect to the data pins 2 & 3 for the fingerprint sensor
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial); 
uint8_t id;                                                     //Create an unassigned integer 'id' with the length of 8 bits

//Proximity Sensor
#include "Adafruit_APDS9960.h"                                  //Library for the proximity sensor
Adafruit_APDS9960 apds;                                         //Create the APDS9960 object for Proximity Sensor

//Keypad
#include <Keypad.h>                                             //Library for the keypad
int i = 0;                                                      //Keycounter for keypad
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
 {'1','2','3'},
 {'4','5','6'},
 {'7','8','9'},
 {'*','0','#'}
};
byte rowPins[ROWS] = {13, 12, 11, 10};                          //Connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7};                                 //Connect to the column pinouts of the keypad
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
char inputArray[4];                                             //Password length
char Password[4] = {'1','4','3','6'};                           //Password






void setup() {

  Serial.begin(9600);

//SETUP CODE FOR THE PROXIMITY SENSOR

  if(!apds.begin()){
    Serial.println("failed to initialize the proximity sensor! Please check your wiring.");
  }
  //else Serial.println("Proximity sensor initialized!");

  //enable proximity mode
  apds.enableProximity(true);
  

//SETUP CODE FOR THE FINGERPRINT SENSOR

  // set the data rate for the sensor serial port
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    //Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  finger.getTemplateCount();
  //Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  //Serial.println("Waiting for valid finger...");

}






void loop() {
  // put your main code here, to run repeatedly:

  //PROCEDURE: 1 = authentication procedure, 2 = keypad pincode procedure, 3 = new user procedure

  if (Serial.available() > 0) {       //if data is being received
 
    procedure = readnumber();
  } 

  if (procedure > 4) {               //If random number is being sent, ignore it
    procedure = 1;
  }
 
  if (procedure == 1) {               //Authenticate
    authenticate();
  }

  if (procedure == 2) {               //Keypad pincode procedure
    Serial.println("STARTING KEYPAD PINCODE PROCEDURE");
    
    while (state != 4) {              //Only when the correct pincode is given, de keypad will stop reading codes and authentication will continue
     keypad_pincode();
    }
    procedure = 1;
    
  }

  if (procedure == 3) {               //New user procedure
    Serial.println("STARTING NEW USER PROCEDURE");
    new_user();
    procedure = 1;                    //Put back in authenticate mode 
  }

  if (procedure == 4) {               //Facial recognition ok
    Serial.println("Facial recognition ok");
    facial_recognition = 1;
    delay(2000);
    procedure = 1;                    //Put back in authenticate mode 
  }
}






void authenticate() {                       //Main function that will authenticate users with the various sensors
  
  if ((proximity()) == true) {              //Checking for motion around the door knob
    
     state = 1;
     Serial.println("STEP 1: Proximity detected, scanning for fingers now (state = 1)"); Serial.println("");
    
     int attempts = 0;
     while (attempts < 4) {
        int starttime = millis();
        int endtime = starttime;
        while (((endtime - starttime) <=2000)) {
          getFingerprintIDez();                  //Start scanning for fingerprints
          delay(50);
          endtime = millis();
        }
        if (finger.confidence > 50) {     //Limiting factor: if confidence isn't high enough, acces is denied
          attempts = 0;
          state = 2;
          break;
        }
        if (attempts == 3) {            
          state = 3;
          Serial.println("ERROR: Failed attempt (state = 3)"); Serial.println("");
        }
        ++attempts;                       //increments 'attemps' by one
        Serial.println(attempts);
     }
    
     if (state == 2) {         
      Serial.print("STEP 2: Found ID #"); Serial.print(finger.fingerID); 
      Serial.print(" with confidence of "); Serial.print(finger.confidence); Serial.println(" (state = 2)"); Serial.println("");
      finger.confidence = 0;
      
      if (facial_recognition == 1) {
        Serial.println("Opening lock");
        // open lock
        facial_recognition = 0;
      }
     
     } 
  }
}







void keypad_pincode() {
  
 char key = kpd.getKey();
 
  if(key) {                 //if a key is pressed
  inputArray[i] = key;      //store entry into array
  i++;
  Serial.print(key);        //print keypad character entry to serial port

  if (key=='*')
    {
      Serial.println("");
      Serial.println("Reset");
      i=0; //reset i
    }

  if (i == 4)               //if 4 presses have been made
    {
      {

    //match input array to password array
    if (inputArray[0] == Password[0] &&
    inputArray[1] == Password[1] &&
    inputArray[2] == Password[2] &&
    inputArray[3] == Password[3])
       {
        //Action if code is correct:
        state = 4;
        Serial.println("");
        Serial.println("Correct password");     
       } else {
          Serial.println(""); 
          Serial.println("Wrong password");
       }
      }
      {
      i=0; //reset i
      }
    }
  }
}






void new_user() {                         //Function that will run when a new user wants to be put in the database

  Serial.println("Ready to enroll a fingerprint!");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
  id = readnumber();
  if (id == 0) {// ID #0 not allowed, try again!
     return;
  }
  Serial.print("Enrolling ID #");
  Serial.println(id);
  
  getFingerprintEnroll();
  
}





bool proximity() {                        //Action is taken if proximity goes over a certain value
  
  if ((apds.readProximity()) > 50) {
    return true;
  } else {
    return false;
  }
}





//CODE TO READ NUMBERS FROM SERIAL PORT
uint8_t readnumber(void) {
  uint8_t num = 0;
  
  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}






//CODE FOR THE FINGERPRINT SCANNING
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  //Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  //Serial.print(" with confidence of "); Serial.println(finger.confidence); 

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  
  // found a match!
  //Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  //Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID; 
}







//CODE FOR FINGERPRINT ENROLL

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
}


