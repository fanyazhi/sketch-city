#include <Servo.h>

#define FULL_POWER_CCW   180
#define MID_POWER_CCW    120
#define LOW_POWER_CCW    91

#define FULL_POWER_CW  0
#define MID_POWER_CW   60
#define LOW_POWER_CW   89

#define SERVO_STOP     90

// Declare two Servo objects - one to control each servo
Servo right_servo;
Servo left_servo; 

// Stores the pin # that is connected to the IR sensor
const int right_sensor_pin = 0;
const int left_sensor_pin = 1;
const int center_sensor_pin = 2;

// Will use this to store the value from the ADC
int right_sensor_value;
int left_sensor_value;
int center_sensor_value;

enum state{
  STOP,
  STRAIGHT,
  SLIGHT_RIGHT,
  SLIGHT_LEFT,
  INTERSECTION,
  RIGHT,
  LEFT
};

volatile state current_state = STRAIGHT;
volatile state next_state = STRAIGHT;
const state moves[] = {RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, STOP};     
volatile int n_moves = 9;
volatile int move_idx = 0;

unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

void setup() {  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Connect right servo to pin 9, left servo to pin 10
  right_servo.attach(10);
  left_servo.attach(9);
  
  right_servo.write(SERVO_STOP);
  left_servo.write(SERVO_STOP);
  
  Serial.begin(9600);
}

void updateSensors() {
  right_sensor_value = analogRead(right_sensor_pin);
  left_sensor_value = analogRead(left_sensor_pin);
  center_sensor_value = analogRead(center_sensor_pin);
  /*Serial.print("Left: ");
  Serial.print(left_sensor_value);
  Serial.print(" Center: ");
  Serial.print(center_sensor_value);
  Serial.print(" Right: ");
  Serial.print(right_sensor_value);
  //Serial.print(" Current State: ");
  //Serial.print(current_state);
  //Serial.print(" Next_State; ");
  //Serial.print(next_state);
  Serial.print("\n");*/
 
}

void loop() {
  if(move_idx > n_moves - 1) move_idx = 0;
  current_state = next_state;

  updateSensors();
  
  // State Outputs
  switch(current_state) {
    case STOP:
      //Serial.println("STOP");
      right_servo.write(SERVO_STOP);
      left_servo.write(SERVO_STOP);
      delay(5000);
      break;
    case STRAIGHT:
      //Serial.println("STRAIGHT");
      if(right_sensor_value > 850 && left_sensor_value > 850)           // At intersection, do next turn
        next_state = INTERSECTION;
      else if(right_sensor_value > 850)  next_state = SLIGHT_RIGHT;    // Drifting left, correct right
      else if(left_sensor_value > 850)   next_state = SLIGHT_LEFT;     // Drifting right, correct left
      else {                                                           // Go striaght
        right_servo.write(FULL_POWER_CW);
        left_servo.write(FULL_POWER_CCW);
        next_state = STRAIGHT;
      }
      break;
    case SLIGHT_RIGHT:      // Drifting left, correct right
      //Serial.println("SLIGHT RIGHT");
      right_servo.write(LOW_POWER_CW);
      left_servo.write(MID_POWER_CCW);
      
      if(right_sensor_value > 850 && left_sensor_value > 850)           // At intersection, do next turn
        next_state = INTERSECTION;
      else if(right_sensor_value > 850) next_state = SLIGHT_RIGHT;
      else next_state = STRAIGHT;
      break;
    case SLIGHT_LEFT:           // Drifting right, correct left
      //Serial.println("SLIGHT LEFT");
      right_servo.write(FULL_POWER_CW);
      left_servo.write(MID_POWER_CCW);
      
      if(right_sensor_value > 850 && left_sensor_value > 850)           // At intersection, do next turn
        next_state = INTERSECTION;
      else if(left_sensor_value > 850) next_state = SLIGHT_LEFT;
      else next_state = STRAIGHT;
      break;
    case INTERSECTION:
      //Serial.println("INTERSECTION");
      if(center_sensor_value > 850) {
        next_state = moves[move_idx];
        move_idx += 1;
        previousMillis = 0;
      }
      else next_state = INTERSECTION;
      break;
    case RIGHT:
      //Serial.println("RIGHT");
      right_servo.write(FULL_POWER_CCW);
      left_servo.write(FULL_POWER_CCW);

      //right_servo.write(MID_POWER_CCW);
      //left_servo.write(MID_POWER_CCW);

      if(previousMillis == 0) {
         previousMillis = millis();
      }
      
      currentMillis = millis();
      
      if(currentMillis - previousMillis > 420) next_state = STRAIGHT;
      else next_state = RIGHT;
      break;
    case LEFT:
      //Serial.println("LEFT");
      right_servo.write(FULL_POWER_CW);
      left_servo.write(FULL_POWER_CW);

      //right_servo.write(MID_POWER_CW);
      //left_servo.write(MID_POWER_CW);

      if(previousMillis == 0) {
         previousMillis = millis();
      }
      
      currentMillis = millis();
      
      if(currentMillis - previousMillis > 420) next_state = STRAIGHT;
      else next_state = LEFT;
      break;
    default:
      //Serial.println("DEFAULT");
      right_servo.write(SERVO_STOP);
      left_servo.write(SERVO_STOP);
  }
  delay(15);
}

