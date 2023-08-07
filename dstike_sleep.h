
extern "C" {
  #include "gpio.h"
}
extern "C" {
  #include "user_interface.h"
}

void wakeupFromMotion(void) {
  wifi_fpm_close;
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();

  gpio_pin_wakeup_disable();
}

void inline dstike_sleep_and_autocontinuecode() {
  yield();
  delay(100);
  yield();
  
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); //light sleep mode
  gpio_pin_wakeup_enable(GPIO_ID_PIN(DSTIKE_BUTTON_1_PIN), GPIO_PIN_INTR_LOLEVEL); //set the interrupt to look for HIGH pulses on Pin 0 (the PIR).
  wifi_fpm_open();

  delay(100); //def 100
  wifi_fpm_set_wakeup_cb(wakeupFromMotion); //wakeup callback
  wifi_fpm_do_sleep(0xFFFFFFF);
  delay(100); // def 100
  yield();
}
