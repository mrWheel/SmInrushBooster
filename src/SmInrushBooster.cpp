/*
** ATtiny85 Slimme Meter Inrush Booster controller
** 
** Copyright 2023 Willem Aandewiel
** Version 1.0  23-03-2023
** 
** Use avrfuses.app to set fuses:
**      Extended  : 0xFF
**      High      : 0xDE (but 0xDF might also "work" --> brown-out detection disabled)
**      Low       : 0xE2
**
**    [Program] -> [Verify] -> [Read]
**
** -->> AVRfuses: Serial programdownloading (SPI) enabled
** -->> AVRfuses: Brown-out Detector trigger level: VCC=1.8 V
** -->> AVRfuses: Select Clock source: Int. RC Osc. 8MHz; 
**                  Start-up time PWRDWN/RESET: 6 CK/1 4CK +64 ms
** 
** Board              : "ATtiny25/45/85 (no bootloader)"
** Chip               : "ATtiny85"
** Clock Source       : "8MHz (internal)"
** Timer 1 Clock      : "CPU (CPU Frequency)"
** LTO(1.6.11+ only)  : "disabled"
** millis()/micros()  : "Enabled"                                      
** save EEPROM        : "EEPROM not retained"
** B.O.D. Level (Only set on bootload): "B.O.D. Enabled (1.8v)"
** 
**  Copyright (c) 2023 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.
**
** ATMEL ATTINY85
**                        +--\/--+
**             RESET PB5 1|      |8 VCC
**    -->[AnalogIn]  PB3 2|      |7 PB2  --> HSSW1_YEL
**  <-- mySerial(Tx) PB4 3|      |6 PB1  --> HSSW2_BLU
**                   GND 4|      |5 PB0  --> HSSW1_RED 
**                        +------+
**
*/

#include <arduino.h>
#ifdef ARDUINO_AVR_UNO
  #define mySerial  Serial
#else
  #include <SoftwareSerial.h>
  SoftwareSerial mySerial(-1,4);  //rx, tx
#endif 

#define HSSW1_RED   0
#define HSSW2_BLU   1
#define HSSW1_YEL   2

float       battV;
int16_t     prevBattV10;
uint32_t    loopCount = 0;

//----------------------------------------------------------------
float readAnalogVoltage(int pin) 
{
  // Set the analog reference to 1.1 volts.
  //analogReference(INTERNAL);
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2)); // clear prescaler bits
  ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);   // 128
  //ADMUX  =  bit (REFS0) | bit (REFS1);  // Internal 1.1V reference

  uint16_t avarage = 0;
  uint16_t analogValue;

  for (int i = 0; i < 15; i++)
  {
    // Read the analog value from the specified pin.
    analogValue = analogRead(pin);
    avarage += analogValue; 
    delay(10);
  }
  analogValue = avarage / 15;
  /*
  mySerial.print("analogValue [");
  mySerial.print(analogValue);
  mySerial.print("] ");
  */
  // Calculate the voltage from the analog value.
  //int16_t voltageI10 = (int)(analogValue * 5 / 1024.0); 
  float voltage = (analogValue * 5.0) / 1024.0;

  return voltage;

} //  readAnalogVoltage()


//----------------------------------------------------------------
void setup() 
{
  mySerial.begin(9600);
  delay(500);
  mySerial.flush();
  mySerial.println("\r\nAnd than it begins ..\r\n");

  digitalWrite(HSSW1_RED, LOW);
  digitalWrite(HSSW2_BLU, LOW);
  digitalWrite(HSSW1_YEL, LOW);
  pinMode(HSSW1_RED, OUTPUT);
  digitalWrite(HSSW1_RED, LOW);
  pinMode(HSSW2_BLU, OUTPUT);
  digitalWrite(HSSW2_BLU, LOW);
  pinMode(HSSW1_YEL, OUTPUT);
  digitalWrite(HSSW1_YEL, LOW);

  loopCount = 0;

} // setup()


//----------------------------------------------------------------
void loop() 
{
  delay(1000);
  digitalWrite(HSSW1_RED, !digitalRead(HSSW1_RED));
  //digitalWrite(HSSW2_BLU, LOW);
  //digitalWrite(HSSW1_YEL, LOW);
  
  battV = readAnalogVoltage(3);
  int16_t battV10 = battV * 100;
  battV = battV10 / 100.0;
  loopCount++;

  if ( ((loopCount%60) == 0) || (prevBattV10 != (int)(battV*10)) )
  //if ( (loopCount%10) == 0) 
  {
    prevBattV10 = (int)battV*10;
    mySerial.print("battVoltage is [");
    mySerial.print(battV);
    mySerial.print("]  ");

    if (battV > 3.9)
    {
      digitalWrite(HSSW2_BLU, HIGH);
      mySerial.println("Switch booster to 'ON' ..");
    }
    else if (battV > 3.6)
    {
      digitalWrite(HSSW1_YEL, HIGH);
      digitalWrite(HSSW2_BLU, LOW);
      mySerial.println("almost there ..");
    }
    else 
    {
      digitalWrite(HSSW2_BLU, LOW);
      digitalWrite(HSSW1_YEL, LOW);
      mySerial.println();
    }
  }

} // loop()



/***************************************************************************
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to permit
** persons to whom the Software is furnished to do so, subject to the
** following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
** OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************
*/
// eof //
