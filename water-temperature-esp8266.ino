/* 
 * Send temperature and power to opensensemap
 * connect
 * esp8266 and DS18B20 at GPIO 2
 *  
 * Sources
 * - https://webnist.de/ds18b20-temperatursensor-am-esp8266-mit-2-aa-batterien-verwenden/
 * - https://opensensemap.org/
 * 
 * copy&paste work: Louis Kniefs
 * April 2020
 */

 //senseBox ID
#define SENSEBOX_ID "***"

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
 
#define ONE_WIRE_BUS 2 // GPIO des ESP
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
 
ADC_MODE(ADC_VCC); // ADC an zur Auslesung der Spannung
 
EspClass ESPm;
 
// Konstantendefinition
const char* ssid = "***"; 
const char* pass = "***";
const char* serverIp = "ingress.opensensemap.org"; 
const int intervall = 1800000000; //Mikrosekunden für Sleep.Mode = 30 Minuten
int conn_time;
 
WiFiClient client;
 
void setup()
{
delay(1000);
 
Serial.begin(115200);
Serial.println("Starte Temperatur Messprogramm");
 
delay(1000);
 
sensors.begin(); /* Inizialisieren der Dallas Temperature library */
 
WiFi.begin(ssid, pass);
 
Serial.println("Verbindungsaufbau");
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  conn_time++;
  if (conn_time > 20) {
    break;
  }
}
if (WiFi.status() == WL_CONNECTED) {
  Serial.println("");
  Serial.println("Verbunden mit WiFi");
  Serial.println("meine IP Addresse: ");
  Serial.println(WiFi.localIP());
}
else {
  Serial.println("");
  Serial.println("keine WiFi Verbindung - kurze Pause");
  ESP.deepSleep(100000);
  Serial.println("Ende des kurzen Schlafmodus");
}
 
if (!client.connect(serverIp, 80)) {
  Serial.println("Verbindung zu Server fehlgeschlagen");
}
else {
  Serial.println("Verbindung zu Server vorhanden");
}
 
 
// Abfrage Temperatur
double temperatur;
 
sensors.requestTemperatures(); 
temperatur = sensors.getTempCByIndex(0);
//send to opensensemap
const String sensorid_temp = "***";
postFloatValue(temperatur, 1, sensorid_temp);
 
Serial.println();
Serial.print(temperatur);
Serial.print(" Grad Celsius");
 
 
// Batteriespannung messen
uint ADCValue = 0;
String batterie;
ADCValue = ESPm.getVcc() + 144; // hier Korrektur Wert eintragen
float ADCfloat = float(ADCValue);
//float batterie = String(ADCfloat, 2);
//batterie = String(ADCfloat / 1000, 2);
const String sensorid_bat = "***";
postFloatValue(ADCfloat, 1, sensorid_bat);

Serial.println();
Serial.print(batterie);
Serial.print(" V");
 
// ESP8266 schlafen schicken
Serial.println("ESP geht in Ruhemodus");
ESP.deepSleep(intervall); // Pause
}
 
void loop()
{
}

void postFloatValue (float measurement, int digits, String sensorId) {
  //Float zu String konvertieren
  char obs[10];
  dtostrf(measurement, 5, digits, obs);
  //Json erstellen
  String jsonValue = "{\" value\":";
  jsonValue += obs;
  jsonValue += "}";
  //Mit OSeM Server verbinden und POST Operation durchführen
  Serial.println("-------------------------------------");
  Serial.print("Connectingto OSeM Server...");
  if (client.connect(serverIp, 80)) {
    Serial.println("connected!");
    Serial.println("-------------------------------------");
    //HTTP Header aufbauen
    client.print("POST /boxes/"); client.print(SENSEBOX_ID); client.print("/"); client.print(sensorId); client.println(" HTTP/1.1");
    client.print("Host:");
    client.println(serverIp);
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: "); client.println(jsonValue.length());
    client.println();
    //Daten senden
    client.println(jsonValue);
  } else {
    Serial.println("failed!");
    Serial.println("-------------------------------------");
  }
  //Antwort von Server im seriellen Monitor anzeigen
  waitForServerResponse();
}

void waitForServerResponse () {
  //Ankommende Bytes ausgeben
  boolean repeat = true;
  do {
    if (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
    //Verbindung beenden
    if (!client.connected()) {
      Serial.println();
      Serial.println("--------------");
      Serial.println("Disconnecting.");
      Serial.println("--------------");
      client.stop();
      repeat = false;
    }
  } while (repeat);
}
