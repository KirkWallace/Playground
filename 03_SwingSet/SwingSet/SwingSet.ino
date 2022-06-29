// Proximity Sensor Midi Controller, 4ch /////
// SETTINGS:
#define channel 5         //  midi channel -1 (starts at 0 = channel 1)
#define cc1 60           // first midi CC number + Analog: CC 0-7 / Buttons: CC 8-11
#define min_dist 10      //*** cm distance to Trigger within (122=4' 190~6')
#define max_dist 190   // maximum distance (in cm) to check
#define echo 15     // delay time in ms between sensor reads (29 ms to avoid ping overlap)
//const byte thechannel = 5;

#include <NewPing.h>
#include <MIDIUSB.h>


const int DEBUG = 2;

const int MIDI_cc = 1; 
const int MIDI_note = 2; 
int MIDI_DataType;

// default action is Momentary. to change a sensor to Toggle, change it's 1 to a 0 in it's position below:
bool m[4] = {1, 1, 1, 1}; // MOMENTARY MODE for each sensor
// Sensor:   1  2  3  4


//Timing Variables setup
long cycle = 0; //used to determine how many milliseconds since the last ping cycle
int cycleLED = 0; //used to keep track of how long the LED has been on and turn it off within a second
const int NUMSENSORS = 4; 
const int READSPERCYCLE = 10; //roughly the number of times you can read each sensor within 500ms given an echo of 33ms 
int sensorLog[NUMSENSORS][READSPERCYCLE];
int triggeredSensors[NUMSENSORS];
int tempSum[NUMSENSORS];
const int hitTriggerON = 6;
const int hitTriggerOFF = 0;  
const int SEC = 1000; //define 1 second as 1000 ms

NewPing sonar[4] = {   // Sensor object array.
  NewPing(10, 16, max_dist), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(14, 15, max_dist),
  NewPing(18, 19, max_dist),
  NewPing(20, 21, max_dist)
};

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
  //while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  //}
  cycle = millis();

  //initialize tempSum to avoid floating piont values
  for(int i=0; i<NUMSENSORS; i++){
    tempSum[i] = 0;
  }

  //set the midi data type
  MIDI_DataType = MIDI_note; //options: MIDI_note or MIDI_cc
  //onboard LED to notify locally of triggers
  pinMode(LED_BUILTIN, OUTPUT);

  //delay(5*SEC); //keep the system from sending triggers for 30 seconds during boot up
}


void loop() {
      int refreshTime = millis() - cycle; 
      if(DEBUG > 0){ // used to tell how long it takes to process the data
      Serial.print("_____________________________cycle refresh = ");
      Serial.print(refreshTime);
      Serial.println("ms");
      cycle = millis(); 
      }

      //turn off LED
      int ledTimeON = cycleLED - millis();
      if(ledTimeON > refreshTime && ledTimeON < 1000){
        digitalWrite(LED_BUILTIN, LOW); //turn off bulliten light
      }

      //check the sensors readsPerCycle times ex: 500ms/33ms = 15.15 or 15
      for(int x=0; x<READSPERCYCLE; x++){
        for(int i = 0 ; i<NUMSENSORS; i++){ //now Cycle through and collect the values from each sensor
        sensorLog[i][x] = sonar[i].ping_cm();
        if(sensorLog[i][x] >= min_dist) tempSum[i]++;
       }
        delay(echo); 
      }

      if(DEBUG > 3){ ///distances being read by the Sensors for one cycle in centimeters 
          for(int i = 0 ; i<NUMSENSORS; i++){
            Serial.print("sensor["); Serial.print(i); Serial.print("] = { ");  
            for(int x=0; x<READSPERCYCLE; x++){
              Serial.print(sensorLog[i][x]); 
              if(x<READSPERCYCLE - 1) Serial.print(" , ");
            }
            Serial.println(" }"); 
          }
      }



      for(int i = 0 ; i<NUMSENSORS; i++){
        if(DEBUG > 2){ ///number of triggered distances met by each sensor in the last cycle
          Serial.print("tempSum["); Serial.print(i); Serial.print("] = "); Serial.println(tempSum[i]);
          }
        if((tempSum[i]>=hitTriggerON) && !triggeredSensors[i]){ //new Event on sensor "i"; send midi signal to turn on
          triggeredSensors[i] = 1; //log the sensor as on
          digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
          cycleLED = millis(); //log the time the LED was turned on
              if(DEBUG > 1){
                  Serial.print(" turning ON sensor # ");
                  Serial.println(i);
                  }
          ///Send midi data note on
          if(MIDI_DataType == MIDI_note){ // 53 is F, 55 is G, 57 is A, 59 is B 
            noteOn(channel, 53 + 2 * i , 127);
            
            if(DEBUG > 1){
                Serial.print(" Playing Note :: ");
                int note = 53 + 2 * i;
                switch(note){
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
          } else if (MIDI_DataType == MIDI_cc){
            controlChange(5, cc1 + i , 127);
              if(DEBUG > 1){
                Serial.print(" MIDI_data :: CC ");
                Serial.print(cc1 + i);
                Serial.println(" :: value 127 ");
              }
          }
          
        } else if ((tempSum[i]<= hitTriggerOFF) && triggeredSensors[i]){ //new off signal
          triggeredSensors[i] = 0; //log the sensor as off
          digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
          cycleLED = millis(); //log the time the LED was turned on
              if(DEBUG > 1){
                Serial.print(" turning OFF sensor # ");
                Serial.println(i);
              }
          ///Send midi data note off
          if(MIDI_DataType == MIDI_note){ // 53 is F, 55 is G, 57 is A, 59 is B 
            noteOn(channel, 53 + 2 * i , 127); 
              if(DEBUG > 1){
                Serial.print(" Ending Note :: ");
                int note = 53 + 2 * i;
                switch(note){
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
          } else if (MIDI_DataType == MIDI_cc){
            controlChange(5, cc1 + i , 0);
              if(DEBUG > 1){
                Serial.print(" MIDI_data :: CC ");
                Serial.print(cc1 + i);
                Serial.println(" :: value 0 ");
              }
          }
        }
        tempSum[i] = 0; //clear for the next cycle
      }

}
