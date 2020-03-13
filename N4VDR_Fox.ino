/*
 * 
 * 2019-06-19
 * 
 * Arduino Nano 168
 *    
 * Does 3 things
 *   1)  Sends morse code ID
 *   2)  Timer loop so tx is not constant
 *   3)  Listens via DTMF for commands
 *   
 * Arduino controlled Fox Hunting Xmtr and / or Beacon
 * 
 * Base concept from:  http://www.ne4ga.org/projects/arduinocontrolledfox
 * 
 * This code will loop through a string of characters and convert these to morse code.  
 * It will blink an LED light and play audio on a speaker.  
 *
 * This code is adjusted to comply with parts:
 *     47 CFR Part 97 - AMATEUR RADIO SERVICE
 *         Subpart A - General Provisions
 *             § 97.3 Definitions
 *             § 97.5 Station license required
 *             § 97.7 Control operator required
 *         Subpart B - Station Operation Standards
 *             § 97.101 General standards
 *             § 97.105 Control operator duties
 *             § 97.109 Station control
 *             § 97.111 Authorized transmissions
 *             § 97.119 Station identification
 *         Subpart C - Special Operations
 *             § 97.201 Auxiliary station
 *             § 97.203 Beacon station
 *
 * Notes:
 *   1)  Per 97.201, below freqs are *NOT* allowed
 *       144.0-144.5 MHz
 *       145.8-146.0 MHz
 *       219-220 MHz
 *       222.00-222.15 MHz
 *       431-433 MHz
 *       435-438 MHz
 *   2)  Per 97.203, below freqs are allowed
 *       28.20-28.30 MHz
 *       50.06-50.08 MHz
 *       144.275-144.300 MHz
 *       222.05-222.06 MHz
 *       432.300-432.400 MHz
 *       33 cm and shorter wavelength bands
 */

//============  Includes / Defines / Declarations  ============
#include <DTMF.h>

//#define DEBUG_ON

//============  Constants  ============
/*
 *Activate Pins Used 
 */
// --------- Digital Pins ---------
//int [pin] = 0;              // DNU - RX PIN
//int [pin] = 1;              // DNU - TX PIN
int advLed = 2;               // Dig I/O
//int [pin] = 3;              // Dig I/O         PWM
//int [pin] = 4;              // Dig I/O
//int [pin] = 5;              // Dig I/O         PWM
//int [pin] = 6;              // Dig I/O         PWM
//int [pin] = 7;              // Dig I/O
//int [pin] = 8;              // Dig I/O
//int [pin] = 9;              // Dig I/O         PWM
//int [pin] = 10;             // Dig I/O  SS     PWM
//int [pin] = 11;             // Dig I/O  MISO   PWM
int PTT = 12;                 // Dig I/O  MOSI
int audio = 13;               // Dig I/O  SCK

// --------- Analog Pins ---------
int sensorPin = A0;           // Analog In 0
//int [Apin] = A1;            // Analog In 1
//int [Apin] = A2;            // Analog In 2
//int [Apin] = A3;            // Analog In 3
//int [Apin] = A4;            // Analog In 4   SDA  (I2C Protocol)
//int [Apin] = A5;            // Analog In 5   SCL  (I2C Protocol)
//int [Apin] = A6;            // Analog In 6
//int [Apin] = A7;            // Analog In 7

/*
 * ICSP PIN LAYOUT
 * 
 * Rst            Rst
 * Gnd   2 4 6*   RX0
 * Vin   1 3 5    TX1
 *    
 * 1 = MOSI (aka pin 12)
 * 2 = 5V
 * 3 = SCK  (aka pin 13)
 * 4 = MISO (aka pin 11)
 * 5 = RST
 * 6 = GND
 *  
 */

