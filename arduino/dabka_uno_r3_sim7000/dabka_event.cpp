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
  if(retry(retryFunc, 5000, 10)) {
    Serial.println(F("Retrieved GPS coordinates"));
    status = true;
/*
    //"19/09/08,13:39:18+00"
    if(fona->enableNTPTimeSync(true, F("pool.ntp.org"))) {
      Serial.println(F("Enabled network time syncing"));
    } else {
      Serial.println(F("Could not enable network time syncing"));
    }
*/
    char timeBuffer[23] = {0};
    if(fona->getTime(timeBuffer, sizeof(timeBuffer))) {
      Serial.print(F("Retrieved current timestamp: ")); Serial.println(timeBuffer);

      //Example: "19/09/07,23:21:12-16"
      tm currentTm;
      int currentTimeArr[7] = {0};
      int ret = sscanf(timeBuffer, "\"%d/%d/%d,%d:%d:%d-%d\"", &currentTimeArr[0], &currentTimeArr[1], &currentTimeArr[2], &currentTimeArr[3], &currentTimeArr[4], &currentTimeArr[5], &currentTimeArr[6]);
      switch(ret) {
      case 7:
        {
          currentTm.tm_year = currentTimeArr[0] + 100;
          currentTm.tm_mon = currentTimeArr[1];
          currentTm.tm_mday = currentTimeArr[2];
          currentTm.tm_hour = currentTimeArr[3];
          currentTm.tm_min = currentTimeArr[4];
          currentTm.tm_sec = currentTimeArr[5];
          currentTm.tm_isdst = 0;
          timestamp = mktime(&currentTm) + UNIX_OFFSET;
          Serial.print(F("Converted current timestamp: ")); Serial.println(timestamp);
        }
        break;
      default:
        Serial.print(F("Error converting timestamp: ")); Serial.println(ret);
        break;
      }
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
const char DabkaEvent::LOCATION_TMPL[] = "{\"id\": \"%s\", \"deviceTimestamp\": %ld000, \"location\": {\"longitude\": %s, \"latitude\": %s}}";
char DabkaEvent::locationBuffer[sizeof(LOCATION_TMPL) + 32 + 15 + 15 + 15] = {0};
