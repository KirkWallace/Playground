//Merry Go Round Diagnostic code
// Directions for use:
//    1) Flash with board type: Arduino Leonardo
//    2) Open serial monitor (top right button) with baud rate 115200 (set this at the bottom right after opening the serial monitor)
//    3) Expected behavior: When you spin the Merry Go round system will print out "Merry Go Round spinning" to the serial monitor.
//                         The system will also print which note/cc value it should be sending to Ableton
//    NOTE: switches need to be wired with the COM1 terminal to GND (black) and NO3 terminal to signal wire (yellow or blue)

// MIDI SETTINGS:
#define channel 4         // Can be anywhere from 0-15 reported to user as 1-16: These need to be unique
#define ccControl 90      // need to avoid 32 (used for lots of things internally in Ableton 
// control number (0-119). for safety dont overlap with any other playground piece
#define ccON 100          // CC value above 64 is on signal to ableton
#define ccOFF 10          // CC value below 63 is off signal to ableton

#include <MIDIUSB.h>
const int MIDI_cc = 1;
const int MIDI_note = 2;
int MIDI_DataType;
int note = 48; //48 is middle C

/////----DEBUG SETTINGS:
//--DEBUG = -2 :: Logic flow
//--DEBUG = -1 :: Timing info
//--DEBUG = 0  :: Production state NEED TO BE 0 FOR FINAL INSTALL
//--DEBUG == 1 :: Switch logic; how many switches are on in the merry go round
//--DEBUG == 2 :: RAW data for the switches.
//--DEBUG >= 3 :: Logic control for switching merry go round on and off. also what note is being sent 
const int DEBUG = 3;

// CONTROL LOGIC SETTINGS: Timing Variables and data structure
unsigned long cycle = 0; //used to determine how many milliseconds since the last cycle
unsigned long cycleLED = 0; //used to keep track of how long the LED has been on and turn it off within a second
const int NUMSENSORS = 8; // 2 sensors per spring
const int READSPERCYCLE = 10; //can be used for debouncing the switches if needed
int sensorLog[NUMSENSORS];  //raw data within each cycle
int merryGo_data[NUMSENSORS]; //an array containing true or false data: first value new trigger, second value valid data
int merryGo_data_lastCycle[NUMSENSORS]; //an array containing true or false data: first value new trigger, second value valid data
int merryGo_continuousPress[NUMSENSORS]; 
unsigned long merryGo_timing[NUMSENSORS]; //the actual time each sensor was last hit. 
int triggeredSensors[NUMSENSORS]; //used to bridge the cycles
int tempSum = 0; //sums all sensors into one read out
int lastTempSum = 0;
int tempSumThresh = 0; //used to "delete" continuous btn readings
const int SEC = 1000; //define 1 second as 1000 ms
const int minOFFdelay = 4 * SEC; //used to hold the merry go round on if it is used for less than 3 seconds.
const int keepGoingDelay = 2 * SEC; //used for when we want to keep it going
const int btnHoldTresh = 50; //the threshold that we consider the btn to be continually pressed, and thus turn it off. 
//TIMING EX: if the merry go round is only spun for 1 sec, then hold the music on for minOFFdelay
//           if the merry go round is spun for >minOFFdelay, then hold music on until after a sensor hasn't been hit within keepGoingDelay time
const int cycleRefreshDelay = 10; // 200ms delays to reads
unsigned long lastON; //time of the last ON signal sent. used to make sure the trigger is on for longer than minOFFdelay
unsigned long lastSWITCH; //time of the last switch hit. used to hold the melody on for the keepGoingDelay duration
int lastSensorPin; //keeps track of the last sensor that was hit.
bool playingMelody = false; //keeping track of what the Arduino thinks is going on with the music

int sensorPins[NUMSENSORS] = //pin numbers for each sensor
{ 2, 3,   //Spring 1
  4, 5,   //Spring 2
  18, 19, //Spring 3
  20, 21  //Spring 4
};  


