/*
**  ATtiny85 Slimme Meter Inrush Booster controller
** 
**  Copyright (c) 2023 Willem Aandewiel
**  TERMS OF USE: MIT License. See bottom of file.
**
**  Version 2.0  25-08-2023
** 
**  Use PROJECT TASKS -> Platform -> "Set Fuses":
**        Extended  : 0xFF
**
**              Serial program downloading (SPI) enabled
**              brown-out Detection 1.8v (0xDE)
**              brown-out detection 2.7v (0xDD)
**              brown-out detection 4.3v (0xDC)
**        High      : 0xDD 
**
**              Clock source Int.RC Osc. 8MHz PWRDWN/RESET: 6 CK/1
**        Low       : 0xE2
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
** ATMEL ATTINY85
**                        +--\/--+
**             RESET PB5 1|      |8 VCC
**    -->[AnalogIn]  PB3 2|      |7 PB2  --> HSSW2U1_CHARGE
**  <-- mySerial(Tx) PB4 3|      |6 PB1  --> HSSW1U3_STEPUP
**                   GND 4|      |5 PB0  --> HSSW0U2_SM_PWR 
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

#define HSSW0U2_SM_PWR    0
#define HSSW1U3_STEPUP    1
#define HSSW2U1_CHARGE    2

#define _BAT_V_LIMIT      3.5
#define _BATT_LOW_LIMIT   2.8
#define _MAX_STEPUP_TIME  20000
#define _SM_GRACE_TIME    20000

#define INV_ON        LOW
#define INV_OFF       HIGH

enum    { _STATE_UNKNOWN, _STATE_INIT, _STATE_CHARGE, _STATE_STRT_STEPUP, _STATE_STEPUP, _STATE_SM_PWR, _STATE_NORMAL };

float       battV;
int8_t      state = _STATE_INIT;
int16_t     battV10, battV100, prevBattV10, prevBattV100;
uint32_t    loopCount = 0;
uint32_t    stepupTimer, smGraceTimer;

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
  // Calculate the voltage from the analog value.
  //int16_t voltageI10 = (int)(analogValue * 5 / 1024.0); 
  float voltage = (analogValue * 5.0) / 1024.0;

  if (voltage >= 4.98)
  {
    //mySerial.println("Probably no power from SM!!  ");
    voltage = 0.01;
  }

  return voltage;

} //  readAnalogVoltage()


//----------------------------------------------------------------
void showBattVoltage()
{
    prevBattV10  = battV10;
    prevBattV100 = battV100;
    mySerial.print("battVoltage is [");
    mySerial.print(battV);
    /***
    mySerial.print("], battV10[");
    mySerial.print(battV10);
    mySerial.print("], battV100[");
    mySerial.print(battV100);
    ***/
    mySerial.println("]");
    mySerial.flush();

} //  showBattVoltage()


//----------------------------------------------------------------
int8_t startCharging()
{
  mySerial.println("\r\nstartCharging() ..");
  mySerial.flush();
  digitalWrite(HSSW2U1_CHARGE, INV_ON);
  digitalWrite(HSSW0U2_SM_PWR, INV_OFF);
  digitalWrite(HSSW1U3_STEPUP, INV_OFF);
  return _STATE_CHARGE;

} //  startCharging();


//----------------------------------------------------------------
int8_t continueCharging()
{
  digitalWrite(HSSW2U1_CHARGE, INV_ON);
  digitalWrite(HSSW0U2_SM_PWR, INV_OFF);
  digitalWrite(HSSW1U3_STEPUP, INV_OFF);

  if (battV >= _BAT_V_LIMIT) 
  { 
    showBattVoltage();
    stepupTimer = millis();
    return _STATE_STRT_STEPUP; 
  }

  return _STATE_CHARGE;

} //  continueCharging();


