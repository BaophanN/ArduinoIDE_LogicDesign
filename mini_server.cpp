#include "Adafruit_NeoPixel.h"
#include "DHT20.h"
#include "LiquidCrystal_I2C.h"
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
// Define your AP credentials


void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndRelay(void *pvParameters);
void TaskLightAndLED(void *pvParameters);
void TaskPrintLCD(void *pvParameters);

// Components
Adafruit_NeoPixel pixels3(4, D5, NEO_GRB + NEO_KHZ800);
DHT20 dht20;
LiquidCrystal_I2C lcd(33, 16, 2);

// Web Server instance
WebServer server(80);

// Global sensor data variables
float currentTemperature = 0;
float currentHumidity = 0;
int currentSoilMoisture = 0;
int currentLight = 0;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    dht20.begin();
    lcd.begin();
    pixels3.begin();
    pinMode(D7, OUTPUT);
    pinMode(A0, INPUT); // Soil moisture sensor
    pinMode(A1, INPUT); // Light sensor

    // Set ESP32 as an Access Point
    WiFi.softAP(ssid, password);

    // Print AP details
    Serial.println("Access Point Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Set up web server route
    server.on("/", handleRoot);

    // Register the route for data (sensor JSON)
    server.on("/data", handleData);
    server.begin();
    Serial.println("HTTP server started");

    // Start tasks
    xTaskCreate(TaskBlink, "Task Blink", 2048, NULL, 2, NULL);
    xTaskCreate(TaskTemperatureHumidity, "Task Temperature", 2048, NULL, 2, NULL);
    xTaskCreate(TaskSoilMoistureAndRelay, "Task Soil & Moisture", 2048, NULL, 2, NULL);
    xTaskCreate(TaskLightAndLED, "Task Light LED", 2048, NULL, 2, NULL);
    xTaskCreate(TaskPrintLCD, "Task LCD", 2048, NULL, 2, NULL);
}

void loop() {
    server.handleClient(); // Handle web requests
}

// Task to blink LED
void TaskBlink(void *pvParameters) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (1) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(2000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(2000);
    }
}

// Task to handle temperature and humidity
void TaskTemperatureHumidity(void *pvParameters) {
    while (1) {
        dht20.read();
        currentTemperature = dht20.getTemperature();
        currentHumidity = dht20.getHumidity();
        delay(5000);
    }
}

// Task to handle soil moisture and relay
void TaskSoilMoistureAndRelay(void *pvParameters) {
    while (1) {
        currentSoilMoisture = analogRead(A0);
        delay(1000);
    }
}

// Task to handle light sensor and LED
void TaskLightAndLED(void *pvParameters) {
    while (1) {
        currentLight = analogRead(A1);
        delay(1000);
    }
}

// Task to display data on LCD
void TaskPrintLCD(void *pvParameters) {
    while (1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("T: ");
        lcd.print(currentTemperature);
        lcd.print(" S: ");
        lcd.print(currentSoilMoisture);
        lcd.setCursor(0, 1);
        lcd.print("H: ");
        lcd.print(currentHumidity);
        lcd.print(" L: ");
        lcd.print(currentLight);
        delay(2000);
    }
}

void handleRoot() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP32 Real-Time Dashboard</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                margin: 0;
                padding: 0;
                background-color: #f4f4f9;
                color: #333;
                text-align: center;
            }
            h1 {
                background-color: #4CAF50;
                color: white;
                margin: 0;
                padding: 20px;
            }
            .grid-container {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
                gap: 20px;
                padding: 20px;
                margin: 20px;
            }
            .grid-item {
                background-color: #ffffff;
                border: 2px solid #4CAF50;
                border-radius: 10px;
                box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.1);
                padding: 20px;
                text-align: center;
                font-size: 18px;
                font-weight: bold;
            }
            .value {
                font-size: 24px;
                color: #4CAF50;
            }
        </style>
        <script>
            function fetchData() {
                fetch('/data')
                    .then(response => response.json())
                    .then(data => {
                        document.getElementById('temp-value').innerText = data.temperature + " °C";
                        document.getElementById('humidity-value').innerText = data.humidity + " %";
                        document.getElementById('soil-value').innerText = data.soilMoisture;
                        document.getElementById('light-value').innerText = data.light;
                    })
                    .catch(err => console.log("Error: ", err));
            }
            setInterval(fetchData, 1000); // Update every second
        </script>
    </head>
    <body>
        <h1>ESP32 Sensor Dashboard</h1>
        <div class="grid-container">
            <div class="grid-item">
                Temperature
                <div id="temp-value" class="value">Loading...</div>
            </div>
            <div class="grid-item">
                Humidity
                <div id="humidity-value" class="value">Loading...</div>
            </div>
            <div class="grid-item">
                Soil Moisture
                <div id="soil-value" class="value">Loading...</div>
            </div>
            <div class="grid-item">
                Light Level
                <div id="light-value" class="value">Loading...</div>
            </div>
        </div>
    </body>
    </html>
    )rawliteral";

    server.send(200, "text/html", html);
}
void handleData() {
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["temperature"] = currentTemperature;
    jsonDoc["humidity"] = currentHumidity;
    jsonDoc["soilMoisture"] = currentSoilMoisture;
    jsonDoc["light"] = currentLight;

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    server.send(200, "application/json", jsonString);
}