char beaconString[] = "CQ N4VDR/FOX";                         //   Type the String to Convert to Morse Code Here
char cmdValue1[] = "1 qsl";                                   //   Type the String to Convert to Morse Code Here
char cmdValue2[] = "2 qsl";                                   //   Type the String to Convert to Morse Code Here
char cmdValue3[] = "3 qsl";                                   //   Type the String to Convert to Morse Code Here
char cmdValue4[] = "4 qsl";                                   //   Type the String to Convert to Morse Code Here
char cmdValue5[] = "5 qsl";                                   //   Type the String to Convert to Morse Code Here
char cmdValue6[] = "6 qsl";                                   //   Type the String to Convert to Morse Code Here
char cmdValue7[] = "7 qsl";                                   //   Type the String to Convert to Morse Code Here
char cmdValue8[] = "8 qsl";                                   //   Type the String to Convert to Morse Code Here
char cmdValue9[] = "9 qsl";                                   //   Type the String to Convert to Morse Code Here

const int freq = 700;                                         // Frequency of audio tone.  Industry "standard" is 700 hz
/*
  Set the speed of your morse code
  Adjust the 'ditLen' length to speed up or slow down your morse code
    Per http://www.kent-engineers.com/codespeed.htm
      for 5 WPM, set ditLen to approx 240
      for 10 WPM, set ditLen to approx 120
      for 13 WPM, set ditLen to approx 92
      for 15 WPM, set ditLen to approx 80
      for 20 WPM, set ditLen to approx 60
      for 25 WPM, set ditLen to approx 48
      for 30 WPM, set ditLen to approx 40
  Equation:
      ditLen = (60 seconds / (WPM * 50))*1000
      ditLen in milliseconds
  Initial speed is 20WPM, but is adjustable via DTMF pad by 5
*/

const float n = 128.0;                                        // used in DTMF setup, Number of Samples, not to exceed 160
const float sampling_rate = 8926.0;                           // used in DTMF setup, Sampling Rate
float d_mags[8];

const int ditLenArray[6] = {240,120,80,60,48,40};             // {5,10,15,20,25,30} length of the morse code 'dit'
 
/* NOTE that N MUST NOT exceed 160
 *  
 * This is the number of samples which are taken in a call to .sample. The smaller the value of N the wider the bandwidth.
 * For example, with N=128 at a sample rate of 8926Hz the tone detection bandwidth will be 8926/128 = 70Hz. If you make N
 * smaller, the bandwidth increases which makes it harder to detect the tones because some of the energy of one tone can 
 * cross into an adjacent (in frequency) tone. But a larger value of N also means that it takes longer to collect the samples.
 * A value of 64 works just fine, as does 128.  
 * 
 * NOTE that the value of N does NOT have to be a power of 2.
 */
 
//============  Library Declarations  ============
DTMF dtmf = DTMF(n, sampling_rate);

//============  VARIABLES (will change)  ============
bool txEn = false;                                            // TX Enable 1 = on, 0 = off
bool rxEn = true;                                             // RX Enable 1 = on, 0 = off
bool novice = true;                                           // false means "Advanced" group, short tx, true means "new" group, longer tx
bool storage = true;                                          // used for storing values
int pause = 60000;                                            // value in milliseconds
int rMin = 60000;                                             // random minimum for time delay
int rMax = 240000;                                            // random maximum for time delay
int wpmCW = 3;                                                // Words Per Minute.  Location in ditLenArray
long prevMilli = millis();
int cmd;                                                      // DTMF command input
int prevCmd;

//============  Setup Function  ============
void setup() {
  #ifdef DEBUG_ON
    Serial.begin(9600);
    Serial.println(F("Serial Monitor started"));
  #endif
  pinMode(PTT, OUTPUT);
  pinMode(advLed, OUTPUT);
  digitalWrite(PTT, HIGH);
  randomSeed(analogRead(A0));
  txEn = true;
  digitalWrite(advLed, !novice);                              // if in Advanced mode, turn on LED
  #ifdef DEBUG_ON
    Serial.println(F("Setup Function:"));
    Serial.print(F("Transmit Mode set to: "));
    Serial.println(txEn);
    Serial.print(F("Receive Mode set to: "));
    Serial.println(rxEn);
    Serial.print(F("Novice/Advanced Mode set to: "));
    Serial.println(novice);
    Serial.println(F("Setup Function Complete"));
  #endif
}

