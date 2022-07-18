//Effects Wall Production code
// Directions for use:
//    1) Flash with board type: Arduino Leonardo
//    2) Connect back to the server (ethernet USB extender) - verify all connections are secure
//    3) make sure playground piece to test is the only one plugged in
//    4) open MIDI viewer, select "arduino leonardo", test the playground piece
//    5) Expected behavior: When you spin a nob the MIDI viewer will display the changing MIDI data
//                          When you push the "top" button you should see control 60 with value ccON  
//                          When you push the "second from top" button you should see control 60 with ccOFF value
//                          Next btn will mimic the merry go round on so: channel 5, control 90, value ccON
//                          Bottom btn will mimic the merry go round on so: channel 5, control 90, value ccOFF
   

// MIDI SETTINGS:
#define channel 1         // Can be anywhere from 0-15 reported to user as 1-16: These need to be unique
#define ccPot 50          // need to avoid 32 (used for lots of things internally in Ableton 
                          // control number (0-119). for safety dont overlap with any other playground piece
#define ccBtn 60          // need to avoid 32 (used for lots of things internally in Ableton 
                          // control number (0-119). for safety dont overlap with any other playground piece
#define ccON 100          // CC value above 64 is on signal to ableton
#define ccOFF 10          // CC value below 63 is off signal to ableton

#include <MIDIUSB.h>
const int MIDI_cc = 1; 
const int MIDI_note = 2; 
int MIDI_DataType;

// default button action is Momentary, to change a button to Toggle, change it's 1 to a 0 in it's position below:
bool m[4] = {0, 0, 0, 0}; // MOMENTARY MODE for each button
// Button:   1  2  3  4 

const int DEBUG = 0;

int btnNotes[4] = {49, 51, 54, 56}; //midi notes to send: { 49 = C#, 51 = D#, F#, G#, A# }
int btnTiming[4] ={0 ,0 ,0 ,0 };
byte buttonPin[4] = {2, 3, 5, 7};  //pinout for the buttons
char noteNames[5] = {'C','D','F','G','A'};

//CONTROL LOGIC SETTINGS: Timing Variables and data structure
unsigned long cycle = 0; //used to determine how many milliseconds since the last ping cycle
unsigned long cycleLED = 0; //used to keep track of how long the LED has been on and turn it off within a second
const int SEC = 1000; //define 1 second as 1000 ms

  #define ANALOGUE_PIN_ORDER A0, A1, A2, A3, A6, A7, A8, A9 // Pins: A0 A1 A2 A3 4 6 8 9  (see pro micro pinout)
  #define NUM_AI 8  // number of analog cc inputs
  #define MIDI_CC MIDI_CC_GENERAL1

  // A knob or slider movement must initially exceed this value to be recognised as an input. Note that it is
  // for a 7-bit (0-127) MIDI value.

  #define FILTER_AMOUNT 4

  // Timeout is in microseconds
  #define ANALOGUE_INPUT_CHANGE_TIMEOUT 200000

  // Array containing a mapping of analogue pins to channel index. This array size must match NUM_AI above.
  byte analogueInputMapping[8] = {ANALOGUE_PIN_ORDER};

  // Contains the current value of the analogue inputs.
  byte analogueInputs[8];

  // Variable to hold temporary analogue values, used for analogue filtering logic.
  byte tempAnalogueInput;

  byte i = 0;
  byte digitalOffset = 0;
  byte analogueDiff = 0;
  boolean analogueInputChanging[8];
  unsigned long analogueInputTimer[8];



bool buttonState[4] = {0, 0, 0, 0}; //0 = low = off
bool buttonLast[4] = {0, 0, 0, 0};  //0 = low = off

