#include <SoftSerial.h>
#include <FastCRC.h>

#define RX_PIN 5
#define TX_PIN 4
#define SERIAL_BAUD 9600

#define MOT_1_PIN 0 // TODO color
#define MOT_2_PIN 2 // TODO color
#define MOT_3_PIN 1 // TODO color
#define MOT_4_PIN 4 // TODO color
#define MOT_STEPS 2038L // the number of steps in one revolution of the 28BYJ-48 motor
#define MODULE_STEPS ((MOT_STEPS * 39L) / 32L) // 39:32 module gear ratio

#define POS_SENSE_PIN 3

#define STATUS_MOVING 1
#define STATUS_IDLE 0
#define POS_UNINIT ((1L << 12) - 1L) // 12 ones binary

SoftSerial serial(RX_PIN, TX_PIN, true /* inverse logic */);
FastCRC8 CRC8;

bool curStatus;
long targetPosition = POS_UNINIT;

void setup() {
  pinMode(POS_SENSE_PIN, INPUT);
  pinMode(MOT_1_PIN, OUTPUT);
  pinMode(MOT_2_PIN, OUTPUT);
  pinMode(MOT_3_PIN, OUTPUT);
  pinMode(MOT_4_PIN, OUTPUT);
  serial.begin(SERIAL_BAUD);

  // Turn all output off by default
  digitalWrite(MOT_1_PIN, LOW);
  digitalWrite(MOT_2_PIN, LOW);
  digitalWrite(MOT_3_PIN, LOW);
  digitalWrite(MOT_4_PIN, LOW);
}

void loop() {
  if (targetPosition != POS_UNINIT) {
    curStatus = stepOnceTowards(targetPosition) ? STATUS_IDLE : STATUS_MOVING;
  }
  handleIncoming();
}

void handleIncoming() {
  // Packet buffer includes 3 bytes of data as descibed below and one byte of checksum.
  // Packet structure:
  // Byte 0, bits 0-7: MSBits of Src/Dst
  // Byte 1, bits 6-7: LSBits of Src/Dst
  // Byte 1, bits 0-5: MSBits of Position (ignored for response packet)
  // Byte 2, bits 2-7: LSBits of Position (ignored for response packet)
  // Byte 2, bit 1:    Packet Type (0: response, 1: command)
  // Byte 2, bit 0:    Status (0: idle, 1: moving)
  // Byte 3:           SMBUS CRC-8 checksum
  static byte buf[4] = {0, 0, 0, 0};
  
  if (!serial.available()) {
    return;
  }

  buf[0] = buf[1];
  buf[1] = buf[2];
  buf[2] = buf[3];
  buf[3] = serial.read();

  if (CRC8.smbus(buf, 3) != buf[3]) {
    return;
  }

  // Extract packet data
  unsigned long srcDst = ((unsigned long)buf[0] << 2) | ((unsigned long)buf[1] >> 6);
  bool cmdPacket = ((unsigned long)buf[2] >> 1) & 1L;

  if (srcDst == 0) { // This packet was destined for us
    if (!cmdPacket) { // Invalid packet - must be a command
      return;
    }
    buf[2] = (unsigned long)buf[2] & ~0b10L; // Change packet type to response
    buf[2] = ((unsigned long)buf[2] & ~1L) | (curStatus == STATUS_IDLE ? 0L : 1L); // Set current status
    targetPosition = ((((1L << 6) - 1L) & (unsigned long)buf[1]) << 6) | ((unsigned long)buf[2] >> 2); // Update target position
  }

  // Update src/dst & CRC
  srcDst = srcDst + (cmdPacket ? -1L : 1L);
  buf[3] = CRC8.smbus(buf, 3);

  // Send out packet & clear buffer for future receptions.
  serial.write(buf, 4);
  buf[0] = buf[1] = buf[2] = buf[3] = 0;
}

// Returns true if we've arrived at loc, otherwise false and step forward.
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
