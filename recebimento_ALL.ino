/* 

Este código visa captar mensagens vindas a partir de um outro dispositivo , que esteja utilizando comunicação lora;
Para utilizá-lo configure no modo:
- SIMULANDO A COMUNICAÇÃO DO SENSOR: Configurar dois dispositivos, emissor e receptor, em " ESCOLHA DE DISPOSITIVO " , logo abaixo; e em seguida fazer upload em cada um deles;
  - voce pode também configurar o endereço do DISPOSITIVO EMISSOR, no trecho: " ENDEREÇOS " (Obs não modifique o )
- OUVINDO A COMUNICAÇÃO DO SENSOR: configure no trecho " ESCOLHA DE DISPOSITIVO " o uso para 

O código está sobre a licença MIT, que está no repositório: https://github.com/jpdegit/LoraScan
Autor: João Paulo Da silva Vasconcelos; Endereço para acessar CV: http://lattes.cnpq.br/7664730361329519
repositório: https://github.com/jpdegit/LoraScan
Referências: @sandeepmistry

*/
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>              // include libraries
#include <LoRa.h>

// ESCOLHA DE DISPOSITIVO : COMENTE AQUELE QUE VOCE NÃO USARÁ / DESCOMENTE AQUELE QUE VOCE USARÁ ------------------

  #define MASTER 1    //  DISPOSITIVO RECEPTOR: Aquele que tentará ouvir toda a comunicação lora à sua volta;  
  //#define SLAVE 1   //  DISPOSITIVO EMISSOR : Aquele que tentará simular o envio de mensagem lora  do sensor;

//------------------------------------------------------------------------------------------------------------------
#ifdef ARDUINO_SAMD_MKRWAN1300
#error "This example is not compatible with the Arduino MKR WAN 1300 board!"
#endif

#define SCK 5   // GPIO5  SCK
#define MISO 19 // GPIO19 MISO
#define MOSI 27 // GPIO27 MOSI
#define SS 18   // GPIO18 CS
#define RST 14  // GPIO14 RESET
#define DI00 26 // GPIO26 IRQ(Interrupt Request)

const int csPin = 18;          // LoRa radio chip select
const int resetPin = 14;       // LoRa radio reset
const int irqPin = 26;         // change for your board; must be a hardware interrupt pin

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages

//ENDEREÇOS :  -------------------------------------------------------------------------------------------------------------------------------

  //ENDEREÇO DO EMISSOR:  ----------------------------------------------------------------------------------------------------------------------
  #ifdef SLAVE    
  byte localAddress = 0x55;     // Endereço deste dispositivo
  byte destination = 0x26;      // Endereço de destino
  #endif
  
  //ENDEREÇO DO RECEPTOR: 
  #ifdef MASTER
  byte localAddress = 0xFF;     // address of this device 
  byte destination = 0xFF;      // send to all            
  #endif
//-----------------------------------------------------------------------------------------------------------------------------------------------------
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

float optionsFrequencies[] = { 300E6, 433E6, 600E6, 866E6, 915E6 };
int gainOptions[] = {0, 1, 2, 3, 4, 5, 6};
int spreadingFactorOptions[] = {6, 7, 8, 9, 10, 11, 12};
float signalBandwithOptions[] = {7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, 500E3};
int codingRateOptions[] = {5, 6, 7, 8};
int lastVerified = 0;

int atualOptionFrequency = 0;
int atualOptionGain = 0;
int atualOptionSpreadingFactor = 0;
int atualOptionSignalBandwith = 0;
int atualOptionCodingRate = 0;

int recipient = 0;          // recipient address
byte sender = 0;            // sender address
byte incomingMsgId = 0;     // incoming msg ID
byte incomingLength = 0;    // incoming msg length

int receivedRSSI = 0;
String snr = "0";
bool receivedData = false;

long frequency = 915E6;
int tries = 0;

void setup() {    
  Serial.begin(9600);                   // initialize serial
  while (!Serial);
  LoRa.onReceive(onReceive);
  LoRa.receive();

  Serial.println("LoRa Duplex with callback");
  
  Serial.print("Address: ");
  Serial.print(localAddress);
  Serial.println();
  
  Serial.print("Destination: ");
  Serial.print(destination);
  Serial.println();

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin
  
  if (!LoRa.begin(frequency)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }
  
  //LoRa.enableCrc();
  
  LoRa.setGain(1);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(500E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(20);
  LoRa.onReceive(onReceive);
  LoRa.receive();
  
  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (millis() - lastSendTime > interval) {
    tries++;
    
    if(tries == 10){
      //frequency -= 150E6;
      //LoRa.setFrequency(frequency);
      //tries = 0;
    }

    
    
    String message = "HeLoRa World!";   // send a message
    sendMessage(message);
    
    Serial.print("Frequency: ");
    Serial.println(frequency);
    
    Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;     // 2-3 seconds
   
    
    
    
    
      LoRa.receive();
                      // go back into receive mode
  
  

  }
}

void sendMessage(String outgoing) {

  if(!LoRa.beginPacket()){ // start packet
    Serial.println("ERRO AO ENVIAR MENSAGEM");
  }
  
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  
  if(!LoRa.endPacket(true)){ // start packet
    Serial.println("ERRO AO FINALIZAR MENSAGEM");
  }

  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  recipient = LoRa.read();              // recipient address
  sender = LoRa.read();                 // sender address
  incomingMsgId = LoRa.read();          // incoming msg ID
  incomingLength = LoRa.read();         // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  Serial.print("Message received from: 0x");
  Serial.print(sender, HEX);
  Serial.print(". ");
  Serial.print(incoming);
  Serial.println();
}
