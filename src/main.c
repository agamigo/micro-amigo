#include <stdlib.h>
#include "mgos.h"
#include "mgos_rpc.h"
#include "mgos_prometheus_metrics.h"
#include "mgos_arduino_onewire.h"
#include "mgos_arduino_dallas_temp.h"

#define DEVICE_DISCONNECTED_C -127
#define DEVICE_RESET_C 85
#define ONEWIRE_PIN_COUNT_MAX 16
#define DT_SENSORS_PER_BUS_MAX 8

typedef struct OWConfigs {
  int pin_count;
  int *pins;
  int temp_resolution;
} OWConfig;

typedef struct TempSensors {
  char address[8];
  char address_pretty[18];
  float temp;
  uint32_t read_count;
  int error;
} TempSensor;

typedef struct TempBuses {
  OneWire *ow_bus;
  DallasTemperature *dt_bus;
  bool is_active;
  int ow_pin;
  int sensors_active;
  TempSensor sensors[DT_SENSORS_PER_BUS_MAX];
} TempBus;

OWConfig ow_config;
TempBus *dt_buses;

void config_onewire() {
  int pin, idx = 0;
  ow_config.temp_resolution = mgos_sys_config_get_temperature_resolution();
  ow_config.pin_count = mgos_sys_config_get_onewire_pin_count();
  const char *str_orig = mgos_sys_config_get_onewire_pins();
  char str[100];
  memset(str, '\0', sizeof(str));
  strcpy(str, str_orig);

  char *t;
  t = strtok(str,",");
  while (t != NULL) {
    if (idx >= ow_config.pin_count) {
      LOG(LL_ERROR, ("onewire.pins: More pins than onewire.pin_count\n"));
      return;
    }
    pin = atoi(t);
    if (pin == 0) {
      LOG(LL_ERROR, ("onewire.pins: Invalid OneWire pin config found\n"));
      return;
    }
    ow_config.pins[idx++] = pin;
    t = strtok(NULL,",");
  }
}

void config_dt_buses() {
  // Loop through OneWire pins, configure DallasTemperature driver
  for (int i = 0; i < ow_config.pin_count; i++) {
    if (dt_buses[i].is_active && dt_buses[i].ow_pin == ow_config.pins[i]) continue;
    dt_buses[i].ow_pin = ow_config.pins[i];
    dt_buses[i].ow_bus = mgos_arduino_onewire_create(ow_config.pins[i]);
    dt_buses[i].dt_bus = mgos_arduino_dt_create(dt_buses[i].ow_bus);
    mgos_arduino_dt_begin(dt_buses[i].dt_bus);
    if (ow_config.temp_resolution != mgos_arduino_dt_get_global_resolution(dt_buses[i].dt_bus)) {
      mgos_arduino_dt_set_global_resolution(dt_buses[i].dt_bus, ow_config.temp_resolution);
    }
    dt_buses[i].is_active = true;
  }

  // Make sure remaining buffer of DallasTemperature buses are not active.
  // This cleans things up if pin_count is lowered.
  for (int i = ow_config.pin_count; i < ONEWIRE_PIN_COUNT_MAX; i++) {
    if (dt_buses[i].is_active) {
      mgos_arduino_dt_close(dt_buses[i].dt_bus);
      dt_buses[i].is_active = false;
    }
  }
}

void detect_dt_sensors() {
  static char *hex = "0123456789ABCDEF";

  // Loop through OneWire pins, detect DallasTemperature sensors
  for (int i = 0; i < ow_config.pin_count; i++) {
    dt_buses[i].sensors_active = mgos_arduino_dt_get_device_count(dt_buses[i].dt_bus);

    LOG(LL_INFO, ("Num of sensors found on pin %i: %i\n",
          ow_config.pins[i], dt_buses[i].sensors_active));

    if (dt_buses[i].sensors_active <= 0) continue;

    LOG(LL_INFO, ("Global resolution on pin %i: %i\n",
          dt_buses[i].ow_pin,
          mgos_arduino_dt_get_global_resolution(dt_buses[i].dt_bus)));

    for (int j = 0; j < dt_buses[i].sensors_active; j++) {
      if (mgos_arduino_dt_get_address(dt_buses[i].dt_bus,
            dt_buses[i].sensors[j].address, j)) {
        dt_buses[i].sensors[j].read_count = 0;
        dt_buses[i].sensors[j].error = false;

        LOG(LL_INFO, ("Sens#%d address: ", j + 1));
        int l = 0;
        for (int k = 0; k < 8; k++) {
          dt_buses[i].sensors[j].address_pretty[l++] =
            hex[dt_buses[i].sensors[j].address[k] / 16];
          dt_buses[i].sensors[j].address_pretty[l++] =
            hex[dt_buses[i].sensors[j].address[k] & 15];
        }
        dt_buses[i].sensors[j].address_pretty[l] = '\0';
        LOG(LL_INFO, ("%s", dt_buses[i].sensors[j].address_pretty));
        LOG(LL_INFO, ("\n"));
      }
    }
  }
}

