// see tutorial at http://learn.adafruit.com/trinket-usb-volume-knob

#include <Arduino.h>
#include "TrinketHidCombo.h"
#include "Signal.h"

// When the dial moves the mouse cursor the wrong way, reverse the values of
// the PIN_ENCODER constants.
#define PIN_ENCODER_A PIN2
#define PIN_ENCODER_B PIN0

static byte lastEncoderA;

Signal led(PIN1);

void setup()
{
  // Signal on the built-in led that code is about to run. I change the counter
  // on each compile to ensure that I'm running current code. Sometimes the
  // Digispark fails to program and I miss the error message in the output.
  led.blink(500, 3);
  
  // Set pins as input with internal pull-up resistors enabled.
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  digitalWrite(PIN_ENCODER_A, HIGH);
  digitalWrite(PIN_ENCODER_B, HIGH);

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
  else
  {
    // USB maintenance call
    TrinketHidCombo.poll();
  }

  lastEncoderA = currentEncoderA;
}
