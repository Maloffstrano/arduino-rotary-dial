// Name: Rotary-dial
// Project: A rotary encoder used to simulate a dial for games on a M.A.M.E.
//          arcade cabinet. Simulates a three character keyboard.
// Author: Sheldon Maloff
// Creation Date: 2020-April-02
// License: Public domain.
// 
//
// This code is based off the tutorial at:
// http://learn.adafruit.com/trinket-usb-volume-knob
// It has been modified to use named constants in place of all the cryptic,
// hard-coded, bit positions.

// IMPORTANT! The file usbconfig.h in the Trinket keyboard library has been
// modified to uniquely identify this device!

#include <Arduino.h>
#include "TrinketKeyboard.h"

// You may comment out the following line if you do not want the rotary
// encoder switch enabled (or if you don't have a switch).
#define SWITCH_ENABLED

#ifdef SWITCH_ENABLED
static boolean switchIsPressed = false;
#endif

// Keycodes sent in response to left and right rotational motion and switch.
// Choose whatever characters you want to send. No other code modifications
// are required. I chose (L)eft, (R)ight and (F)ire.
const int keycodeRotateLeft  = KEYCODE_L;
const int keycodeRotateRight = KEYCODE_R;
const int keycodeSwitch      = KEYCODE_F;
const int keycodeRelease     = 0;
const int keycodeNoModifiers = 0;

// Sending a full key press/release does not work with M.A.M.E. The keyboard
// dial will only move a character or paddle a small distance. M.A.M.E. is
// designed to keep a character moving as long as a key is pressed. This
// constant defines how long a character will move after the code determines
// the rotary encoder rotated. You can adjust it to adjust sensitivity.
const int send_duration_millis = 100;
const unsigned long keyboardTimerStopped = 0;
static unsigned long start_millis = keyboardTimerStopped;

const byte pinEncoderA = PIN2;
const byte pinEncoderB = PIN0;
const byte pinEncoderSwitch = PIN1;

const byte portReadPinMaskA = _BV(pinEncoderA);
const byte portReadPinMaskB = _BV(pinEncoderB);
const byte portReadSwitchMask = _BV(pinEncoderSwitch);
const byte portReadPinMaskBShift = 1;
const byte portReadPinMask = portReadPinMaskA | portReadPinMaskB | portReadSwitchMask;

const int encoderSwitchDebounceDelay = 5;
const byte encoderSwitchPressed = LOW;

signed char const encoderRotationNone = 0;
signed char const encoderRotationRight = 1;
signed char const encoderRotationLeft = -1;

const byte encoderLowA = _BV(0);
const byte encoderLowB = _BV(1);
const byte encoderBothHigh = 0;
const byte encoderBothLow = encoderLowA | encoderLowB;
const byte encoderHighBLowA = encoderLowA;
const byte encoderLowBHighA = encoderLowB;

const byte encoderFlagResetForNextLoop = B00000;
const byte encoderFlagHighToLowEdgeA   = B00001;
const byte encoderFlagHighToLowEdgeB   = B00010;
const byte encoderFlagMidStep          = B10000;
const byte encoderFlagLowToHighEdgeA   = B01000;
const byte encoderFlagLowToHighEdgeB   = B00100;

static byte encoderFlags = encoderFlagResetForNextLoop;
static byte encoderPositionPrevious = encoderBothHigh;

const byte encoderButtonBeginState = 0;
const byte encoderButtonLeftIdentificationState = 1;
const byte encoderButtonRightIdentificationState = 2;
const byte encoderButtonRunningState = 3;
static byte encoderButtonState = encoderButtonBeginState;

void setup()
{
  // Set pins as input with internal pull-up resistors enabled
  pinMode(pinEncoderA, INPUT);
  pinMode(pinEncoderB, INPUT);
  digitalWrite(pinEncoderA, HIGH);
  digitalWrite(pinEncoderB, HIGH);

  // get an initial reading on the encoder pins
  if (digitalRead(pinEncoderA) == LOW)
  {
    encoderPositionPrevious |= encoderLowA;
  }
  if (digitalRead(pinEncoderB) == LOW)
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
  pinMode(pinEncoderSwitch, INPUT);
  digitalWrite(pinEncoderSwitch, HIGH);
#endif
}

