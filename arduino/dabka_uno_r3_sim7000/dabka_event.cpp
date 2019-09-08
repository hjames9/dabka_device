#include "dabka_event.h"

#include <time.h>

#include "Adafruit_FONA.h"

DabkaEvent::DabkaEvent(Adafruit_FONA_LTE* fona, bool sample) :
  fona(fona), longitude(0.0), latitude(0.0), speedKph(0.0), heading(0.0), altitude(0.0), timestamp(0), status(false) {
  if(sample) {
    //Start up GPS receiver
    Serial.println(F("Starting GPS receiver"));

    auto retryFunc = [&fona]() -> bool {
      Serial.println(F("Attempting to start GPS receiver"));
      return fona->enableGPS(true);
    };
    if(retry(retryFunc, 2000, 10)) {
      Serial.println(F("Successfully enabled GPS receiver"));
      getData();
    } else {
      Serial.println(F("Failed enabling GPS receiver"));
    }
  }
}

DabkaEvent::~DabkaEvent() {
  fona->enableGPS(false);
  status = false;
  Serial.println(F("Disabled GPS receiver"));
}

void DabkaEvent::getData() {
  Serial.println(F("Retrieving GPS coordinates"));

  auto retryFunc = [this]() -> bool {
    Serial.println(F("Attempting to retrieve GPS coordinates"));
    return fona->getGPS(&latitude, &longitude, &speedKph, &heading, &altitude);
  };
  if(retry(retryFunc, 3000, 10)) {
    Serial.println(F("Retrieved GPS coordinates"));
    status = true;

    char timeBuffer[23] = {0};
    if(fona->getTime(timeBuffer, sizeof(timeBuffer))) {
      Serial.print(F("Retrieved current timestamp: ")); Serial.println(timeBuffer);

      //Example: "19/09/07,23:21:12-16"
      tm tm;
      int timezone;
      sscanf(timeBuffer, "%d/%d/%d,%d:%d:%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &timezone);
      tm.tm_year += 100;
      timestamp = mktime(&tm) * 1000; //Convert to milliseconds
      Serial.print(F("Converted current timestamp: ")); Serial.println(timestamp);
    } else {
      Serial.println(F("Could not retrieve current timestamp"));
    }
  } else {
    Serial.println(F("Could not retrieve GPS coordinates"));
  }
}

float DabkaEvent::HaversineDistance(const DabkaEvent& e1, const DabkaEvent& e2) {
  float latRad1 = degreeToRadian(e1.latitude);
  float latRad2 = degreeToRadian(e2.latitude);
  float lonRad1 = degreeToRadian(e1.longitude);
  float lonRad2 = degreeToRadian(e2.longitude);

  float diffLa = latRad2 - latRad1;
  float doffLo = lonRad2 - lonRad1;

  float computation = asin(sqrt(sin(diffLa / 2) * sin(diffLa / 2) + cos(latRad1) * cos(latRad2) * sin(doffLo / 2) * sin(doffLo / 2)));
  return 2 * EARTH_RADIUM_KM * computation;
}

const float DabkaEvent::EARTH_RADIUM_KM = 6372.8;
const float DabkaEvent::THRESHOLD_DISTANCE_M = 3;
const char DabkaEvent::DOG_NAME[] = "algo";
const char DabkaEvent::LOCATION_TMPL[] = "{\"id\": \"%s\", \"deviceTimestamp\": %d, \"location\": {\"longitude\": %s, \"latitude\": %s}}";
char DabkaEvent::locationBuffer[sizeof(LOCATION_TMPL) + 32 + 15 + 15 + 15] = {0};
