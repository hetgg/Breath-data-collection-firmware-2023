/*
  Firmware: MS_Project_4_test 
     This Code has been tested end to end and recived the data to the cloud end.

   This is the most up to date and recent version as of Nov 30, 2023

   By - Het Patel
*/

/*
 * Version Info
  Implemented fucntionalitites:
    *Low power mode
    *SD card data logging capability
    *Data recieved to Notehub
    *Second breath will wait unit the button is presed
 */



#include <Notecard.h>
#include <NotecardPseudoSensor.h> // Not needed except you'd like to use temp and hum data onbaord
#include<SD.h>
#include<SPI.h> // May not needed 
#include<FS.h>
#include <ArduinoJson.h>

#define SERIAL_DEBUG_LEVEL 1


#define SENSOR1_2612 25 
#define SENSOR2_2602 34
#define SENSOR3_2620 39


#define SENS_POWER 17
#define R_LED 15
#define G_LED 33
#define BUTTON_PIN GPIO_NUM_14
#define BUZZ_PIN 32
#define chipSelect 27


#define SENS_WARMING_TIME 240


#define BEEP_TIME 240
#define LONG_PRESS_DURATION 2000


//an array capable to store integer values
int sensor1_2612A[3]; 
int sensor2_2602A[3]; 
int sensor3_2620A[3];


int sensor1_2612B[6]; 
int sensor2_2602B[6]; 
int sensor3_2620B[6];


struct ArrayData { // Define a structure to hold an array of sensor data
    int VOL_ADC_VALUE[27];
};


#define usbSerial Serial 
#define productUID "edu.sonoma.patelhe:islgetstarted" 
using namespace blues;


Notecard notecard; 
NotecardPseudoSensor sensor(notecard);
File DataFile;


QueueHandle_t xQueue;


TaskHandle_t xTask1Handle;
TaskHandle_t xTask2Handle; 
TaskHandle_t xTask3Handle; 
TaskHandle_t xTask4Handle; 

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}


