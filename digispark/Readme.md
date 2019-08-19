You need to flash your Digispark board to ensure it has the latest
bootloader as well as to disable the reset pin in order to be able
to use it as GPIO.

# Requirements

* You need a full fledged Arduino board like Nano or Duo to use as
programmer for the Digispark. Nano is your best bet as it's cheap and
it's the one I've used myself.
* `avrdude` or other AVRISP compatible programmer software.

# Steps

1. Program `arduino_isp.ino` in this directory to your Arduino Nano.
2. Hook up Nano to Digispark using the following pin mapping:

  * Digispark P0 (MOSI) -> Nano D11 (MOSI)
  * Digispark P1 (MISO) -> Nano D12 (MISO)
  * Digispark P2 (SCK) -> Nano D13 (SCK)
  * Digispark P5 (RESET) -> Nano D10

TODO
