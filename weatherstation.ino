#include <WiFi.h>
#include <WiFiClient.h>

#include <WiFiServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

#include <MQ135.h>
#include <DHT.h>

#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <DNSServer.h>
#include <WiFiManager.h> 

#define TRIGGER_PIN 0


#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

/** Pin 21 et 22 **/
Adafruit_BMP280 bmp; // I2C

const int mq135Pin = 34;     // Pin sur lequel est branché de MQ135
const int rainPin = 32;

MQ135 gasSensor = MQ135(mq135Pin);  // Initialise l'objet MQ135 sur le Pin spécifié

//here we use 14 of ESP32 to read data
#define DHTPIN 14
//our sensor is DHT11 type
#define DHTTYPE DHT11
//create an instance of DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// lowest and highest sensor readings:
const int sensorMin = 0;     // sensor minimum
const int sensorMax = 1024;  // sensor maximum

// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

/* const char* ssid = "Bbox-1F56F7E5";
const char* password =  "A535F3267A6E31D56E56DF5A52A117";
*/
WebServer server(80);


/** Initialisation */
void setup()
{
  Serial.begin(115200);
  Serial.println("DHT11 sensor!");
  /** Init WiFi */
  // connectToNetwork();
  // setup_task();  
  pinMode(TRIGGER_PIN, INPUT);   
  /** Init Pollution */
  float rzero = gasSensor.getRZero();
  Serial.println(rzero);
  /** Init Humidité et Température */
  //call begin to start sensor
  dht.begin();

  /** Capteur BMP280 **/
  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  // setup_routing();
  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

}

void setup_routing() {
  Serial.println("setup_routing");     
  server.on("/temperature", getTemperature);     
  server.on("/co2", getCO2);     
  server.on("/humidity", getHumidity);     
  server.on("/altitude", getAltitude);
  server.on("/pressure", getPressure);  
  server.on("/env", getEnv);     
  //server.on("/led", HTTP_POST, handlePost);    
       
  // start server    
  server.begin();    
}

void getEnv(){
  
}

void getAltitude(){

  create_json("altitude", bmp.readAltitude(1022), "m");
  server.send(200, "application/json", buffer);
  
}

void getPressure()
{
  Serial.print(F("Pressure = "));
  Serial.print(bmp.readPressure()/100);
  Serial.println(" Pa");
  create_json("pressure", bmp.readPressure()/100, "hPa");
  server.send(200, "application/json", buffer);
}


/**
 * Récupérer température
 */
void getTemperature()
{
  float t = dht.readTemperature();  
 
  create_json("temperature", t, "C°");
  server.send(200, "application/json", buffer);
}

/**
 * Récupérer CO2
 */
void getCO2()
{
    float ppm = gasSensor.getPPM();
    Serial.print("A0: ");
    Serial.print(analogRead(mq135Pin));
    Serial.print(" ppm CO2: ");
    Serial.println(ppm);
    create_json("co2", ppm, "ppm");
    server.send(200, "application/json", buffer);
}

/**
 * Récupérer humidité
 */
void getHumidity()
{
  float h = dht.readHumidity();
  if (isnan(h)){
    h = 0;
  }
  create_json("humidity", h, "%");
  server.send(200, "application/json", buffer);
}

 
void create_json(char *tag, float value, char *unit) {  
  jsonDocument.clear();  
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);
}
 
void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit; 
}

/*void connectToNetwork() {
  //WiFi.begin(ssid, password);
 
  //while (WiFi.status() != WL_CONNECTED) {
  //  delay(1000);
  //  Serial.println("Establishing connection to WiFi..");
  //}
 
  //Serial.println("Connected to network");
  //Serial.print("Connected. IP: ");
  //Serial.println(WiFi.localIP());
 
}*/

 
void loop() {
   // is configuration portal requested?
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //reset settings - for testing
    //wifiManager.resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    //wifiManager.setTimeout(120);

    //it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
    //WiFi.mode(WIFI_STA);
    
    if (!wifiManager.startConfigPortal("OnDemandAP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    setup_routing(); 
      
    // server.handleClient();   

    Serial.println("Connecté");
  
   
  }

 
  if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();   
        //Serial.println(server);
        Serial.println("Connected to network");
        Serial.print("Connected. IP: ");
        Serial.println(WiFi.localIP());
        float ppm = gasSensor.getPPM();
        Serial.print("A0: ");
        Serial.print(analogRead(mq135Pin));
        Serial.print(" ppm CO2: ");
        Serial.println(ppm);
            

  //  delay(1000);
  //  Serial.println("Establishing connection to WiFi..");
  }
 
     

  // use the functions which are supplied by library.
  // float h = dht.readHumidity();
  // float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  // float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  // if (isnan(h) || isnan(t)) {
  //  Serial.println("Failed to read from DHT sensor!");
  //  return;
  // }

  
  
  // print the result to Terminal
  /*Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");*/
  /** Capteur pluie */
  /*int sensorReading = analogRead(rainPin);
  Serial.print(analogRead(rainPin));
  int range = map(sensorReading, sensorMin, sensorMax, 0, 3);
  Serial.println("Rain Sensor : ");
  Serial.println(sensorReading);
  Serial.println(range);
  switch (range) {
   case 0:    // Sensor getting wet
      Serial.println("Flood");
      break;
   case 1:    // Sensor getting wet
      Serial.println("Rain Warning");
      break;
   case 2:    // Sensor dry - To shut this up delete the " Serial.println("Not Raining"); " below.
      Serial.println("Not Raining");
      break;
  }*/
  
  // we delay a little bit for next read
  // delay(2000);
}
