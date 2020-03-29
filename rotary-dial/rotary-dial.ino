// see tutorial at http://learn.adafruit.com/trinket-usb-volume-knob

#include <Arduino.h>
#include "TrinketHidCombo.h"

// When the dial moves the mouse cursor the wrong way, reverse the values of
// the PIN_ENCODER constants.
#define PIN_ENCODER_A PIN2
#define PIN_ENCODER_B PIN0

signed char const encoderRotationNone = 0;
signed char const encoderRotationRight = 1;
signed char const encoderRotationLeft = -1;

const byte encoderLowA = _BV(0);
const byte encoderLowB = _BV(1);
const byte encoderBothHigh = 0;
const byte encoderBothLow = encoderLowA | encoderLowB;
const byte encoderHighBLowA = encoderLowA;
const byte encoderLowBHighA = encoderLowB;

//                             bit number 43210
const byte encoderFlagResetForNextLoop = B00000;
const byte encoderFlagHighToLowEdgeA   = B00001;
const byte encoderFlagHighToLowEdgeB   = B00010;
const byte encoderFlagMidStep          = B10000;
const byte encoderFlagLowToHighEdgeA   = B01000;
const byte encoderFlagLowToHighEdgeB   = B00100;
//                             bit number 43210

static byte encoderFlags = encoderFlagResetForNextLoop;
static byte encoderPositionPrevious = encoderBothHigh;

void setup()
{
  // set pins as input with internal pull-up resistors enabled
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  digitalWrite(PIN_ENCODER_A, HIGH);
  digitalWrite(PIN_ENCODER_B, HIGH);

  TrinketHidCombo.begin(); // start the USB device engine and enumerate

  // get an initial reading on the encoder pins
  if (digitalRead(PIN_ENCODER_A) == LOW)
  {
    encoderPositionPrevious |= encoderLowA;
  }
  if (digitalRead(PIN_ENCODER_B) == LOW)
  {
    encoderPositionPrevious |= encoderLowB;
  }
}

void loop()
{
  signed char encoderRotation = encoderRotationNone;

  byte encoderPositionCurrent = 0 /*encoderBothHigh*/;

  // How a rotary encoder works:
  //
  // Clockwise                        Counter-clockwise
  //   ___      ____      ____            ____      ____      __
  // A    |____|    |____|    |____   ___|    |____|    |____|
  //       0    1    0    1               0    1    0    1
  //
  //     ____      ____      ____     _      ____      ____
  // B _|    |____|    |____|    |_    |____|    |____|    |____
  //       1    0    1    0               0    1    0    1
  //
  // Output A and B are 90-degrees out of phase. When the values differ,
  // the direction is clockwise. When the values are the same, direction
  // is counter-clockwise.
  //
  // See: https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/

  byte encoderCurrentPosition = encoderBothHigh;

  if (digitalRead(PIN_ENCODER_A) == LOW)
  {
    encoderCurrentPosition |= encoderLowA;
  }
  if (digitalRead(PIN_ENCODER_B) == LOW)
  {
    encoderCurrentPosition |= encoderLowB;
  }

  // Has encoder rotated?
  if (encoderPositionCurrent != encoderPositionPrevious)
  {
    if (encoderPositionPrevious == encoderBothHigh)
    {
      // Has encoder transitioned high to low; the leading edge?
      if (encoderPositionCurrent == encoderHighBLowA)
      {
        encoderFlags |= encoderFlagHighToLowEdgeA;
      }
      else if (encoderPositionCurrent == encoderLowBHighA)
      {
        encoderFlags |= encoderFlagHighToLowEdgeB;
      }
    }

    if (encoderCurrentPosition == encoderBothLow)
    {
      encoderFlags |= encoderFlagMidStep;
    }
    else if (encoderCurrentPosition == encoderBothHigh)
    {
      // Has encoder transitioned low to high; the trailing edge?
      if (encoderPositionPrevious == encoderLowB)
      {
        encoderFlags |= encoderFlagLowToHighEdgeB;
      }
      else if (encoderPositionPrevious == encoderLowA)
      {
        encoderFlags |= encoderFlagLowToHighEdgeA;
      }

      // Check leading an trailing edge, or when an edge is missing, require the
      // middle step. This will reject bounces and false movements.
      if ((encoderFlags & encoderFlagHighToLowEdgeA) && (encoderFlags & (encoderFlagLowToHighEdgeB | encoderFlagMidStep)))
      {
        encoderRotation = encoderRotationRight;
      }
      else if ((encoderFlags & encoderFlagLowToHighEdgeB) && (encoderFlags & (encoderFlagHighToLowEdgeA | encoderFlagMidStep)))
      {
        encoderRotation = encoderRotationRight;
      }
      else if ((encoderFlags & encoderFlagHighToLowEdgeB) && (encoderFlags & (encoderFlagLowToHighEdgeA | encoderFlagMidStep)))
      {
        encoderRotation = encoderRotationLeft;
      }
      else if ((encoderFlags & encoderFlagLowToHighEdgeA) && (encoderFlags & (encoderFlagHighToLowEdgeB | encoderFlagMidStep)))
      {
        encoderRotation = encoderRotationLeft;
      }

      encoderFlags = encoderFlagResetForNextLoop;
    }
  }

  encoderPositionPrevious = encoderCurrentPosition;

  switch (encoderRotation) {
  case encoderRotationRight:
    TrinketHidCombo.pressMultimediaKey(MMKEY_VOL_UP);
    break;
  case encoderRotationLeft:
    TrinketHidCombo.pressMultimediaKey(MMKEY_VOL_DOWN);
    break;
  default:
    TrinketHidCombo.poll();
    break;
  }
}