void vTask1(void *pvParameters) { 
  while(digitalRead(BUTTON_PIN) == HIGH){
    Serial.println("-------------------------Program Started-------------------------");
    unsigned long lastMillis = 0;
    unsigned long task1StartTime = 0; 
    unsigned long task1EndTime = 0; 
    ArrayData arrayData; 


    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);


    Serial.println("---------------------Sensor Power has been Fed-------------------");
    digitalWrite(SENS_POWER, HIGH);
    lastMillis = millis()/1000;


    while ((millis()/1000-lastMillis) < (SENS_WARMING_TIME)) {
      digitalWrite(R_LED, HIGH);
      delay(250);
      digitalWrite(R_LED, LOW);
      delay(600);
      
      checkLongPress();
      delay(10);  
      Serial.print("Elapsed time: ");
      Serial.print((millis() - lastMillis) / 1000);
      Serial.println(" seconds");
    }
    Serial.println("--> Sensor warming completed");


    // while(1)
    // { // 1st Air Readings 
      Serial.println("--> First AIR reading");
      for (int i = 0; i < 3; i++) {
        sensor1_2612A[i] = analogRead(SENSOR1_2612);
        delay(10);
        sensor2_2602A[i] = analogRead(SENSOR2_2602); 
        delay(10);
        sensor3_2620A[i] = analogRead(SENSOR3_2620);
        delay(500);
      }
      
      lastMillis = millis();
      int i = 0, j = 0;
      
      while (!digitalRead(BUTTON_PIN)) {  
        digitalWrite(R_LED, HIGH);
        delay(100);  
        digitalWrite(R_LED, LOW);
        delay(100);
        tone(BUZZ_PIN,457,250);//Sound the buzzer
        delay(100);
     


      if ((millis()-lastMillis)>1000){
        if (i > 2) {
          i = 0;
        }
        // 2nd Air Readings
        Serial.println("--> Second AIR reading (overwritting)");
        sensor1_2612A[i] = analogRead(SENSOR1_2612); //reads sensor1 input for air
        delay(10);
        sensor2_2602A[i] = analogRead(SENSOR2_2602); //reads sensor2 input for air
        delay(10);
        sensor3_2620A[i] = analogRead(SENSOR3_2620); //reads sensor3 input for air
        delay(10);
        i++;
        lastMillis = millis();
        j++;
      }
       if (j > BEEP_TIME){
         Low_Power_Sleep();
      }
    }
    digitalWrite(R_LED, LOW); //turn LED off
    delay(10);


    //FOR USER: BREATHE INTO THE SENSOR when blinking green  
    //Wait four seconds


    for (int i = 0; i < 20; i++) {
      digitalWrite(G_LED, HIGH);
      delay(100);
      digitalWrite(G_LED, LOW);
      delay(100);
    }


    //Read the Sensors - First Reading (1)
    Serial.println("--> First BREATH Reading");
    for (int i = 0; i < 3; i++) {
      sensor1_2612B[i] = analogRead(SENSOR1_2612);
      delay(10);
      sensor2_2602B[i] = analogRead(SENSOR2_2602);
      delay(10);
      sensor3_2620B[i] = analogRead(SENSOR3_2620);
      delay(10); 


    for (int i = 0; i < 5; i++) {
      digitalWrite(G_LED, HIGH);
      delay(100);
      digitalWrite(G_LED, LOW);
      delay(100);
      }
    }
    Serial.println("--> Taken");




    for (int i = 0; i < 3; i++) {
      digitalWrite(R_LED, HIGH);
      delay(100);
      digitalWrite(R_LED, LOW);
      delay(100);
      tone(BUZZ_PIN,457,100);//Sound the buzzer
      delay(50);
    }
  
   lastMillis = millis();
   int second_read = 1;
   int skip_second_read = 0; 


    Serial.println("--> Waiting for the button to press, Second Breath");
     // If button is pressed and taken the second reading...do following -- while (!digitalRead(BUTTON_PIN)==0 && second_read) {  
    while((!digitalRead(BUTTON_PIN) == HIGH) && second_read == 1){  // updated on Nov 30th
      Serial.println("Inside the while loop");
      Serial.print("--> BUTTON PIN value is: ");
      Serial.println(BUTTON_PIN);
      digitalWrite(R_LED, HIGH);
      delay(100);  
      digitalWrite(R_LED, LOW);
      delay(300); 


      if (second_read && (((millis()-lastMillis)/1000)>60)){
        second_read = 0;
        skip_second_read = 1;
        // Assign second breath readings as zeros if no breath within 60 seconds   
        // Discard reading    
        for (int i = 3; i < 6; i++) {
          sensor1_2612B[i] = 0;
          delay(10);
          sensor2_2602B[i] = 0;
          delay(10);
          sensor3_2620B[i] = 0;
          delay(10);
        }     
      }Serial.println("60 sec passed and exits the while loop");
    }


      //FOR USER: BREATHE INTO THE SENSOR when blinking green


      if(!skip_second_read){
        for (int i = 0; i < 20; i++) {
          digitalWrite(G_LED, HIGH);
          delay(100);
          digitalWrite(G_LED, LOW);
          delay(100);
        }
  //Read the Sensors
   Serial.println("--> Reading Sensors NOW last time, Second Breath Reading");
   for (int i = 3; i < 6; i++) {
    sensor1_2612B[i] = analogRead(SENSOR1_2612); //reads sensor1 input
    delay(10);
    sensor2_2602B[i] = analogRead(SENSOR2_2602); //reads sensor2 input
    delay(10);
    sensor3_2620B[i] = analogRead(SENSOR3_2620); //reads sensor3 input
    delay(10);


   for (int i = 0; i < 5; i++) {
    digitalWrite(G_LED, HIGH);
    delay(100);
    digitalWrite(G_LED, LOW);
    delay(100);
    }
   }
  }


  // ------------ Three fast BEEP ----------- END OF THE TEST ----------- //


  if(!skip_second_read){
     for (int i = 0; i < 3; i++) {
      digitalWrite(R_LED, HIGH);
      delay(100);
      digitalWrite(R_LED, LOW);
      delay(100);
      tone(BUZZ_PIN,457,100);//Sound the buzzer
      delay(50);
     }
  }


  Serial.println("Test Completed Succefully");
  
  // Disable Sensor Heating
  digitalWrite(SENS_POWER, LOW);
  delay(10);


  digitalWrite(G_LED, LOW);
  delay(10);
  digitalWrite(R_LED, LOW);
  delay(10);
   
  // Convert data and add it to the array
  arrayData.VOL_ADC_VALUE[0] = sensor1_2612A[0];
  arrayData.VOL_ADC_VALUE[1] = sensor1_2612A[1];
  arrayData.VOL_ADC_VALUE[2] = sensor1_2612A[2];


  arrayData.VOL_ADC_VALUE[3] = sensor2_2602A[0];
  arrayData.VOL_ADC_VALUE[4] = sensor2_2602A[1];
  arrayData.VOL_ADC_VALUE[5] = sensor2_2602A[2];


  arrayData.VOL_ADC_VALUE[6] = sensor3_2620A[0];
  arrayData.VOL_ADC_VALUE[7] = sensor3_2620A[1];
  arrayData.VOL_ADC_VALUE[8] = sensor3_2620A[2];


  arrayData.VOL_ADC_VALUE[9] = sensor1_2612B[0];
  arrayData.VOL_ADC_VALUE[10] = sensor1_2612B[1];
  arrayData.VOL_ADC_VALUE[11] = sensor1_2612B[2];
  arrayData.VOL_ADC_VALUE[12] = sensor1_2612B[3];
  arrayData.VOL_ADC_VALUE[13] = sensor1_2612B[4];
  arrayData.VOL_ADC_VALUE[14] = sensor1_2612B[5];


  arrayData.VOL_ADC_VALUE[15] = sensor2_2602B[0];
  arrayData.VOL_ADC_VALUE[16] = sensor2_2602B[1];
  arrayData.VOL_ADC_VALUE[17] = sensor2_2602B[2];
  arrayData.VOL_ADC_VALUE[18] = sensor2_2602B[3];
  arrayData.VOL_ADC_VALUE[19] = sensor2_2602B[4];
  arrayData.VOL_ADC_VALUE[20] = sensor2_2602B[5];
  
  arrayData.VOL_ADC_VALUE[21] = sensor3_2620B[0];
  arrayData.VOL_ADC_VALUE[22] = sensor3_2620B[1];
  arrayData.VOL_ADC_VALUE[23] = sensor3_2620B[2];
  arrayData.VOL_ADC_VALUE[24] = sensor3_2620B[3];
  arrayData.VOL_ADC_VALUE[25] = sensor3_2620B[4];
  arrayData.VOL_ADC_VALUE[26] = sensor3_2620B[5];  


/* Added for debugging purporse
  unsigned k = 0;
  Serial.println("DATA added to array #: ");
  Serial.println(k);
  k++;
*/


  xQueueSend(xQueue, &arrayData, portMAX_DELAY);
  task1EndTime = millis();


  // Serial.print("--> Task 1 Execution Time: "); 
  // Serial.println(task1EndTime - task1StartTime); 
  // delay(250);


  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // this needs to go on top.
  }
  // } while(1) loop ends here
}


