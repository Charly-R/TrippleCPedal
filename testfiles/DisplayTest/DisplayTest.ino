#include <Arduino.h>
#include <U8g2lib.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

#include <SPI.h>
#include <Wire.h>

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, 13, 11, 10, 9);
char test[8];
char encVal[8];
int switches;

ClickEncoder *encoder;
int16_t last, value, inkrement;

void timerIsr() {
  encoder->service();
}

void setup(void) {
  u8g2.begin();
  
  encoder = new ClickEncoder(6, 5, A5);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  last = -1;

  DDRB = DDRB | B00010001;
  PORTB = PORTB | B00010001;

  DDRD = DDRD | B10000000;
  PORTD = PORTD | B00000000;

  pinMode(A0, INPUT);           // set pin to input
  pinMode(A1, INPUT);           // set pin to input
  pinMode(A2, INPUT);           // set pin to input
  pinMode(A3, INPUT);           // set pin to input
  pinMode(A4, INPUT);           // set pin to input
  pinMode(A5, INPUT);           // set pin to input
  
  digitalWrite(A1, HIGH);       // turn on pullup resistors
  digitalWrite(A2, HIGH);       // turn on pullup resistors
  digitalWrite(A3, HIGH);       // turn on pullup resistors
  digitalWrite(A4, HIGH);       // turn on pullup resistors
  digitalWrite(A5, HIGH);       // turn on pullup resistors
}

void loop(void) {

  switches = PINC;

  value += encoder->getValue();
  if (value >= last+3) {
    last = value;
    inkrement++;
  }

  String thisString = String(switches, BIN);
  thisString.toCharArray(test, 8);
  String encWert = String(inkrement);
  encWert.toCharArray(encVal, 8);
  
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_8x13_t_symbols);
    u8g2.drawStr(0,24,encVal);
    u8g2.drawStr(0,64,test);
  } while ( u8g2.nextPage() );
}
