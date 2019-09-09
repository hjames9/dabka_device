#ifndef __DABKA_PUBLISHER_H__
#define __DABKA_PUBLISHER_H__

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"

class Adafruit_FONA_LTE;
class DabkaEvent;

class DabkaPublisher
{
public:
  DabkaPublisher(Adafruit_FONA_LTE* fona, const char* mqttServer, int mqttPort, const char* imei);
  ~DabkaPublisher();

  void publishEvent(const DabkaEvent& event);
  inline operator bool();

private:
  Adafruit_FONA_LTE* fona;

  const char* imei;
  const char* mqttServer;
  int mqttPort;
  bool status;

  Adafruit_MQTT_FONA mqtt;
  Adafruit_MQTT_Publish feed;
};

inline DabkaPublisher::operator bool() {
  return status;
}

#endif
