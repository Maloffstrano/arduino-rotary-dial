// see tutorial at http://learn.adafruit.com/trinket-usb-volume-knob

#include <Arduino.h>
#include "TrinketKeyboard.h"

// When the dial moves the mouse cursor the wrong way, reverse the values of
// the PIN_ENCODER constants.
#define PIN_ENCODER_A PIN2
#define PIN_ENCODER_B PIN0
#define PIN_ENCODER_SWITCH PIN1

#define SWITCH_PRESSED LOW
#define SWITCH_DEBOUNCE_DELAY 5

// You may comment out the following line if you do not want the rotary
// encoder switch enabled (or if you don't have a switch).
#define SWITCH_ENABLED

#ifdef SWITCH_ENABLED
static boolean switchIsPressed = false;
#endif

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

  TrinketKeyboard.begin(); // start the USB device engine and enumerate

  // get an initial reading on the encoder pins
  if (digitalRead(PIN_ENCODER_A) == LOW)
  {
    encoderPositionPrevious |= encoderLowA;
  }
  if (digitalRead(PIN_ENCODER_B) == LOW)
  {
    encoderPositionPrevious |= encoderLowB;
  }

#ifdef SWITCH_ENABLED
  // Set the switch as input with internal pull-up resistor enabled.
  // Note: On the Tiny85, the LED interferes with the very weak 25Kohm
  // pull-up resistors. There is a tiny white-box above P3 between the pin 5
  // of the integrated circuit and the LED. You need to carefully cut the
  // copper trace under the white box to disable the LED and enable proper
  // digital read of the switch.
  //
  // See: https://digistump.com/wiki/digispark/tutorials/basics
  pinMode(PIN_ENCODER_SWITCH, INPUT);
  digitalWrite(PIN_ENCODER_SWITCH, HIGH);
#endif
}

void loop()
{
  signed char encoderRotation = encoderRotationNone;

  byte encoderPositionCurrent = 0 /*encoderBothHigh*/;

  // How a rotary encoder works:
  //
  // Clockwise                      Counter-clockwise
  //          ____        ____        ____        ____
  // A ______|    |______|    |   ___|    |______|    |___
  //    0  0  1  1  0  0  1  1     0  1  1  0  0  1  1  0
  //    |  |  |  |  |  |  |  |     |  |  |  |  |  |  |  |
  //    |  |  |  |  |  |  |  |     |  |  |  |  |  |  |  |
  //    1  2  3  4  1  2  3  4     1  2  3  4  1  2  3  4
  //    |  |  |  |  |  |  |  |     |  |  |  |  |  |  |  |
  //    |  |  |  |  |  |  |  |     |  |  |  |  |  |  |  |
  //       ____        ____              ____        ____
  // B ___|    |______|    |___   ______|    |______|    |
  //    0  1  1  0  0  1  1  0     0  0  1  1  0  0  1  1
  //          *           *              *           *
  //
  // Each segment 1, 2, 3, 4 is 90-degrees. All four is 360-degrees.
  //
  // Written together, the signals from A and B look like this:
  //
  // A  0  0  1  1  0  0  1  1     0  1  1  0  0  1  1  0
  // B  0  1  1  0  0  1  1  0     0  0  1  1  0  0  1  1
  //          *           *              *           *
  //
  // Output A and B are 90-degrees out of phase. When the values differ,
  // the direction is clockwise. When the values are the same, direction
  // is counter-clockwise.
  //
  // The * indicates the state that the code refers to as: encoderFlagMidStep
  //
  // See: https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/
  // See: wikipedia article PLACE THAT FIRST!

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

      // Check leading and trailing edge, or when an edge is missing, require the
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

#ifdef SWITCH_ENABLED
  if (digitalRead(PIN_ENCODER_SWITCH) == SWITCH_PRESSED)
  {
    // Send event only on transition, not while button is held down.
    if (switchIsPressed == false)
    {
      TrinketKeyboard.pressKey(0, KEYCODE_F);
      TrinketKeyboard.pressKey(0, 0);
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

  switch (encoderRotation)
  {    
    case encoderRotationRight:
      TrinketKeyboard.pressKey(0, KEYCODE_R);
      TrinketKeyboard.pressKey(0, 0);
      break;
    case encoderRotationLeft:
      TrinketKeyboard.pressKey(0, KEYCODE_L);
      TrinketKeyboard.pressKey(0, 0);
      break;
    default:
      TrinketKeyboard.poll();
      break;
  }
}
