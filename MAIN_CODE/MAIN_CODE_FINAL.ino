
//Fingerprint scanner
#include <Adafruit_Fingerprint.h>                               //Library for the fingerprint scanner
#include <SoftwareSerial.h>                                     //Library for Software Serial
SoftwareSerial mySerial(2, 3);                                  //Pin 2 is IN from sensor, pin 3 is OUT from Arduino
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);  //No hardware serial on the UNO, so we must use software serial
uint8_t id;                                                     //Create an unassigned integer 'id' with the length of 8 bits for the fingerprint 'enroll' procedure

//Proximity Sensor
#include "Adafruit_APDS9960.h"                                  //Library for the proximity sensor
Adafruit_APDS9960 apds;                                         //Create the APDS9960 object for the Proximity Sensor

//Keypad
#include <Keypad.h>                                             //Library for the keypad
int correct_pincode = 0;                                        //If this integer is 1, a correct pincode was given
char Password[4] = {'1','1','1','1'};                           //Standard password (never used, just to create 'Password')

//Lock solenoid
int solenoid = 5;                                               //Pin number for the solenoid is 5

//Push button
int buttonPin = 6;                                              //Pin number for the push button is 6
int button_pushed = 0;                                          //Integer that is 1 when the button is pushed, 0 when the button is not pushed




void setup() {

//Global 
  Serial.begin(9600);                        //Set the baud rate for the serial communications 
  
//Fingerprint scanner
  finger.begin(57600);                       //Set the data rate for the sensor serial port
  if (!finger.verifyPassword()) {
    Serial.println("error");                 //Warn the Raspberry Pi if the fingerprint sensor isn't detected
  } 
  finger.getTemplateCount();                 //"finger.templateCount" gives the amount of fingerprints stored in the sensor

//Proximity sensor
  if(!apds.begin()){
    Serial.println("error");                 //Warn the Raspberry Pi if the proximity sensor isn't detected
  }
  apds.enableProximity(true);                //Enable Proximity Mode (the sensor has three modes: Color, Gesture & Proximity)
  
//Solenoid
  pinMode(solenoid, OUTPUT);                 //Define pin 5 as an output pin for the solenoid
  
//Push button
  pinMode(buttonPin, INPUT);                 //Define pin 6 as an input pin for the push button
  
}




void loop() {

//This allows the user to unlock the door from the INSIDE of the house to go outside or to let someone in:
  button();                                   
  if (button_pushed == 1) {                        
    digitalWrite(solenoid, HIGH);                  
    delay(10);
    digitalWrite(solenoid,LOW);
    button_pushed = 0;
  }
  
  String procedure = "authenticate";               //Standard order is to authenticate            

  procedure = raspberry_read();                    //Read orders coming from the Raspberry Pi
 
  if (procedure == "authenticate") {               //Authenticate
    authenticate();
  }
    

  if (procedure == "new_user") {                   //New user procedure
    new_user();
    procedure = "authenticate";                    //Put back in authenticate mode 
  }
}




void authenticate() {                       //Main function that will take the fingerprint of a user when proximity is detected and communicate with the Raspberry Pi 
                                            //and possibly unlock the door


  int state = 0;                            //State = 1 ==> Raspberry scans for faces / State = 2 ==> Raspberry sends a pincode and keypad procedure is started
  
  if ((proximity()) == true) {              //Nothing happens as long no motion/proximity is detected

     //Once motion is detected, give the user 6 attempts to scan their finger
     int attempts = 0;
     while (attempts < 7) {
        int starttime = millis();
        int endtime = starttime;
        while (((endtime - starttime) <=2000)) {
          getFingerprintIDez();                       //Start scanning for fingerprints
          delay(50);
          endtime = millis();
          }
        
        if (finger.confidence > 50) {                 //Limiting factor: if confidence isn't high enough, acces is denied
          attempts = 0;
          state = 1;
          break;
          }
        
        if (attempts == 6) {                          //6 failed attempts
          state = 2;
          }
        
        ++attempts;                                   //increments 'attemps' by one
     }
    
     if (state == 1) {         
      
      Serial.print("fingerprint_ID:");
      Serial.println(finger.fingerID);                        //Triggers the Raspberry Pi to start scanning for the face of the correct user
      finger.confidence = 0;                                  //Put finger.confidence back to 0, else the code will think there is always a correct fingerprint

      String facial_recognition = ".";
      while (! (facial_recognition == "correct_face" ||  facial_recognition == "failed_face")) {
        facial_recognition = raspberry_read();
        delay(500);
        }
      
      if (facial_recognition == "correct_face") {
        
        open_door();                                   //Unlock the door
        } 

      if (facial_recognition == "failed_face") {
        state = 2;
        }
     } 

     if (state == 2) {

      Serial.println("pincode");                      //Triggers the Raspberry Pi to send a randomly generated pincode to the Arduino that is also sent to the user 
                                                      //if the user requests a backup password

      String pincode = "authenticate";
      while (pincode == "authenticate") {             //Standard message returned by raspberry_read() is "authenticate", so keep reading untill a pincode is received
        pincode = raspberry_read();
        }

      if (pincode == "abort") {                       //If the user doesn't request a backup password on the website, the lock will go back to authenticating
        authenticate();
        
      } else {                                     

       pincode.toCharArray(Password, 5);               //Reads the pincode received from the Arduino and changes the Password to it

       keypad_pincode();                               //Keypad procedure 
      

       if (correct_pincode == 1) {
        
         open_door();                                  //Unlock the door
         correct_pincode = 0;                          //Reset the pincode controller to 0
         } 
       }
     }
  }
}




