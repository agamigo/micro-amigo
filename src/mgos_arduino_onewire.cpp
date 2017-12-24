#include "mgos_arduino_onewire.h"

OneWire *mgos_arduino_onewire_create(uint8_t pin) {
  // OneWire *ow = (OneWire*) malloc(sizeof(OneWire));
  // ow->OneWire(pin);
  return new OneWire(pin);
}
