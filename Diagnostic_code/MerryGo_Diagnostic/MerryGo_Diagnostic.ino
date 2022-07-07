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

/////----DEBUG SETTINGS:
//--DEBUG = -1 :: Timing info
//--DEBUG = 0  :: Production state NEED TO BE 0 FOR FINAL INSTALL
//--DEBUG >= 1 :: Switch logic, what switch is triggered and the note/cc being sent
//--DEBUG >= 2 :: RAW data for the switches.
const int DEBUG = 1;

// CONTROL LOGIC SETTINGS: Timing Variables and data structure
long cycle = 0; //used to determine how many milliseconds since the last cycle
int cycleLED = 0; //used to keep track of how long the LED has been on and turn it off within a second
const int NUMSENSORS = 8; // 2 sensors per spring
const int READSPERCYCLE = 10; //can be used for debouncing the switches if needed
int sensorLog[NUMSENSORS];  //raw data within each cycle
int merryGo_data[NUMSENSORS]; //an array containing true or false data: first value new trigger, second value valid data
//int triggeredSensors[NUMSENSORS]; //used to bridge the cycles
int tempSum = 0; //sums all sensors into one read out
const int SEC = 1000; //define 1 second as 1000 ms
const int minOFFdelay = 4 * SEC; //used to hold the merry go round on if it is used for less than 3 seconds.
const int keepGoingDelay = 1 * SEC; //used for when we want to keep it going
//TIMING EX: if the merry go round is only spun for 1 sec, then hold the music on for minOFFdelay
//           if the merry go round is spun for >minOFFdelay, then hold music on until after a sensor hasn't been hit within keepGoingDelay time
const int cycleRefresh = 200; // 200ms delays to reads
int lastON; //time of the last ON signal sent. used to make sure the trigger is on for longer than minOFFdelay
int lastSWITCH; //time of the last switch hit. used to hold the melody on for the keepGoingDelay duration
int lastSensorTime; //keeps track of the last time a sensor was hit.
bool playingMelody = false; //keeping track of what the Arduino thinks is going on with the music

int sensorPins[NUMSENSORS] = //pin numbers for each sensor
{ 2, 3,  //Spring 1
  4, 5, //Spring 2
  18, 19, //Spring 3
  20, 21
};  //Spring 4


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

  Serial.begin(115200);

  cycle = millis();

  //set the midi data type
  MIDI_DataType = MIDI_cc; //options: MIDI_note or MIDI_cc

  tempSum = 0;
  lastON = -1; //lastON < 0 is off.
  lastSWITCH = -1; //lastSWITCH < 0 is off.

  //initialize data structure and pinModes
    for (int k = 0; k < NUMSENSORS; k++) {
      sensorLog[k] = 0;
      pinMode(sensorPins[k], INPUT_PULLUP);
    }

  //onboard LED to notify locally of triggers
  pinMode(LED_BUILTIN, OUTPUT);

}

