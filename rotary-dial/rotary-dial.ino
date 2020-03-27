// see tutorial at http://learn.adafruit.com/trinket-usb-volume-knob

#include <Arduino.h>
#include "TrinketHidCombo.h"
#include "Signal.h"

// When the dial moves the mouse cursor the wrong way, reverse the values of
// the PIN_ENCODER constants.
#define PIN_ENCODER_A PIN2
#define PIN_ENCODER_B PIN0
#define PIN_ENCODER_SWITCH PIN1

#define SWITCH_PRESSED LOW
#define SWITCH_DEBOUNCE_DELAY 5

// You may comment out the following line if you do not want the rotary
// encoder switch enabled (or if you don't have a switch).
//#define SWITCH_ENABLED

static byte lastEncoderA;
static boolean switchIsPressed = false;

// Signal led(PIN3);

void setup()
{
  // Signal on the built-in led that code is about to run. I change the counter
  // on each compile to ensure that I'm running current code. Sometimes the
  // Digispark fails to program and I miss the error message in the output.
  // led.blink(500, 3);
  
  // Set pins as input with internal pull-up resistors enabled.
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  digitalWrite(PIN_ENCODER_A, HIGH);
  digitalWrite(PIN_ENCODER_B, HIGH);

#ifdef SWITCH_ENABLED
  // Set the switch as input with internal pull-up resistor enabled.
  // Note: On the Tiny85, the LED interferes with the very weak 25Kohm
  // pull-up resistors. There is a tiny white-box above P3 between the pin 5
  // of the integrated circuit and the LED. You need to carefully cut the 
  // copper trace under the white box to disable the LED and enable proper
  // digital read of the switch.
  pinMode(PIN_ENCODER_SWITCH, INPUT);
  digitalWrite(PIN_ENCODER_SWITCH, HIGH);
#endif

  // Start the USB.
  TrinketHidCombo.begin();

  //Capture current encoder state.
  lastEncoderA = digitalRead(PIN_ENCODER_A);
}

void loop()
{
  // How a rotary encoder works:
  //
  // Clockwise                  Counter-clockwise
  //      __    __    __        __    __    __
  // A __|  |__|  |__|  |__       |__|  |__|  |__
  //         0  1  0  1            0  1  0  1
  //
  //        __    __    __           __    __
  // B ____|  |__|  |__|  |__   ____|  |__|  |__
  //         1  0  1  0            0  1  0  1
  //
  // Output A and B are 90-degrees out of phase. When the values differ,
  // the direction is clockwise. When the values are the same, direction
  // is counter-clockwise. Only check output B when output A changes state.
  //
  // See: https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/

  byte currentEncoderA = digitalRead(PIN_ENCODER_A);

  if (currentEncoderA != lastEncoderA)
  {
    // Output A has changed state.
    if (digitalRead(PIN_ENCODER_B) != currentEncoderA)
    {
      // Clockwise - output A differs from output B
      TrinketHidCombo.pressMultimediaKey(MMKEY_VOL_UP);
    }
    else
    {
      // Counter-clockwise - output A is same as B
      TrinketHidCombo.pressMultimediaKey(MMKEY_VOL_DOWN);
    }
  }
  lastEncoderA = currentEncoderA;

#ifdef SWITCH_ENABLED
  if (digitalRead(PIN_ENCODER_SWITCH) == SWITCH_PRESSED)
  {
    // Send event only on transition, not while button is held down.
    if (switchIsPressed == false)
    {
      TrinketHidCombo.pressMultimediaKey(MMKEY_MUTE);
      delay(SWITCH_DEBOUNCE_DELAY);
    }
    switchIsPressed = true;
  }
  else
  {
    if (switchIsPressed)
    {
      delay(SWITCH_DEBOUNCE_DELAY);
    }
    switchIsPressed = false;
  }
#endif

  // USB maintenance call
  TrinketHidCombo.poll();
}
