#ifndef _MGOS_ARDUINO_ONEWIRE_H_
#define _MGOS_ARDUINO_ONEWIRE_H_

#ifdef __cplusplus
#include "OneWire.h"
#else
typedef struct OneWireTag OneWire;
#endif


#ifdef __cplusplus
extern "C" {
#endif

void delay(uint32_t time_millis);

OneWire *mgos_arduino_onewire_create(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* _MGOS_ARDUINO_ONEWIRE_H_ */
