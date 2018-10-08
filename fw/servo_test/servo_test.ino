#include <SimpleServo.h>
#include <SoftSerial.h>

#define DEBUG_TX_RX_PIN         0 //Adjust here your Tx/Rx debug pin
SoftSerial _dbgSerial(DEBUG_TX_RX_PIN, DEBUG_TX_RX_PIN, false); //true allows to connect to a regular RS232 without RS232 line driver
 
#define LED_BUILTIN 1
#define SW 2

enum states_t
{
  STATE_IDLE = 0,
  STATE_CLOSING = 1,
  STATE_CLOSED = 2,
  STATE_OPENING = 3,
  STATE_OPENED = 4,
  STATE_WAITING_TO_CLOSE = 5
};
int _valve_state = STATE_CLOSED;
SimpleServo _servo; 
int _servo_degree;    
unsigned long _last_time = 0;
unsigned long _seconds_counter = 0;
const unsigned long INTERVAL_SECONDS = 1000; 
const int FULLY_OPEN = 84;
const int FULLY_CLOSED = 10;
const int STEP = 3;

bool _last_sw_state;
void blink(unsigned char times){
  for(int i = 0; i < times; ++i){
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(200);                       
    digitalWrite(LED_BUILTIN, LOW);    
    delay(200);                       
  }
}
void update_state(int new_state){
  _dbgSerial.print(F("\nNew state = "));
  _dbgSerial.println(new_state);
  _valve_state = new_state;
  _last_time = millis();
  _seconds_counter = 0;
}
void setup() {
  _dbgSerial.begin(9600); 
  _dbgSerial.println(F("\n****************"));
  _dbgSerial.txMode(); //Before sending a message, switch to txMode
  pinMode(LED_BUILTIN, OUTPUT);  
  pinMode(SW, INPUT_PULLUP);  
  blink(3);
  _servo.attach(LED_BUILTIN);  // attaches the servo on pin 9 to the servo object
  _last_time = millis();
  _last_sw_state = false;
  _seconds_counter = 0;
  update_state(STATE_CLOSED);
}
bool move_servo(int final_degree, int increment)
{
  if((increment > 0) && _servo_degree >= final_degree){
    _servo_degree = final_degree;
    return true; 
  }
  if((increment < 0) && _servo_degree <= final_degree){
    _servo_degree = final_degree;
    return true; 
  }
  _servo_degree += increment;
  _servo.write(_servo_degree);
  return false;
}
bool close_valve(){
  return move_servo(FULLY_CLOSED, -STEP);
}
bool open_valve(){
  return move_servo(FULLY_OPEN, STEP);
}
void update_state_from_input(){
  int sw_state = digitalRead(SW);
  if( _last_sw_state != sw_state){
    // State has changed
    _last_sw_state = sw_state;
    if(sw_state){
      _dbgSerial.println(F("\nOPEN TRIGGERED"));
      update_state(STATE_OPENING);
    }else{
      _dbgSerial.println(F("\nCLOSED TRIGGERED"));
      update_state(STATE_CLOSING); 
    }
  }
}
bool has_elapsed_second(){
 bool result = ((unsigned long)(millis() - _last_time) >= INTERVAL_SECONDS);
 if(result){
  _dbgSerial.print(F("[s] "));  
  _dbgSerial.println(_seconds_counter);
  _last_time = millis();
 }
  return result;
}
void loop() {
  int microseconds;
  delay(10);
  update_state_from_input();
  switch(_valve_state){
    case STATE_CLOSING:
      if(close_valve()){
        //! Now the valve is fully close
        update_state(STATE_CLOSED);
      }
      break;
    case STATE_CLOSED:
      _servo_degree = FULLY_CLOSED;
      _servo.write(_servo_degree);
      microseconds = _servo.readMicroseconds();
      _servo.writeMicrosecondsMillis(microseconds,0);
      if(has_elapsed_second()){
        if(_seconds_counter++ > 5){
          _dbgSerial.println(F("Valve fully closed"));
          update_state(STATE_IDLE);
        } 
        
      }     
      break;
    case STATE_OPENING:
      if(open_valve()){
        //! Now the valve is fully open
        update_state(STATE_OPENED);
      }
      break;
    case STATE_OPENED:
      _servo_degree = FULLY_OPEN;
      _servo.write(_servo_degree);
      microseconds = _servo.readMicroseconds();
      _servo.writeMicrosecondsMillis(microseconds,0);
      if(has_elapsed_second()){
        if(_seconds_counter++ > 5){
          update_state(STATE_WAITING_TO_CLOSE);
        }
      }
      break;
    case STATE_WAITING_TO_CLOSE:
      if(has_elapsed_second()){
        if(_seconds_counter++ > 60*60){
          _dbgSerial.println(F("\nTime up no more hot water!"));
          update_state(STATE_CLOSING);
        }       
      }
      break;
    case STATE_IDLE:
    default:
      _valve_state = STATE_IDLE;
  }
}