void loop()
{
  if (encoderButtonState != encoderButtonBeginState) {
    // Poll on every loop, to ensure the USB remains connected.
    TrinketKeyboard.poll();
  }

  signed char encoderRotation = encoderRotationNone;
  byte encoderPositionCurrent = encoderBothHigh;

  // How a rotary encoder works:
  //
  // Counter-clockwise            Clockwise
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
  //    |  |  |  |  |  |  |  |     |  |  |  |  |  |  |  |
  //         1111        2222           3333        2222
  //
  // Each segment 1, 2, 3, 4 is 90-degrees. All four is 360-degrees.
  //
  // The * indicates the state that the code refers to as: encoderFlagMidStep.
  // It always corresponds to phase 3.
  //
  // The numbers 1111 to 4444 correspond to the if-statements in the code
  // below. The code is looking for leading and trailing edges and mid-steps.
  // Each if statement covers phase 3 and a portion of phase 2 and 4.
  //
  // The signals from A and B look like this when written together:
  //
  // A  0  0  1  1  0  0  1  1     0  1  1  0  0  1  1  0
  // B  0  1  1  0  0  1  1  0     0  0  1  1  0  0  1  1
  //          *           *              *           *
  //
  // Output A and B are 90-degrees out of phase. When the values differ,
  // the direction is clockwise. When the values are the same, direction
  // is counter-clockwise.
  //
  // See: https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/
  // See: wikipedia article PLACE THAT FIRST!

  // Speed and accuracy: use a direct port read to get A & B and switch at once.
  byte portRead = (*portInputRegister(PORT_B_ID) & portReadPinMask);
  byte encoderCurrentPosition = encoderBothHigh;

  if ((portRead & portReadPinMaskA) == LOW)
  {
    encoderCurrentPosition |= encoderLowA;
  }
  if ((portRead & portReadPinMaskB) == LOW)
  {
    encoderCurrentPosition |= encoderLowB;
  }

  // Has encoder rotated?
  if (encoderPositionCurrent != encoderPositionPrevious)
  {
    if (encoderPositionPrevious == encoderBothHigh)
    {
      // Has encoder transitioned high to low (the leading edge)?
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
      // Has encoder transitioned low to high (the trailing edge)?
      if (encoderPositionPrevious == encoderLowB)
      {
        encoderFlags |= encoderFlagLowToHighEdgeB;
      }
      else if (encoderPositionPrevious == encoderLowA)
      {
        encoderFlags |= encoderFlagLowToHighEdgeA;
      }

      // Check leading and trailing edge, or when an edge is missing, require 
      // the middle step. This will reject mechanical bounces and false 
      // movements. Each statement corresponds to the diagram above.
      if ((encoderFlags & encoderFlagHighToLowEdgeA) && (encoderFlags & (encoderFlagLowToHighEdgeB | encoderFlagMidStep)))
      {
        encoderRotation = encoderRotationRight; /* 1111 in diagram above */
      }
      else if ((encoderFlags & encoderFlagLowToHighEdgeB) && (encoderFlags & (encoderFlagHighToLowEdgeA | encoderFlagMidStep)))
      {
        encoderRotation = encoderRotationRight; /* 2222 in diagram above */
      }
      else if ((encoderFlags & encoderFlagHighToLowEdgeB) && (encoderFlags & (encoderFlagLowToHighEdgeA | encoderFlagMidStep)))
      {
        encoderRotation = encoderRotationLeft; /* 3333 in diagram above */
      }
      else if ((encoderFlags & encoderFlagLowToHighEdgeA) && (encoderFlags & (encoderFlagHighToLowEdgeB | encoderFlagMidStep)))
      {
        encoderRotation = encoderRotationLeft; /* 4444 in diagram above */
      }

      encoderFlags = encoderFlagResetForNextLoop;
    }
  }

  encoderPositionPrevious = encoderCurrentPosition;

#ifdef SWITCH_ENABLED
  if ((portRead & portReadSwitchMask) == encoderSwitchPressed) 
  {
    // Send event only on transition, not while button is held down.
    if (switchIsPressed == false)
    {
      switch (encoderButtonState) {
        case encoderButtonBeginState:
          // Start the USB device engine and enumerate
          TrinketKeyboard.begin();
          encoderButtonState = encoderButtonLeftIdentificationState;
          break;
        case encoderButtonLeftIdentificationState:
          // On macOS, identify the keyboard by pressing the key to the right 
          // of the left SHIFT key
          TrinketKeyboard.pressKey(keycodeNoModifiers, KEYCODE_Z);
          TrinketKeyboard.pressKey(keycodeNoModifiers, keycodeRelease);
          encoderButtonState = encoderButtonRightIdentificationState;
          break;
        case encoderButtonRightIdentificationState:
          // On macOS, identify the keyboard by pressing the key to the left
          // of the right SHIFT key
          TrinketKeyboard.pressKey(keycodeNoModifiers, KEYCODE_SLASH);
          TrinketKeyboard.pressKey(keycodeNoModifiers, keycodeRelease);
          encoderButtonState = encoderButtonRunningState;
          break;
        case encoderButtonRunningState:
          // We're finally running. Issue the normal keypress.
          TrinketKeyboard.pressKey(keycodeNoModifiers, keycodeSwitch);
          TrinketKeyboard.pressKey(keycodeNoModifiers, keycodeRelease);
          break;
        }

        delay(encoderSwitchDebounceDelay);
    }
    switchIsPressed = true;
  }
  else
  {
    if (switchIsPressed)
    {
      delay(encoderSwitchDebounceDelay);
    }
    switchIsPressed = false;
  }
#endif

  if (encoderButtonState == encoderButtonRunningState)
  {
    switch (encoderRotation)
    {
    case encoderRotationRight:
      if (start_millis == keyboardTimerStopped)
      {
        TrinketKeyboard.pressKey(keycodeNoModifiers, keycodeRotateRight);
      }
      start_millis = millis(); // start or renew the timer
      break;
    case encoderRotationLeft:
      if (start_millis == keyboardTimerStopped)
      {
        TrinketKeyboard.pressKey(keycodeNoModifiers, keycodeRotateLeft);
      }
      start_millis = millis(); // start or renew the timer
      break;
    default:
      break;
    }
  }

  if (start_millis && millis() - start_millis > send_duration_millis) {
    // Keyboard timer is running and the duration expired, release the key
    TrinketKeyboard.pressKey(keycodeNoModifiers, keycodeRelease);
    start_millis = keyboardTimerStopped;
  }
}
