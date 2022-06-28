
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>


#define pin_CE 9
#define pin_CSN 8

#define numData 4


//create an RF24 object
RF24 radio(pin_CE, pin_CSN);  // CE, CSN

//address through which two modules communicate.
const byte address[6] = "00001";

int dataStream[] = {0,0,0,0}; 

const int DEBUG = 1; 

void setup()
{
      //while (!Serial);
    Serial.begin(115200);
    
  radio.begin();

  for(int i=0; i<numData; i++){
    dataStream[i] = 0; 
  }
  radio.setChannel(120);
  radio.setDataRate(RF24_250KBPS);
  // Set power level: RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm.
    radio.setPALevel(RF24_PA_HIGH); 
    //radio.setPALevel(RF24_PA_MAX);
  
  //set the address
  radio.openWritingPipe(address);
  
  //Set module as transmitter
  radio.stopListening();
  

}
void loop(){
  //test data streaming
      for(int i = 0 ; i<4; i++){
        dataStream[i]++;
       }
      bool txSuccess = radio.write(&dataStream, sizeof(dataStream));
      if(DEBUG >0){
      Serial.print(dataStream[0]);
      Serial.print(" ::  "); 
      Serial.println(txSuccess); 
      delay(2000);
      }
      /*
            for(int i = 0 ; i<4; i++){
        dataStream[i]=0;
       }
      radio.write(&dataStream, sizeof(dataStream));
            delay(1000);
*/
}

/*
void loop()
{
  //measure humidity

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float c = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  
  //make Data
  for(int i=0; i<numData; i++){
    switch(i){
      case 0:
        dataStream[i] = h;
        break;
      case 1:
        dataStream[i] = c;
        break;
      case 2:        
        dataStream[i] = dht.computeHeatIndex(c, h, false);;
        break;
      case 3:
        dataStream[i] = f;
        break;
      case 4:        
        dataStream[i] = dht.computeHeatIndex(f, h);;
        break;
    }
 
  }
  radio.write(&dataStream, sizeof(dataStream));
  
  delay(1000);
}
*/
