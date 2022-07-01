//SwingSet diagnostic code
//flash the arduino with this code and open the serial monitor in the top right of the arduino IDE
//The system will print out which swing has been triggered to the serial monitor.
//The system will also print which note it should be sending to Ableton

// SETTINGS:
#define channel 5         //  midi channel -1 (starts at 0 = channel 1)

#include <MIDIUSB.h>

/////----DEBUG SETTINGS:
//--DEBUG = -1 :: Timing info
//--DEBUG = 0  :: Production state NEED TO BE 0 FOR FINAL INSTALL
//--DEBUG >= 1  :: Swing logic, what swing is triggered and the note being sent
//--DEBUG = 2  :: Swing sensor data. Prints out both sensor values in 'real' time (every 1 second)
const int DEBUG = 1;

//Timing Variables setup
long cycle = 0; //used to determine how many milliseconds since the last ping cycle
int cycleLED = 0; //used to keep track of how long the LED has been on and turn it off within a second
const int NUMSWINGS = 4;  // 4 swings per swing set
const int NUMSENSORS = 2; // 2 sensors per swing
const int READSPERCYCLE = 10; //can be used for debouncing the switches.
int sensorLog[NUMSWINGS][NUMSENSORS];  //raw data within each cycle
int triggeredSwings[NUMSWINGS]; //used to bridge the cycles
int tempSum[NUMSWINGS]; //sums two sensors into one read out
const int SEC = 1000; //define 1 second as 1000 ms
const int minOFFdelay = 1 * SEC; //used to hold the merry go round on if it is used for less than 3 seconds.
const int cycleRefresh = 200; // 200ms delays to reads
int lastON[NUMSWINGS]; //time of the last ON signal sent. used to make sure the trigger is on for longer than minOFFdelay

int swingSetPins[NUMSWINGS][NUMSENSORS] = //pin numbers for each swing. two sensors per swing
{ {10,16},//{10, 16}, //Swing 1
  {14, 15}, //Swing 2
  {18, 19}, //Swing 3
  {20, 21}  //Swing 4
};


void noteOn(byte thechannel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | thechannel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void setup() {

  Serial.begin(115200);

  cycle = millis();

  //initialize data structure and pinModes
  for (int i = 0; i < NUMSWINGS; i++) {
    tempSum[i] = 0;
    lastON[i] = -1; //lastON < 0 is off.

    for (int k = 0; k < NUMSENSORS; k++) {
      sensorLog[i][k] = 0;
      pinMode(swingSetPins[i][k], INPUT_PULLUP);
    }
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
  if (ledTimeON > refreshTime && ledTimeON < 1000) {
    digitalWrite(LED_BUILTIN, LOW); //turn off bulliten light
  }

  //check the sensors
  for (int i = 0 ; i < NUMSWINGS; i++) { //now Cycle through and collect the values from each sensor
    for (int k = 0; k < NUMSENSORS; k++) {
      sensorLog[i][k] = digitalRead(swingSetPins[i][k]);
      if (sensorLog[i][k] == LOW) tempSum[i]++;
      delay(5); // delay 10 ms to let the other swith depress on each swing
    }
  }

  //delay(cycleRefresh); //simply slows down the reads


  for (int i = 0 ; i < NUMSWINGS; i++) {

    if ((tempSum[i] > 0) && !triggeredSwings[i] && lastON[i] < 0) { //new Event on sensor "i"; send midi signal to turn on
      triggeredSwings[i] = 1; //log the swing as on
      digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
      cycleLED = millis(); //log the time the LED was turned on
      lastON[i] = millis(); //log the time we are turning on the swing



      if (DEBUG >= 1) { /// Raw sensors data being read

        Serial.print("Swing "); Serial.print(i+1); Serial.print(" :: (");
        Serial.print(sensorLog[i][0]); Serial.print(" , ");
        Serial.print(sensorLog[i][1]);
        Serial.print(" ) :: Turning ON :: with note ");
        
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
      
      /*
        ///Send midi data note on
        if (DEBUG == 0) { // 53 is F, 55 is G, 57 is A, 59 is B
        noteOn(channel, 53 + 2 * i , 127);

        if (DEBUG > 1) {
          Serial.print(" Playing Note :: ");
          int note = 53 + 2 * i;
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
        } else if (MIDI_DataType == MIDI_cc) {
        controlChange(5, cc1 + i , 127);
        if (DEBUG > 1) {
          Serial.print(" MIDI_data :: CC ");
          Serial.print(cc1 + i);
          Serial.println(" :: value 127 ");
        }
        }
      */
    } else if ((tempSum[i] == 0) && triggeredSwings[i] && ((millis() - lastON[i]) > minOFFdelay)) { //new off signal
      triggeredSwings[i] = 0; //log the sensor as off
      lastON[i] = -1;
      digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
      cycleLED = millis(); //log the time the LED was turned on



      if (DEBUG >= 1) { /// Raw sensors data being read

        Serial.print("Swing # "); Serial.print(i); Serial.print(" (");
        Serial.print(sensorLog[i][0]); Serial.print(" , ");
        Serial.print(sensorLog[i][1]);
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
      /*
        ///Send midi data note off
        if (MIDI_DataType == MIDI_note) { // 53 is F, 55 is G, 57 is A, 59 is B
        if (DEBUG == 0 ) noteOn(channel, 53 + 2 * i , 127);
        if (DEBUG > 1) {
          Serial.print(" Ending Note :: ");
          int note = 53 + 2 * i;
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
        } else if (MIDI_DataType == MIDI_cc) {
        if (DEBUG > 1) {
          Serial.print(" MIDI_data :: CC ");
          Serial.print(cc1 + i);
          Serial.println(" :: value 0 ");
        }
        }
      */
    } 
    /*
    else if (tempSum[i] == 1 ) { //ERROR: one of the switches is not reading notify the
      if (DEBUG > 0) {
        Serial.println();
        Serial.println();
        Serial.print("FAULTY SWITCH on swing # ");
        Serial.println(i);
        Serial.println();
        Serial.println();
      }
    }
     */
    


      tempSum[i] = 0; //clear for the next cycle


    if (DEBUG == 2) { /// Raw sensors data being read
      for (int i = 0 ; i < NUMSWINGS; i++) {
          Serial.print("Swing # "); Serial.print(i); Serial.print(" (");
          for (int k = 0; k < NUMSENSORS; k++) {
            Serial.print(sensorLog[i][k]);
            if (k < NUMSENSORS - 1) Serial.print(" , ");
          }
          Serial.println(" ) ::");
        }
      }

  }

}
