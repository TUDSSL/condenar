#include <Arduino.h>
// Quick hardware test for SPI card access.
//
#include <SPI.h>
#include "SdFat.h"
#include "driver/uart.h"
#include "sdios.h"

#define PIN_BUTT_1 25
#define PIN_BUTT_2 26
#define PIN_BUTT_3 27
#define PIN_BUTT_4 22
#define PIN_BUTT_5 13
#define PIN_BUTT_6 19
#define PIN_BUTT_7 36

#define PIN_SOLAR_SENSE 34
#define PIN_CRANK_SENSE 35
#define PIN_CRANK2_SENSE 32
#define PIN_TOTAL_SENSE 33
#define PIN_JOY_1 38
#define PIN_JOY_2 39
#define PIN_VBAT 37

#define PIN_SD_DO 14
#define PIN_SD_CD 12
#define PIN_SD_SK 21
#define PIN_SD_CS 8
#define PIN_SD_DI 7

#define PIN_LOG_UART PIN_BUTT_7//20
#define PIN_ESP_ISOLATION 4

SdFs sd;

// Serial streams
ArduinoOutStream cout(Serial);

// input buffer for line
char cinBuf[4096];
ArduinoInStream cin(Serial, cinBuf, sizeof(cinBuf));
char cinBuf1[4096];
ArduinoInStream cin1(Serial1, cinBuf1, sizeof(cinBuf1));

// SD card chip select
int chipSelect = PIN_SD_CS;

void SpinLoop(){
  cout << "Spin looping!" << endl;
  delay(2500);
  esp_restart();  
}

void ApolloSerialTask(void* arg);
void ProcessLogMessage(char* message);
void LoggingTask(void* arg);


#define CHANNEL_COUNT 25

volatile bool expStart = false;
volatile int expStartID = 0;
volatile bool expStop = false;
volatile bool eventChannels[CHANNEL_COUNT] = {0};
volatile int dataChannels[CHANNEL_COUNT] = {0};


void setup() {
  setCpuFrequencyMhz(80);
  Serial.begin(500000);

  SPI.begin(PIN_SD_SK, PIN_SD_DO, PIN_SD_DI, PIN_SD_CS); // sclk, miso, mosi, ss

  pinMode(PIN_BUTT_1, INPUT_PULLUP);
  pinMode(PIN_BUTT_2, INPUT_PULLUP);
  pinMode(PIN_BUTT_3, INPUT_PULLUP);
  pinMode(PIN_BUTT_4, INPUT_PULLUP);
  pinMode(PIN_BUTT_5, INPUT_PULLUP);
  pinMode(PIN_BUTT_6, INPUT_PULLUP);
  pinMode(PIN_BUTT_7, INPUT_PULLUP);

  pinMode(PIN_ESP_ISOLATION, OUTPUT);
  digitalWrite(PIN_ESP_ISOLATION, HIGH);

  pinMode(PIN_SOLAR_SENSE, ANALOG);
  pinMode(PIN_CRANK_SENSE, ANALOG);
  pinMode(PIN_CRANK2_SENSE, ANALOG);
  pinMode(PIN_TOTAL_SENSE, ANALOG);
  pinMode(PIN_JOY_1, ANALOG);
  pinMode(PIN_JOY_2, ANALOG);
  pinMode(PIN_VBAT, ANALOG);

  pinMode(PIN_SD_CD, INPUT_PULLUP);

  delay(1000);

  if(digitalRead(PIN_SD_CD) == HIGH){
    cout << endl<<endl<< "SD card not present." << endl << "Going to sleep..." << endl;

    esp_sleep_enable_timer_wakeup(2*1000*1000);
    esp_deep_sleep_start();
  }

  attachInterrupt(digitalPinToInterrupt(PIN_SD_CD), [](){
    ets_printf("SD card removed\n");
    delayMicroseconds(10000);
    esp_restart();   
  }, HIGH);



  if (!sd.begin(chipSelect, 4000000)) {
    if (sd.card()->errorCode()) {      
      cout << "Failed to initialize SD card!" << endl;
      SpinLoop();
    }
    cout << F("\nCard successfully initialized.\n");
    if (sd.vol()->fatType() == 0) {
      cout << F("Can't find a valid FAT16/FAT32 partition.\n");
      SpinLoop();
    }
    cout << F("Can't determine error type\n");
    SpinLoop();
  }
  cout << F("\nCard successfully initialized.\n");
  cout << endl;

  uint32_t size = sd.card()->sectorCount();
  if (size == 0) {
    cout << F("Can't determine the card size.\n");
    SpinLoop();
  }
  uint32_t sizeMB = 0.000512 * size + 0.5;
  cout << F("Card size: ") << sizeMB;
  cout << F(" MB (MB = 1,000,000 bytes)\n");
  cout << endl;
  cout << F("Volume is FAT") << int(sd.vol()->fatType());
  cout << F(", Cluster size (bytes): ") << sd.vol()->bytesPerCluster();
  cout << endl << endl;

  sd.mkdir("logs");

  FsFile fileTemp = sd.open("/test.txt", O_CREAT | O_WRITE | O_TRUNC);
  if (!fileTemp.isOpen()) {
    cout << F("Can't create file.\n");
    SpinLoop();
    return;
  }
  fileTemp.println("Everything seems to be working :)");  
  fileTemp.close();

  cout << F("Files found (date time size name):\n");
  sd.ls(LS_R | LS_DATE | LS_SIZE);

  cout << F("\n SD init success!\n");

  xTaskCreate(ApolloSerialTask, "ApolloSerialTask", 8192, NULL, 1, NULL);
  xTaskCreate(LoggingTask, "LoggingTask", 8192, NULL, 1, NULL);
}