//CONTROL LOGIC:
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
  if (ledTimeON > refreshTime && ledTimeON < 1000) {
    digitalWrite(LED_BUILTIN, LOW); //turn off bulliten light
  }

  //check the sensors
  for (int i = 0 ; i < NUMSENSORS; i++) { //now Cycle through and collect the values from each sensor
    sensorLog[i] = digitalRead(sensorPins[i]); //read both sensors at the same time
    merryGo_data[i] = sensorLog[i];
    tempSum += sensorLog[i]; //switch is triggered with a LOW = 0 reading.
    //instead of flipping the to count from 0, sum all and look for tempSum < 8 as an indicator of when a switch has been hit.
    //delay(7); // delay 10 ms to let the other swith depress on each swing

  }
  if (NUMSENSORS - tempSum > 0 ) {
    lastSWITCH = millis();
  }

  if (DEBUG >= 1) {
    Serial.print("Merry-Go Round # of sensors hit :: ");
    Serial.println(NUMSENSORS - tempSum); 
    //delay(cycleRefresh); //simply slows down the reads if necessary
  } else 
  if (DEBUG >= 2) {
    Serial.print("Sensors 1=OFF 0=ON : (");
    for (int x = 0; x < NUMSENSORS; x++) {
    Serial.print(sensorLog[x]);
    if(x<NUMSENSORS-1) Serial.print(" , "); 
    else Serial.println(" )"); 
    }
    delay(cycleRefresh); //simply slows down the reads if necessary
  }


  //determine when and what midi signals to send
  for (int i = 0 ; i < NUMSENSORS; i++) {

    if ((NUMSENSORS - tempSum > 0) && !playingMelody && lastON < 0) { //new Event ON sensor "i"; send midi signal to turn on
      //triggeredSwings[i] = 1; //log the sensor as on
      digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
      cycleLED = millis(); //log the time the LED was turned on
      lastON = millis(); //log the time we are turning on the swing
      playingMelody = true; 
      
      if (DEBUG == 0) { /// PRODUCTION CODE: midi signals being sent to server
        if (MIDI_DataType == MIDI_cc) {
          controlChange(channel, ccControl + i, ccON);
        }
        else if (MIDI_DataType == MIDI_note) {
          //Note Swing 0 (53 = F), Swing 1 (55 = G), Swing 2 (57 = A), Swing 3 (59 = B)
          int note = 53 + 2 * i; //log which note should be sent
          noteOn(channel, note, 127);
          //noteOff(channel, note, 127); //only needed if ableton needs an off signal
        }
      }
      else if (DEBUG >= 3) { /// Raw sensors data being read, and midi signals being sent in Serial Monitor
        if (MIDI_DataType == MIDI_cc) {
          Serial.print("Swing "); Serial.print(i + 1); Serial.print(" :: Sensors(");
          Serial.print(sensorLog[i]); Serial.print(" , ");
          Serial.print(sensorLog[i]);
          Serial.print(" ) :: Turning ON (value = 100) :: control# ");
          Serial.println(ccControl + i);
        }
        else if (MIDI_DataType == MIDI_note) {
          Serial.print("Merry-Go Round turning ON :: Sensors(");
          for (int i = 0; i < NUMSENSORS; i++) {
            Serial.print(" ");
          }

          int note = 53 + 2 * i; //log which note should be sent
          switch (note) {
            case 53:
              Serial.println("F");
              break;
            case 55:
              Serial.println("G");
              break;
            case 57:
              Serial.println("A");
              break;
            case 59:
              Serial.println("B");
              break;
          }
        }
      }
    } else if ((NUMSENSORS - tempSum == 0) && playingMelody && ((millis() - lastON) > minOFFdelay) && ((millis() - lastSWITCH) > keepGoingDelay) ) { //new off signal

      //change all the control values in the data structure
      //triggeredSwings[i] = 0; //log the sensor as off
      lastON = -1;
      digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
      cycleLED = millis(); //log the time the LED was turned on
      playingMelody = false; 
      
      if (DEBUG == 0) { /// PRODUCTION CODE: midi signals being sent to server
        if (MIDI_DataType == MIDI_cc) {
          controlChange(channel, ccControl + i, ccOFF);
        }
        else if (MIDI_DataType == MIDI_note) {
          //Note Swing 0 (53 = F), Swing 1 (55 = G), Swing 2 (57 = A), Swing 3 (59 = B)
          int note = 53 + 2 * i; //log which note should be sent
          noteOn(channel, note, 127);
          //noteOff(channel, note, 127); //only needed if ableton needs an off signal
        }
      }
      else if (DEBUG >= 3) { /// Raw sensors data being read, and midi signals being sent in Serial Monitor
        if (MIDI_DataType == MIDI_cc) {
          Serial.print("Swing "); Serial.print(i + 1); Serial.print(" :: Sensors(");
          Serial.print(sensorLog[i]); Serial.print(" , ");
          Serial.print(sensorLog[i]);
          Serial.print(" ) :: Turning OFF (value = 10) :: control# ");
          Serial.println(ccControl + i);
        }
        else if (MIDI_DataType == MIDI_note) {
          Serial.print("Swing "); Serial.print(i + 1); Serial.print(" :: Sensors(");
          Serial.print(sensorLog[i]); Serial.print(" , ");
          Serial.print(sensorLog[i]);
          Serial.print(" ) :: Turning OFF :: with note ");

          int note = 53 + 2 * i; //log which note should be sent
          switch (note) {
            case 53:
              Serial.println("F");
              break;
            case 55:
              Serial.println("G");
              break;
            case 57:
              Serial.println("A");
              break;
            case 59:
              Serial.println("B");
              break;
          }
        }
      }
    }

    tempSum = 0; //clear for the next cycle

  }

}
