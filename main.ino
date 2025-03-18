#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

#define MQ135_PIN 34    // MQ-135 analog output to GPIO34
#define DHT_PIN 13       // DHT22 data pin to GPIO4
#define DHT_TYPE DHT22  // Define the sensor type (DHT22)
#define RL 10           // Load resistor value in kilo-ohms

// Sample sensor resistance values for gases (in kΩ)
#define AMMONIA_R0 6.0
#define CO2_R0 20.0
#define NO2_R0 60.0
#define BENZENE_R0 50.0

// Conversion factors
#define CO2_CONVERSION_FACTOR 3.5
#define AMMONIA_CONVERSION_FACTOR 2.0
#define NO2_CONVERSION_FACTOR 1.5
#define BENZENE_CONVERSION_FACTOR 1.8

// Wi-Fi credentials
const char* ssid = "";
const char* password = "";

AsyncWebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);

// Variables to store last valid sensor readings
float lastCo2Concentration = 0.0;
float lastAmmoniaConcentration = 0.0;
float lastNo2Concentration = 0.0;
float lastBenzeneConcentration = 0.0;
float lastTemperature = 0.0;
float lastHumidity = 0.0;

void setup() {
    Serial.begin(115200);
    pinMode(MQ135_PIN, INPUT);
        // Configure GPIO26 as Output for VCC
    pinMode(26, OUTPUT);
    digitalWrite(26, HIGH);
    dht.begin();

    Serial.println("Warming up MQ-135 sensor...");
    delay(10000);  // Allow MQ-135 sensor to stabilize (1 min)

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Define API endpoint
    server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
        String jsonResponse = readSensors();
        request->send(200, "application/json", jsonResponse);
    });

    server.begin();
    Serial.println("Server started");
}

void loop() {
    // Nothing needed, server handles requests asynchronously
}

// **Function to Read All Sensor Data and Return JSON Response**
String readSensors() {
    // **Read MQ-135 Sensor**
    int sensorValue = 0, numReadings = 10;
    for (int i = 0; i < numReadings; i++) {
        sensorValue += analogRead(MQ135_PIN);
        delay(50);
    }
    sensorValue /= numReadings;

    Serial.print("Raw MQ-135 ADC Value: ");
    Serial.println(sensorValue);

    if (sensorValue > 0) {
        float voltage = sensorValue * (3.3 / 4095.0);
        float Rs = ((3.3 * RL) - (RL * voltage)) / voltage;
        lastCo2Concentration = CO2_CONVERSION_FACTOR * (Rs / CO2_R0);
        lastAmmoniaConcentration = AMMONIA_CONVERSION_FACTOR * (Rs / AMMONIA_R0);
        lastNo2Concentration = NO2_CONVERSION_FACTOR * (Rs / NO2_R0);
        lastBenzeneConcentration = BENZENE_CONVERSION_FACTOR * (Rs / BENZENE_R0);
    } else {
        Serial.println("MQ-135 reading failed, using last valid values.");
    }

    // **Read DHT22 Sensor**
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (!isnan(temp) && !isnan(hum)) {
        lastTemperature = temp;
        lastHumidity = hum;
    } else {
        Serial.println("DHT22 reading failed, using last valid values.");
    }

    // **Create JSON Response**
    String jsonResponse = "{";
    jsonResponse += "\"CO2\": " + String(lastCo2Concentration) + ",";
    jsonResponse += "\"Ammonia\": " + String(lastAmmoniaConcentration) + ",";
    jsonResponse += "\"NO2\": " + String(lastNo2Concentration) + ",";
    jsonResponse += "\"Benzene\": " + String(lastBenzeneConcentration) + ",";
    jsonResponse += "\"Temperature\": " + String(lastTemperature) + ",";
    jsonResponse += "\"Humidity\": " + String(lastHumidity);
    jsonResponse += "}";

    Serial.println("Response: " + jsonResponse);
    return jsonResponse;
}
