// AVR High-voltage Serial Programmer
// Originally created by Paul Willoughby 03/20/2010
// http://www.rickety.us/2010/03/arduino-avr-high-voltage-serial-programmer/
// Inspired by Jeff Keyzer http://mightyohm.com
// Serial Programming routines from ATtiny25/45/85 datasheet

// Desired fuse configuration
#define  HFUSE  0xDF   // Defaults for ATtiny25/45/85
#define  LFUSE  0x62   



#define  RST     13    // Output to level shifter for !RESET from transistor to Pin 1
#define  CLKOUT  12    // Connect to Serial Clock Input (SCI) Pin 2
#define  DATAIN  11    // Connect to Serial Data Output (SDO) Pin 7
#define  INSTOUT 10    // Connect to Serial Instruction Input (SII) Pin 6
#define  DATAOUT  9    // Connect to Serial Data Input (SDI) Pin 5 
#define  VCC      8    // Connect to VCC Pin 8




int inByte = 0;         // incoming serial byte Computer
int inData = 0;         // incoming serial byte AVR

void setup()
{
  // Set up control lines for HV parallel programming
  pinMode(VCC, OUTPUT);
  pinMode(RST, OUTPUT);
  pinMode(DATAOUT, OUTPUT);
  pinMode(INSTOUT, OUTPUT);
  pinMode(CLKOUT, OUTPUT);
  pinMode(DATAIN, OUTPUT);  // configured as input when in programming mode
  
  // Initialize output pins as needed
  digitalWrite(RST, HIGH);  // Level shifter is inverting, this shuts off 12V
  
  // start serial port at 9600 bps:
  Serial.begin(9600);
  
  establishContact();  // send a byte to establish contact until receiver responds 
  
}


void loop()
{
  // if we get a valid byte, run:
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
    Serial.println(byte(inByte));
    Serial.println("Entering programming Mode\n");

    // Initialize pins to enter programming mode
    pinMode(DATAIN, OUTPUT);  //Temporary
    digitalWrite(DATAOUT, LOW);
    digitalWrite(INSTOUT, LOW);
    digitalWrite(DATAIN, LOW);
    digitalWrite(RST, HIGH);  // Level shifter is inverting, this shuts off 12V
    
    // Enter High-voltage Serial programming mode
    digitalWrite(VCC, HIGH);  // Apply VCC to start programming process
    delayMicroseconds(20);
    digitalWrite(RST, LOW);   //Turn on 12v
    delayMicroseconds(10);
    pinMode(DATAIN, INPUT);   //Release DATAIN
    delayMicroseconds(300);
    
    //Programming mode
    
    readFuses();
    
    //Write hfuse
    Serial.println("Writing hfuse");
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x40, 0x4C);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, HFUSE, 0x2C);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x74);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x7C);
    
    //Write lfuse
    Serial.println("Writing lfuse\n");
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x40, 0x4C);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, LFUSE, 0x2C);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x64);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x6C);

    readFuses();    
    
    Serial.println("Exiting programming Mode\n");
    digitalWrite(CLKOUT, LOW);
    digitalWrite(VCC, LOW);
    digitalWrite(RST, HIGH);   //Turn off 12v
  }
}


void establishContact() {
  while (Serial.available() <= 0) {
    Serial.println("Enter a character to continue");   // send an initial string
    delay(1000);
  }
}

int shiftOut2(uint8_t dataPin, uint8_t dataPin1, uint8_t clockPin, uint8_t bitOrder, byte val, byte val1)
{
	int i;
        int inBits = 0;
        //Wait until DATAIN goes high
        while (!digitalRead(DATAIN));
        
        //Start bit
        digitalWrite(DATAOUT, LOW);
        digitalWrite(INSTOUT, LOW);
        digitalWrite(clockPin, HIGH);
  	digitalWrite(clockPin, LOW);
        
	for (i = 0; i < 8; i++)  {
                
		if (bitOrder == LSBFIRST) {
			digitalWrite(dataPin, !!(val & (1 << i)));
                        digitalWrite(dataPin1, !!(val1 & (1 << i)));
                }
		else {
			digitalWrite(dataPin, !!(val & (1 << (7 - i))));
                        digitalWrite(dataPin1, !!(val1 & (1 << (7 - i))));
                }
                inBits <<=1;
                inBits |= digitalRead(DATAIN);
                digitalWrite(clockPin, HIGH);
		digitalWrite(clockPin, LOW);
                
	}

        
        //End bits
        digitalWrite(DATAOUT, LOW);
        digitalWrite(INSTOUT, LOW);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
        
        return inBits;
}

void readFuses(){
     //Read lfuse
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x04, 0x4C);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x68);
    inData = shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x6C);
    Serial.print("lfuse reads as ");
    Serial.println(inData, HEX);
    
    //Read hfuse
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x04, 0x4C);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x7A);
    inData = shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x7E);
    Serial.print("hfuse reads as ");
    Serial.println(inData, HEX);
    
    //Read efuse
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x04, 0x4C);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x6A);
    inData = shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x6E);
    Serial.print("efuse reads as ");
    Serial.println(inData, HEX);
    Serial.println(); 
}
