#ifndef _MGOS_ARDUINO_DALLAS_TEMP_H_
#define _MGOS_ARDUINO_DALLAS_TEMP_H_

#include "mgos_arduino_onewire.h"
#ifdef __cplusplus
#include "DallasTemperature.h"
#else
typedef struct DallasTemperatureTag DallasTemperature;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Initialize DallasTemperature driver.
// Return value: handle opaque pointer.
DallasTemperature *mgos_arduino_dt_create(OneWire *ow);

// Close DallasTemperature handle. Return value: none.
void mgos_arduino_dt_close(DallasTemperature *dt);

// Initialise 1-Wire bus.
void mgos_arduino_dt_begin(DallasTemperature *dt);

// Returns the number of devices found on the bus.
// Return always 0 if an operaiton failed.
int mgos_arduino_dt_get_device_count(DallasTemperature *dt);

// Finds an address at a given index on the bus.
// Return false if the device was not found or an operaiton failed.
// Returns true otherwise.
bool mgos_arduino_dt_get_address(DallasTemperature *dt, char *addr, int idx);

// Sends command for all devices on the bus to perform a temperature conversion.
// Returns false if a device is disconnected or if an operaiton failed.
// Returns true otherwise.
void mgos_arduino_dt_request_temperatures(DallasTemperature *dt);

// Returns temperature in degrees C * 100
// or DEVICE_DISCONNECTED_C if an operaiton failed.
int mgos_arduino_dt_get_tempc(DallasTemperature *dt, const char *addr);

#ifdef __cplusplus
}
#endif

#endif /* _MGOS_ARDUINO_DALLAS_TEMP_H_ */
