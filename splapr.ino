#include <SoftSerial.h>

#define RX_PIN 5
#define TX_PIN 4
#define MOT_1_PIN 1
#define MOT_2_PIN 3
#define MOT_3_PIN 2
#define MOT_4_PIN 4
#define POS_SENSE_PIN 0
#define MOT_STEPS 2038L // the number of steps in one revolution of the 28BYJ-48 motor
#define MODULE_STEPS ((MOT_STEPS * 39L) / 32L) // 39:32 gear ratio

SoftSerial serial(RX_PIN, TX_PIN, true /* inverse logic */);

void setup() {
  pinMode(POS_SENSE_PIN, INPUT);
  pinMode(MOT_1_PIN, OUTPUT);
  pinMode(MOT_2_PIN, OUTPUT);
  pinMode(MOT_3_PIN, OUTPUT);
  pinMode(MOT_4_PIN, OUTPUT);
  serial.begin(9600);
}

void loop() {
  for (int i = 0; i < 10; i++) {
    stepOnceTowards(2000);
  }
}

bool stepOnceTowards(long loc) {
  static long realLoc = -1L;
  static long normalizedLoc = 0L;
  static long lastMaxLoc = MODULE_STEPS - 1L;

  if (loc > lastMaxLoc) {
    loc = 0;
  }
  if (realLoc >= 0) {
    if (loc == normalizedLoc) {
      return true;
    }
    
    if (normalizedLoc != realLoc) {
      normalizedLoc++;
      if (normalizedLoc > lastMaxLoc) {
        normalizedLoc = 0;
      }
      if (loc == normalizedLoc) {
        return true;
      }
      return false;
    }
  
    normalizedLoc++;
  }

  do {
    byte lastSenseState = digitalRead(POS_SENSE_PIN);
    motStep();
    if (lastSenseState == LOW && digitalRead(POS_SENSE_PIN) == HIGH && (realLoc == -1 || realLoc > MODULE_STEPS / 2)) {
      if (realLoc >= 0) {
        lastMaxLoc = realLoc;
      }
      realLoc = 0L;
    } else if (realLoc >= 0L) {
      realLoc++;
    }
  } while (digitalRead(TX_PIN) == HIGH);

  serial.println(realLoc);

  // Turn motor power off to avoid consuming power and heating up the motor.
  digitalWrite(MOT_1_PIN, LOW);
  digitalWrite(MOT_2_PIN, LOW);
  digitalWrite(MOT_3_PIN, LOW);
  digitalWrite(MOT_4_PIN, LOW);
}

void motStep() {
  static byte curStep = 0;
  switch (curStep) {
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
  delayMicroseconds(2000);
  curStep = (curStep + 1) % 4;
}
