/*
 * heater-with-nokia-display.ino
 *
 * Copyright 2021 A. Bagaglia, derived from Victor Chew 2020
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Libraries required:
// - NTC_Thermistor (Yurii Salimov) [Source: Arduino Library Manager]
// - PID (Brett Beuuregard) [Source: Arduino Library Manager]
// - Adafruit_PCD8544 Monochrome Nokia 5110 LCD Displays (Limor Fried/Ladyada  for Adafruit Industries) [Source: Arduino Library Manager]
// - TimerOne (Paul Stoffregen et al.) [Source: Arduino Library Manager]
// - ClickEncoder (0xPIT) [Source: https://github.com/0xPIT/encoder/tree/arduino]
// - avr/sleep.h Arduino IDE install (valid only for AVR boards)

#include <Adafruit_PCD8544.h> // Monochrome Nokia 5110 LCD Displays
#include <TimerOne.h>
#include <ClickEncoder.h>
#include <NTC_Thermistor.h>
#include <SmoothThermistor.h>
#include <PID_v1.h>
#include <avr/sleep.h> // Used to freeze arduino in case of thermal runaway (valid only for AVR boards)

//PID parameters
#define KP 17.07
#define KI 0.75
#define KD 96.57

#define SMOOTHING_WINDOW 5
#define MOSFET_GATE_PIN 3
#define THERMISTOR_PIN A6

//Define parameters for rotary encoder
#define KY040_CLK 9
#define KY040_DT 8
#define KY040_SW 2
#define KY040_STEPS_PER_NOTCH 2

// Define the pins used to control the LCD module
#define LCD_SCLK    13
#define LCD_DIN     11
#define LCD_DC      5
#define LCD_CS      7
#define LCD_RST     6

bool heaterOn = false, pOnM = false; 
double pwmOut, curTemp, setTemp = 0;

NTC_Thermistor* thermistor = new NTC_Thermistor(THERMISTOR_PIN, 10000, 100000, 25, 3950);
SmoothThermistor* sthermistor = new SmoothThermistor(thermistor, SMOOTHING_WINDOW);
PID pid(&curTemp, &pwmOut, &setTemp, KP, KI, KD, P_ON_E, DIRECT);
ClickEncoder encoder(KY040_CLK, KY040_DT, KY040_SW, KY040_STEPS_PER_NOTCH);

// Create a global instance of the display object
Adafruit_PCD8544 display = Adafruit_PCD8544(
  LCD_SCLK, LCD_DIN, LCD_DC, LCD_CS, LCD_RST
);

void refreshDisplay() {
  display.clearDisplay();
  display.setCursor(0,0);
  char msg[64];
  sprintf(msg, "Temp.: %dc", (int)curTemp);
  display.println(msg);
  sprintf(msg, "Target: %dc", (int)setTemp);
  display.println(msg);
  if (heaterOn)
  {sprintf(msg, "Heating...");}
  else 
  {sprintf(msg, "- Heater off -");}
  display.println(msg);
  if (curTemp > setTemp) // HEATER ON doesn't mean the MOSFET is ON, depends on the thermal runaway condition below
  {sprintf(msg, "MOSFET OFF"); Serial.println("MOSFET OFF");}
  else  
  {sprintf(msg, "MOSFET ON"); Serial.println("MOSFET ON");}
  display.println(msg);
}

void encoderISR() {
  encoder.service();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOSFET_GATE_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pid.SetMode(AUTOMATIC);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  encoder.setAccelerationEnabled(true);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(encoderISR); 

  display.begin();
  display.setContrast(50);
  display.clearDisplay();
  display.setCursor(0,0);

  display.println("Bottigliatore 1.0beta"); //put whatever you like, splash screen
  display.println("Bagaglia 2021");
  display.display();
  delay(3000);
  display.clearDisplay();
}

void loop() {
  // Get rotary encoder values and update display
  int _curTemp = sthermistor->readCelsius();
  int _setTemp = setTemp - encoder.getValue()*5; // minus because signal from this rotary switch works in the opposite direction
  _setTemp = min(300, max(0, _setTemp));
  bool _heaterOn = heaterOn;
   if (encoder.getButton() == ClickEncoder::Clicked) 
   {
    _heaterOn = !_heaterOn;
    if (!_heaterOn) _setTemp = 0;
   }

  // Update display only if something changes
   if (_curTemp != curTemp || _setTemp != setTemp || _heaterOn != heaterOn) {
     curTemp = _curTemp;
     setTemp = _setTemp;
     heaterOn = _heaterOn;
     refreshDisplay();
   }

  // Turn on LED if we are within 1% of target temperature
  digitalWrite(LED_BUILTIN, heaterOn ? abs(_curTemp-setTemp)/setTemp <= 0.01 : LOW);

  // To speed things up, only switch to proportional-on-measurement when we are near target temperature
  // See: http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/
  
  if (!pOnM && abs(curTemp-setTemp) <= max(curTemp, setTemp)*0.2) {
    pOnM = true;
    pid.SetTunings(KP, KI, KD, P_ON_M);
    display.println("P_ON_M activated.");
  }

  // Prevent thermal overrun in case of PID malfunction
  pid.Compute();
  if ((curTemp < 300) && (curTemp > -10)) { // at least check also if thermistor cable is disconnected (gives negative readings), if you live in a very cold place, put a lower negative number
    if (curTemp > setTemp) // this condition makes the temp to be very stable around what you set with the encoder (+- 1 degree C) aside from what pid decides
    {analogWrite(MOSFET_GATE_PIN, 0); Serial.println("HEATER OFF");} //no need to send this message to display here, it is done by refreshDisplay() function
    else 
    {analogWrite(MOSFET_GATE_PIN, heaterOn ? pwmOut : 0); Serial.println("HEATER ON");}
  }
  else { //print results either via serial monitor and LCD
    analogWrite(MOSFET_GATE_PIN, 0);
    Serial.println("Thermal overrun detected!");
    Serial.println("System Halted!");
    display.clearDisplay();
    display.setCursor(0,0);
    char msg2[64];
    sprintf(msg2, "Thermal overrun detected! System Halted!");
    display.println(msg2);
    display.display();
    delay(500);
    sleep_mode(); // turns off arduino completely untill poweron/off/reset cycle
  }
  display.display();
    // Display stats to serial monitor anyway
  Serial.print("pwmOut = "); Serial.print(pwmOut); Serial.print(", "); 
  Serial.print("diff = "); Serial.print(setTemp - curTemp); Serial.print(", "); 
  Serial.print("curTemp = "); Serial.print(round(curTemp)); 
  Serial.print(" / "); Serial.println(round(setTemp));
  delay(300);
}
