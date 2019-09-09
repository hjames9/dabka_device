#ifndef __DABKA_EVENT_H__
#define __DABKA_EVENT_H__

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

template<typename RetryFunc>
bool retry(RetryFunc retryFunc, long delayTime, int retryCount)
{
  bool result = false;
  int count = 0;

  do {
    result = retryFunc();
    delay(delayTime);
  }
  while(!result && ++count < retryCount);

  return result;
}

class Adafruit_FONA_LTE;

class DabkaEvent
{
public:
  DabkaEvent(Adafruit_FONA_LTE* fona, bool sample = true);
  ~DabkaEvent();

  inline const char* getJson();
  inline bool operator==(const DabkaEvent& rhs);
  inline bool operator!=(const DabkaEvent& rhs);
  inline operator const char*();
  inline operator bool();

private:

  void getData();
  inline float degreeToRadian(float angle);
  float HaversineDistance(const DabkaEvent& e1, const DabkaEvent& e2);

  Adafruit_FONA_LTE* fona;

  float longitude;
  float latitude;
  float speedKph;
  float heading;
  float altitude;
  long timestamp;
  bool status;

  static const float EARTH_RADIUM_KM;
  static const float THRESHOLD_DISTANCE_M;

  static const char DOG_NAME[];
  static const char LOCATION_TMPL[];
  static char locationBuffer[];
};

inline const char* DabkaEvent::getJson() {
  char longitudeStr[14] = {0};
  char latitudeStr[13] = {0};

  dtostrf(longitude, 0, 8, longitudeStr);
  dtostrf(latitude, 0, 8, latitudeStr);
  sprintf(locationBuffer, LOCATION_TMPL, DOG_NAME, timestamp, longitudeStr, latitudeStr);

  return locationBuffer;
}

inline bool DabkaEvent::operator==(const DabkaEvent& rhs) {
  return HaversineDistance(*this, rhs) <= THRESHOLD_DISTANCE_M;
}

inline bool DabkaEvent::operator!=(const DabkaEvent& rhs) {
  return !(*this == rhs);
}

inline DabkaEvent::operator const char*() {
  return getJson();
}

inline DabkaEvent::operator bool() {
  return status;
}

inline float DabkaEvent::degreeToRadian(float angle) {
  return M_PI * angle / 180.0;
}

#endif
