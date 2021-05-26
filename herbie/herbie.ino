
#include "LedControl.h"
#include "binary.h"

#define DIN 11
#define CLK 12
#define CS 10
#define TOUCH_SENSOR 13
#define BUZZER 3
#define DIST_TRIG 7
#define DIST_ECHO 6
#define LEFT_MOVEMENT 2
#define RIGHT_MOVEMENT 9
#define PHOTORESISTOR_ANALOG A5

LedControl lc = LedControl(DIN, CLK, CS, 2);

byte matrix_size = 8;
float blink_time = millis();

enum eyes_status {
  forward, squint, left, right
};
eyes_status eyes = forward;
eyes_status prev_eyes = forward;


//-------------------------------------------------------------------
//eyes

byte forward_left[] = { B00011110, B00100001, B01000001, B10011101, B10011101, B10011101, B10000001, B01111110 };
byte forward_right[] = { B01111000, B10000100, B10000010, B10111001, B10111001, B10111001, B10000001, B01111110 };
byte left_look_right[] = { B00011110, B00100001, B01000001, B10000001, B10001111, B10001111, B10001111, B01111110 };
byte left_look_left[] = { B00011110, B00100001, B01000001, B10000001, B11110001, B11110001, B11110001, B01111110 };
byte right_look_left[] = { B01111000, B10000100, B10000010, B10000001, B11110001, B11110001, B11110001, B01111110 };
byte right_look_right[] = {  B01111000, B10000100, B10000010, B10000001, B10001111, B10001111, B10001111, B01111110 };
byte close_left[] = { B00000000, B00000000, B00000000, B11111111, B11111111, B00000000, B00000000, B00000000 };
byte close_right[] = { B00000000, B00000000, B00000000, B11111111, B11111111, B00000000, B00000000, B00000000 };
byte half_left[] = { B00000000, B00000000, B00000000, B11111111, B10000001, B10011101, B01111110, B00000000 };
byte half_right[] = { B00000000, B00000000, B00000000, B11111111, B10000001, B10111001, B01111110, B00000000 };
byte half2_left[] = { B00000000, B00111100, B01000010, B10000001, B10111001, B10111001, B01111110, B00000000 };
byte half2_right[] = { B00000000, B00111100, B01000010, B10000001, B10011101, B10011101, B01111110, B00000000 };
byte smile_both[] = { B00000000, B00000000, B00000000, B00111100, B01111110, B10000001, B00000000, B00000000 };
byte angry_left[] = { B00000000, B00110000, B00001100, B00000010, B00000010, B00001100, B00110000, B00000000 };
byte angry_right[] = { B00000000, B00001100, B00110000, B01000000, B01000000, B00110000, B00001100, B00000000 };
byte sleep_left[] = { B11110000, B00100000, B01000000, B11110000, B00000000, B00000000, B11111111, B11111111 };
byte sleep_right[] = { B00011111, B00000010, B00000100, B00001000, B00011111, B00000000, B11111111, B11111111 };

//------------------------------------------------------------------
//methods

void setOnMatrix(byte *left, byte *right){
    for(int i = 0, j = 0; i < matrix_size; i++) {
        lc.setColumn(0, 7 - i, left[i]);  //rotated in left
        lc.setColumn(1, 7 - i, right[i]);
    }
}

void blink() {
  close();
  open();
}

void check_if_blink() {
  if (blink_time < millis()) {
    blink_time += random(1000, 5000);
    if (random(1, 3) == 1) {
      blink();
    }
  }
}

void check_if_touch() {
  bool before_sound = true;
  if(digitalRead(TOUCH_SENSOR) == HIGH) {
    delay(200);
    if (digitalRead(TOUCH_SENSOR) == LOW) {
      angry();
    } else {
      close();
      while(digitalRead(TOUCH_SENSOR) == HIGH) {
        smile();
        if (before_sound) {
          make_happy_sound();
          before_sound = false;
        }
      }
      open();
    }
  }
}

void check_if_sleep_disturb() {
  long time_to_sound = millis();
  if(digitalRead(TOUCH_SENSOR) == HIGH) {
    open_one();
    while(digitalRead(TOUCH_SENSOR) == HIGH) {
      check_distance();
      check_movement();
      setOnMatrix(sleep_left, eyes == forward ? forward_right : 
                              eyes == right ? right_look_right
                              : right_look_left);
      if (time_to_sound < millis()) {
        time_to_sound += 3000;
        make_sound("a    a    a", 30);
      }
      delay(100);
    }
    close_one();
  }
}

void sleep() {
  setOnMatrix(sleep_left, sleep_right);
  delay(1000);
  setOnMatrix(sleep_right, sleep_left);
  delay(1000);
}