J *req = NULL;
ArrayData arrayData; 

void vTask2(void *pvParameters) { 
    unsigned long task2StartTime = 0;
    unsigned long task2EndTime = 0;
    
    while (1) { 
        task2StartTime = millis();
        float voltage;    // Useless remove it later on    
        if (xQueueReceive(xQueue, &arrayData, portMAX_DELAY) == pdTRUE) { 
            // float temperature = sensor.temp(); 
            // float humidity = sensor.humidity();
            
            req = notecard.newRequest("note.add");  
            if (req != NULL) { 
                JAddStringToObject(req, "file", "sensors.qo"); 
                JAddBoolToObject(req, "sync", true); 
                J *body = JAddObjectToObject(req, "body");
                if (body) { 
                    // JAddNumberToObject(body, "temp", temperature); 
                    // JAddNumberToObject(body, "humidity", humidity);
                    JAddNumberToObject(body, "Sensor_A_1_1", arrayData.VOL_ADC_VALUE[0]); 
                    JAddNumberToObject(body, "Sensor_A_1_2", arrayData.VOL_ADC_VALUE[1]); 
                    JAddNumberToObject(body, "Sensor_A_1_3", arrayData.VOL_ADC_VALUE[2]); 
                    JAddNumberToObject(body, "Sensor_A_2_1", arrayData.VOL_ADC_VALUE[3]); 
                    JAddNumberToObject(body, "Sensor_A_2_2", arrayData.VOL_ADC_VALUE[4]); 
                    JAddNumberToObject(body, "Sensor_A_2_3", arrayData.VOL_ADC_VALUE[5]); 
                    JAddNumberToObject(body, "Sensor_A_3_1", arrayData.VOL_ADC_VALUE[6]); 
                    JAddNumberToObject(body, "Sensor_A_3_2", arrayData.VOL_ADC_VALUE[7]); 
                    JAddNumberToObject(body, "Sensor_A_3_3", arrayData.VOL_ADC_VALUE[8]); 
                    JAddNumberToObject(body, "Sensor_1_1", arrayData.VOL_ADC_VALUE[9]); 
                    JAddNumberToObject(body, "Sensor_1_2", arrayData.VOL_ADC_VALUE[10]); 
                    JAddNumberToObject(body, "Sensor_1_3", arrayData.VOL_ADC_VALUE[11]); 
                    JAddNumberToObject(body, "Sensor_1_4", arrayData.VOL_ADC_VALUE[12]);
                    JAddNumberToObject(body, "Sensor_1_5", arrayData.VOL_ADC_VALUE[13]); 
                    JAddNumberToObject(body, "Sensor_1_6", arrayData.VOL_ADC_VALUE[14]); 
                    JAddNumberToObject(body, "Sensor_2_1", arrayData.VOL_ADC_VALUE[15]);
                    JAddNumberToObject(body, "Sensor_2_2", arrayData.VOL_ADC_VALUE[16]); 
                    JAddNumberToObject(body, "Sensor_2_3", arrayData.VOL_ADC_VALUE[17]); 
                    JAddNumberToObject(body, "Sensor_2_4", arrayData.VOL_ADC_VALUE[18]);
                    JAddNumberToObject(body, "Sensor_2_5", arrayData.VOL_ADC_VALUE[19]); 
                    JAddNumberToObject(body, "Sensor_2_6", arrayData.VOL_ADC_VALUE[20]); 
                    JAddNumberToObject(body, "Sensor_3_1", arrayData.VOL_ADC_VALUE[21]); 
                    JAddNumberToObject(body, "Sensor_3_2", arrayData.VOL_ADC_VALUE[22]); 
                    JAddNumberToObject(body, "Sensor_3_3", arrayData.VOL_ADC_VALUE[23]); 
                    JAddNumberToObject(body, "Sensor_3_4", arrayData.VOL_ADC_VALUE[24]);
                    JAddNumberToObject(body, "Sensor_3_5", arrayData.VOL_ADC_VALUE[25]); 
                    JAddNumberToObject(body, "Sensor_3_6", arrayData.VOL_ADC_VALUE[26]); 
                                       
                }
                 task2EndTime = millis(); 
                //  Serial.println("--> Task2 Ends here");
                //  Serial.print("Task 2 Execution Time: "); 
                //  Serial.println(task2EndTime - task2StartTime); 
                //  Serial.println("ms"); 
                 xTaskNotify(xTask3Handle, 0x01, eSetBits); // Send a notification to task 3 to start execution
            }
        }
    }
}





