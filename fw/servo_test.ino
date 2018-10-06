#include <SimpleServo.h>
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
int _valve_state = FULLY_CLOSED;
SimpleServo _servo; 
int _servo_degree;    
unsigned long _last_time;
const unsigned long INTERVAL_TO_STABLE = 60000; // 
const unsigned long INTERVAL_OPEN = 1000 * 60 * 2; // 2h
const int FULLY_OPEN = 84;
const int FULLY_CLOSED = 10;
const int STEP = 1;

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
  //blink(new_state + 1);
  _valve_state = new_state;
  _last_time = millis();
}
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);  
  pinMode(SW, INPUT_PULLUP);  
  blink(3);
  _servo.attach(LED_BUILTIN);  // attaches the servo on pin 9 to the servo object
  _last_time = millis();
  _last_sw_state = false;
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
      update_state(STATE_OPENING);
    }else{
      update_state(STATE_CLOSING); 
    }
  }
}
void loop() {
  delay(10);
  /*if(digitalRead(SW)){
    open_valve();
  }else{
   close_valve(); 
  }
  return;*/
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
      if((millis() - _last_time) >= INTERVAL_TO_STABLE){
        update_state(STATE_IDLE);
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
      if((millis() - _last_time) >= INTERVAL_TO_STABLE){
        update_state(STATE_WAITING_TO_CLOSE);
      }
      break;
    case STATE_WAITING_TO_CLOSE:
      if((millis() - _last_time) >= INTERVAL_OPEN){
        update_state(STATE_CLOSING);
      }
      break;
    case STATE_IDLE:
    default:
      _valve_state = STATE_IDLE;
  }
}

