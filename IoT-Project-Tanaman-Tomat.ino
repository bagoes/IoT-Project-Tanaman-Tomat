#include <WiFi.h>
#include <ThingsBoard.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include "RTClib.h"
#include "DHTesp.h"

RTC_DS1307 rtc;

Servo myservo;

const int DHT_PIN = 15;
DHTesp dhtSensor;

#define LDR 36
const float GAMMA = 0.7;
const float RL10 = 50;

#define relayLampu 14
#define relayKipas 12
#define relay2pompa 4
#define relay1pompa 2

#define WIFI_AP "Wokwi-GUEST"
#define WIFI_PASSWORD ""

#define TOKEN "wRKcanTpWWQswBRqzsYm"
char thingsboardServer[] = "thingsboard.cloud";

WiFiClient wificlient;

ThingsBoard tb(wificlient);

int status = WL_IDLE_STATUS;
unsigned long lastSend;
 
void setup()
{
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relayLampu, OUTPUT);
  pinMode(relayKipas, OUTPUT);
  pinMode(relay1pompa, OUTPUT);
  pinMode(relay2pompa, OUTPUT);
  myservo.attach(13); 
  delay(10);
  InitWiFi();
  lastSend = 0;

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
}

void loop() {
  if ( !tb.connected() ) {
    reconnect();
  }

  waktu();

  cahaya();

  suhuLembab();

}

void InitWiFi() {
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

void reconnect() {
  // Loop until we're reconnected
  while (!tb.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    if ( tb.connect(thingsboardServer, TOKEN) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED]" );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

void waktu() {
  DateTime now = rtc.now();
  Serial.print("Pukul : ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
  /*
  if(now.hour() == 7) { // nyala pagi 1 jam 
    Serial.print("Waktu siram : ");
    digitalWrite(relay1pompa, HIGH);
    Serial.println("Pompa nyala (LED hijau)");
  }
  else if(now.hour() == 17) { // nyala sore 1 jam 
    Serial.print("Waktu siram : ");
    digitalWrite(relay1pompa, HIGH);
    Serial.println("Pompa nyala (LED hijau)");
  }
  else {
    Serial.print("Waktu normal : ");
    digitalWrite(relay1pompa, LOW);
    Serial.println("Pompa mati (LED hijau)");
  }
  */
  if(now.hour() == 11 && now.minute() >= 5 && now.minute() <= 6) { // 2 menit nyala
    Serial.print("Waktu siram : ");
    digitalWrite(relay1pompa, HIGH);
    Serial.println("Pompa nyala (LED hijau)");
  } 
  else if(now.hour() == 11 && now.minute() == 8) { // 1 menit nyala
    Serial.print("Waktu siram : ");
    digitalWrite(relay1pompa, HIGH);
    Serial.println("Pompa nyala (LED hijau)");
  } 
  else if(now.hour() == 11 && now.minute() >= 10 && now.minute() <= 11) { // 2 menit nyala
    Serial.print("Waktu siram : ");
    digitalWrite(relay1pompa, HIGH);
    Serial.println("Pompa nyala (LED hijau)");
  }
  else {
    Serial.print("Waktu normal : ");
    digitalWrite(relay1pompa, LOW);
    Serial.println("Pompa mati (LED hijau)");
  }
  delay(1000);
}

void cahaya() {
  int analogValue = analogRead(LDR);
  float voltage = analogValue * 5/4095.0;
  float resistance = 2000 * voltage / (1 - voltage / 5);
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  Serial.print(lux);
  Serial.println(" Lux");
  int relayL = digitalRead(relayLampu);

  if(lux <= 50) {
    Serial.print("Cahaya kurang : ");
    digitalWrite(relayLampu, HIGH);
    Serial.println("Lampu nyala (LED ungu)");
  }
  else {
    Serial.print("Cahaya cukup : ");
    digitalWrite(relayLampu, LOW);
    Serial.println("Lampu mati (LED ungu)");
  }
  /*
  String payload = "{";
  payload += "\"Intensitas Cahaya\":";payload += lux;
  payload += "}";

  char attributes[1000];
  payload.toCharArray( attributes, 1000 );
  Serial.println( attributes ); */
  tb.sendTelemetryInt("Lux", lux);
  tb.sendTelemetryInt("Lampu", relayL);
}

void suhuLembab() {
  TempAndHumidity  data = dhtSensor.getTempAndHumidity();
  Serial.println("Temperature : " + String(data.temperature, 2) + "°C");
  int relayK = digitalRead(relayKipas);

  if(data.temperature > 25) {
    Serial.print("Suhu panas : ");
    digitalWrite(relayKipas, HIGH);
    Serial.println("Kipas nyala (LED merah)");
  }
  else {
    Serial.print("Suhu cukup : ");
    digitalWrite(relayKipas, LOW);
    Serial.println("Kipas mati (LED merah)");
  }

  Serial.println("Humidity : " + String(data.humidity, 1) + "%");
  int relay1 = digitalRead(relay1pompa);
  int relay2 = digitalRead(relay2pompa);
  Serial.println(relay1);
  if(relay1 == LOW && data.humidity < 25) { // waktu normal & kering
    Serial.print("Kelembaban kering : ");
    digitalWrite(relay2pompa, HIGH);
    Serial.println("Pompa nyala (LED hijau)");
    myservo.write(0);
    Serial.println("Atap terbuka");
  }
  else if(relay1 == LOW && data.humidity >= 25 && data.humidity <= 60) { // waktu normal & cukup
    Serial.print("Kelembaban cukup : ");
    digitalWrite(relay2pompa, LOW);
    Serial.println("Pompa mati (LED hijau)");
    myservo.write(0);
    Serial.println("Atap terbuka");
  }
  else if(relay1 == LOW && data.humidity > 60) { // waktu normal & basah
    Serial.print("Kelembaban basah : ");
    digitalWrite(relay2pompa, LOW);
    Serial.println("Pompa mati (LED hijau)");
    myservo.write(90);
    Serial.println("Atap tertutup");
  }
  else if(relay1 == HIGH && data.humidity < 25) { // waktu siram & kering
    Serial.print("Kelembaban kering : ");
    digitalWrite(relay2pompa, HIGH);
    Serial.println("Pompa nyala (LED hijau)");
    myservo.write(0);
    Serial.println("Atap terbuka");
  }
  else if(relay1 == HIGH && data.humidity >= 25 && data.humidity <= 60) { // waktu siram & cukup
    Serial.print("Kelembaban cukup : ");
    digitalWrite(relay2pompa, HIGH);
    Serial.println("Pompa nyala (LED hijau)");
    myservo.write(0);
    Serial.println("Atap terbuka");
  }
  else if(relay1 == HIGH && data.humidity > 60) { // waktu siram & basah
    Serial.print("Kelembaban basah : ");
    digitalWrite(relay2pompa, LOW);
    Serial.println("Pompa mati (LED hijau)");
    myservo.write(90);
    Serial.println("Atap tertutup");
  }
  else {
    Serial.print("Kelembaban cukup : ");
    digitalWrite(relay2pompa, LOW);
    Serial.println("Pompa mati (LED hijau)");
    myservo.write(0);
    Serial.println("Atap terbuka");
  }
  
  tb.sendTelemetryInt("Kipas", relayK);
  tb.sendTelemetryInt("Temperature", data.temperature);
  tb.sendTelemetryFloat("Humidity", data.humidity);
  tb.sendTelemetryFloat("Servo", myservo.read());
  tb.sendTelemetryInt("Siram", relay1);
  tb.sendTelemetryInt("Kering", relay2);
}