//TouchPad Production code
// Directions for use:
//    1) Flash with board type: Arduino Leonardo
//    2) Connect back to the server (ethernet USB extender) - verify all connections are secure
//    3) make sure playground piece to test is the only one plugged in
//    4) open MIDI viewer, select "arduino leonardo", test the playground piece
//    5) Expected behavior: 
//                          When you push the buttons you should see sharp notes, or CC data
//                          The buttons here only send on values because they trigger samples in the music

#define channel 2   //  midi channel -1 (starts at 0 = channel 1)
#define ccBtn 70       // first midi CC number + Analog: CC 0-7 / Buttons: CC 8-11
#define ccON 100          // CC value above 64 is on signal to ableton
#define ccOFF 10          // CC value below 63 is off signal to ableton

// default button action is Momentary, to change a button to Toggle, change it's 1 to a 0 in it's position below:
bool m[5] = {1, 1, 1, 1, 1}; // MOMENTARY MODE for each button
// Button:   1  2  3  4  5
int btnNotes[5] = {49, 51, 54, 56, 58}; //midi notes to send: { 49 = C#, 51 = D#, F#, G#, A# } 
char noteNames[5] = {'C','D','F','G','A'};
int btnTiming[5] ={0 ,0 ,0 ,0 ,0 };


#include <MIDIUSB.h>

const int MIDI_cc = 1; 
const int MIDI_note = 2; 
int MIDI_DataType;

/////----DEBUG SETTINGS:
//--DEBUG = -1 :: Timing info
//--DEBUG = 0  :: Production state NEED TO BE 0 FOR FINAL INSTALL
//--DEBUG >= 1  :: btn logic, what touch pad is triggered and the cc/note being sent
const int DEBUG = 1;


//CONTROL LOGIC SETTINGS: Timing Variables and data structure
long cycle = 0; //used to determine how many milliseconds since the last ping cycle
int cycleLED = 0; //used to keep track of how long the LED has been on and turn it off within a second
const int SEC = 1000; //define 1 second as 1000 ms
int debounce = 10; //10ms to debounce the button after it is triggered.

#define buttonPinsDef 2, 3, 5, 7, 9 // Pins: 2 3 5 7 10
byte buttonPin[5] = {buttonPinsDef};


bool buttonState[5] = {1, 1, 1, 1, 1}; //1 = high = off
bool buttonLast[5] = {1, 1, 1, 1, 1};  //1 = high = off

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

void noteOff(byte thechannel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | thechannel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
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

void checkButtons() {
  
  for (int b = 0; b < 5; b++) {
    bool buttonNow = !digitalRead(buttonPin[b]);  // Low = On
    
    if(buttonState[b] != buttonNow ){
      
      buttonState[b] = buttonNow;
      
      if(buttonState[b] && (millis() - btnTiming[b]) > 150){
      if(MIDI_DataType == MIDI_cc){
          controlChange(channel, ccBtn + b, ccON);
          if(DEBUG>0){
            Serial.print("Touch Pad ");
            Serial.print(b+1);
            Serial.print(" hit :: Sending midi (Channel, Control, Value) = (");
            Serial.print(channel);
            Serial.print(" , ");
            Serial.print(ccBtn + b);
            Serial.print(" , ");
            Serial.print(ccON); 
            Serial.print(")  time::");
            Serial.println(millis());
          }

      }else if(MIDI_DataType == MIDI_note){
          noteOn(channel, btnNotes[b] , 127 );        
          if(DEBUG>0){
            Serial.print("Touch Pad ");
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
      delay(debounce);
      } 
    } else {
      
      
    }
  }
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
    digitalWrite(LED_BUILTIN, LOW); //turn off bulliten light
  }

  checkButtons();

}
