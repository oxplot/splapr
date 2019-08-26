// splapr_module.ino - Controller code that runs on a single splapr module.
//
// Copyright (C) 2019 Mansour Behabadi <mansour@oxplot.com>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// NOTE This code is intentionally simple as minimize bugs and increase robustness
// because splapr modules are not meant to be reprogrammed or maintained.
// All the heavy lifting is done in the more powerful and accessible main controller
// which also has inter-module knowledge.

#ifndef ARDUINO_AVR_PRO
#error This code is only tested for Arduino Pro Mini.
#endif

#include <FastCRC.h>

#define MOT_1_PIN A1 // Blue wire
#define MOT_2_PIN A0 // Yellow wire
#define MOT_3_PIN A2 // Pink wire
#define MOT_4_PIN 13 // Orange wire

// Due to arrangement of the components and position of the pins, it's much easier
// to power the hall effect sensor through the uC instead of routing wires around to
// VCC and GND on the board.
#define HALL_VCC_PIN 2
#define HALL_GND_PIN 3
#define HALL_OUT_PIN 4

#define SERIAL_BAUD 115200
#define MOT_STEPS 2038L // the number of steps in one revolution of the 28BYJ-48 motor
#define MODULE_STEPS ((MOT_STEPS * 39L) / 32L) // 39:32 module gear ratio
#define STEP_DELAY 2000
#define POS_UNINIT ((1L << 12) - 1L) // 12 ones binary
#define DATA_FLOW_RTL 1
#define DATA_FLOW_LTR -1
#define DATA_FLOW_DIR DATA_FLOW_LTR

FastCRC8 CRC8;

void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(HALL_VCC_PIN, OUTPUT);
  digitalWrite(HALL_VCC_PIN, HIGH);
  pinMode(HALL_GND_PIN, OUTPUT);
  pinMode(HALL_OUT_PIN, INPUT);
  pinMode(MOT_1_PIN, OUTPUT);
  pinMode(MOT_2_PIN, OUTPUT);
  pinMode(MOT_3_PIN, OUTPUT);
  pinMode(MOT_4_PIN, OUTPUT);
}

long targetPos = POS_UNINIT;
long curPos = POS_UNINIT;

void loop() {
  handleComms();

  if (curPos == targetPos) {
    return;
  }

  byte lastSenseState = digitalRead(HALL_OUT_PIN);
  motStep();
  if (lastSenseState == LOW && digitalRead(HALL_OUT_PIN) == HIGH) {
    curPos = 0L;
  } else if (curPos >= 0L && curPos < MODULE_STEPS - 1) {
    curPos++;
  }

  // Turn motor power off to avoid consuming power and heating up the motor.
  digitalWrite(MOT_1_PIN, LOW);
  digitalWrite(MOT_2_PIN, LOW);
  digitalWrite(MOT_3_PIN, LOW);
  digitalWrite(MOT_4_PIN, LOW);
}

void motStep() {
  static signed char curStep = 0;
  switch (curStep * DATA_FLOW_DIR) {
    case 0:
      digitalWrite(MOT_1_PIN, HIGH);
      digitalWrite(MOT_2_PIN, LOW);
      digitalWrite(MOT_3_PIN, LOW);
      digitalWrite(MOT_4_PIN, HIGH);
    break;
    case 1:
      digitalWrite(MOT_1_PIN, LOW);
      digitalWrite(MOT_2_PIN, HIGH);
      digitalWrite(MOT_3_PIN, LOW);
      digitalWrite(MOT_4_PIN, HIGH);
    break;
    case 2:
      digitalWrite(MOT_1_PIN, LOW);
      digitalWrite(MOT_2_PIN, HIGH);
      digitalWrite(MOT_3_PIN, HIGH);
      digitalWrite(MOT_4_PIN, LOW);
    break;
    case 3:
      digitalWrite(MOT_1_PIN, HIGH);
      digitalWrite(MOT_2_PIN, LOW);
      digitalWrite(MOT_3_PIN, HIGH);
      digitalWrite(MOT_4_PIN, LOW);
    break;
  }
  delayWithComms(STEP_DELAY);
  curStep = (curStep + DATA_FLOW_DIR) % 4;
}

// Delay while handling comms.
void delayWithComms(unsigned int d) {
  unsigned long dStart = micros();
  unsigned long dEnd = micros() + d;
  if (dStart < dEnd) {
    while (micros() >= dStart && micros() < dEnd) handleComms;
  } else {
    while (micros() >= dStart || micros() < dEnd) handleComms;
  }
}

void handleComms() {
  // Each packet is made up of 5 bytes. MSBits are transmitted first. Packet format is as follows:
  // Byte 0:           Constant 0xd4 as packet header. Also helps with avoiding valid packets of all zeros.
  // Byte 1, bits 0-7: MSBits of Src/Dst
  // Byte 2, bits 6-7: LSBits of Src/Dst
  // Byte 2, bits 0-5: MSBits of Position (ignored for response packet)
  // Byte 3, bits 2-7: LSBits of Position (ignored for response packet)
  // Byte 3, bit 1:    Packet Type (0: response, 1: command)
  // Byte 3, bit 0:    Status (0: idle, 1: moving)
  // Byte 4:           SMBUS CRC-8 checksum
  static byte buf[5];
  
  if (!Serial.available()) {
    return;
  }

  buf[0] = buf[1];
  buf[1] = buf[2];
  buf[2] = buf[3];
  buf[3] = buf[4];
  buf[4] = (byte) Serial.read();

  if (buf[0] != 0xd4 || CRC8.smbus(buf, 4) != buf[4]) {
    return;
  }

  // Extract packet data
  unsigned long srcDst = ((unsigned long) buf[1] << 2) | ((unsigned long) buf[2] >> 6);
  bool cmdPacket = ((unsigned long) buf[3] >> 1) & 1L;
  unsigned long pos = ((((1L << 6) - 1L) & (unsigned long) buf[2]) << 6) | ((unsigned long) buf[3] >> 2);

  if (srcDst == 0) { // This packet was destined for us
    if (!cmdPacket) { // Invalid packet - must be a command - drop
      return;
    }
    if (pos != POS_UNINIT && pos > MODULE_STEPS - 1) { // Invalid position - drop
      return;
    }
    if (pos != POS_UNINIT) {
      targetPos = pos;
    }
    buf[3] = buf[3] & 0b11111101; // Change packet type to response
    cmdPacket = false;
    buf[3] = (buf[3] & 0b11111110) | (targetPos == curPos ? 0 : 1); // Set current status
    // Update position to current position
    buf[2] = (buf[2] & 0b11000000) | (curPos >> 6);
    buf[3] = (buf[3] & 0b00000011) | ((curPos & 0b00111111) << 2);
  }

  // Update src/dst & CRC
  srcDst = srcDst + (cmdPacket ? -1L : 1L);
  buf[1] = srcDst >> 2;
  buf[2] = (0b00111111 & buf[2]) | ((srcDst & 0b00000011) << 6);
  buf[4] = CRC8.smbus(buf, 4);

  // Send out packet
  Serial.write(buf, 5);
}
