
//Fingerprint scanner
#include <Adafruit_Fingerprint.h>                               //Library for the fingerprint scanner
#include <SoftwareSerial.h>                                     //Library for Software Serial
SoftwareSerial mySerial(2, 3);                                  //Pin 2 is IN from sensor, pin 3 is OUT from Arduino
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);  //No hardware serial on the UNO, so we must use software serial
uint8_t id;                                                     //Create an unassigned integer 'id' with the length of 8 bits for the fingerprint 'enroll' procedure

//Proximity Sensor
#include "Adafruit_APDS9960.h"                                  //Library for the proximity sensor
Adafruit_APDS9960 apds;                                         //Create the APDS9960 object for Proximity Sensor

//Keypad
#include <Keypad.h>                                             //Library for the keypad
int correct_pincode = 0;                                        //If this integer is 1, a correct pincode was given
char Password[4] = {'1','1','1','1'};                           //Standard password (never used, just to create 'Password')

//Lock solenoid
int solenoid = 5;                                               //Pin number for the solenoid






void setup() {

  Serial.begin(9600);

//SETUP CODE FOR THE PROXIMITY SENSOR

  if(!apds.begin()){
    Serial.println("error");                 //Warn the Raspberry Pi if the proximity sensor isn't detected
  }

  //enable proximity mode
  apds.enableProximity(true);
  

//SETUP CODE FOR THE FINGERPRINT SENSOR

  // set the data rate for the sensor serial port
  finger.begin(57600);
  
  if (!finger.verifyPassword()) {
    Serial.println("error");                 //Warn the Raspberry Pi if the fingerprint sensor isn't detected
  } 

  finger.getTemplateCount();               //"finger.templateCount" gives the amount of fingerprints stored in the sensor
  

//SETUP CODE FOR THE SOLENOID

  pinMode(solenoid, OUTPUT);               //Define pin 5 as an output pin for the solenoid

}






void loop() {                                      //The Arduino will run the "authenticate()" function, except when the Raspberry Pi requests the "new_user()" function
  
  String procedure = "authenticate";               //Standard order is to authenticate            

  procedure = raspberry_read();                    //Read orders coming from the Raspberry Pi
 
  if (procedure == "authenticate") {               //Authenticate
    authenticate();
  }
    

  if (procedure == "new_user") {                  //New user procedure
    new_user();
    procedure = "authenticate";                   //Put back in authenticate mode 
  }
}




void authenticate() {                       //Main function that will authenticate users with the various sensors


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

      Serial.println("pincode");                      //Triggers the Raspberry Pi to send a randomly generated pincode that is also sent to the user

      String pincode = "authenticate";
      while (pincode == "authenticate") {             //Standard message returned by raspberry_read() is "authenticate", so keep reading untill a pincode is received
        pincode = raspberry_read();
        }

      pincode.toCharArray(Password, 5);               //Reads the pincode received from the Arduino and changes the Password to it

      keypad_pincode();                               //Keypad procedure 
      

      if (correct_pincode == 1) {
        
        open_door();                                  //Unlock the door
        correct_pincode = 0;                          //Reset the pincode controller to 0
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
  byte rowPins[ROWS] = {13, 12, 11, 10};                          //Connect to the row pinouts of the keypad
  byte colPins[COLS] = {9, 8, 7};                                 //Connect to the column pinouts of the keypad
  Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
  char inputArray[4];                                             //Password length
  int i = 0;                                                      //Keycounter for keypad
  //char key; 
  
  while (correct_pincode != 1) {

    char key = kpd.getKey();
  
    //if a key is pressed
    if(key) {
      inputArray[i] = key; //store entry into array
      i++;
      //Serial.print(key); //print keypad character entry to serial port
      }

    if (key=='*') {
      //Serial.println("");
      //Serial.println("Reset");
      i=0; //reset i
      }

    if (i == 4) {     //if 4 presses have been made 
      {
      //match input array to password array
      if (inputArray[0] == Password[0] &&
      inputArray[1] == Password[1] &&
      inputArray[2] == Password[2] &&
      inputArray[3] == Password[3]) {
        //Action if code is correct:
        correct_pincode = 1;         
        }
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
      Serial.println("take_picture");  
      delay(100);             
      }
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

void open_door() {                 //This function will open the lock solenoid when called

  digitalWrite(solenoid, HIGH);    //Switch Solenoid ON
  delay(10000);                    //Give the user 10 seconds to open the door
  digitalWrite(solenoid, LOW);     //Switch Solenoid OFF
  }


//CODE TO READ SERIAL INPUT FROM RASPBERRY PI

String raspberry_read() {

  String message = "authenticate";          //Standard message is "authenticate"
  
  if (Serial.available() > 0) {             //if data is being received
    message = Serial.readString();          //Read the string being sent from the Raspberry Pip
  } 
  return message;
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

