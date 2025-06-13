#include <Arduino.h>
#include "Adafruit_CCS811.h"

#include <Arduino_JSON.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// Network and Firebase credentials
#define WIFI_SSID "omar"
#define WIFI_PASSWORD "123456788."

#define Web_API_KEY "AIzaSyCo_em8ALsa0j2DSgTgfQHW5LL0gWYn1ZI"
#define DATABASE_URL "https://weather-station-60ff7-default-rtdb.firebaseio.com/"
#define USER_EMAIL "omar.mejri.16@gmail.com"
#define USER_PASSWORD "12345678"

#define RXD2 16  // RX2 sur ESP32 (UART2)
#define TXD2 17  // TX2

// User function
void processData(AsyncResult &aResult);
String is_gaz(bool gaz);
String is_flame(bool flame);
Adafruit_CCS811 ccs;
// Authentication
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASSWORD);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000;

// Variable to save USER UID
String uid;

// Variables to save database paths
String databasePath;
String tempPath;
String humPath;
String presPath;
String gazPath;
String flamePath;
String co2Path;
String btn1Path ;
String btn2Path;
String btn3Path;

// LED GPIOs
#define LED1_PIN 5
#define LED2_PIN 4
#define LED3_PIN 0
#define LED_esp_PIN 2

// Valeurs des boutons
int btn1 = 0;
int btn2 = 0;
int btn3 = 0;

float temperature;
float humidity;
float pressure;
bool gaz = false; // Variable to simulate gas detection
bool flame = false; // Variable to simulate gas detection
int co2 = 0;

void setup(){
  Serial.begin(115200);

  // LEDs
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED_esp_PIN, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)    {
    digitalWrite(LED_esp_PIN, HIGH);
    delay(150);
    digitalWrite(LED_esp_PIN, LOW);
    delay(150);
    Serial.print(".");
  }
  digitalWrite(LED_esp_PIN, LOW);
  Serial.println();

  ssl_client.setInsecure();
  #if defined(ESP32)
    // ssl_client.setConnectionTimeout(1000);
    ssl_client.setHandshakeTimeout(5);
  #elif defined(ESP8266)
    ssl_client.setTimeout(1000); // Set connection timeout
    ssl_client.setBufferSizes(4096, 1024); // Set buffer sizes
  #endif

  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
    if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
    while(1);
  }
    // Wait for the sensor to be ready
  while(!ccs.available());
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);  
}

void loop(){


    while (Serial2.available()) {
    String received = Serial2.readStringUntil('\n');
    Serial.println("inloop***********************");
    JSONVar data = JSON.parse(received);

    if (JSON.typeof(data) == "undefined") {
      Serial.println("Erreur de parsing JSON !");
      Serial.println(received);
    } else {
      // Utilise .as<>() pour convertir les types explicitement
      pressure    = double(data["pressure"]);
      temperature = double(data["temperature"]);
      humidity    = double(data["humidity"]);
      gaz         = int(data["gaz"]);
      flame       = int(data["flame"]);
      if(ccs.available()){
        if(!ccs.readData()){
          Serial.print("CO2: ");
          Serial.print(ccs.geteCO2());
          Serial.print("ppm, TVOC: ");
          Serial.println(ccs.getTVOC());
        }
        else{
          Serial.println("ERROR!");
        }
      }
      co2 = ccs.geteCO2();
      Serial2.printf("%d\n", co2);
      Serial.println("===== Données recues =====");
      Serial.print("Pression: "); Serial.print(pressure); Serial.println(" Pa");
      Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
      Serial.print("Humidite: "); Serial.print(humidity); Serial.println(" %");
      Serial.print("Gaz: "); Serial.println(gaz);
      Serial.print("Flamme: "); Serial.println(flame);
      Serial.print("Co2: "); Serial.println(co2);
      Serial.println("==========================");
      if(gaz == 1 || flame == 1)
      {
        digitalWrite(LED1_PIN, HIGH);
        digitalWrite(LED2_PIN, HIGH);
        // delay(500);
        // digitalWrite(LED1_PIN, LOW);
        // digitalWrite(LED2_PIN, LOW);
        // delay(500);
      }
      else
      {
        digitalWrite(LED1_PIN, btn1);
        digitalWrite(LED2_PIN, btn2);
      }
    }
  }

    // Maintain authentication and async tasks
  app.loop();
   // Check if authentication is ready
  if (app.ready()){
    digitalWrite(LED_esp_PIN, LOW);
  // Periodic data sending every 10 seconds
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval){
      digitalWrite(LED_esp_PIN, HIGH);
      // Update the last send time
      lastSendTime = currentTime;

      // Get User UID
      Firebase.printf("User UID: %s\n", app.getUid().c_str());
      uid = app.getUid().c_str();
      databasePath = "UsersData/" + uid;

      // Update database path for sensor readings
      tempPath = databasePath + "/temperature"; // --> UsersData/<user_uid>/temperature
      humPath = databasePath + "/humidity"; // --> UsersData/<user_uid>/humidity
      presPath = databasePath + "/pressure"; // --> UsersData/<user_uid>/pressure
      gazPath = databasePath + "/gaz"; // --> UsersData/<user_uid>/pressure
      flamePath = databasePath + "/flame"; // --> UsersData/<user_uid>/pressure
      co2Path = databasePath + "/co2"; // --> UsersData/<user_uid>/pressure
      btn1Path = databasePath + "/button1";
      btn2Path = databasePath + "/button2";
      btn3Path = databasePath + "/button3";


      Serial.println("Writing to: " + tempPath);

      Database.set<float>(aClient, tempPath, temperature, processData, "RTDB_Send_Temperature");
      Database.set<float>(aClient, humPath, humidity, processData, "RTDB_Send_Humidity");
      Database.set<float>(aClient, presPath, pressure, processData, "RTDB_Send_Pressure");
      Database.set<String>(aClient, gazPath, is_gaz(gaz), processData, "RTDB_Send_gaz");
      Database.set<String>(aClient, flamePath, is_flame(flame), processData, "RTDB_Send_flame");
      Database.set<int>(aClient, co2Path, co2, processData, "RTDB_Send_co2");
      // Lecture des boutons et commande LEDs
      btn1 = Database.get<int>(aClient, btn1Path);
      digitalWrite(LED1_PIN, btn1);
      Serial.printf("Button1 = %d\n", btn1);

      btn2 = Database.get<int>(aClient, btn2Path);
      digitalWrite(LED2_PIN, btn2);
      Serial.printf("Button2 = %d\n", btn2);

      btn3 = Database.get<int>(aClient, btn3Path);
      digitalWrite(LED3_PIN, btn3);
      Serial.printf("Button3 = %d\n", btn3);
    }
  }
}

void processData(AsyncResult &aResult){
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}
String is_gaz(bool gaz)
{
  if (gaz == false)
  {
    return "No gaz detected";
  }
  else
  {
    return " Gaz detected";
  }

}

String is_flame(bool flame)
{
  if (flame == false)
  {
    return "No flame detected";
  }
  else
  {
    return " flame detected";
  }

}