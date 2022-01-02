# Filament Joiner Updated for better temp control and nokia display

Code for setting up and driving a 3D printer hotend rigged up to an Arduino to provide a temperature-controlled platform for anything going from filament join to make filament from plastic bottles.

- follow randseq instructions at (https://www.randseq.org/2020/02/3d-printer-filament-joiner.html) for oled display
- Schematic is in images folder
- If you use a nokia lcd 5110 consider it works at 3.3 volts provided at pin 17 of arduino (5v burns it), forthermore the nokia display has the following pinout:
1) GND (to arduino nano GND)
2) Backlight (to arduino nano GND)
3) Vcc (3.3v pin arduino nano) 
4) CLK (Pin D13 arduino nano)
5) DIN (Pin D11 arduino nano MOSI)
6) DC (Pin D5 arduino nano)
7) CE (Pin D7 arduino nano)
8) Reset (Pin D6 arduino nano)


## Libraries required

- NTC_Thermistor (Yurii Salimov)
- PID (Brett Beuuregard) [Source: Arduino Library Manager]
- Adafruit_PCD8544 Monochrome Nokia 5110 LCD Displays (Limor Fried/Ladyada  for Adafruit Industries) [Source: Arduino Library Manager]
- TimerOne (Paul Stoffregen et al.) [Source: Arduino Library Manager]
- ClickEncoder (0xPIT) [Source: https://github.com/0xPIT/encoder/tree/arduino]
- avr/sleep.h Arduino IDE install (valid only for AVR boards)
