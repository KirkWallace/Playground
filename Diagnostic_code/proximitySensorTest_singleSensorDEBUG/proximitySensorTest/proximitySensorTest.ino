// SETTINGS:
#define min_dist 10     //minimum distance for the sensor to trigger
//(30cm = 1' :: 61cm = 2' :: 122cm = 4' :: 190cm ~ 6')
//NB: change max_dist back to 220cm before distributing
#define max_dist 220   // maximum distance (in cm) to check
#define echo 25     // delay time in ms between sensor reads (29 min to avoid ping overlap)

#include <NewPing.h>

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

NewPing sensor = NewPing(21,20, max_dist);
/*
NewPing sonar[4] = {   // Sensor object array.
  NewPing(2, 3, max_dist), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(4, 5, max_dist),
  NewPing(18, 19, max_dist),
  NewPing(20, 21, max_dist)
};
*/
void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200); //115200 baud required for midi and RF24 signals

}

void loop() {
  // put your main code here, to run repeatedly:
  int sensorRead = sensor.ping_cm();//sonar[0].ping_cm(); //sensor pins 2 trigger and 3 echo
  Serial.print(sensorRead); 
  Serial.println(" cm" );
  delay(500);

}
