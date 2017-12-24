#include "mgos_arduino_dallas_temp.h"

DallasTemperature *mgos_arduino_dt_create(OneWire *ow) {
  if (ow == nullptr) return nullptr;
  DallasTemperature *dt = (DallasTemperature*) malloc(sizeof(DallasTemperature));
  dt->setOneWire(ow);
  return dt;
}

void mgos_arduino_dt_close(DallasTemperature *dt) {
  if (dt != nullptr) {
    delete dt;
    dt = nullptr;
  }
}

void mgos_arduino_dt_begin(DallasTemperature *dt) {
  if (dt == nullptr) return;
  dt->begin();
}

int mgos_arduino_dt_get_device_count(DallasTemperature *dt) {
  if (dt == nullptr) return 0;
  return dt->getDeviceCount();
}

bool mgos_arduino_dt_get_address(DallasTemperature *dt, char *addr, int idx) {
  if (dt == nullptr) return false;
  return dt->getAddress((uint8_t *)addr, idx);
}

void mgos_arduino_dt_request_temperatures(DallasTemperature *dt) {
  if (dt == nullptr) return;
  dt->requestTemperatures();
}

int mgos_arduino_dt_get_tempc(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return DEVICE_DISCONNECTED_C;
  return round(dt->getTempC((uint8_t *)addr) * 100.0);
}
