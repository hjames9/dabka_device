#include "dabka_publisher.h"

#include "dabka_event.h"

#include "Adafruit_FONA.h"

DabkaPublisher::DabkaPublisher(Adafruit_FONA_LTE* fona, const char* mqttServer, int mqttPort, const char* imei) :
fona(fona), imei(imei), mqttServer(mqttServer), mqttPort(mqttPort), status(false) {
  //Enable network connectivity via LTE
  Serial.println(F("Starting GPRS receiver"));

  auto retryFunc = [&fona]() -> bool {
    Serial.println(F("Attempting to start GPRS receiver"));
    return fona->enableGPRS(true);
  };
  if(retry(retryFunc, 2000, 10)) {
    Serial.println(F("Enabled GPRS receiver"));
    status = true;
  } else {
    Serial.println(F("Failed enabling GPRS receiver"));
  }
}

DabkaPublisher::~DabkaPublisher() {
  fona->enableGPRS(false);
  status = false;
  Serial.println(F("Disabled GPRS receiver"));
}

void DabkaPublisher::publishEvent(const DabkaEvent& event) {
  const char* json = event.getJson();

  Serial.print(F("Publishing json: ")); Serial.println(json);

  auto tcpRetry = [this]() -> bool {
    Serial.println(F("Attempting TCP connection to MQTT server"));
    return fona->TCPconnect(mqttServer, mqttPort);
  };

  auto mqttConnRetry = [this]() -> bool {
    Serial.println(F("Attempting MQTT connection to MQTT server"));
    return fona->MQTTconnect("MQTT", imei, "", "");
  };

  auto mqttPubRetry = [this, &json]() -> bool {
    Serial.println(F("Attempting to publish json to MQTT server"));
    return fona->MQTTpublish("location", json);
  };

  do {
    if(!retry(tcpRetry, 2000, 5)) {
      Serial.println(F("Unsuccessful TCP connection to MQTT server"));
      break;
    }
    Serial.println(F("Successful TCP connection to MQTT server"));

    if(!retry(mqttConnRetry, 2000, 5)) {
      Serial.println(F("Unsuccessful MQTT connection to MQTT server"));
      break;
    }
    Serial.println(F("Successful MQTT connection to MQTT server"));

    if(!retry(mqttPubRetry, 2000, 5)) {
      Serial.println(F("Unsuccessful publish of json message to MQTT server"));
      break;
    }
    Serial.println(F("Successful publish of json message to MQTT server"));
  } while(false);
}