// This line defines a function named vTask3 that takes a pointer to void as a parameter. This is a common pattern for tasks in FreeRTOS, where the parameter can be used to pass data to the task.
void vTask3(void *pvParameters) { 
// This line declares a variable of type ArrayData, which was defined earlier in your code as a structure to hold an array of sensor data.
// These lines declare variables to hold the start and end times of the task, initialized to 0.    
    unsigned long task3StartTime = 0; // Declare a variable to hold the start time of task 3
    unsigned long task3EndTime = 0; // Declare a variable to hold the end time of task 3
    // This line starts an infinite loop, which is common in FreeRTOS tasks.
    while (1) { // Loop indefinitely
      // This line records the start time of the task in milliseconds.
        task3StartTime = millis(); // Get the current time in milliseconds
        // This line causes the task to block until it receives a notification. This is a common pattern in FreeRTOS for synchronizing tasks.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for a notification to start execution
        // This line sends a request to the Notecard. The req variable is not defined in this function, so it must be a global variable or defined elsewhere in your code.
        notecard.sendRequest(req); // Send the Notecard request created in task 2

        // These lines control the state of the red and green LEDs. They are turned on, the task waits for 2 seconds, and then they are turned off.
        digitalWrite(R_LED, HIGH); // Turn on the red LED
        digitalWrite(G_LED, HIGH); // Turn on the green LED
        delay(2000); // Wait for 2 seconds
        digitalWrite(R_LED, LOW); // Turn off the red LED
        digitalWrite(G_LED, LOW); // Turn off the green LED


        //--------------------------- Log data to SD Card ---------------------------


        Serial.println("--> Data Logging to SD Card");
        
        DynamicJsonDocument jsonDoc(1024); // Create a JSON document to hold the data
        String jsonStr; // String to hold the JSON data

            // Create a JSON object from the request- For debugging 
        Serial.println("-----------------Notecard DATA-----------------");
        Serial.println(String(arrayData.VOL_ADC_VALUE[0]));
        Serial.println(String(arrayData.VOL_ADC_VALUE[1]));
        Serial.println(String(arrayData.VOL_ADC_VALUE[2]));
        Serial.println(String(arrayData.VOL_ADC_VALUE[3]));
        Serial.println(String(arrayData.VOL_ADC_VALUE[4]));
        Serial.println(String(arrayData.VOL_ADC_VALUE[5]));
        Serial.println(String(arrayData.VOL_ADC_VALUE[6]));
        Serial.println(String(arrayData.VOL_ADC_VALUE[7]));
        Serial.println(String(arrayData.VOL_ADC_VALUE[8]));
        Serial.println(String(arrayData.VOL_ADC_VALUE[9]));
            
        Serial.println((arrayData.VOL_ADC_VALUE[0]));
        Serial.println((arrayData.VOL_ADC_VALUE[1]));
        Serial.println((arrayData.VOL_ADC_VALUE[2]));
        Serial.println((arrayData.VOL_ADC_VALUE[3]));
        Serial.println((arrayData.VOL_ADC_VALUE[4]));
        Serial.println((arrayData.VOL_ADC_VALUE[5]));
        Serial.println((arrayData.VOL_ADC_VALUE[6]));
        Serial.println((arrayData.VOL_ADC_VALUE[7]));
        Serial.println((arrayData.VOL_ADC_VALUE[8]));
        Serial.println((arrayData.VOL_ADC_VALUE[9]));
            
        JsonObject jsonObj = jsonDoc.to<JsonObject>();
        jsonObj["Sensor_A_1_1"] = String(arrayData.VOL_ADC_VALUE[0]);
        jsonObj["Sensor_A_1_2"] = String(arrayData.VOL_ADC_VALUE[1]);
        jsonObj["Sensor_A_1_3"] = String(arrayData.VOL_ADC_VALUE[2]);
        jsonObj["Sensor_A_2_1"] = String(arrayData.VOL_ADC_VALUE[3]);
        jsonObj["Sensor_A_2_2"] = String(arrayData.VOL_ADC_VALUE[4]);
        jsonObj["Sensor_A_2_3"] = String(arrayData.VOL_ADC_VALUE[5]);
        jsonObj["Sensor_A_3_1"] = String(arrayData.VOL_ADC_VALUE[6]);
        jsonObj["Sensor_A_3_2"] = String(arrayData.VOL_ADC_VALUE[7]);
        jsonObj["Sensor_A_3_3"] = String(arrayData.VOL_ADC_VALUE[8]);
        jsonObj["Sensor_1_1"] = String(arrayData.VOL_ADC_VALUE[9]);
        jsonObj["Sensor_1_2"] = String(arrayData.VOL_ADC_VALUE[10]);
        jsonObj["Sensor_1_3"] = String(arrayData.VOL_ADC_VALUE[11]);
        jsonObj["Sensor_1_4"] = String(arrayData.VOL_ADC_VALUE[12]);
        jsonObj["Sensor_1_5"] = String(arrayData.VOL_ADC_VALUE[13]);
        jsonObj["Sensor_1_6"] = String(arrayData.VOL_ADC_VALUE[14]);
        jsonObj["Sensor_2_1"] = String(arrayData.VOL_ADC_VALUE[15]);
        jsonObj["Sensor_2_2"] = String(arrayData.VOL_ADC_VALUE[16]);
        jsonObj["Sensor_2_3"] = String(arrayData.VOL_ADC_VALUE[17]);
        jsonObj["Sensor_2_4"] = String(arrayData.VOL_ADC_VALUE[18]);
        jsonObj["Sensor_2_5"] = String(arrayData.VOL_ADC_VALUE[19]);
        jsonObj["Sensor_2_6"] = String(arrayData.VOL_ADC_VALUE[20]);
        jsonObj["Sensor_3_1"] = String(arrayData.VOL_ADC_VALUE[21]);
        jsonObj["Sensor_3_2"] = String(arrayData.VOL_ADC_VALUE[22]);
        jsonObj["Sensor_3_3"] = String(arrayData.VOL_ADC_VALUE[23]);
        jsonObj["Sensor_3_4"] = String(arrayData.VOL_ADC_VALUE[24]);
        jsonObj["Sensor_3_5"] = String(arrayData.VOL_ADC_VALUE[25]);
        jsonObj["Sensor_3_6"] = String(arrayData.VOL_ADC_VALUE[26]);

        // Serialize the JSON object to a string-
        serializeJson(jsonObj, jsonStr);
        // Append the JSON data to the SD card
        appendFile(SD, "/data.txt", jsonStr.c_str());
        readFile(SD, "/data.txt");
        task3EndTime = millis(); // Get the current time in milliseconds
        
        // Serial.println("Task 3 Execution Time: "); 
        // Serial.println(task3EndTime - task3StartTime); 
        // Serial.println("ms");  
        Serial.println("--> Task3 Ends here");       
        
        Low_Power_Sleep();
        xTaskNotify(xTask1Handle, 0x01, eSetBits); // Send a notification to task 1 to start execution
        Serial.println("");
        Serial.println("exited the task 3");
       
    }
}




