// Radio Frequency Merry Go Round TRANSMITTER, 4ch //
// Board: Pro Micro pinout
// Flash board as: Arduino LEONARDO
// This board reads proximity sensors and sends that data via Radio Frequency (aka: wifi protocol)
// to the interpreter board. the interpreter board will take this data and convert it to midi signals for ableton


//// ---DEBUG settings:
//-3: only cyclesOn debug statements
//-2: only Transmission success and number on the merry go round
//-1: logic flow - only when things are really broken
//0: prints nothing during the loop
//1: prints the cycle refresh timing and radio transmisions
//2: prints the sensors that are triggered only when they are triggered.
//3: prints each sensors triggered status every cycle
//4: prints the actual distances being read by the sensors every cycle

const int DEBUG = 0;


// SETTINGS:
#define min_dist 10     //minimum distance for the sensor to trigger
//(30cm = 1' :: 61cm = 2' :: 122cm = 4' :: 190cm ~ 6')
//NB: change max_dist back to 220cm before distributing
#define max_dist 90   // maximum distance (in cm) to check
#define echo 29     // delay time in ms between sensor reads (29 min to avoid ping overlap)


#include <NewPing.h>
#include <SPI.h> //required for wifi
#include <nRF24L01.h> //required for wifi
#include <RF24.h> //required for wifi

//Timing and Logic Variables setup
const int NUMSENSORS = 4;
const int READSPERCYCLE = 10; //roughly the number of times you can read each sensor within 500ms given an echo of 33ms
const int hitTriggerON = 6;
const int hitTriggerOFF = 0;
const int SEC = 1000; //define 1 second as 1000 ms
long cycle = 0; //used to determine how many milliseconds since the last ping cycle
int cycleLED = 0; //used to keep track of how long the LED has been on and turn it off within a second
int sensorLog[NUMSENSORS][READSPERCYCLE];  //Stores distances of initial sensor readings
int cyclesOn[NUMSENSORS]; //Keep sensor on for at least 4 cycles
int cycleThresh = 4; 
int triggeredSensors[NUMSENSORS];          //keeps track of which sensors are on beyond each cycle
int tempSum[NUMSENSORS];                   //stores number of sensor readings that met our trigger settings
int numOnMerryGo = 0;   //as it says. mainly used in determining the final midi data send since we only send midi data when the number of people is 0 or greater than 0.
bool playingMelody = false; //keeping track of what the Arduino thinks is going on with the music
bool txSuccess = true; //needed to be true for the first time through the logic to work


//wifi modules setup
#define pin_CE 9  //definition things
#define pin_CSN 8  //definition things
#define numSpots 4 //spots on the merry go round
int merryGo_data[numSpots] = {0, 0, 0, 0}; //an array containing true or false data: first value new trigger, second value valid data
RF24 radio(pin_CE, pin_CSN); //initialize the radio object
const byte address[6] = "00001";  //address through which two modules communicate.

//sensor setup
NewPing sonar[4] = {   // Sensor object array
  NewPing(2, 3, max_dist), // Each sensor's (trigger pin, echo pin, and max distance to ping)
  NewPing(4, 5, max_dist),
  NewPing(18, 19, max_dist),
  NewPing(20, 21, max_dist)
};

void printSettings() { //only use this in debug mode
  Serial.println("--Booting up Merry Go Round TRANSMITTER--");
  Serial.println("--Settings are as follows:");
  Serial.print("    --Min Distance: ");
  Serial.print(min_dist);
  Serial.print(" cm    --Max Distance: ");
  Serial.print(max_dist);
  Serial.println(" cm");
  Serial.println("--Settings are as follows:");  
  Serial.print("Channel: ");
  Serial.println(radio.getChannel());
  Serial.print("Data Rate: ");
  Serial.print(radio.getDataRate());
  Serial.println("   ::  0 = min, 1 = low, 2 = high, 3 = max");
}

////-------------BEGIN THE ACTUAL SETUP PROCESS----------------////
void setup() {

  Serial.begin(115200); //115200 baud required for midi and RF24 signals
  cycle = millis();

  for (int i = 0; i < numSpots; i++) { //clear the data storage
    merryGo_data[i] = 0;
        cyclesOn[i]= -1; //turn the cycle count off for every sensor 
 
  }

  //-----setup the radio
  radio.begin();
  radio.setChannel(120); //127 wifi channels available. channels lower than 102 may conflict with commercial 2.4gHz wifi routers
  radio.setDataRate(RF24_250KBPS);
  /// Set power level: RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_HIGH=-6dBM, and RF24_PA_MAX=0dBm.
  //radio.setPALevel(RF24_PA_MIN);
  //radio.setPALevel(RF24_PA_LOW);
  //radio.setPALevel(RF24_PA_HIGH);
  radio.setPALevel(RF24_PA_MAX);

  radio.openWritingPipe(address);//set the address

  radio.stopListening();//Set module as transmitter

  pinMode(LED_BUILTIN, OUTPUT); //onboard LED to notify locally of triggers
  //NB: attempting to notify for triggers, not really working at the moment

  if (DEBUG > 0) printSettings();
}


