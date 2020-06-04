// ***********************************************************
// ** Include external libraries                            **
// ***********************************************************
#include <Password.h> //http://playground.arduino.cc/uploads/Code/Password.zip use password library<br>
#include <Keypad.h>  //http://www.arduino.cc/playground/uploads/Code/Keypad.zip  //tells to use keypad library
#include <MPU6050.h>
#include <EEPROM.h>



// BASTIAAN -- TEST PASSWORD CHANGE
char pass[4], pass1[4];
char customKey = 0;
// BASTIAAN -- TEST PASSWORD CHANGE




// ******************************************
// ** BEGIN OF USER CHANGABLE VARIABLES    **
// ******************************************
// **************************************************************
// ** PINCODE ; can be set to custom pincode                   **
// **************************************************************
Password password = Password((char *)"1234");

// **************************************************************
// ** Sensitivity of alarm ; can be set to custom values       **
// **************************************************************
const float float_triggerspeed=1.5; // Speed that triggers the alarm in km/h
const int int_triggermovement=5; // Number of movements that triggers the alarm

const boolean debug=true; // Sets output to monitor on (true) or off (false)
// ******************************************
// ** END OF USER CHANGABLE VARIABLES      **
// ******************************************



// ***********************************************************
// ** Declaration of variables and constants for MPU6050    **
// ***********************************************************
MPU6050 accelgyro;

const float CONST_16G = 2048;
const float CONST_G = 9.81;
const float KMPH = 3.6;

unsigned long last_read_time;
int16_t ax, ay, az, gx, gy, gz;
int16_t ax_offset, ay_offset;

// **************************************************************
// ** Declaration of variables and constants of keypad         **
// ** Keypad ROW0, ROW1, ROW2 and ROW3 goes to Arduino 0,1,2,3 **
// ** Keypad COL0, COL1, COL2 and COL3 goes to Arduino 4,5,6,7 **
// **************************************************************
const byte ROWS = 4; // Four rows
const byte COLS = 4; // columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = { 0, 1, 2, 3 };
byte colPins[COLS] = { 4, 5, 6, 7 };
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// **************************************************************
// ** Declaration of parts and connection to Arduino pins      **
// **************************************************************
const int int_buzzer = 8;
const int int_greenled = 19;
const int int_redled = 18;

// **************************************************************
// ** Declaration of triggers                                  **
// **************************************************************
boolean boolean_isarmed = false;
int int_goalarm = 0;

// **************************************************************
// ** Begin program ; First the setup                          **
// **************************************************************
void(* resetFunc) (void) = 0;
void setup() {
  for (int j = 0; j < 4; j++) {
    pass[j] = EEPROM.read(j);
  }

  if(debug) {
    Serial.begin(9600);
    Serial.println("Begin setup...");
  
// initialize MPU6050
    Serial.println("initializing MPU6050...");
  }
  accelgyro.initialize();

// test MPU6050 connection
  if(debug) {
    Serial.println("testing MPU6050 connection...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
  }
  
  accelgyro.setFullScaleAccelRange(0x03);
  accelgyro.setFullScaleGyroRange(0x03);

  if(boolean_isarmed) {
    calibrate_sensors();
  }

  pinMode(int_buzzer, OUTPUT);   // set buzzer always to off
  pinMode(int_greenled, OUTPUT); // set green led always to off
  pinMode(int_redled, OUTPUT);   // set red led always to off

// initialize keypad
  keypad.addEventListener(keypadEvent);

  digitalWrite(int_greenled,HIGH); // set green led to on to show the device is working
}


void loop() {
  if(boolean_isarmed) {
    unsigned long t_now = millis();
    float dt = get_delta_time(t_now);
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float ax_p = (ax - ax_offset) / CONST_16G;
    float ay_p = (ay - ay_offset) / CONST_16G;

    float vel_x = (ax_p * dt * CONST_G);
    float vel_y = (ay_p * dt * CONST_G);
    float vel = sqrt(pow(vel_x, 2) + pow(vel_y, 2)) * KMPH;

    if((int_goalarm < int_triggermovement) && (debug)) {
      Serial.print("\tvel: ");
      Serial.print(vel, 2);
      Serial.print("km/h,");
    }
  
    if(vel>float_triggerspeed) {
      int_goalarm++;
    }

    if (int_goalarm >= int_triggermovement) {    // Alarm is triggered here!
      if(debug) { 
        Serial.print("ALARM IS TRIGGERED!"); 
      }
      digitalWrite(int_greenled, HIGH); // BASTIAAN -- DEZE MOET OP LOW IN FINAL CODE !!
      digitalWrite(int_redled, LOW);
      digitalWrite(int_buzzer, HIGH);
      delay(10);
      keypad.getKey();
      checkPassword();
      digitalWrite(int_greenled, LOW); // BASTIAAN -- DEZE MOET OP LOW IN FINAL CODE !!
      digitalWrite(int_redled, HIGH);
      digitalWrite(int_buzzer, LOW);
      delay(10);
    }

    set_last_time(t_now);
    delay(5);
    calibrate_sensors();
  }
  keypad.getKey();
  checkPassword();
}

// **************************************************************
// ** FUNCTIONS                                                **
// **************************************************************
// *********************
// ** KEYPAD FUNCTION **
// *********************
void keypadEvent(KeypadEvent eKey) {
  switch (keypad.getState()) {
    case PRESSED:
      if(debug) {
        Serial.print("Enter: ");
        Serial.println(eKey);
        Serial.write(254);
      }
      delay(10); // BASTIAAN -- DEZE DELAY KAN WEG VOLGENS MIJ!!
      switch (eKey) {
        case '#': {
             if(debug) {
               Serial.print("# (pound) is pressed");
             }
             if (not password.evaluate()) {  // if password is right open box BASTIAAN -- Huh?
               for (int x = 0; x < 2; x++) {
                 digitalWrite(int_greenled, HIGH);
                 delay(200);
                 digitalWrite(int_greenled, LOW);
                 delay(200);
                 if(debug) {
                   Serial.print(x);
                 }
               }
               password.reset();
               if(debug) {
                 Serial.print("password input is reset");
               }
               delay(1);
             }
             break;
        }
        case '*': {
             if(debug) {
               Serial.print("* (asterix) is pressed");
             }
             if(not boolean_isarmed) {
               password.reset(); 
               for (int x = 0; x < 5; x++) {
                 digitalWrite(int_greenled, HIGH);
                 delay(200);
                 digitalWrite(int_greenled, LOW);
                 digitalWrite(int_buzzer, LOW);
                 delay(200);
               }
               boolean_isarmed = true;
               digitalWrite(int_redled, HIGH);
               delay(1); 
             }
//             password.reset(); // BASTIAAN -- Deze kan weg?
             break;   
        }
        case 'A': {
          password_change();
        }
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
             if(boolean_isarmed) {
               if(debug) {
                Serial.print("a number (0..9) is pressed");
               }
               password.append(eKey);
               delay(1);
             }
        } // end case 0..9
      } // end switch(eKey)
  } // end keypad.GetState
} // end keypadEvent(KeypadEvent eKey)