void keypad_pincode() {

  const byte ROWS = 4; //four rows
  const byte COLS = 3; //three columns
  char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
  };
  byte rowPins[ROWS] = {12, 13, 7, 8};                                    //Define row pinouts of the keypad
  byte colPins[COLS] = {9, 11, 10};                                       //Define column pinouts of the keypad
  Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );  //Maps the keys to the pins
  char inputArray[4];                                                     //Password length
  int i = 0;                                                              //Keycounter for keypad 

  int starttime = millis();
  int endtime = starttime;
  while (correct_pincode != 1) {
    if (((endtime - starttime) > 120000)) {          //User has one minute to put in a valid pincode
      break;
    }

    char key = kpd.getKey();
  
    if(key) {                                        //If a key is pressed
      inputArray[i] = key;                           //Store entry into array
      i++;
      }

    if (key=='*') {
      i=0;                                           //Reset i
      getFingerprintIDez();
      }

    if (i == 4) {                                    //If 4 presses have been made 
      {
      //Match input array to password array
      if (inputArray[0] == Password[0] &&
      inputArray[1] == Password[1] &&
      inputArray[2] == Password[2] &&
      inputArray[3] == Password[3]) {
        //Action if code is correct:
        correct_pincode = 1;       
        } else {
          getFingerprintIDez();
        }
      i=0;                                            //Reset i
      }
    } 
  }
}




void new_user() {                         //Function that will run when a new user wants to be put in the database

  id = readnumber();
  if (id == 0) {    // ID #0 not allowed, try again!
     return;
  }

  getFingerprintEnroll();

  Serial.println("finger_success");          //Confirms to the Raspberry Pi that the new fingerprint is succesfully stored                           

  //Start of the picture procedure for the facial recognition, this will take a picture when the user shows proximity untill 5 pictures are taken
  //These pictures are stored on the Raspberry Pi and are used to train the facial recognition algorithm to recognize the user
  
  while (raspberry_read() != "picture_procedure") {     //Waiting for the Raspberry Pi to start the picture procedure
    }

  delay(1000);

  int picture_count = 0;
  while (picture_count < 5) {                           //Take a picture when the user shows proximity untill 5 pictures are taken
    if (proximity() == true) {
      Serial.println("take_picture");                   //Commands the Raspberry Pi to take a picture with the attached camera
      while (raspberry_read() != "picture_taken") {     //Waiting for the Raspberry Pi to take a picture
      //wait            
      }
      getFingerprintIDez();                             //This lights up the fingerprint scanner LED to let the user know the picture was taken
      ++picture_count;
      
    }
  }
}




bool proximity() {                        //Action is taken if proximity goes over a certain value

  if ((apds.readProximity()) > 50) {
    return true;
  } else {
    return false;
  }
}




void button() {
  if (digitalRead(buttonPin) == HIGH) {
    button_pushed = 1;
  } 
}




void open_door() {                  //This function will open the lock solenoid when called

  digitalWrite(solenoid, HIGH);     //Switch Solenoid ON
  delay(7000);                      //Give the user 7 seconds to open the door
  digitalWrite(solenoid, LOW);      //Switch Solenoid OFF
}



  
String raspberry_read() {                   //Code to read strings from the serial input from the Raspberry Pi

  String message = "authenticate";          //Standard message is "authenticate"
  
  if (Serial.available() > 0) {             //If data is being received
    message = Serial.readString();          //Read the string being sent from the Raspberry Pi
  } 
  return message;
}




uint8_t readnumber(void) {                  //Code to read numbers from the serial input from the Raspberry Pi
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
      //Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      //Serial.println("Imaging error");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    //Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    //Serial.println("Did not find a match");
    return p;
  } else {
    //Serial.println("Unknown error");
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
  //Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      //Serial.println("Imaging error");
      break;
    default:
      //Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }
  
  //Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  //Serial.print("ID "); Serial.println(id);
  p = -1;
  //Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      //Serial.println("Imaging error");
      break;
    default:
      //Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  //Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    //Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    //Serial.println("Fingerprints did not match");
    getFingerprintEnroll();
    //return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }   
  
  //Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    //Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    //Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    //Serial.println("Error writing to flash");
    return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }   
}


