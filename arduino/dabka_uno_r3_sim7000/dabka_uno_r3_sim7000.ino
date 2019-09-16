#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>

#include "dabka_event.h"
#include "dabka_publisher.h"
#include "secrets.h"

//FONA settings
static const int FONA_PWRKEY = 6;
static const int FONA_RST = 7;
static const int FONA_TX = 10; // Microcontroller TX
static const int FONA_RX = 11; // Microcontroller RX

//Sensor/GPS sampling rate
static const int SAMPLING_RATE = 30;

//MQTT settings
static const char MQTT_SERVER[] = SECRET_MQTT_SERVER;
static const int  MQTT_PORT = SECRET_MQTT_PORT;

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

DabkaEvent lastEvent(&fona, false);
char imei[16] = {0};

void powerOn() {
  pinMode(FONA_RST, OUTPUT);
  digitalWrite(FONA_RST, HIGH);
  pinMode(FONA_PWRKEY, OUTPUT);

  //Power on
  digitalWrite(FONA_PWRKEY, LOW);
  delay(100);
  digitalWrite(FONA_PWRKEY, HIGH);
}

void serialSetup() {
  Serial.begin(9600);
  Serial.println(F("Dabka collar on UNO R3 SIM7000 starting up"));

  fonaSS.begin(115200); // Default SIM7000 shield baud rate
  Serial.println(F("Configuring to 9600 baud"));
  fonaSS.println(F("AT+IPR=9600")); // Set baud rate
  delay(100); // Short pause to let the command run
  fonaSS.begin(9600);
  if(!fona.begin(fonaSS)) {
    Serial.println(F("Couldn't find FONA"));
    while(true); // Don't proceed if it couldn't find the device
  }
}

void getFonaType() {
  //Get FONA type
  uint8_t type = fona.type();
  switch (type) {
    case SIM7000A:
      Serial.println(F("SIM7000A (American)")); break;
    case SIM7000C:
      Serial.println(F("SIM7000C (Chinese)")); break;
    case SIM7000E:
      Serial.println(F("SIM7000E (European)")); break;
    case SIM7000G:
      Serial.println(F("SIM7000G (Global)")); break;
    case SIM7500A:
      Serial.println(F("SIM7500A (American)")); break;
    case SIM7500E:
      Serial.println(F("SIM7500E (European)")); break;
    default:
      Serial.print(F("Unknown type: ")); Serial.println(type); break;
  }
}

void getIMEI() {
  //Get IMEI
  uint8_t imeiLen = fona.getIMEI(imei);
  if(imeiLen > 0) {
    Serial.print("IMEI: "); Serial.println(imei);
  } else {
    Serial.println(F("Could not get IMEI value"));
  }
}

void getRSSI() {
  //Get RSSI (signal strength)
  uint8_t rssi = fona.getRSSI();
  int8_t level;

  Serial.print(F("RSSI = ")); Serial.print(rssi); Serial.print(": ");
  if(rssi == 0) level = -115;
  if(rssi == 1) level = -111;
  if(rssi == 31) level = -52;
  if((rssi >= 2) && (rssi <= 30)) {
    level = map(rssi, 2, 30, -110, -54);
  }
  Serial.print(level); Serial.println(F(" dBm"));
}

bool getNetworkStatus() {
  // Get network/cellular status
  uint8_t status = fona.getNetworkStatus();
  Serial.print(F("Network status "));
  Serial.print(status);
  Serial.print(F(": "));
  switch(status) {
    case 0: Serial.println(F("Not registered")); break;
    case 1: Serial.println(F("Registered (home)")); break;
    case 2: Serial.println(F("Not registered (searching)")); break;
    case 3: Serial.println(F("Denied")); break;
    case 4: Serial.println(F("Unknown")); break;
    case 5: Serial.println(F("Registered roaming")); break;
    default: Serial.print(F("Unknown status: ")); Serial.println(status); break;
  }

  return status == 1 || status == 5;
}

void setup() {
  powerOn();
  serialSetup();

  getIMEI();

  //Initial connection to LTE tower
  fona.setFunctionality(1);
  fona.setNetworkSettings(F("hologram"));

  getFonaType();
  getRSSI();

  while(!getNetworkStatus()) {
    Serial.println(F("Network not ready, retrying"));
    delay(5000);
  }
}

void loop() {
  delay(SAMPLING_RATE * 1000UL);

  DabkaPublisher publisher(&fona, MQTT_SERVER, MQTT_PORT, imei);
  DabkaEvent currentEvent(&fona);

  if(currentEvent) {
    if((currentEvent != lastEvent) || !lastEvent.wasPublished()) {
      if(publisher) {
        publisher.publishEvent(currentEvent);
        Serial.println(F("Published GPS data"));
      } else {
        Serial.println(F("Could not connect to publish GPS data"));
      }
    } else {
      Serial.println(F("Similar GPS data as before, not publishing"));
    }

    lastEvent = currentEvent;
  } else {
    Serial.println(F("Could not fetch GPS data"));
  }
}