// *****************************
// ** CHECK PASSWORD FUNCTION **
// *****************************
boolean checkPassword() {
  if (password.evaluate()) {  // if password is correct, open box and make sure alarm is off, turn green led back on
    if(debug) {
      Serial.println("password is correct");
      Serial.write(254);
    }
    digitalWrite(int_greenled, HIGH);
    digitalWrite(int_buzzer, LOW);
    boolean_isarmed = false;
    digitalWrite(int_redled, LOW);
    int_goalarm = 0;
    // BASTIAAN -- Put code here to unlock the box
    // BASTIAAN -- Put code here to unlock the box
    // BASTIAAN -- Put code here to unlock the box
    // BASTIAAN -- Put code here to unlock the box
    // BASTIAAN -- Put code here to unlock the box
  }
}


// ****************************************
// ** CALIBRATE MPU6050 SESNSOR FUNCTION **
// ****************************************
void calibrate_sensors() {
  int   num_readings = 100;
  float x_accel = 0;
  float y_accel = 0;

  // Discard the first set of values read from the IMU
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Read and average the raw values from the IMU
  for (int i = 0; i < num_readings; i++) {
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    x_accel += ax;
    y_accel += ay;
  }
  x_accel /= num_readings;
  y_accel /= num_readings;

  // Store the raw calibration values globally
  ax_offset = x_accel;
  ay_offset = y_accel;
}

inline unsigned long get_last_time() {
  return last_read_time;
}

inline void set_last_time(unsigned long _time) {
  last_read_time = _time;
}

inline float get_delta_time(unsigned long t_now) {
  return (t_now - get_last_time()) / 1000.0;
}

void password_change()
{
  int j = 0;
//  lcd.clear();
//  lcd.print("Current Passcode:");
//  lcd.setCursor(0, 1);
  digitalWrite(int_buzzer,HIGH);
  delay(100);
  digitalWrite(int_buzzer,LOW);
  delay(100);
  while (j < 4)
  {
    char key = keypad.getKey();
    if (key)
    {
      pass1[j++] = key;
//      lcd.print(key);
    }
    key = 0;
  }
  delay(500);

  if ((strncmp(pass1, pass, 4)))
  {
//    lcd.clear();
//    lcd.print("Wrong Passkey...");


//    lcd.setCursor(1, 1);
//    lcd.print("DENIED");
  digitalWrite(int_buzzer,HIGH);
  delay(100);
  digitalWrite(int_buzzer,LOW);
  delay(100);
  digitalWrite(int_buzzer,HIGH);
  delay(100);
  digitalWrite(int_buzzer,LOW);
  delay(100);
    customKey = 0;
    resetFunc();
  }
  else
  {
    j = 0;

//    lcd.clear();
//    lcd.print("New Passcode:");
//    lcd.setCursor(0, 1);
    digitalWrite(int_buzzer,HIGH);
    delay(100);
    digitalWrite(int_buzzer,LOW);
    delay(100);
    digitalWrite(int_buzzer,HIGH);
    delay(100);
    digitalWrite(int_buzzer,LOW);
    delay(100);
    while (j < 4)
    {
      char key = keypad
.getKey();
      if (key)

      {
        pass[j] = key;
//        lcd.print(key);
        EEPROM.write(j, key);
        j++;

      }
    }
//    lcd.print("  Done......");
    digitalWrite(int_buzzer,HIGH);
    delay(100);
    digitalWrite(int_buzzer,LOW);
    delay(100);
    digitalWrite(int_buzzer,HIGH);
    delay(100);
    digitalWrite(int_buzzer,LOW);
    delay(100);
    digitalWrite(int_buzzer,HIGH);
    delay(100);
    digitalWrite(int_buzzer,LOW);
    delay(100);
//    digitalWrite(Relay, LOW);
    resetFunc();
  }
}
