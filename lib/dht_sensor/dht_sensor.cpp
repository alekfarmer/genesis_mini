#include "dht_sensor.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <math.h>

namespace {
// Catalog init for AX22-0011: DHT_Unified, type DHT11 (never DHT22/DHT21).
DHT_Unified* g_dht = nullptr;
}  // namespace

DhtSensor::DhtSensor(uint8_t pin) : pin_(pin) {}

void DhtSensor::begin() {
  static DHT_Unified dht(pin_, DHT11);
  g_dht = &dht;
  dht.begin();  // returns void; failures show up as NaN readings
}

DhtSensor::Reading DhtSensor::read() {
  Reading r{false, NAN, NAN};
  if (g_dht == nullptr) {
    return r;
  }
  sensors_event_t event;
  g_dht->temperature().getEvent(&event);
  r.temp_c = event.temperature;
  // The library caches the raw read for 2 s, so this second call reuses the
  // same measurement instead of polling the sensor again.
  g_dht->humidity().getEvent(&event);
  r.humidity_pct = event.relative_humidity;
  r.ok = !isnan(r.temp_c) && !isnan(r.humidity_pct);
  return r;
}