bool firstTry = true;
void loop() {
  delay(100);
}


char serial1Buffer[4096];
char commandBuffer[4096];
int serial1BufferIndex = 0;
void ApolloSerialTask(void* arg){
  Serial.println("Starting apollo serial task");

  const uart_port_t uart_num = UART_NUM_1;
  uart_config_t uart_config = {
      .baud_rate = 19200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
      .rx_flow_ctrl_thresh = 122,
  };
  // Configure UART parameters
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1,-1, PIN_BUTT_7, -1, -1));
  // Setup UART buffered IO with event queue
  const int uart_buffer_size = (1024 * 2);
  QueueHandle_t uart_queue;
  // Install UART driver using an event queue here
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

  Serial1.setTimeout(10000000);
  while(true){    

      /*size_t dataReadyCount = 0;
      do{  //Sleep until there is UART data available
        esp_sleep_enable_timer_wakeup(100*1000);
        esp_light_sleep_start();
        cout << "Woke up!" << endl;
        uart_get_buffered_data_len(UART_NUM_1, (size_t*)&dataReadyCount);
        cout << "Data ready: " << dataReadyCount << endl;
      }while(dataReadyCount <= 0);*/
      
      int readNow = uart_read_bytes(UART_NUM_1,serial1Buffer+serial1BufferIndex, min(10, 4095-serial1BufferIndex), 10);      
      for(int i=0; i<readNow; i++){
        if(serial1Buffer[serial1BufferIndex] != '\n'){
          serial1BufferIndex++;
        }else{
          memcpy(commandBuffer, serial1Buffer, serial1BufferIndex);
          int commandLength = serial1BufferIndex;
          commandBuffer[serial1BufferIndex] = '\0';

          memmove(serial1Buffer, serial1Buffer+serial1BufferIndex+1, 4095-serial1BufferIndex);
          serial1BufferIndex -= serial1BufferIndex;

          cout << "Command from apollo: " << commandBuffer << endl;

          if(strncmp(commandBuffer, "TUD",3) != 0){
            cout << "Command not recognized!" << endl;
            continue;
          }

          int checksum = 0;
          for(int i=7; i<commandLength; i++){
            checksum += commandBuffer[i];
          }
          checksum = checksum % 256;
          
          if(atoi(commandBuffer+3) != checksum){
              cout << "Checksum failed!" << endl;
              continue;
          }

          ProcessLogMessage(commandBuffer+7);        
        }
      }

      if(serial1BufferIndex > 4085){
        cout << "Serial buffer overflow!" << endl;
        serial1BufferIndex = 0;
      }     
  }
 
}

void ProcessLogMessage(char* message){
  cout << "Got message of length " << strlen(message) <<  ": " << message << endl;
  char* token = strtok(message, ",");
  if(token == NULL){
    cout << "No tokens found!" << endl;
    return;
  }
  
  if(strcmp(token, "EXPSTART") == 0){
    
    token = strtok(NULL, ",");
    if(token == NULL){
      cout << "No tokens found!" << endl;
      return;
    }
    int experimentNumber = atoi(token);
    cout << "Starting experiment number: " << experimentNumber << endl;
    expStart = true;
    expStartID = experimentNumber;
    return;
  }else if(strcmp(token, "EXPEND") == 0){    
    cout << "Ending experiment" << endl;
    expStop = true;
    return;
  }else if(strcmp(token, "EVENT") == 0){    
    token = strtok(NULL, ",");
    if(token == NULL){
      cout << "No tokens found!" << endl;
      return;
    }
    int eventNumber = atoi(token);
    cout << "Got event number: " << eventNumber << endl;
    if(eventNumber >= CHANNEL_COUNT){
      cout << "Event number too high!" << endl;
      return;
    }
    eventChannels[eventNumber] = true;
  }else if(strcmp(token, "CHANNEL") == 0){
    token = strtok(NULL, ",");
    if(token == NULL){
      cout << "No tokens found!" << endl;
      return;
    }
    int channelNumber = atoi(token);

    token = strtok(NULL, ",");
    if(token == NULL){
      cout << "No tokens found!" << endl;
      return;
    }
    int channelValue = atoi(token);
    
    cout << "Got channel number: " << channelNumber << " with value: " << channelValue << endl;  
    if(channelNumber >= CHANNEL_COUNT){
      cout << "Channel number too high!" << endl;
      return;
    }
    dataChannels[channelNumber] = channelValue;
  }else{
    cout << "Unknown message type: " << token << endl;
  }
}