void check_if_light() {
  bool was_sleeping = false;
  int analog_value = analogRead(PHOTORESISTOR_ANALOG);
  if (analog_value < 200) {
    close();
    was_sleeping = true;
  }
  while(analog_value < 200) {
    analog_value = analogRead(PHOTORESISTOR_ANALOG);
    sleep();
    check_if_sleep_disturb();
    delay(50);
  }
  if(was_sleeping) {
    open();
    blink_time = millis();
  }
  delay(100);
}

void close_one() {
  setOnMatrix(sleep_left, half2_right);
  delay(100);
  setOnMatrix(sleep_left, half_right);
  delay(100);
  setOnMatrix(sleep_left, close_right);
  delay(100);
}

void open_one() {
  setOnMatrix(sleep_left, half_right);
  delay(100);
  setOnMatrix(sleep_left, half2_right);
  delay(100);
  setOnMatrix(sleep_left, eyes == forward ? forward_right : 
                          eyes == right ? right_look_right
                          : right_look_left);
  delay(100);
}

void close() {
  setOnMatrix(half2_left, half2_right);
  delay(100);
  setOnMatrix(half_left, half_right);
  delay(100);
  setOnMatrix(close_left, close_right);
  delay(100);
}

void open() {
  setOnMatrix(half_left, half_right);
  delay(100);
  setOnMatrix(half2_left, half2_right);
  delay(100);
  look_normal();
  delay(100);
}

void play_tone(int tone, int time) {
  for (long i = 0; i < time * 1000L; i += tone * 2) {
    digitalWrite(BUZZER, HIGH);
    delayMicroseconds(tone);
    digitalWrite(BUZZER, LOW);
    delayMicroseconds(tone);
  }
}

void play(char note, int time) {
  char ch[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };
  for (int i = 0; i < 8; i++) {
    if (ch[i] == note) {
      play_tone(tones[i], time);
    }
  }
}

void make_sound(char notes[], int speed) {
  for (int i = 0; i < 12; i++) {
    if (notes[i] == ' ') {
      delay(speed);
    } else {
      play(notes[i], speed);
    }
    delay(speed);
  }
}

void make_angry_sound() {
  make_sound("gagagagaggaga ", 30);
}

void make_happy_sound() {
  make_sound("cCa a cCa a a", 50);
}

void smile() {
  setOnMatrix(smile_both, smile_both);
  delay(500);
}

void angry() {
  setOnMatrix(angry_left, angry_right);
  make_angry_sound();
  delay(1000);
}

int calculateCmDistance() {
  digitalWrite(DIST_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(DIST_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(DIST_TRIG, LOW);
  return pulseIn(DIST_ECHO, HIGH) / 58;
}

void check_distance() {
  int distance_cm = calculateCmDistance();
  if (distance_cm < 20) {
    eyes = squint;
  } 
  else {
    eyes = forward;
  }
  delay(100);
}

void look_normal() {
  if (eyes == forward)
    setOnMatrix(forward_left, forward_right);
  else if (eyes == left) {
    setOnMatrix(left_look_left, right_look_left);
  }
  else if (eyes == right) {
    setOnMatrix(left_look_right, right_look_right);
  } else setOnMatrix(left_look_right, right_look_left);
}

void check_movement() {
  if (eyes != squint) {
    int left_movement = digitalRead(LEFT_MOVEMENT);
    int right_movement = digitalRead(RIGHT_MOVEMENT);
    if (right_movement == HIGH && left_movement == HIGH) {
      if (prev_eyes == right) {
        eyes = left;
        prev_eyes = left;
      } else {
        eyes = right;
        prev_eyes = right;
      }
    } else if(right_movement == HIGH && eyes != right) {
      eyes = right;
      prev_eyes = right;
    }
    else if(left_movement == HIGH && eyes != left) {
      eyes = left;
      prev_eyes = left;
    }
    else  {
      eyes = forward;
    }
    delay(100);
  }
}

//------------------------------------------------------------------
//main

void setup() {
  pinMode(TOUCH_SENSOR, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  
  pinMode(DIST_ECHO, INPUT);
  pinMode(DIST_TRIG, OUTPUT);

  pinMode(LEFT_MOVEMENT, INPUT);
  pinMode(RIGHT_MOVEMENT, INPUT);
  
  for(int i = 0; i < lc.getDeviceCount(); i++) {
      lc.shutdown(i, false); 
      lc.setIntensity(i, 1);
      lc.clearDisplay(i);
  }
}

void loop(){
  look_normal();
  
  check_if_blink();
  check_if_touch();
  check_if_light();
  check_distance();
  check_movement();
  
  delay(50);
}
