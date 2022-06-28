

//Include Libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>


#define pin_CE 9
#define pin_CSN 8


//create an RF24 object
RF24 radio(pin_CE, pin_CSN);  // CE, CSN

//address through which two modules communicate.
const byte address[6] = "00001";

const int DEBUG = 0; //must be 0 if powered externally

void setup()
{
  //while (!Serial);
    Serial.begin(115200);
  
  radio.begin();
  radio.setChannel(120);
  radio.setDataRate(RF24_250KBPS);
  // Set power level: RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm.
    radio.setPALevel(RF24_PA_MIN); 
    //radio.setPALevel(RF24_PA_MAX);

  
  //set the address
  radio.openReadingPipe(0, address);
  
  //Set module as receiver
  radio.startListening();
}

void loop()
{
  //Read the data if available in buffer
  if (radio.available())
  {
    int data[32] = {0};
    radio.read(&data, sizeof(data));
    printData(data);
    if(DEBUG >0){
      
        Serial.print("merry 1:: ");
        Serial.print(data[0]);
        Serial.print("  ----merry 2:: ");
        Serial.print(data[1]);
        Serial.print("  ----merry 3:: ");
        Serial.print(data[2]);
        Serial.print("  ----merry 4:: ");
        Serial.println(data[3]);
    }
    
   
  }
  else{
    //Serial.println("-----wifi connection lost-------");
    //delay(1000); 
  }
}

void printData(int x[]){
      
  for(int k=0; k<4; k++){
    switch(k){
      case 0:
        Serial.print("merry 1:: ");
        Serial.print(x[k]);
        Serial.print("  ----merry 2:: ");
        break;
      case 1:
        Serial.print(x[k]);
        Serial.print("  ----merry 3:: ");
        break;
      case 2:        
        Serial.print(x[k]);
        Serial.print("  ----merry 4:: ");
        break;
      case 3:
        Serial.println(x[k]);
        
        break;

    }
  }
}