//============  Loop Function  ============
void loop(){
  if (rxEn == true){
    dtmfListen();
  }

  if ((millis() - prevMilli >= pause)&&(txEn == true)){
    if (novice == true){
      beacon();
      space();
      beacon();
    } else {
      beacon();
    }
    pause = random(rMin,rMax);
    #ifdef DEBUG_ON
      Serial.print(F("Delay time set to: "));
      Serial.println(pause/1000);
    #endif
    prevMilli = millis();
  }
}

//============  DTMF Listen Function  ============
void dtmfListen() {                                           // Listen for commands via DTMF
  dtmf.sample(sensorPin);
  dtmf.detect(d_mags, 506);
  cmd = dtmf.button(d_mags, 1800.) - 48;
  #ifdef DEBUG_ON
    if ((prevCmd != cmd)&&(cmd > 0)){
      Serial.print(F("Command received: "));
      Serial.println(cmd);
    }
  #endif

  if (cmd > 0){
    prevCmd = cmd;
    switch (cmd){
      case 1:
        dtmfOne();
        break;
      case 2:
        dtmfTwo();
        break;
      case 3:
        dtmfThree();
        break;
      case 4:
        dtmfFour();
        break;
      case 5:
        dtmfFive();
        break;
      case 6:
        dtmfSix();
        break;
      case 7:
        dtmfSeven();
        break;
      /*
      case 8:
        dtmfEight();
        break;
      case 9:
        dtmfNine();
        break;
      */
      default:
        dtmfError();
        break;
    }
  }
  delay(1);
}

//============  Beacon Function  ============
void beacon() {                                               // Beacon Sub Routine
  #ifdef DEBUG_ON
    Serial.println(F("Beacon Sub-routine called"));
  #endif
  rxEn = false;
  digitalWrite(PTT, LOW);
  delay(1500);
  for (int c = 0; c < 2; c++) {                               // Loop through the entire beacon string 2 times with a 1.5 second delay between loops
    for (int i = 0; i < sizeof(beaconString) - 1; i++)        // Loop through the string and get each character one at a time until the end is reached
    {
      char tmpChar = beaconString[i];                         // Get the character in the current position
      tmpChar = toLowerCase(tmpChar);                         // Set the case to lower case
      getChar(tmpChar);                                       // Call the subroutine to get the morse code equivalent for this character
    }
    delay(1500);
  }
  delay(500);
  digitalWrite(PTT, HIGH);
  rxEn = true;
}

//============  DTMF 1 Function  ============
void dtmfOne() {                                              // Enable timed TX
  #ifdef DEBUG_ON
    Serial.println(F("DTMF 1 Sub-routine called - TX Enabled"));
  #endif
  storage = txEn;
  txEn = false;
  rxEn = false;
  delay(1500);
  digitalWrite(PTT, LOW);
  delay(1500);
  for (int i = 0; i < sizeof(cmdValue1) - 1; i++)             // Command Acknowledge loop
  {
    char tmpChar = cmdValue1[i];
    tmpChar = toLowerCase(tmpChar);
    getChar(tmpChar);
  }
  delay(500);
  digitalWrite(PTT, HIGH);
  txEn = true;                                                // set the flag to enable transmissions
  rxEn = true;
  cmd = 0;
  #ifdef DEBUG_ON
    Serial.print(F("TX Enabled set to: "));
    Serial.println(txEn);
  #endif
}

//============  DTMF 2 Function  ============
void dtmfTwo() {                                              // Increase WPM by 5
  #ifdef DEBUG_ON
    Serial.println(F("DTMF 2 Sub-routine called - WPM +5"));
  #endif
  storage = txEn;
  txEn = false;
  rxEn = false;
  delay(1500);
  digitalWrite(PTT, LOW);
  delay(1500);
  for (int i = 0; i < sizeof(cmdValue2) - 1; i++)             // Command Acknowledge loop
  {
    char tmpChar = cmdValue2[i];
    tmpChar = toLowerCase(tmpChar);
    getChar(tmpChar);
  }
  delay(500);
  digitalWrite(PTT, HIGH);
  if (wpmCW < 5) {
    wpmCW++;
  } else {
    wpmCW = 5;
  }
  rxEn = true;
  txEn = storage;
  cmd = 0;
  #ifdef DEBUG_ON
    Serial.print(F("WPM set to: "));
    Serial.println((wpmCW + 1)*5);
    Serial.print(F("ditLen set to: "));
    Serial.println(ditLenArray[wpmCW]);
  #endif
}

