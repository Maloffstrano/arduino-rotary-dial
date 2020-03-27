// #include <cmdline_defs.h>
// #include <TrinketHidCombo.h>
// #include <TrinketHidComboC.h>
// #include <usbconfig.h>

// see tutorial at http://learn.adafruit.com/trinket-usb-volume-knob

#include <Arduino.h>
#include "TrinketHidCombo.h"
#include "Signal.h"

Signal led(PIN1);

// When the dial moves the mouse cursor the wrong way, reverse the values of
// the PIN_ENCODER constants.
#define PIN_ENCODER_A PIN2
#define PIN_ENCODER_B PIN0
#define TRINKET_PINx PINB

static byte enc_prev_pos = 0;
static byte enc_flags = 0;

void setup()
{
  led.blink(500);
  led.blink(500);
  led.blink(500);
  
  // set pins as input with internal pull-up resistors enabled
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  digitalWrite(PIN_ENCODER_A, HIGH);
  digitalWrite(PIN_ENCODER_B, HIGH);

  TrinketHidCombo.begin(); // start the USB device engine and enumerate

  // get an initial reading on the encoder pins
  if (digitalRead(PIN_ENCODER_A) == LOW)
  {
    enc_prev_pos |= _BV(0);
  }
  if (digitalRead(PIN_ENCODER_B) == LOW)
  {
    enc_prev_pos |= _BV(1);
  }
}

void loop()
{
  char enc_action = 0; // 1 or -1 if moved, sign is direction

  // note: for better performance, the code will now use
  // direct port access techniques
  // http://www.arduino.cc/en/Reference/PortManipulation
  byte enc_cur_pos = 0;
  
  // read in the encoder state first
  if (bit_is_clear(TRINKET_PINx, PIN_ENCODER_A))
  {
    enc_cur_pos |= _BV(0);
  }
  if (bit_is_clear(TRINKET_PINx, PIN_ENCODER_B))
  {
    enc_cur_pos |= _BV(1);
  }

  // if any rotation at all
  if (enc_cur_pos != enc_prev_pos)
  {
    if (enc_prev_pos == 0x00)
    {
      // this is the first edge
      if (enc_cur_pos == 0x01)
      {
        enc_flags |= _BV(0);
      }
      else if (enc_cur_pos == 0x02)
      {
        enc_flags |= _BV(1);
      }
    }

    if (enc_cur_pos == 0x03)
    {
      // this is when the encoder is in the middle of a "step"
      enc_flags |= _BV(4);
    }
    else if (enc_cur_pos == 0x00)
    {
      // this is the final edge
      if (enc_prev_pos == 0x02)
      {
        enc_flags |= _BV(2);
      }
      else if (enc_prev_pos == 0x01)
      {
        enc_flags |= _BV(3);
      }

      // check the first and last edge
      // or maybe one edge is missing, if missing then require the middle state
      // this will reject bounces and false movements
      if (bit_is_set(enc_flags, 0) && (bit_is_set(enc_flags, 2) || bit_is_set(enc_flags, 4)))
      {
        enc_action = 1;
      }
      else if (bit_is_set(enc_flags, 2) && (bit_is_set(enc_flags, 0) || bit_is_set(enc_flags, 4)))
      {
        enc_action = 1;
      }
      else if (bit_is_set(enc_flags, 1) && (bit_is_set(enc_flags, 3) || bit_is_set(enc_flags, 4)))
      {
        enc_action = -1;
      }
      else if (bit_is_set(enc_flags, 3) && (bit_is_set(enc_flags, 1) || bit_is_set(enc_flags, 4)))
      {
        enc_action = -1;
      }

      enc_flags = 0; // reset for next time
    }
  }

  enc_prev_pos = enc_cur_pos;

  if (enc_action > 0)
  {
    TrinketHidCombo.pressMultimediaKey(MMKEY_VOL_UP);
  }
  else if (enc_action < 0)
  {
    TrinketHidCombo.pressMultimediaKey(MMKEY_VOL_DOWN);
  }
  else
  {
    TrinketHidCombo.poll(); // do nothing, check if USB needs anything done
  }
}