void setup() {
  // put your setup code here, to run once:  
    Serial.println("Setup started");
    Serial.print("\n");
    Serial.println("-------------Welcome to ISL Research Study Prototype-------------");
    delay(500); // Wait for 0.5 seconds
    usbSerial.begin(115200); // Initialize the USB serial port with a baud rate of 115200
  
    if(!SD.begin(27)){
      Serial.println("Card Mount Failed");
      return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
      Serial.println("MMC");
    } else if(cardType == CARD_SD){
      Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
    } else {
      Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    pinMode(SENS_POWER, OUTPUT); // Set the sensor power pin as an output
    pinMode(R_LED, OUTPUT); // Set the red LED pin as an output
    pinMode(G_LED, OUTPUT); // Set the green LED pin as an output
    pinMode(chipSelect, OUTPUT);
  
    notecard.begin(); // Initialize the Notecard
    notecard.setDebugOutputStream(usbSerial); // Set the debug output stream for the Notecard to the USB serial port


    xQueue = xQueueCreate(30, sizeof(ArrayData)); // Create a queue to hold 30 elements of type ArrayData


    if (xQueue == NULL){
       /* Queue was not created and must not be used. */
      return;
    }
    
    xTaskCreate(vTask1, "Task 1", 2000, NULL, 1, &xTask1Handle); // Create task 1 with a stack size of 2000 bytes and priority 1
    xTaskCreate(vTask2, "Task 2", 2000, NULL, 1, &xTask2Handle); // Create task 2 with a stack size of 2000 bytes and priority 1
    xTaskCreate(vTask3, "Task 3", 3000, NULL, 1, &xTask3Handle); // Create task 3 with a stack size of 2000 bytes and priority 1
    
    xTaskNotify(xTask1Handle, 0x01, eSetBits); // Send a notification to task 1 to start execution
}


void loop() {
  // put your main code here, to run repeatedly:


}


void checkLongPress_Reset() {
  int long_press_r = 0;


  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
    long_press_r++;
    delay(100);


    // Check for a long press
    if (long_press_r > LONG_PRESS_DURATION / 100) {
      for (int i = 0; i < 3; i++) {
        digitalWrite(R_LED, HIGH);
        delay(125);
        digitalWrite(R_LED, LOW);
        delay(250);
        tone(BUZZ_PIN, 457, 100); // Sound the buzzer
        delay(50);
      }
      delay(1000);
      Serial.print("\n");
      Serial.print("Restarting...");


      // Turn off the buzzer
      noTone(BUZZ_PIN);
      delay(100);


      // Perform any additional cleanup or actions before sleeping - if needed


      // Call your sleep function
      Low_Power_Sleep();
    }
  }
}


