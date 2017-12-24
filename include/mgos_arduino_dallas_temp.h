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

// Get global resolution.
int mgos_arduino_dt_get_global_resolution(DallasTemperature *dt);

// Set global resolution to 9, 10, 11, or 12 bits.
void mgos_arduino_dt_set_global_resolution(DallasTemperature *dt, int res);

// Returns the device resolution: 9, 10, 11, or 12 bits.
// Returns 0 if device not found or if an operaiton failed.
int mgos_arduino_dt_get_resolution(DallasTemperature *dt, const char *addr);

// Set resolution of a device to 9, 10, 11, or 12 bits.
// If new resolution is out of range, 9 bits is used.
// Return true if a new value was stored.
// Returns false otherwise.
bool mgos_arduino_dt_set_resolution(DallasTemperature *dt, const char *addr, int res, bool skip_global_calc);

#ifdef __cplusplus
}
#endif

#endif /* _MGOS_ARDUINO_DALLAS_TEMP_H_ */
