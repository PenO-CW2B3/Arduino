//This code is based on code found on the Arduino Forum (https://forum.arduino.cc/index.php?topic=83846.0), coded by 'ironicDeveloper' on 12/18/2011.
//This code has been modified by group CW2B3, second year Civil Engineering students @ KU Leuven, for a P&0 project.


//Libraries
#include <Keypad.h>


//Variables
int i = 0;

//Keypad declarations
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
 {'1','2','3'},
 {'4','5','6'},
 {'7','8','9'},
 {'*','0','#'}
};
byte rowPins[ROWS] = {13, 12, 11, 10}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7}; //connect to the column pinouts of the keypad
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
char inputArray[4]; //password length
char Password[4] = {'1','4','3','6'}; //Password



void setup() {
  
  Serial.begin(9600); //open serial port

}



void loop() {
  
 char key = kpd.getKey();
 
 //if a key is pressed
   if(key) {
  inputArray[i] = key; //store entry into array
  i++;
  Serial.print(key); //print keypad character entry to serial port

  if (key=='*')
    {
      Serial.println("");
      Serial.println("Reset");
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
        Serial.println("");
        Serial.println("Correct password");

               
       }
      }
      {
      i=0; //reset i
      }
    }
  }
}