//============  DTMF 3 Function  ============
void dtmfThree() {                                            // Set mode to Novice (New hunters)
  #ifdef DEBUG_ON
    Serial.println(F("DTMF 3 Sub-routine called - Novice Mode"));
  #endif
  storage = txEn;
  txEn = false;
  rxEn = false;
  delay(1500);
  digitalWrite(PTT, LOW);
  delay(1500);
  for (int i = 0; i < sizeof(cmdValue3) - 1; i++)             // Command Acknowledge loop
  {
    char tmpChar = cmdValue3[i];
    tmpChar = toLowerCase(tmpChar);
    getChar(tmpChar);
  }
  delay(500);
  digitalWrite(PTT, HIGH);
  novice = true;
  digitalWrite(advLed, LOW);
  rMin = 60000;                                               // 60 seconds (1 minute)
  rMax = 240000;                                              // 240 seconds (4 minutes)
  pause = random(rMin,rMax);
  rxEn = true;
  txEn = storage;
  cmd = 0;
  #ifdef DEBUG_ON
    Serial.print(F("Novice set to: "));
    Serial.println(novice);
    Serial.print(F("Delay set to: "));
    Serial.println(pause/1000);
  #endif
}

//============  DTMF 4 Function  ============
void dtmfFour() {                                             // Disable timed TX
  #ifdef DEBUG_ON
    Serial.println(F("DTMF 4 Sub-routine called - TX Disabled"));
  #endif
  storage = txEn;
  txEn = false;
  rxEn = false;
  delay(1500);
  digitalWrite(PTT, LOW);
  delay(1500);
  for (int i = 0; i < sizeof(cmdValue4) - 1; i++)             // Command Acknowledge loop
  {
    char tmpChar = cmdValue4[i];
    tmpChar = toLowerCase(tmpChar);
    getChar(tmpChar);
  }
  delay(500);
  digitalWrite(PTT, HIGH);
  txEn = false;
  rxEn = true;
  cmd = 0;
  #ifdef DEBUG_ON
    Serial.print(F("TX Enabled set to: "));
    Serial.println(txEn);
  #endif
}

//============  DTMF 5 Function  ============
void dtmfFive() {                                             // Decrease WPM by 5
  #ifdef DEBUG_ON
    Serial.println(F("DTMF 5 Sub-routine called - WPM -5"));
  #endif
  storage = txEn;
  txEn = false;
  rxEn = false;
  delay(1500);
  digitalWrite(PTT, LOW);
  delay(1500);
  for (int i = 0; i < sizeof(cmdValue5) - 1; i++)             // Command Acknowledge loop
  {
    char tmpChar = cmdValue5[i];
    tmpChar = toLowerCase(tmpChar);
    getChar(tmpChar);
  }
  delay(500);
  digitalWrite(PTT, HIGH);
  if (wpmCW > 0) {
    wpmCW--;
  } else {
    wpmCW = 0;
  }
  rxEn = true;
  txEn = storage;
  cmd = 0;
  #ifdef DEBUG_ON
    Serial.print(F("WPM set to: "));
    Serial.println((wpmCW + 1)*5);
    Serial.print(F("ditLen set to: "));
    Serial.println(ditLenArray[wpmCW]);
  #endif
}

