
// Radio Frequency Merry Go Round RECEIVER, 4ch //
// Board: Pro Micro pinout
// Flash board as: Arduino LEONARDO
// This board reads proximity sensors and sends that data via Radio Frequency (aka: wifi protocol)
// to the interpreter board. the interpreter board will take this data and convert it to midi signals for ableton

//DEBUG settings
// 0 : if in production mode
// 1 : if need to understand when triggers are being received
// 2 : if you need to see what data is actually sent

const int DEBUG = 0;

//Include Libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <MIDIUSB.h>

#define channel 5
#define pin_CE 9
#define pin_CSN 8

//create an RF24 object
RF24 radio(pin_CE, pin_CSN);  // CE, CSN

//address through which two modules communicate.
const byte address[6] = "00001";

int triggered = 0;
bool firstTime = true;

void noteOn(byte thechannel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | thechannel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void printSettings() {
  Serial.println("--Booting up Merry Go Round  RECEIVER--");
  Serial.println("--Settings are as follows:");
  Serial.print("Channel: ");
  Serial.println(radio.getChannel());
  Serial.print("Data Rate: ");
  Serial.print(radio.getDataRate());
  Serial.println("   ::  0 = min, 1 = low, 2 = high, 3 = max");
  Serial.print("Debug setting = ");
  Serial.println(DEBUG);
  Serial.print("Expected info: ");
  switch (DEBUG) {
    case 1:
      Serial.println("only merry go round triggeres received :: and the number of times received");
      break;
    case 2:
      Serial.println("Trigger data + which sensors were triggered"); //NB: maybe accurate based on the data sent but it is the data stream
      break;
  }
}

void printTriggers(int x[]) {
  if (x[0]) {
    Serial.println("Triggered");
  } else {
    Serial.println("Not Triggered");
  }
}

void printDataStream(int x[]) {
  for (int k = 0; k < 4; k++) {
    switch (k) {
      case 0:
        Serial.print("{ ");
        Serial.print(x[k]);
        Serial.print(", ");
        break;
      case 1:
        Serial.print(x[k]);
        Serial.print(" , ");
        break;
      case 2:
        Serial.print(x[k]);
        Serial.print(" , ");
        break;
      case 3:
        Serial.print(x[k]);
        Serial.println("}");
        break;
    }
  }
}

void sendMIDI_toAbleton( int x[]) {
  if (x[0] > 0) {
    noteOn(5, 48 , 127);
    triggered++;
    if (DEBUG > 0) Serial.print("Merry Go Round triggered  ::");
    if (DEBUG > 0) Serial.println(triggered++);
  } else if (x[1]) {
    //error data : aka the number of people on the merry go round is negative or above 4
  } else {
    //Serial.print("nothing sent...");
  }
}
///////----------------------------------------------------------------

void setup()
{
  Serial.begin(115200);  //115200 baud required for midi and RF24 signals

  //-----setup the radio
  radio.begin();
  radio.setChannel(120); //127 wifi channels available. channels lower than 102 may conflict with commercial 2.4gHz wifi routers
  radio.setDataRate(RF24_250KBPS);
  /// Set power level: RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_HIGH=-6dBM, and RF24_PA_MAX=0dBm.
  //radio.setPALevel(RF24_PA_MIN);
  radio.setPALevel(RF24_PA_LOW);
  //radio.setPALevel(RF24_PA_HIGH);
  //radio.setPALevel(RF24_PA_MAX);

  //set the address
  radio.openReadingPipe(0, address);

  //Set module as receiver
  radio.startListening();

  if (DEBUG > 0) printSettings();
}

void loop()
{
  /*
  if (DEBUG > 0) { //restart serial Data upon Serial open if in debug mode
    //NB: NOT TESTED LOGIC
    Serial.begin(115200);
    while (!Serial) {}
  }
*/
  //Read the data if available in buffer
  if (radio.available())
  {
    int data[32] = {0};
    radio.read(&data, sizeof(data));

    if (DEBUG > 0) printTriggers(data);
    if (DEBUG > 1) printDataStream(data);

    sendMIDI_toAbleton(data);

  }
  else {
    //Serial.println("-------");
    //delay(500);
  }

}
