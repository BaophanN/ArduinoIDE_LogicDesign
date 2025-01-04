#include "Adafruit_NeoPixel.h"
#include "DHT20.h"
#include "LiquidCrystal_I2C.h"
#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
// Define your tasks here


#define WIFI_SSID       "BaoIphone"
#define WIFI_PASSWORD   "123456789"
#define ADAFRUIT_IO_USERNAME  "khanhhuy2003"
#define ADAFRUIT_IO_KEY       "aio_lvVc393MViJyeUidjiXK7ptohMmg"

// Define MQTT server and port for Adafruit IO
#define MQTT_SERVER      "io.adafruit.com"
#define MQTT_PORT        1883

void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndRelay(void *pvParameters);
void TaskLightAndLED(void *pvParameters);
void TaskPrintLCD(void *pvParameters);
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY);

// Feeds
Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, ADAFRUIT_IO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, ADAFRUIT_IO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish soilMoistureFeed = Adafruit_MQTT_Publish(&mqtt, ADAFRUIT_IO_USERNAME "/feeds/soil");
Adafruit_MQTT_Publish lightFeed = Adafruit_MQTT_Publish(&mqtt, ADAFRUIT_IO_USERNAME "/feeds/light");

Adafruit_NeoPixel pixels3(4, D5, NEO_GRB + NEO_KHZ800);
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);


void setup() {

  // Initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  dht20.begin();
  lcd.begin(); 
  pixels3.begin();
  pinMode(D7, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  xTaskCreate( TaskBlink, "Task Blink" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskTemperatureHumidity, "Task Temperature" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskSoilMoistureAndRelay, "Task Soil & Moisture" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskLightAndLED, "Task Light LED" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskPrintLCD, "Task LCD" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskSendToAdafruitIO, "Task Adafruit IO", 2048, NULL, 2, NULL);
  //Now the task scheduler is automatically started.
  Serial.printf("Basic Multi Threading Arduino Example\n");

}

void loop() {
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/



void TaskBlink(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  // initialize digital LED_BUILTIN on pin 13 as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  

  while(1) {                          
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED ON
    delay(2000);
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED OFF
    delay(2000);
    
  }
}


void TaskTemperatureHumidity(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  while(1) {                          
    Serial.println("Task Temperature and Humidity");

    dht20.read();
    if(dht20.getTemperature() < 27){
      analogWrite(D7, 255);
    }
    else if(dht20.getTemperature() > 27){
      analogWrite(D7, 0);
    }
    Serial.println(dht20.getTemperature());
    Serial.println(dht20.getHumidity());

    delay(5000);
  }
}

void TaskSoilMoistureAndRelay(void *pvParameters) {
  while(1) {
    int soilMoisture = analogRead(A0);
    Serial.printf("Soil Moisture: %d\n", soilMoisture);
    
    if(soilMoisture > 50) {
      digitalWrite(D3, LOW);
    } else if(soilMoisture < 30) {
      digitalWrite(D3, HIGH);
    }
    
    delay(1000);
  }
}

void TaskLightAndLED(void *pvParameters) {  // This is a task.

  while(1) {                          
    Serial.println("Task Light and LED");
    Serial.println(analogRead(A1));
    
    if(analogRead(A1) < 350){
      pixels3.setPixelColor(0, pixels3.Color(0,255,0));
      pixels3.setPixelColor(1, pixels3.Color(0,255,0));
      pixels3.setPixelColor(2, pixels3.Color(0,255,0));
      pixels3.setPixelColor(3, pixels3.Color(0,255,0));
      pixels3.show();
    }
    if(analogRead(A1) > 550){
      pixels3.setPixelColor(0, pixels3.Color(0,0,0));
      pixels3.setPixelColor(1, pixels3.Color(0,0,0));
      pixels3.setPixelColor(2, pixels3.Color(0,0,0));
      pixels3.setPixelColor(3, pixels3.Color(0,0,0));
      pixels3.show();
    }
    delay(1000);
  }
     
}

void TaskPrintLCD(void *pvParameters) {
  
  while(1) {                          
    Serial.println("Task LCD");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T: ");
    lcd.print(dht20.getTemperature());
    lcd.print(" S: ");
    lcd.print(analogRead(A0));
    lcd.setCursor(0, 1);
    lcd.print("H: ");
    lcd.print(dht20.getHumidity());
    lcd.print(" L: ");
    lcd.print(analogRead(A1));
    delay(2000);
  }
}
void TaskSendToAdafruitIO(void *pvParameters) {
  while(1) {
    if(mqtt.connected()) {
      mqtt.processPackets(10000);  // Process incoming packets

      double temperature = dht20.getTemperature();
      double humidity = dht20.getHumidity();
      double soilMoisture = analogRead(A0);
      double lightLevel = analogRead(A1);

      // Publish data to Adafruit IO feeds
      temperatureFeed.publish((double)temperature);
      humidityFeed.publish((double)humidity);
      soilMoistureFeed.publish((double)soilMoisture);
      lightFeed.publish((double)lightLevel);

      Serial.println("Data sent to Adafruit IO");
    } else {
      // Reconnect if disconnected
      mqtt.connect();
    }
    delay(1000);  // Send every 10 seconds
  }
}