////-------------THE MAIN PROCESS THAT RUNS CONTINUOUSLY----------------////
void loop() {

  if(DEBUG>0){ //restart serial Data upon Serial open if in debug mode 
    //NB: NOT TESTED LOGIC
    Serial.begin(115200);
    while(!Serial){}
  }

  //------Cycle Resets
  int refreshTime = millis() - cycle;
  if (DEBUG > 0) { // used to tell how long it takes to process the data
    Serial.print("_____________________________cycle refresh = ");
    Serial.print(refreshTime);
    Serial.println("ms");
    cycle = millis();
  }

  //turn off LED
  int ledTimeON = cycleLED - millis();
  if ( ledTimeON > 1000) {//ledTimeON > refreshTime && ledTimeON < 1000
    digitalWrite(LED_BUILTIN, LOW); //turn off bulliten light
  }


  //------Read Sensors
  //--only read sensors if the transmission is successful, otherwise suspend sensor reading until the message is sent
  if (txSuccess) { //if the last transmission was successful read the sensors
    //otherwise move on and try to resend the data we already collected.

    //read the sensors readsPerCycle times (ex: 500ms/33ms = 15.15 or 15)
    //and then store the values
    for (int x = 0; x < READSPERCYCLE; x++) {
      for (int i = 0 ; i < NUMSENSORS; i++) { //now Cycle through and collect the values from each sensor
        sensorLog[i][x] = sonar[i].ping_cm(); //store the distances read
        if (sensorLog[i][x] >= min_dist) tempSum[i]++; //keep track of which distances are triggers
      }
      delay(echo);
    }
    if (DEBUG == -1) Serial.println("made it past sensor readings");

    if (DEBUG > 3) { ///DEBUG SECTION: distances being read by the Sensors for one cycle in centimeters
      for (int i = 0 ; i < NUMSENSORS; i++) {
        Serial.print("sensor["); Serial.print(i); Serial.print("] = { ");
        for (int x = 0; x < READSPERCYCLE; x++) {
          Serial.print(sensorLog[i][x]);
          if (x < READSPERCYCLE - 1) Serial.print(" , ");
        }
        Serial.println(" }");
      }
    }

    //------Once sensors read figure out how many are triggered
    ///Now average the number of triggers met by each sensor to determine if the sensor should be 'triggered'
    ///this is needed because the sensors are a little noisy and don't always read that someone is there
    ///so we make ten readings and if the sensor reports a distance within the trigger more than hitTriggersON
    ///we consider the sensor triggered. when that number falls below hitTriggerOFF we consider the sensor not 'triggered'
    for (int i = 0 ; i < NUMSENSORS; i++) {
      if (DEBUG > 2) { ///number of triggered distances met by each sensor in the last cycle
        Serial.print("tempSum["); Serial.print(i); Serial.print("] = "); Serial.println(tempSum[i]);
      }
      if ((tempSum[i] >= hitTriggerON) && !triggeredSensors[i] && cyclesOn[i]<0) { //new Event on sensor "i"; send midi signal to turn on
        triggeredSensors[i] = 1; //log the sensor as on
        cyclesOn[i]=0;
        digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
        cycleLED = millis(); //log the time the LED was turned on
        if (DEBUG > 1) {
          Serial.print(" someone got on ");
          Serial.println(i);
        }
        numOnMerryGo += 1;

      } else if ((tempSum[i] <= hitTriggerOFF) && triggeredSensors[i]&& cyclesOn[i]>cycleThresh) { //new off signal
        triggeredSensors[i] = 0; //log the sensor as off
        digitalWrite(LED_BUILTIN, HIGH); //tell the board we have a triggered sensor
        cycleLED = millis(); //log the time the LED was turned on
        if (DEBUG > 1) {
          Serial.print(" someone got off ");
          Serial.println(i);
        }
        numOnMerryGo += -1;

        //reset the cyclesOn
        cyclesOn[i]= -1; //negative denotes an off sensor with no cycles triggered.
      } else if(cyclesOn[i]>=0){
          cyclesOn[i]++;
        }

      
      tempSum[i] = 0; //clear for the next cycle
    }
          if(DEBUG == -3 ){ ///distances being read by the Sensors for one cycle in centimeters 
          for(int i = 0 ; i<NUMSENSORS; i++){
            Serial.print("Cycles On for sensor["); Serial.print(i); Serial.print("] = { ");  
            Serial.print(cyclesOn[i]);
            Serial.println(" }"); 
          }
      }

    
    if (DEBUG == -1) Serial.println("made it passed summing");
  } else {
    if (DEBUG > 0) Serial.println("Tx failed...resending");
    txSuccess = radio.write(&merryGo_data, sizeof(merryGo_data));
  }

  //------Take the Data of triggered data and figure out if we need to send an RF signal for midi data
  //now with that data we need to figure out if the merry go round needs to send a signal.
  //we only send a signal when we go from 'no sensors triggered' -> 'some sensors triggered' orrrr
  //'some sensors triggered' -> 'no sensors triggered' ::: aka we don't care when we go from 1 to 3
  //people on the merry go round or 4 to 1. only when everyone gets off or someone steps on
  
   if (numOnMerryGo > 0 && !playingMelody && txSuccess) { 
    //START playing Melody
    //the case when we go from numOnMerryGo=0 to numOnMerryGo>0 and music needs to play and the last transmision was successful
    if (DEBUG > 0) {
      Serial.print("Trigger sent: # of people on Merry Go = ");
      Serial.println(numOnMerryGo);
    }
    playingMelody = true;
    ///store data to Send via rf to turn the track on
    merryGo_data[0] = 1;
    txSuccess = radio.write(&merryGo_data, sizeof(merryGo_data));

    if (txSuccess) {
      //radio.flush_tx(); //clear the transmission buffer
      merryGo_data[0] = 0; //tells the controller no changes need to be made to the playing state
      //radio.write(&merryGo_data, sizeof(merryGo_data)); //send the null state, but dont record the transmission state
    }

  }
  else if (numOnMerryGo == 0 && playingMelody && txSuccess) {
    //STOP playing Melody
    //the case when we go from numOnMerryGo>0 to numOnMerryGo=0 and music needs to stop playing and the last transmision was successful
    if (DEBUG > 0) {
      Serial.print("Trigger sent: # of people on Merry Go = ");
      Serial.println(numOnMerryGo);
    }
    playingMelody = false;
    ///store data to Send via rf to turn the track on
    ///(yes it is the same as the 'turn on' signal: this is simply a toggle in ableton and requires the same signal)
    merryGo_data[0] = 1;
    txSuccess = radio.write(&merryGo_data, sizeof(merryGo_data));

    if (txSuccess) {
      //radio.flush_tx(); //clear the transmission buffer
      merryGo_data[0] = 0; //tells the controller no changes need to be made to the playing state
      //radio.write(&merryGo_data, sizeof(merryGo_data)); //send the null state, but dont record the transmission state
    }
  }
  else if ( numOnMerryGo < 0 || numOnMerryGo > 4) {  //ERROR: numOnMerryGo falls outside of 4 sensors...shouldn't happen, but this handles the issue gracefully
    // RESET: the variable keeping track of the triggers because of a logic error
    if (DEBUG > 0) Serial.println("------ERROR TOO MANY PEOPLE ON MERRY GO ROUND----------- Resetting.....");
    numOnMerryGo = 0; //resets the variable in case it wanders outside the acceptable range
  }
  else if ( !txSuccess) { //resend the previous data 
    //RESEND the data because we lost contact with the receiver
    if (DEBUG == -2) Serial.println("---Resending TX---");
    txSuccess = radio.write(&merryGo_data, sizeof(merryGo_data));
  }
  else {
    //merryGo_data[0] = 0; //tells the controller no changes need to be made to the playing state
  }


  if (DEBUG == -2) { //transmission info and the number of "people" triggered sensors
    Serial.print("Transmission Success = ");
    Serial.print(txSuccess);
    Serial.print("   ::   Number of triggered sensors = ");
    Serial.println(numOnMerryGo);
  }


  //SUBMIT THE DATA to the controller - moved radio call into logic to keep track of tx send success

  //radio.flush_tx(); //not sure we really need to do this, but its possible to clear the tx cache

  if (DEBUG == -1) Serial.println("made it passed radio transmit");
}