void controlChange(byte thechannel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | thechannel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
void noteOn(byte thechannel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | thechannel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void setup() {
  
  Serial.begin(115200); 
  
  cycle = millis();

  //set the midi data type
  MIDI_DataType = MIDI_cc; //options: MIDI_note or MIDI_cc
  
  
  for (byte b = 0; b < 5; b++) {
    pinMode(buttonPin[b], INPUT_PULLUP);
  }

    //onboard LED to notify locally of triggers
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  
  int refreshTime = millis() - cycle;
  if (DEBUG == -1 ) { // used to tell how long it takes to process the data
    Serial.print("_____________________________cycle refresh = ");
    Serial.print(refreshTime);
    Serial.println("ms");
    cycle = millis();
  }

  //turn off LED
  int ledTimeON = millis() - cycleLED;
  if (ledTimeON > 1*SEC) {
    digitalWrite(LED_BUILTIN, LOW); //turn off builtin light
  }
  
  checkButtons();
  checkAnalog();
}


void checkButtons() {
  
  for (int b = 0; b < 4; b++) {
    bool buttonNow = !digitalRead(buttonPin[b]);  // High = On
    
    if(buttonState[b] != buttonNow ){
      
      buttonState[b] = buttonNow;
      
      if(buttonState[b] && (millis() - btnTiming[b]) > 150){
      if(MIDI_DataType == MIDI_cc){
        int controlVal = 0;
          switch(b){
            case 0: 
              controlVal = ccBtn + b;
              controlChange(channel, controlVal, ccON);
              break;
            case 1: 
              controlVal = ccBtn + b ;
              controlChange(channel, controlVal, ccON);
              break;
            case 2: 
              controlVal = ccBtn + b;
              controlChange(4, 90, ccON);
              break;
            case 3: 
              controlVal = ccBtn + b - 1;
              controlChange(4, 90, ccOFF);
              break;
          }
          if(DEBUG>0){
            Serial.print("Button ");
            Serial.print(b+1);
            Serial.print(" hit :: Sending midi (Channel, Control, Value) = (");
            Serial.print(channel);
            Serial.print(" , ");
            Serial.print(controlVal);
            Serial.print(" , ");
            if(b % 2 == 0){
              Serial.print("ON"); 
            }else {
              Serial.print("OFF");
            }
            Serial.print(")  time::");
            Serial.println(millis());
          }
      }else if(MIDI_DataType == MIDI_note){
          noteOn(channel, btnNotes[b] , 127 );        
          if(DEBUG>0){
            Serial.print("Button ");
            Serial.print(b+1);
            Serial.print(" hit :: Sending midi (Channel, Note, Velocity) = (");
            Serial.print(channel);
            Serial.print(" , ");
            Serial.print(noteNames[b]);
            Serial.print("# , ");
            Serial.print(127); 
            Serial.print(")  time::");
            Serial.println(millis());
          }
      }
      
      digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
      cycleLED = millis(); //log the time the LED was turned on

      btnTiming[b] = millis();
      } 
    } else {
      
      
    }
  }
}



void checkAnalog() {

  for (i = 0; i < 8; i++) {
    // Read the analog input pin, dividing it by 8 so the 10-bit ADC value (0-1023) is converted to a 7-bit MIDI value (0-127).
    tempAnalogueInput = analogRead(analogueInputMapping[i]) / 8;

    // Take the absolute value of the difference between the curent and new values
    analogueDiff = abs(tempAnalogueInput - analogueInputs[i]);
    // Only continue if the threshold was exceeded, or the input was already changing
    if ((analogueDiff > 0 && analogueInputChanging[i] == true) || analogueDiff >= FILTER_AMOUNT)
    {
      // Only restart the timer if we're sure the input isn't 'between' a value
      // ie. It's moved more than FILTER_AMOUNT
      if (analogueInputChanging[i] == false || analogueDiff >= FILTER_AMOUNT)
      {
        // Reset the last time the input was moved
        analogueInputTimer[i] = micros();

        // The analogue input is moving
        analogueInputChanging[i] = true;
      }
      else if (micros() - analogueInputTimer[i] > ANALOGUE_INPUT_CHANGE_TIMEOUT)
      {
        analogueInputChanging[i] = false;
      }

      // Only send data if we know the analogue input is moving
      if (analogueInputChanging[i] == true)
      {
        // Record the new analogue value
        analogueInputs[i] = tempAnalogueInput;
        ///// send the midi message
        controlChange(channel, ccPot + i, tempAnalogueInput);

        if(DEBUG>0){
          Serial.print("Effect ");
          Serial.print(i);
          Serial.print(" on channel ");
          Serial.print(channel);
          Serial.print(" with control ");
          Serial.print(ccPot + i); 
          Serial.print(" and value = "); 
          Serial.println(tempAnalogueInput);
        }

      }
    }
  }
}
