#include <Arduino.h>

String inputString = "";      // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

void PulseClock() {
  PORTD &= B11111011;
  PORTD |= B00000100;
}

void PulseAddressLoad() {
  PORTD &= B11110111;
  PORTD |= B00001000;
}

uint16_t LoadAddress() {
  uint16_t address = 0;
  PulseAddressLoad();
  for (int i = 0; i < 16; i++) {
    address = address << 1 | (PIND >> 4 & B00000001);
    PulseClock();
  };
  return address;
}

void PulseDataLockAndLoad() {
  PORTD &= B10111111;
  PORTD |= B01000000;
}

void DataEnabled() {
  PORTC &= B11011111;
}

void DataDisabled() {
  PORTC |= B00100000;
}

uint16_t LoadData() {
  uint8_t data = 0;
  PulseDataLockAndLoad();
  for (int i = 0; i < 8; i++) {
    data = data << 1 | (PIND >> 5 & B00000001);
    PulseClock();
  };
  return data;
}

void SetData(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    PORTC = (PORTC & B11101111) | ((data & B10000000) >> 3);
    PulseClock();
    data <<= 1;
  };
  PulseDataLockAndLoad();
  DataEnabled();
}

void setup() {
  // initialize serial:
  Serial.begin(BAUD_RATE);

  // Configure pins - Binary style :)
                      // DataLoad,Data In ,Addr In ,AddrLoad,Clock  
  PORTD |= B01001100; // PD6(D6)H,PD5(D5)H,PD4(D4)H,PD3(D3)H,PD2(D2)H
  DDRD  |= B01001100; // PD6(D6)O,                 ,PD3(D3)O,PD2(D2)O
  DDRD  &= B11001111; //         ,PD5(D5)I,PD4(D4)I,

  PORTC |= B00110000; // PC4(A4)H,PC5(A5)H,
  DDRC  |= B00110000; // PC4(A4)O,PC5(A5)O,

  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
}

uint16_t address;
void loop() {
  // print the string when a newline arrives:
  if (stringComplete) {
    switch (inputString.charAt(0)) {
      case 'a': // retrieve address
        address = LoadAddress();
        Serial.write(address & 0xFF);
        Serial.write(address >> 8);
        break;
      case 'd': // retrieve data
        Serial.write(LoadData());
        break;
      case 'D': // set data
        if (inputString.length() > 2) {
          SetData(inputString.charAt(1));
        } else {
          DataDisabled();
        }
        break;
    }
    inputString = "";
    stringComplete = false;
  }
}

/*
  SerialEventRun occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEventRun() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}