void DoLoggingIteration(char* file, int currentSampleNumber);

char CSVlineBuffer[1024];
void LoggingTask(void* arg){
    while(true){
      if(expStart){
        if(expStop) expStop = false;
        expStart = false;
        cout << "Starting experiment: " << expStartID << endl;

        int extraID = -1;
        char filename[32];
        do{
          extraID++;
          sprintf(filename, "/logs/%d_%d.csv", expStartID,extraID);
        }while(sd.exists(filename));        

        cout << "Creating logging file " << filename << endl;

        sd.mkdir("/logs");
        
        FsFile file = sd.open(filename, O_CREAT | O_WRITE | O_APPEND);
        if (!file.isOpen()) {
          cout << F("Can't create file.\n");
          SpinLoop();
        }

        memset(CSVlineBuffer, 0, 1024);
        int CSVlineBufferIndex = 0;
        CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "Sample,Time,TotalPower,SolarPower,CrankPower,Joy1,Joy2,VBat,");
        CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "B1,B2,B3,B4,B5,B6,");
        for(int i=0; i<CHANNEL_COUNT; i++){
          CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "Event%d,", i);    
        }
        for(int i=0; i<CHANNEL_COUNT; i++){
          CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "Channel%d,", i);    
        }

        file.println(CSVlineBuffer);
        file.flush();
        file.sync();
        file.close();

        cout << "File created" << endl;

        int iterationCount = 0;
        while(!expStop && !expStart){      
          DoLoggingIteration(filename,iterationCount);
          iterationCount++;
        }

        cout << "Stopping experiment with iterationCount of " << iterationCount << endl;
        expStop = false;


      }
    }
}

float totalPower = 0;

void DoLoggingIteration(char* fileName, int currentSampleNumber){

  FsFile file = sd.open(fileName, O_CREAT | O_WRITE | O_APPEND);
  if (!file.isOpen()) {
    cout << F("Can't create file.\n");
    SpinLoop();
    return;
  }

  memset(CSVlineBuffer, 0, 1024);
  int CSVlineBufferIndex = 0;

  long startTime = millis();
  float newTotalPower = 0;
  float crankPower = 0;
  float solarPower = 0;

  long totalCounts = 0;
  long totalCounts2 = 0;

  while(millis()-startTime<50){ //We need to do averaging because these pins might be noisy
    newTotalPower += analogRead(PIN_TOTAL_SENSE);
    if(totalCounts%10==0){
      solarPower += analogRead(PIN_SOLAR_SENSE);
      crankPower += analogRead(PIN_CRANK_SENSE);
      totalCounts2++;
    }       
    totalCounts++;
  }
  newTotalPower /= totalCounts;
  newTotalPower = newTotalPower / 4096 * 3.1 / 0.2 / 100 * 1.33;
  totalPower = newTotalPower*0.2 + totalPower * 0.8;
  solarPower /= totalCounts2;
  solarPower = solarPower / 4096 * 3.1 / 22 / 100 * 1.33;
  crankPower /= totalCounts2;
  crankPower = crankPower / 4096 * 3.1 / 0.05 / 100 * 1.33;

  float joy1 = analogRead(PIN_JOY_1);
  float joy2 = analogRead(PIN_JOY_2);
  float vBat = analogRead(PIN_VBAT) / 4096.0 * 3.1 / 0.182 * 1.33;

  CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "%d,%u,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,",currentSampleNumber,millis(), totalPower, solarPower, crankPower, joy1, joy2, vBat);

  CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "%d,", digitalRead(PIN_BUTT_1));    
  CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "%d,", digitalRead(PIN_BUTT_2));    
  CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "%d,", digitalRead(PIN_BUTT_3));    
  CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "%d,", digitalRead(PIN_BUTT_4));    
  CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "%d,", digitalRead(PIN_BUTT_5));    
  CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "%d,", digitalRead(PIN_BUTT_6));  


  for(int i=0; i<CHANNEL_COUNT; i++){    
    CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "%d,", eventChannels[i] ? 1 : 0);    
  }

  for(int i=0; i<CHANNEL_COUNT; i++){
    CSVlineBufferIndex += sprintf(CSVlineBuffer+CSVlineBufferIndex, "%d,", dataChannels[i]);    
  }
  file.println(CSVlineBuffer);
//file.println("Hou");
 //PrintSafe(file, CSVlineBuffer);
  
  if(!file.close()){
    cout << "Error closing file" << endl;
  }
  cout << "Sample written" << endl;
  cout << CSVlineBuffer << endl;
}

void PrintSafe(FsFile file, char* toPrint){
  int length = strlen(toPrint);
  cout << "Printing " << length << " characters" << endl;

  file.print(toPrint);
  file.println();
 /* for(int i=0;i<length;i++){
    size_t printed = file.print(toPrint[i]);

      if(printed != 1) cout << "Error printing char " << i << " of " << length << endl;

    if(i%1 == 0){
      //file.flush();
      if(!file.sync()){
        cout << "Error syncing file" << endl;
      }
    }  
  }*/

  file.println();
  file.flush();
  file.sync();
}