//----------------------------------------------------------------
int8_t startStepUp()
{
  mySerial.println("startStepUp() ..");
  mySerial.flush();
  digitalWrite(HSSW1U3_STEPUP, INV_ON);
  digitalWrite(HSSW2U1_CHARGE, INV_ON);
  digitalWrite(HSSW0U2_SM_PWR, INV_OFF);
  return _STATE_STEPUP;
  
} //  startStepUp();


//----------------------------------------------------------------
int8_t startSmPower()
{
  mySerial.println("startSmPower() ..");
  mySerial.flush();
  digitalWrite(HSSW0U2_SM_PWR, INV_ON);
  smGraceTimer = millis();
  //-tst-digitalWrite(HSSW2U1_CHARGE, INV_OFF);
  for(int i=0; i<5; i++)
  {
    showBattVoltage();
    delay(500);
  }
  /*
  mySerial.println("stop StepUp ..");
  mySerial.flush();
  digitalWrite(HSSW1U3_STEPUP, INV_OFF);
  */
  return _STATE_NORMAL;
  
} //  startSmPower();


//----------------------------------------------------------------
void setup() 
{
  digitalWrite(HSSW0U2_SM_PWR, INV_OFF);
  digitalWrite(HSSW1U3_STEPUP, INV_OFF);
  digitalWrite(HSSW2U1_CHARGE, INV_OFF);
  pinMode(HSSW0U2_SM_PWR, OUTPUT);
  digitalWrite(HSSW0U2_SM_PWR, INV_OFF);
  pinMode(HSSW1U3_STEPUP, OUTPUT);
  digitalWrite(HSSW1U3_STEPUP, INV_OFF);
  pinMode(HSSW2U1_CHARGE, OUTPUT);

  mySerial.begin(9600);
  delay(500);
  mySerial.println("\r\nAnd than it Begins ...\r\n");
  mySerial.flush();
  for(int i=0; i<5;i++)
  {
    mySerial.println("Toggle HSSW2U1_CHARGE ...");
    mySerial.flush();
    digitalWrite(HSSW2U1_CHARGE, !digitalRead(HSSW2U1_CHARGE));
    delay(500);
  }
  //-- start Charging
  mySerial.println("Switch on HSSW2U1_CHARGE");
  digitalWrite(HSSW2U1_CHARGE, INV_ON);

  loopCount = 0;
  state = _STATE_INIT;

} // setup()


//----------------------------------------------------------------
void loop() 
{
  delay(1000);
  loopCount++;

  battV = readAnalogVoltage(3);
  battV100 = (battV * 100);
  battV = battV100 / 100.0;
  battV10  = (battV * 10);

  if ( ((loopCount%30) == 0) || (prevBattV10 != battV10) )
  {
    showBattVoltage();
  }

  //-- SM disconnected??
  if (battV < 0.5)  state = _STATE_INIT;

  //-- finite State Machine ...
  switch(state)
  {
    case _STATE_UNKNOWN:      state = _STATE_INIT;  
                              break;
    case _STATE_INIT:         state = startCharging();
                              break;
    case _STATE_CHARGE:       state = continueCharging();
                              break;
    case _STATE_STRT_STEPUP:  state = startStepUp();
                              break;
    case _STATE_STEPUP:       {
                                showBattVoltage();
                                if (battV < _BATT_LOW_LIMIT)
                                {
                                  mySerial.println("Battery voltage too low (< 2.5v).");
                                  state = _STATE_INIT;
                                }
                                else if ((millis() - stepupTimer) > _MAX_STEPUP_TIME)
                                {
                                  state = _STATE_SM_PWR;
                                }
                              }
                              break;
    case _STATE_SM_PWR:       state = startSmPower();
                              break;
    case _STATE_NORMAL: 
    default:                  {
                                if ( (millis() - smGraceTimer) > _SM_GRACE_TIME )
                                {
                                  digitalWrite(HSSW2U1_CHARGE, INV_ON);
                                }
                                break;
                              }

  } //-- state

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