// MIDI methods:
void controlChange(byte thechannel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | thechannel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

void noteOn(byte thechannel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | thechannel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte thechannel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | thechannel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

//   ARDUINO SETUP: initialize all variables to their default state and setup serial monitor.
//              -setup pinout for the board.
//              -switches need to be wired with the COM1 terminal to GND (black) and NO3 terminal to signal wire (yellow or blue)
void setup() {

  Serial.begin(9600); //115200 for production code

  cycle = millis();

  //set the midi data type
  MIDI_DataType = MIDI_cc; //options: MIDI_note or MIDI_cc

  tempSum = 0;
  lastON = 0; //lastON < 0 is off.
  lastSWITCH = 0; //lastSWITCH < 0 is off.

  //initialize data structure and pinModes
    for (int k = 0; k < NUMSENSORS; k++) {
      sensorLog[k] = 0;
      triggeredSensors[k] = 0;
      merryGo_data[k]  = 0; 
      merryGo_data_lastCycle[k] = 0;
      merryGo_continuousPress[k] = 0;
      pinMode(sensorPins[k], INPUT_PULLUP);
    }

  //onboard LED to notify locally of triggers
  pinMode(LED_BUILTIN, OUTPUT);

}

//CONTROL LOGIC:
void loop() {

  //// used to tell how long it takes to process the data
  unsigned long refreshTime = millis() - cycle;
  cycle = millis();
  
  if (DEBUG < -1 ) { 
    delay(500);
    Serial.print("____________cycle refresh = ");
    Serial.print(refreshTime);
    Serial.println("ms");
  }

  /*/turn off LED
  unsigned long ledTimeON = millis() - cycleLED;
  if (ledTimeON > refreshTime && ledTimeON < 1000) {
    digitalWrite(LED_BUILTIN, LOW); //turn off bulliten light
  }
*/
/*
  //check the sensors then record all the appropriate timing 
  for (int i = 0 ; i < NUMSENSORS; i++) { //now Cycle through and collect the values from each sensor
    sensorLog[i] = !digitalRead(sensorPins[i]); //read both sensors at the same time
    merryGo_data[i] = sensorLog[i]; //now data is a 1 if the switch is pushed

    //preprocess timing for each btn. Make sure the btn isnt continually pressed and then record the time it was pressed
    if(merryGo_data[i] && !merryGo_data_lastCycle[i]) {  //case where the btn is pressed and we need to update and keep a current time stamp
      
    if(merryGo_timing[i] <= 0 || millis() - merryGo_timing[i] < keepGoingDelay) { //btn isn't continually pressed and has been hit
        merryGo_timing[i] = millis();
        triggeredSensors[i] = 1;
        tempSum ++; //only count btns that aren't continually pressed
      } else { //btn hasnt been pressed within the refresh time, clear the timing info
         merryGo_timing[i] = -1;
        triggeredSensors[i] = 0;
      }
    } else { //case where btn isn't pressed, and the melody is already playing.
    }


  }

*/
  if (DEBUG == -2) {
    Serial.println("___reading Sensors___");
  }

  for (int i = 0 ; i < NUMSENSORS; i++) { //now Cycle through and collect the values from each sensor
    merryGo_data[i] = !digitalRead(sensorPins[i]); //read both sensors at the same time

    //preprocess timing for each btn. Make sure the btn isnt continually pressed and then record the time it was pressed
    if(merryGo_data[i] && !merryGo_data_lastCycle[i]) { //new btn press
      lastSWITCH = millis();
      merryGo_timing[i] = millis();
      if(DEBUG >=3){
        Serial.print("Btn pressed :: ");  
        Serial.println(lastSWITCH);
      }
    }else if(merryGo_data[i] && merryGo_data_lastCycle[i]) {  //btn pressed last cycle and this cycle too
      merryGo_continuousPress[i] = 1; 
    }
  }



    if (DEBUG == -2) {
    Serial.println("___consolidate Data___");
  }
  //take the btn timing and consolidate to get the last switch trigger  
     for(int k = 0; k<NUMSENSORS; k++){
      //lastSWITCH = max(lastSWITCH , merryGo_timing[k]);
      tempSum += merryGo_data[k];
      tempSumThresh += merryGo_continuousPress[k];
     }
  

 


  if (DEBUG == 1) {
    Serial.print("Merry-Go Round # of sensors hit :: ");
    Serial.print(tempSum);
    Serial.print("  Thresh for continuous :: ");
    Serial.println(tempSumThresh); 
    //delay(cycleRefresh); //simply slows down the reads if necessary
  } 
  if (DEBUG == 2) {
    Serial.print("Sensors 1=OFF 0=ON : (");
    for (int x = 0; x < NUMSENSORS; x++) {
    Serial.print(merryGo_data[x]);
    if(x<NUMSENSORS-1) Serial.print(" , "); 
    else Serial.println(" )"); 
    }
    delay(cycleRefreshDelay); //simply slows down the reads if necessary
  }


  if (DEBUG == -2) {
    Serial.println("___MIDI logic___");
  }
    if(!playingMelody){  //handle ON signals
        if(tempSum > tempSumThresh && lastTempSum == tempSumThresh){ //must have a new trigger 
            playingMelody = true;
            lastON = millis();
            if (DEBUG >= 3) {
                Serial.println("Merry Go Round turning on");
            }
        }
    } else { //handle OFF signals
          if(tempSum == tempSumThresh && millis() - lastON > minOFFdelay && millis() - lastSWITCH > keepGoingDelay){ //send off signal because we are on longer than minOffdelay
            playingMelody = false;
            if (DEBUG >= 3) {
                Serial.println("Merry Go Round turning off ---- timed out from lack of movement within minOffdelay");
            }
          } 
          /*else if(tempSum == tempSumThresh && millis() - lastSWITCH > keepGoingDelay){ //send off signal because we have spun longer than minOffdelay and run through our keepOnDelay
            playingMelody = false;
            if (DEBUG >= 3) {
                Serial.println("Merry Go Round turning off ---- timed out from lack of movement within keepGoingDelay");
            }            
          }
          */
    }

/*
  //determine when and what midi signals to send
    if (( tempSum > 0) && !playingMelody) { //new Event ON sensor "i"; send midi signal to turn on
      //triggeredSwings[i] = 1; //log the sensor as on
      digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
      cycleLED = millis(); //log the time the LED was turned on
      lastON = millis(); //log the time we are turning on the swing
      playingMelody = true; 
      
      if (DEBUG == 0) { /// PRODUCTION CODE: midi signals being sent to server
        if (MIDI_DataType == MIDI_cc) {
          controlChange(channel, ccControl, ccON);
        }
        else if (MIDI_DataType == MIDI_note) {
          noteOn(channel, note, 127);
          //noteOff(channel, note, 127); //only needed if ableton needs an off signal
        }
      }
      else if (DEBUG >= 3) { /// Raw sensors data being read, and midi signals being sent in Serial Monitor
        if (MIDI_DataType == MIDI_cc) {
          Serial.print("Merry Go Round Triggered :: Turning ON (ccValue = 100) :: control# ");
          Serial.print(ccControl);
          Serial.print(" :: on channel# ");
          Serial.println(channel);
        }
        else if (MIDI_DataType == MIDI_note) {
          Serial.print("Merry Go Round Triggered :: Turning ON :: playing ");
          switch (note) {
            case 48: Serial.print("C"); break;
            case 53: Serial.print("F"); break;
            case 55: Serial.print("G"); break;
            case 57: Serial.print("A"); break;
            case 59: Serial.print("B"); break;
          }
          Serial.print(" value ");
          Serial.print(note);
          Serial.print(" :: on channel# ");
          Serial.println(channel);
          }

        }
    } else if ((tempSum == 0) && playingMelody && (abs(millis() - lastON) > minOFFdelay) && (abs(millis() - lastSWITCH) > keepGoingDelay) ) { //new off signal

      
      //triggeredSwings[i] = 0; //log the sensor as off
      //logic control reset
      lastON = -1;
      lastSWITCH = -1; 
      digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
      cycleLED = millis(); //log the time the LED was turned on
      playingMelody = false; 
      
      if (DEBUG == 0) { /// PRODUCTION CODE: midi signals being sent to server
        if (MIDI_DataType == MIDI_cc) {
          controlChange(channel, ccControl, ccOFF);
        }
        else if (MIDI_DataType == MIDI_note) {
          noteOn(channel, note, 127);
          //noteOff(channel, note, 127); //only needed if ableton needs an off signal
        }
      }
      else if (DEBUG >= 3) { /// Raw sensors data being read, and midi signals being sent in Serial Monitor
        if (MIDI_DataType == MIDI_cc) {
          Serial.print("Merry Go Round Triggered :: Turning OFF (ccValue = 10) :: control# ");
          Serial.print(ccControl);
          Serial.print(" :: on channel# ");
          Serial.println(channel);
        }
        else if (MIDI_DataType == MIDI_note) {
          Serial.print("Merry Go Round Triggered :: Turning OFF :: playing ");
          switch (note) {
            case 48: Serial.print("C"); break;
            case 53: Serial.print("F"); break;
            case 55: Serial.print("G"); break;
            case 57: Serial.print("A"); break;
            case 59: Serial.print("B"); break;
          }
          Serial.print(" value ");
          Serial.print(note);
          Serial.print(" :: on channel# ");
          Serial.println(channel);
          }

      }
    }
*/

    ///----CYCLE RESETS
      lastTempSum = tempSum;
      tempSum = 0;
      tempSumThresh = 0;  
    for (int k = 0; k < NUMSENSORS; k++) {
      sensorLog[k] = 0;
      triggeredSensors[k] = 0;
      merryGo_data_lastCycle[k] = merryGo_data[k]; //record the last cycle for the next cycle. 
      merryGo_continuousPress[k] = 0; 
    }
    delay(cycleRefreshDelay);
  }
