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
**    -->[AnalogIn]  PB3 2|      |7 PB2  --> HSSW1
**              [x]  PB4 3|      |6 PB1  --> HSSW2
**                   GND 4|      |5 PB0  --> HSSW3 
**                        +------+
**
*/

#include <arduino.h>

#define HSSW1   0
#define HSSW2   1
#define HSSW3   2

//----------------------------------------------------------------
void setup() 
{
  digitalWrite(HSSW1, LOW);
  digitalWrite(HSSW2, LOW);
  digitalWrite(HSSW3, LOW);
  pinMode(HSSW1, OUTPUT);
  digitalWrite(HSSW1, LOW);
  pinMode(HSSW2, OUTPUT);
  digitalWrite(HSSW2, LOW);
  pinMode(HSSW3, OUTPUT);
  digitalWrite(HSSW3, LOW);

} // setup()


//----------------------------------------------------------------
void loop() 
{
  delay(1000);
  digitalWrite(HSSW1, !digitalRead(HSSW1));
  delay(1000);
  digitalWrite(HSSW2, digitalRead(HSSW1));
  delay(1000);
  digitalWrite(HSSW3, digitalRead(HSSW1));

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