//============  DTMF 6 Function  ============
 void dtmfSix() {                                             // Set mode to Advanced Hunters
  #ifdef DEBUG_ON
    Serial.println(F("DTMF 6 Sub-routine called - Advanced Mode"));
  #endif
  storage = txEn;
  txEn = false;
  rxEn = false;
  delay(1500);
  digitalWrite(PTT, LOW);
  delay(1500);
  for (int i = 0; i < sizeof(cmdValue6) - 1; i++)             // Command Acknowledge loop
  {
    char tmpChar = cmdValue6[i];
    tmpChar = toLowerCase(tmpChar);
    getChar(tmpChar);
  }
  delay(500);
  digitalWrite(PTT, HIGH);
  novice = false;
  digitalWrite(advLed, HIGH);
  rMin = 300000;                                              // 300 seconds (5 minutes)
  rMax = 600000;                                              // 600 seconds (10 minutes)
  pause = random(rMin,rMax);
  rxEn = true;
  txEn = storage;
  cmd = 0;
  #ifdef DEBUG_ON
    Serial.print(F("Novice set to: "));
    Serial.println(novice);
    Serial.print(F("Delay set to: "));
    Serial.println(pause/1000);
  #endif
}

//============  DTMF 7 Function  ============
void dtmfSeven() {                                            // 7.5 second tone burst
  #ifdef DEBUG_ON
    Serial.println(F("DTMF 7 Sub-routine called - 7.5 Second Tone Burst"));
  #endif
  storage = txEn;
  txEn = false;
  rxEn = false;
  delay(1500);
  digitalWrite(PTT, LOW);
  delay(1500);
  for (int i = 0; i < sizeof(cmdValue7) - 1; i++)             // Command Acknowledge loop
  {
    char tmpChar = cmdValue7[i];
    tmpChar = toLowerCase(tmpChar);
    getChar(tmpChar);
  }
  delay(500);
  tone(audio, freq);                                          // tone burst
  delay(7500);
  noTone(audio);
  delay(500);
  digitalWrite(PTT, HIGH);
  txEn = storage;
  rxEn = true;
  cmd = 0;
}

