/*
  Signal.cpp - Library for signaling. This version signals using a specified pin.
  Created by Maloffstrano, March 2020.
  Released into the public domain.

  If you instantiate the class using a pin attached to an LED, this class is
  a visual signaller. With the pin attached to a piezo device, it becomes an
  audio signaller. Modify the methods to use PWM output to have an audio
  signaller driven by the Arduino.
*/

#include "Signal.h"

Signal::Signal(byte pinNumber) {
  pin = pinNumber;
  pinMode(pin, OUTPUT);
  off();
}

void Signal::on() {
  digitalWrite(pin, HIGH);
}

void Signal::off() {
  digitalWrite(pin, LOW);
}

void Signal::blink(int delay_ms) {
  on();
  delay(delay_ms);
  off();
  delay(delay_ms);
}