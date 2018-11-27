
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
int correct_pincode = 0;                                        //If this integer is 1, a correct pincode was given
char Password[4] = {'1','1','1','1'};                           //Standard password

//Lock solenoid
int solenoid = 5;                                               //Pin number for the solenoid






void setup() {

  Serial.begin(9600);

//SETUP CODE FOR THE PROXIMITY SENSOR

  if(!apds.begin()){
    Serial.print("error");
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
    Serial.print("error");
    while (1) { delay(1); }
  }

  finger.getTemplateCount();
  //Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  //Serial.println("Waiting for valid finger...");

//SETUP CODE FOR THE SOLENOID

  pinMode(solenoid, OUTPUT);

}






void loop() {
  // put your main code here, to run repeatedly:

  
  String procedure = "authenticate";               //Standard order is to authenticate            

  procedure = raspberry_read();                    //Read orders coming from the Raspberry Pi
 
  if (procedure == "authenticate") {               //Authenticate
    authenticate();
  }
    

  if (procedure == "new_user") {               //New user procedure
    new_user();
    procedure = "authenticate";                //Put back in authenticate mode 
  }
}




void authenticate() {                       //Main function that will authenticate users with the various sensors


  int state = 0;                            //State = 1 ==> Raspberry scans for faces / State = 2 ==> Raspberry sends a pincode and keypad procedure is started
  
  if ((proximity()) == true) {              //Nothing happens as long no motion/proximity is detected

    //Serial.println("(Proximity detected)");

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
          //Serial.println("(Correct fingerprint)");
          attempts = 0;
          state = 1;
          break;
        }
        
        if (attempts == 6) {                          //6 failed attempts
          //Serial.println("(6 failed attempts ==> keypad procedure)");
          state = 2;
        }
        
        ++attempts;                                   //increments 'attemps' by one
        //Serial.print("(");Serial.print(6-attempts);Serial.println("attempts left)");
     }
    
     if (state == 1) {         
      
      Serial.print("fingerprint_ID:");Serial.print(finger.fingerID);     //Triggers the Raspberry Pi to start scanning for the face of the correct user
      finger.confidence = 0;                                  //Put finger.confidence back to 0, else the code will think there is always a correct fingerprint

      String facial_recognition = ".";
      while (! (facial_recognition == "correct_face" ||  facial_recognition == "failed_face")) {
        //Serial.println(".");
        facial_recognition = raspberry_read();
        delay(500);
      }
      
      if (facial_recognition == "correct_face") {

        //Serial.println("(Correct face ==> opening lock)");
        //Serial.println("sleep");
        
        //Unlock the door:
        digitalWrite(solenoid, HIGH);    //Switch Solenoid ON
        delay(10000);                    //Give the user 10 seconds to open the door
        digitalWrite(solenoid, LOW);     //Switch Solenoid OFF
        
        
      } 

      if (facial_recognition == "failed_face") {
        state = 2;
      }
     } 

     if (state == 2) {

      Serial.print("pincode");                        //Triggers the Raspberry Pi to send a randomly generated pincode that is also sent to the user

      String pincode = "authenticate";
      while (pincode == "authenticate") {             //Standard message returned by raspberry_read() is "authenticate", so keep reading untill a pincode is received
        pincode = raspberry_read();
      }

      pincode.toCharArray(Password, 5);      //Reads the pincode received from the Arduino and changes the Password to it

      //Serial.print("(Generated pincode = ");Serial.print(Password);Serial.println(")");
      
      //Keypad procedure
      keypad_pincode();
      

      if (correct_pincode == 1) {
        //Serial.println("(Correct pincode ==> opening lock)");
        
        //Unlock the door:
        
        digitalWrite(solenoid, HIGH);    //Switch Solenoid ON
        delay(10000);                    //Give the user 10 seconds to open the door
        digitalWrite(solenoid, LOW);     //Switch Solenoid OFF
        
        correct_pincode = 0;             //Reset the pincode controller to 0
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


    if (key=='*')
    {
      //Serial.println("");
      //Serial.println("Reset");
      i=0; //reset i
    }

   
    

  if (i == 4) //if 4 presses have been made
    {
      {

    //match input array to password array
    if (inputArray[0] == Password[0] &&
    inputArray[1] == Password[1] &&
    inputArray[2] == Password[2] &&
    inputArray[3] == Password[3])
       {
        //Action if code is correct:
        correct_pincode = 1;      
            
       }
      }
      
    } 
   
}

}






void new_user() {                         //Function that will run when a new user wants to be put in the database

  //Serial.println("(Ready to enroll a fingerprint!)");
  //Serial.println("(Please type in the ID # (from 1 to 127) you want to save this finger as...)");
  //Serial.println("user_id");            //Triggers the Raspberry Pi to enter a user_id
  id = readnumber();
  if (id == 0) {// ID #0 not allowed, try again!
     return;
  }
  //Serial.print("(Enrolling ID #");
  //Serial.print(id);Serial.println(")");
  
  getFingerprintEnroll();

  Serial.print("finger_succes");          //Confirms to the Raspberry Pi that the new fingerprint is succesfully stored
  
}





bool proximity() {                        //Action is taken if proximity goes over a certain value
  
  if ((apds.readProximity()) > 50) {
    return true;
  } else {
    return false;
  }
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