//============  DTMF ERROR Function  ============
void dtmfError() {                                            // Did not understand command
  #ifdef DEBUG_ON
    Serial.println(F("DTMF ERROR Sub-routine called - Command Not Recognized"));
  #endif
  storage = txEn;
  txEn = false;
  rxEn = false;
  delay(1500);
  digitalWrite(PTT, LOW);
  delay(1500);
  for (int i = 0; i < 3; i++)                                 // 3 error beeps
  {
    tone(audio, 350);
    delay(500);
    noTone(audio);
    delay(500);
  }
  delay(500);
  digitalWrite(PTT, HIGH);
  txEn = storage;                                             // place unit back in same mode
  rxEn = true;
  cmd = 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%  EVERYTHING BELOW HERE IS TO CREATE THE MORSE CODE  %%%%%%%%%%%%%%%%%%%%%%%%%

//============  CW Dit Function  ============
void dit()
{
  tone(audio, freq);                                          // start playing a tone
  delay(ditLenArray[wpmCW]);                                  // hold in this position
  noTone(audio);                                              // stop playing a tone
  delay(ditLenArray[wpmCW]);                                  // hold in this position
}

//============  CW Dah Function  ============
void dah()
{
  tone(audio, freq);                                          // start playing a tone
  delay(ditLenArray[wpmCW]*3);                                // hold in this position
  noTone(audio);                                              // stop playing a tone
  delay(ditLenArray[wpmCW]);                                  // hold in this position
}

//============  CW End of Character Function  ============
void endChar()
{
  delay(ditLenArray[wpmCW]*3);                                // hold in this position
}

//============  CW End of Character Function  ============
void space()
{
  delay(ditLenArray[wpmCW]*6);                                // hold in this position
}

//============  CW End of Word Function  ============
void endWord()
{
  delay(ditLenArray[wpmCW]*7);                                // hold in this position
}

//============  Convert Char to CW Function  ============
void getChar(char tmpChar)
{
  // Take the passed character and use a switch case to find the morse code for that character
  // A thorugh Z
  switch (tmpChar) {
    case 'a': 
      dit();
      dah();
      endChar();
    break;
    case 'b':
      dah();
      dit();
      dit();
      dit();
      endChar();
    break;
    case 'c':
      dah();
      dit();
      dah();
      dit();
      endChar();
    break;
    case 'd':
      dah();
      dah();
      dit();
      endChar();
    break;
    case 'e':
      dit();
      endChar();
    break;
    case 'f':
      dit();
      dit();
      dah();
      dit();
      endChar();
    break;
    case 'g':
      dah();
      dah();
      dit();
      endChar();
    break;
    case 'h':
      dit();
      dit();
      dit();
      dit();
      endChar();
    break;
    case 'i':
      dit();
      dit();
      endChar();
    break;
    case 'j':
      dit();
      dah();
      dah();
      dah();
      endChar();
    break;
    case 'k':
      dah();
      dit();
      dah();
      endChar();
    break;
    case 'l':
      dit();
      dah();
      dit();
      dit();
      endChar();
    break;
    case 'm':
      dah();
      dah();
      endChar();
    break;
    case 'n':
      dah();
      dit();
      endChar();
    break;
    case 'o':
      dah();
      dah();
      dah();
      endChar();
    break;
    case 'p':
      dit();
      dah();
      dah();
      dit();
      endChar();
    break;
    case 'q':
      dah();
      dah();
      dit();
      dah();
      endChar();
    break;
    case 'r':
      dit();
      dah();
      dit();
      endChar();
    break;
    case 's':
      dit();
      dit();
      dit();
      endChar();
    break;
    case 't':
      dah();
      endChar();
    break;
    case 'u':
      dit();
      dit();
      dah();
      endChar();
    break;
    case 'v':
      dit();
      dit();
      dit();
      dah();
      endChar();
    break;
    case 'w':
      dit();
      dah();
      dah();
      endChar();
    break;
    case 'x':
      dah();
      dit();
      dit();
      dah();
      endChar();
    break;
    case 'y':
      dah();
      dit();
      dah();
      dah();
      endChar();
    break;
    case 'z':
      dah();
      dah();
      dit();
      dit();
      endChar();
    break;
  // 0 (Zero) Through 9
    case '0':
      dah();
      dah();
      dah();
      dah();
      dah();
      endChar();
    break;
    case '1':
      dit();
      dah();
      dah();
      dah();
      dah();
      endChar();
    break;
    case '2':
      dit();
      dit();
      dah();
      dah();
      dah();
      endChar();
    break;
    case '3':
      dit();
      dit();
      dit();
      dah();
      dah();
      endChar();
    break;
    case '4':
      dit();
      dit();
      dit();
      dit();
      dah();
      endChar();
    break;
    case '5':
      dit();
      dit();
      dit();
      dit();
      dit();
      endChar();
    break;
    case '6':
      dah();
      dit();
      dit();
      dit();
      dit();
      endChar();
    break;
    case '7':
      dah();
      dah();
      dit();
      dit();
      dit();
      endChar();
    break;
    case '8':
      dah();
      dah();
      dah();
      dit();
      dit();
      endChar();
    break;
    case '9':
      dah();
      dah();
      dah();
      dah();
      dit();
      endChar();
    break;
    case '/':     // Slash
      dah();
      dit();
      dit();
      dah();
      dit();
      endChar();
    break;
    case '.':     // Period
      dit();
      dah();
      dit();
      dah();
      dit();
      dah();
      endWord();
    break;
    case '-':     // Dash
      dah();
      dit();
      dit();
      dit();
      dit();
      dah();
      endChar();
    break;
    case ',':     // Comma
      dah();
      dah();
      dit();
      dit();
      dah();
      dah();
      endChar();
    break;
    case '?':     // Question Mark
      dit();
      dit();
      dah();
      dah();
      dit();
      dit();
      endChar();
    break;
    case '!':     // Exclamation Mark
      dah();
      dit();
      dah();
      dit();
      dah();
      dah();
      endChar();
    break;
    case '"':     // Quotes
      dit();
      dah();
      dit();
      dit();
      dah();
      dit();
      endChar();
    break;
    case ':':     // Colon
      dah();
      dah();
      dah();
      dit();
      dit();
      dit();
      endChar();
    break;
    case '@':     // At
      dit();
      dah();
      dah();
      dit();
      dah();
      dit();
      endChar();
    break;
    default: 
    // If a matching character was not found it will default to a blank space
    space();
  }
}
