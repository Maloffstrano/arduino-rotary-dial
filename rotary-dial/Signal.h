/*
  Signal.h - Library for signaling. This version signals using a specified pin.
  Created by Maloffstrano, March 2020.
  Released into the public domain.

  If you instantiate the class using a pin attached to an LED, this class is
  a visual signaller. With the pin attached to a piezo device, it becomes an
  audio signaller. Modify the methods to use PWM output to have an audio
  signaller driven by the Arduino.
*/

#ifndef Signal_H
#define Signal_H
 
#include <Arduino.h>
 
class Signal {
  public:
    Signal(byte pinNumber);
    void on();
    void off();
    void blink(int delay_ms);
    void blink(int delay_ms, int count);

  private:
    byte pin;
};
 
#endif