void get_temps() {
  float temp=0.0;
  for (int i = 0; i < ow_config.pin_count; i++) {
    if (dt_buses[i].sensors_active <= 0) continue;

    mgos_arduino_dt_request_temperatures(dt_buses[i].dt_bus);

    LOG(LL_INFO, ("Retrieving temperatures from DT bus on pin %i\n", dt_buses[i].ow_pin));

    for (int j = 0; j < dt_buses[i].sensors_active; j++) {
      temp = mgos_arduino_dt_get_tempc(dt_buses[i].dt_bus, dt_buses[i].sensors[j].address) / 100.0;
      dt_buses[i].sensors[j].read_count++;

      if (temp == DEVICE_DISCONNECTED_C || temp == DEVICE_RESET_C) {
        LOG(LL_ERROR, ("Failed to read sensor"));
        dt_buses[i].sensors[j].error = temp;
      }
      else {
        LOG(LL_INFO, ("Sens#%d temperature: %f *C\n", i + 1, temp));
        dt_buses[i].sensors[j].error = false;
        dt_buses[i].sensors[j].temp = temp;
      }
    }
  }
}

static void metrics_temperature_fn(struct mg_connection *nc, void *user_data) {
  get_temps();

  for (int i = 0; i < ow_config.pin_count; i++) {
    for (int j = 0; j < dt_buses[i].sensors_active; j++) {
      if (! dt_buses[i].sensors[j].error) {
        mgos_prometheus_metrics_printf(nc, GAUGE, "temperature_celsius",
          "Temperature In Degrees Celsius",
          "{sensor_id=\"ds18b20_%s\",gpio_pin=\"%i\"} %.2f",
          dt_buses[i].sensors[j].address_pretty, dt_buses[i].ow_pin,
          dt_buses[i].sensors[j].temp);
        mgos_prometheus_metrics_printf(nc, GAUGE, "sensor_statuses",
          "Status Of Deployed Sensors",
          "{sensor_id=\"ds18b20_%s\",gpio_pin=\"%i\",sensor_type=\"temperature\",status=\"healthy\"} 1",
          dt_buses[i].sensors[j].address_pretty, dt_buses[i].ow_pin);
      }
      else {
        if (dt_buses[i].sensors[j].error == DEVICE_DISCONNECTED_C) {
          mgos_prometheus_metrics_printf(nc, GAUGE, "sensor_statuses", "Status Of Deployed Sensors",
            "{sensor_id=\"ds18b20_%s\",gpio_pin=\"%i\",sensor_type=\"temperature\",status=\"error_disconnected\"} 0",
            dt_buses[i].sensors[j].address_pretty, dt_buses[i].ow_pin);
        }
        else {
          if (dt_buses[i].sensors[j].error == DEVICE_RESET_C) {
            mgos_prometheus_metrics_printf(nc, GAUGE, "sensor_statuses", "Status Of Deployed Sensors",
              "{sensor_id=\"ds18b20_%s\",gpio_pin=\"%i\",sensor_type=\"temperature\",status=\"error_reset_register\"} 0",
              dt_buses[i].sensors[j].address_pretty, dt_buses[i].ow_pin);
          }
        }
      }
      mgos_prometheus_metrics_printf(nc, COUNTER, "sensor_requests_total",
        "Requests For Data From Sensors",
        "{sensor_id=\"ds18b20_%s\",gpio_pin=\"%i\",sensor_type=\"temperature\"} %i",
        dt_buses[i].sensors[j].address_pretty, dt_buses[i].ow_pin,
        dt_buses[i].sensors[j].read_count);
    }
  }
  (void) user_data;
}

void heartbeat_timer_cb(void *arg) {
  LOG(LL_INFO, ("%lf\n", mgos_uptime()));
  (void) arg;
}

void config_sync_timer_cb(void *arg) {
  config_onewire();
  LOG(LL_INFO, ("Config: Pin count is %i\n", ow_config.pin_count));
  for (int i = 0; i < ow_config.pin_count; i++) {
    LOG(LL_INFO, ("Config: Pin%i is %i\n", i, ow_config.pins[i]));
  }
  (void) arg;
}

void detect_sensors_timer_cb(void *arg) {
  detect_dt_sensors();
  (void) arg;
}

void temp_request_timer_cb(void *arg) {
  get_temps();
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
  ow_config.pins = (int*) malloc(sizeof(int) * ONEWIRE_PIN_COUNT_MAX);
  dt_buses = (TempBus*) malloc(sizeof(TempBus) * ONEWIRE_PIN_COUNT_MAX);
  for (int i = 0; i < ONEWIRE_PIN_COUNT_MAX; i++) {
    dt_buses[i].is_active = false;
  }
  config_onewire();
  config_dt_buses();
  detect_dt_sensors();
  /* get_temps(); */

  mgos_set_timer(10000, true, heartbeat_timer_cb, NULL);
  /* mgos_set_timer(300000, true, config_sync_timer_cb, NULL); */
  /* mgos_set_timer(10000, true, detect_sensors_timer_cb, NULL); */
  /* mgos_set_timer(10000, true, temp_request_timer_cb, NULL); */

  mgos_prometheus_metrics_add_handler(metrics_temperature_fn, NULL);

  return MGOS_APP_INIT_SUCCESS;
}
