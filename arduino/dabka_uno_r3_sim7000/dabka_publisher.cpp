#include "dabka_publisher.h"

#include "dabka_event.h"

#include "Adafruit_FONA.h"

DabkaPublisher::DabkaPublisher(Adafruit_FONA_LTE* fona, const char* mqttServer, int mqttPort, const char* imei) :
fona(fona), imei(imei), mqttServer(mqttServer), mqttPort(mqttPort), status(false), mqtt(fona, mqttServer, mqttPort), feed(&mqtt, "collar/events") {
  //Enable network connectivity via LTE
  Serial.println(F("Starting GPRS receiver"));

  auto retryFunc = [&fona]() -> bool {
    Serial.println(F("Attempting to start GPRS receiver"));
    return fona->enableGPRS(true);
  };
  if(retry(retryFunc, 5000, 10)) {
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

  int8_t ret;
  auto mqttConnRetry = [this, &ret]() -> bool {
    Serial.println(F("Attempting MQTT connection"));
    return mqtt.connected() || ((ret = mqtt.connect()) == 0);
  };

  auto mqttPubRetry = [this, &json]() -> bool {
    Serial.println(F("Attempting to publish json to MQTT server"));
    return feed.publish(json);
  };

  int totalRetryCount = 0;
  do {
    if(!retry(mqttConnRetry, 5000, 5)) {
      Serial.print(F("Unsuccessful MQTT connection to server: ")); Serial.println(mqtt.connectErrorString(ret));
      break;
    }
    Serial.println(F("Successful MQTT connection to server"));

    if(!retry(mqttPubRetry, 3000, 5)) {
      Serial.println(F("Unsuccessful publish of json message to MQTT server"));
      continue;
    }
    Serial.println(F("Successful publish of json message to MQTT server"));
    event.published = true;
    break;
  } while(++totalRetryCount < 5);
}
