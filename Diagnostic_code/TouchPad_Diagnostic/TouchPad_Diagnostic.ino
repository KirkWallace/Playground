//TouchPad Diagnostic code
// Directions for use:
//    1) Flash with board type: Arduino Leonardo
//    2) Connect back to the server (ethernet USB extender) - verify all connections are secure
//    3) make sure playground piece to test is the only one plugged in
//    4) open MIDI viewer, select "arduino leonardo", test the playground piece
//    5) Expected behavior: 
//                          When you push the buttons you should see sharp notes, or CC data
//    

#define channel 2   //  midi channel -1 (starts at 0 = channel 1)
#define ccBtn 70       // first midi CC number + Analog: CC 0-7 / Buttons: CC 8-11

// default button action is Momentary, to change a button to Toggle, change it's 1 to a 0 in it's position below:
bool m[5] = {1, 1, 1, 1, 1}; // MOMENTARY MODE for each button
// Button:   1  2  3  4  5
int btnNotes[5] = {49, 51, 54, 56, 58}; //midi notes to send: { 49 = C#, 51 = D#, F#, G#, A# } 
int btnTiming[5] ={0 ,0 ,0 ,0 ,0 };
const int SEC = 1000; //define 1 second as 1000 ms

#include <MIDIUSB.h>


#define buttonPinsDef 2, 3, 5, 7, 9 // Pins: 2 3 5 7 10
byte buttonPin[5] = {buttonPinsDef};

/* Analog inputs section commented out

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

*/

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
  for (byte b = 0; b < 5; b++) {
    pinMode(buttonPin[b], INPUT_PULLUP);
  }

  //delay(30*SEC); //keep the system from sending triggers for 30 seconds during boot up
}

void checkButtons() {
  
  for (int b = 0; b < 5; b++) {
    bool buttonNow = !digitalRead(buttonPin[b]);  // Low = On
    
    if(buttonState[b] != buttonNow ){
      
      buttonState[b] = buttonNow;
      
      if(buttonState[b] && (millis() - btnTiming[b]) > 150){
      noteOn(channel, btnNotes[b] , 127 );
      Serial.println(b); 
      btnTiming[b] = millis();
      } 
    } else {
      
      
    }
  }
}









/* 
    if (buttonNow != buttonState[b]) {  // check for new press
      buttonState[b] = buttonNow;
      if (m[b]) {
        if (buttonNow){
        //controlChange(channel, cc2 + b, buttonNow * 127);
        //noteOn(channel, btnNotes[b] , 127 );
          delay(150); //debounce button
        }
      } else {
        if (buttonNow) {
          if (buttonLast[b]) {
            buttonLast[b] = 0;
          } else {
            buttonLast[b] = 1;
          }
          //controlChange(channel, cc2 + b, buttonLast[b] * 127);
          delay(150); //debounce button
        }
      }
    }
  }
  */



void loop() {

  checkButtons();

  //  checkAnalog();

}

/*
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
        controlChange(channel, cc1 + i, tempAnalogueInput);

      }
    }
  }
}
*/
