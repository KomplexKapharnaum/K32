/*
  K32_settings.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_settings_h
#define K32_settings_h

#include "Arduino.h"
#include <EEPROM.h>

class K32_settings {
  public:
    K32_settings(const char* keys[16]) {
      this->lock = xSemaphoreCreateMutex();

      xSemaphoreTake(this->lock, portMAX_DELAY);
      for (byte k=0; k<16; k++) this->keys[k] = keys[k];
      EEPROM.begin(16);
      for (byte k=0; k<16; k++) this->values[k] = EEPROM.read(k);
      EEPROM.end();
      xSemaphoreGive(this->lock);
    };

    void set(const char* key, byte value) {
      xSemaphoreTake(this->lock, portMAX_DELAY);
      for (byte k=0; k<16; k++)
        if (this->keys[k] == key) {
          EEPROM.begin(16);
          EEPROM.write(k, value);
          EEPROM.end();
          this->values[k] = value;
          xSemaphoreGive(this->lock);
          return;
        }
       xSemaphoreGive(this->lock);
    }

    byte get(const char* key) {
      xSemaphoreTake(this->lock, portMAX_DELAY);
      for (byte k=0; k<16; k++)
        if (this->keys[k] == key) {
          byte value = this->values[k];
          xSemaphoreGive(this->lock);
          return value;
        }
      xSemaphoreGive(this->lock);
      return 0;
    }


  private:
    SemaphoreHandle_t lock;
    const char* keys[16];
    byte values[16];
};

#endif