#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// Sensor pins
#define MQ135_PIN 34   // MQ-135 analog output to GPIO34
#define DHTPIN 13      // DHT22 data pin (matching your working example)
#define DHTTYPE DHT22  // DHT22 (AM2302, AM2321)
#define RL 10          // Load resistor value in kilo-ohms

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
DHT dht(DHTPIN, DHTTYPE);  // Using the same pin and type as your working example

// Variables to store last valid sensor readings
float lastCo2Concentration = 0.0;
float lastAmmoniaConcentration = 0.0;
float lastNo2Concentration = 0.0;
float lastBenzeneConcentration = 0.0;
float lastTemperature = 0.0;
float lastHumidity = 0.0;

void setup() {
    Serial.begin(115200);
    
    // Configure GPIO26 as Output for VCC if needed
    pinMode(26, OUTPUT);
    digitalWrite(26, HIGH);
    
    // Initialize DHT sensor with the exact same approach as your working code
    Serial.println(F("DHT22 and MQ-135 sensor test"));
    delay(2000);  // Wait for sensor to initialize (from your working code)
    dht.begin();
    
    // Test DHT22 sensor to make sure it's working
    Serial.println("Testing DHT22 sensor...");
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor in setup!");
    } else {
        Serial.print("Initial readings - Humidity: ");
        Serial.print(h);
        Serial.print("% Temperature: ");
        Serial.print(t);
        Serial.println(" C");
        
        // Save first valid readings
        lastHumidity = h;
        lastTemperature = t;
    }
    
    Serial.println("Warming up MQ-135 sensor...");
    delay(5000);  // Reduced warm-up time to prevent potential timeouts
    
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    
    int wifiTimeout = 0;
    while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
        delay(1000);
        Serial.print(".");
        wifiTimeout++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n----------------------------------------");
        Serial.println("WiFi Connected Successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.println("----------------------------------------");
        
        // Define API endpoint
        server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
            String jsonResponse = readSensors();
            request->send(200, "application/json", jsonResponse);
        });
        
        // Define root endpoint to show sensor values in HTML
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            String html = generateHtml();
            request->send(200, "text/html", html);
        });
        
        server.begin();
        Serial.println("Web server started");
        Serial.println("Access the dashboard at http://" + WiFi.localIP().toString());
    } else {
        Serial.println("\nWiFi connection failed! Running in sensor-only mode.");
    }
}

void loop() {
    // *** Replicate your working DHT code approach ***
    delay(2000);  // Wait between measurements - critical timing from your working code
    
    // Read temperature and humidity with the exact approach that works
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float f = dht.readTemperature(true);
    
    // Update stored values if readings are valid
    if (!isnan(h) && !isnan(t)) {
        lastHumidity = h;
        lastTemperature = t;
        
        // Print values in the same way as your working code
        Serial.print(F("Humidity: "));
        Serial.print(h);
        Serial.print(F("% Temperature: "));
        Serial.print(t);
        Serial.print(F(" C / "));
        Serial.print(f);
        Serial.println(F(" F"));
    } else {
        Serial.println("DHT22 reading failed in loop");
    }
    
    // Read MQ135 sensor less frequently to avoid interference
    static unsigned long lastMQ135Reading = 0;
    if (millis() - lastMQ135Reading > 10000) {  // Every 10 seconds
        readMQ135Sensor();
        lastMQ135Reading = millis();
    }
    
    // Periodic IP reminder
    static unsigned long lastIPReminder = 0;
    if (WiFi.status() == WL_CONNECTED && millis() - lastIPReminder > 30000) {  // Every 30 seconds
        Serial.println("----------------------------------------");
        Serial.print("Server running at: http://");
        Serial.println(WiFi.localIP());
        Serial.println("----------------------------------------");
        lastIPReminder = millis();
    }
}

// Function specifically to read MQ135 sensor
void readMQ135Sensor() {
    // Read MQ-135 Sensor
    int sensorValue = 0, numReadings = 10;
    for (int i = 0; i < numReadings; i++) {
        sensorValue += analogRead(MQ135_PIN);
        delay(10);  // Shorter delay to avoid DHT timing issues
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
        
        Serial.print("CO2: ");
        Serial.print(lastCo2Concentration);
        Serial.println(" ppm");
    } else {
        Serial.println("MQ-135 reading failed");
    }
}

// Function to Read All Sensor Data and Return JSON Response
String readSensors() {
    // We don't read sensors here directly anymore
    // Instead we use the most recent valid readings
    // This ensures DHT readings are done with proper timing in the loop()
    
    // Create JSON Response
    String jsonResponse = "{";
    jsonResponse += "\"CO2\": " + String(lastCo2Concentration) + ",";
    jsonResponse += "\"Ammonia\": " + String(lastAmmoniaConcentration) + ",";
    jsonResponse += "\"NO2\": " + String(lastNo2Concentration) + ",";
    jsonResponse += "\"Benzene\": " + String(lastBenzeneConcentration) + ",";
    jsonResponse += "\"Temperature\": " + String(lastTemperature) + ",";
    jsonResponse += "\"Humidity\": " + String(lastHumidity);
    jsonResponse += "}";
    
    Serial.println("API Response: " + jsonResponse);
    return jsonResponse;
}

// Function to generate HTML page
String generateHtml() {
    String html = "<!DOCTYPE html><html>";
    html += "<head><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<meta http-equiv='refresh' content='10'>"; // Auto-refresh every 10 seconds
    html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/mini.css/3.0.1/mini-default.min.css'>";
    html += "<title>Environmental Sensor Dashboard</title></head>";
    html += "<body><div class='container'>";
    html += "<h1>Environmental Sensor Dashboard</h1>";
    html += "<p><strong>Device IP: </strong>" + WiFi.localIP().toString() + "</p>";
    html += "<div class='row'>";
    
    // Temperature and Humidity
    html += "<div class='col-sm-12 col-md-6'>";
    html += "<div class='card'><div class='section'>";
    html += "<h3>Temperature & Humidity</h3>";
    html += "<p>Temperature: <mark class='secondary'>" + String(lastTemperature) + " &deg;C</mark></p>";
    html += "<p>Humidity: <mark class='secondary'>" + String(lastHumidity) + " %</mark></p>";
    html += "</div></div></div>";
    
    // Gas readings
    html += "<div class='col-sm-12 col-md-6'>";
    html += "<div class='card'><div class='section'>";
    html += "<h3>Air Quality</h3>";
    html += "<p>CO2: <mark class='tertiary'>" + String(lastCo2Concentration) + " ppm</mark></p>";
    html += "<p>Ammonia: <mark class='tertiary'>" + String(lastAmmoniaConcentration) + " ppm</mark></p>";
    html += "<p>NO2: <mark class='tertiary'>" + String(lastNo2Concentration) + " ppm</mark></p>";
    html += "<p>Benzene: <mark class='tertiary'>" + String(lastBenzeneConcentration) + " ppm</mark></p>";
    html += "</div></div></div>";
    
    html += "</div>"; // End row
    
    // Add footer with refresh info and API link
    html += "<footer><p>Last updated: " + String(millis() / 1000) + " seconds since boot</p>";
    html += "<p>Raw data available at <a href='/sensor'>/sensor</a> endpoint</p></footer>";
    
    html += "</div></body></html>";
    return html;
}