void checkLongPress(){
  int long_press = 0;
   delay(10);
  while(digitalRead(BUTTON_PIN)==HIGH){
    Serial.println("Button is pressed inside checkLongPress function");
    delay(10);
    long_press++;
    delay(1000);
    if (long_press > 2){
    for (int i = 0; i < 5; i++) {
      digitalWrite(R_LED, HIGH);
      delay(125);
      digitalWrite(R_LED, LOW);
      delay(250);
      tone(BUZZ_PIN,457,100);//Sound the buzzer
      delay(50);
      Serial.println("in the long press reset");
     }
//noTone(BUZZ_PIN); //Turn buzzer off
//     delay(100);
     
     delay(1000);
      Low_Power_Sleep();
      delay(100); 
    }
  }
 }




void Low_Power_Sleep() {
  // Your existing sleep code...
  Serial.println("Going to sleep...");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 1); // GPIO_NUM_14 is mapped to BUTTON_PIN
  esp_deep_sleep_start();
  Serial.println("Button is Pressed and Interrupt called");
}


// void low_power_sleep (int SLEEP_MULTIPLIER){
//   for (int i = 0; i < (10*SLEEP_MULTIPLIER*SLEEP_MULTIPLIER); i++) { //20 sleep for 60 seconds * multiplier - modified by SS July 23, 2022 
//       Watchdog.sleep(6000); //sleep for 6 second
//      // delay(10);
//      // checkLongPress();
//       delay(10);
//      }
//  }
