#include <Arduino.h>

uint8_t clock;
uint8_t nextClock;
uint8_t IRQ;
uint8_t nextIRQ;
uint8_t NMI;
uint8_t nextNMI;
uint8_t reset;
uint8_t nextReset;

uint64_t BIT_64 = 0x8000000000000000;
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

void PulseDataLockAndLatch() {
  PORTD &= B10111111;
  PORTD |= B01000000;
}
void DataEnabled() {
  PORTC &= B11011111;
}
void DataDisabled() {
  PORTC |= B00100000;
}
void SetData(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    PORTC = (PORTC & B11101111) | ((data & B10000000) >> 3);
    PulseClock();
    data <<= 1;
  };
  PulseDataLockAndLatch();
  DataEnabled();
}
uint16_t LoadData() {
  uint8_t data = 0;
  DataDisabled();
  PulseDataLockAndLatch();
  for (int i = 0; i < 8; i++) {
    data = data << 1 | (PIND >> 5 & B00000001);
    PulseClock();
  };
  return data;
}

void PulseFlagLoad() {
  PORTB &= B11111110;
  PORTB |= B00000001;
}
uint16_t LoadFlag() {
  uint8_t flag = 0;
  PulseFlagLoad();
  for (int i = 0; i < 8; i++) {
    flag = flag << 1 | (PIND >> 7 & B00000001);
    PulseClock();
  };
  return flag;
}

void PulseOpCodeLoad() {
  PORTB &= B11111110;
  PORTB |= B00000001;
}
uint16_t LoadOpCode() {
  uint8_t opCode = 0;
  PulseOpCodeLoad();
  for (int i = 0; i < 8; i++) {
    opCode = opCode << 1 | (PINB >> 5 & B00000001);
    PulseClock();
  };
  return opCode;
}

void PulseControlLinesLatch() {
  PORTB &= B11111101;
  PORTB |= B00000010;
}
void ControlLinesEnabled() {
  PORTB &= B11110111;
}
void ControlLinesDisabled() {
  PORTB |= B00001000;
}
void PulseControlLinesLoad() {
  PORTB |= B00010000;
  PORTB &= B11101111;
}
void SetControlLines(uint64_t ctrl) {
  for (int i = 0; i < 48; i++) {
    PORTB = (PORTB & B11111011) | ((ctrl & BIT_64) ? 4 : 0);
    PulseClock();
    ctrl <<= 1;
  };
  PulseControlLinesLatch();
  ControlLinesEnabled();
  PulseControlLinesLoad();
  ControlLinesDisabled();
}

void setup() {
  // initialize serial:
  Serial.begin(BAUD_RATE);

  // Configure pins - Binary style :)
                      // Flag In ,DataLoad,Data In ,Addr In ,AddrLoad,Clock  
  PORTD |= B11111100; // PD7(D7)H,PD6(D6)H,PD5(D5)H,PD4(D4)H,PD3(D3)H,PD2(D2)H
  DDRD  |= B01001100; //         ,PD6(D6)O,                 ,PD3(D3)O,PD2(D2)O
  DDRD  &= B01001111; // PD7(D7)I,        ,PD5(D5)I,PD4(D4)I,

                      // DataData,DataEnbl,Clock   ,Reset   ,NMI     ,IRQ
  PORTC |= B00111000; // PC5(A5)H,PC4(A4)H,PC3(A3)H,PC3(A2)H,PC3(A1)H,PC3(A0)H,
  DDRC  |= B00110000; // PC5(A5)O,PC4(A4)O
  DDRC  &= B11110111; //                  ,PC3(A3)I,PC3(A2)I,PC3(A1)I,PC3(A0)I,

                      // IRRegData,FlagLoad ,CtrlLoad ,CtrlData ,CtrlEnbl,Ctrl Reg
  PORTB |= B00101111; // PB5(D13)H,         ,PB3(D11)H,PB2(D10)H,PB1(D9)H,PB0(D8)H
  PORTB &= B11101111; //          ,PB4(D12)L
  DDRB  |= B00011111; //          ,PB4(D12)O,PB3(D11)O,PB2(D10)O,PB1(D9)O,PB0(D8)O
  DDRB  &= B11011111; // PB5(D13)I

  // Read the current state of IRQ line
  IRQ = PINC & B00000001;
  Serial.write(IRQ > 0 ? "I" : "i");
  Serial.flush();

  // Read the current state of NMI line
  NMI = PINC & B00000010;
  Serial.write(NMI > 0 ? "N" : "n");
  Serial.flush();

  // Read the current state of reset line
  reset = PINC & B00000100;
  Serial.write(reset > 0 ? "R" : "r");
  Serial.flush();

  // Read the current state of the clock
  clock = PINC & B00001000;
  Serial.write(clock > 0 ? "C" : "c");
  Serial.flush();

  // reserve 8 bytes for the inputString:
  inputString.reserve(8);
}
void loop() {
  // print the string when a newline arrives:
  if (stringComplete) {
    uint64_t ctrl;
    uint16_t address;
    switch (inputString.charAt(0)) {
      case 'a': // retrieve address
        address = LoadAddress();
        Serial.write('a');
        Serial.write(address & 0xFF);
        Serial.write(address >> 8);
        Serial.flush();
        break;
      case 'c': // retrieve clock
        Serial.write(clock > 0 ? "C" : "c");
        Serial.flush();

      case 'i': // retrieve clock
        Serial.write(IRQ > 0 ? "I" : "i");
        Serial.flush();

      case 'n': // retrieve clock
        Serial.write(NMI > 0 ? "N" : "n");
        Serial.flush();

      case 'r': // retrieve clock
        Serial.write(reset > 0 ? "R" : "r");
        Serial.flush();

      case 'L': // set control lines
        ctrl = 0;
        if (inputString.length() > 6) {
          for (int i = 1; i <= 6; i++) {
            ctrl |= inputString.charAt(i);
            ctrl <<= 8;
          }
          ctrl <<= 8;
          SetControlLines(ctrl);
        }
        break;

      case 'd': // retrieve data
        Serial.write('d');
        Serial.write(LoadData()); 
        Serial.flush();
        break;

      case 'D': // set data
        if (inputString.length() > 2) {
          SetData(inputString.charAt(1));
        } else {
          DataDisabled();
        }
        break;

      case 'o': // retrieve opCode
        Serial.write('o');
        Serial.write(LoadOpCode());
        Serial.flush();
        break;

      case 's': // retrieve flags & timing
        Serial.write('s');
        Serial.write(LoadFlag());
        Serial.flush();
        break;

    }
    inputString = "";
    stringComplete = false;
  }
  nextIRQ = PINC & B00000001;
  if (nextIRQ != IRQ) {
    IRQ = nextIRQ;
    Serial.write(IRQ > 0 ? "I" : "i");
    Serial.flush();
  }
  nextNMI = PINC & B00000010;
  if (nextNMI != NMI) {
    NMI = nextNMI;
    Serial.write(NMI > 0 ? "N" : "n");
    Serial.flush();
  }
  nextReset = PINC & B00000100;
  if (nextReset != reset) {
    reset = nextReset;
    Serial.write(reset > 0 ? "R" : "r");
    Serial.flush();
  }
  nextClock = PINC & B00001000;
  if (nextClock != clock) {
    clock = nextClock;
    Serial.write(clock > 0 ? "C" : "c");
    Serial.flush();
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