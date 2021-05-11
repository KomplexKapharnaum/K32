/*
  K32_dmx.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_dmx_h
#define K32_dmx_h

#include <LXESP32DMX.h>
#include "_libfast/pixel.h"
#include "esp_task_wdt.h"

enum DmxDirection { DMX_IN, DMX_OUT };

class K32_dmx {
  public:
    K32_dmx(const int DMX_PIN[3], DmxDirection dir) {
      
      // DIR pin
      if (DMX_PIN[0] > 0) {
        pinMode(DMX_PIN[0], OUTPUT);
        digitalWrite(DMX_PIN[0], (dir == DMX_OUT)?HIGH:LOW);
      }
      
      // DMX out
      if (dir == DMX_OUT) 
      {
        if (DMX_PIN[1] > 0) {
          pinMode(DMX_PIN[1], OUTPUT);
          ESP32DMX.startOutput(DMX_PIN[1]);
          outputOK = true;
          LOG("DMX: output STARTED");
        }
        else LOG("DMX: invalid OUTPUT pin, DMXout disabled !");
      }

      // DMX in  // TODO: implement dmx in !
      else 
      {
        if (DMX_PIN[2] > 0) {
          // pinMode(DMX_PIN[2], INPUT);
          // ESP32DMX.startInput(DMX_PIN[2]);
          // inputOK = true;   
        }
        else LOG("DMX: invalid INPUT pin, DMXin disabled !");
      }
      

    };

    // SET one value
    K32_dmx* set(int index, int value) 
    {
      if (outputOK) {
        xSemaphoreTake(ESP32DMX.lxDataLock, portMAX_DELAY);
        ESP32DMX.setSlot(index, value);
        xSemaphoreGive(ESP32DMX.lxDataLock);
      }
      return this;
    }

    // SET multiple values
    K32_dmx* setMultiple(int* values, int size, int offsetAdr = 1) 
    {
      if (outputOK) {
      // LOGF3("DMX: setMultiple %d %d %d\n",values[0], size, offsetAdr);
        xSemaphoreTake(ESP32DMX.lxDataLock, portMAX_DELAY);
        for (int i = 0; i < size; i++)
          ESP32DMX.setSlot(i+offsetAdr, values[i]);
        xSemaphoreGive(ESP32DMX.lxDataLock);
      }
      return this;
    }

    // SET multiple values (RGB array)
    K32_dmx* setMultiple(pixelColor_t* _buffer, int pixCount, int offsetAdr = 1) 
    {
      if (outputOK) {
      // LOGF3("DMX: setMultiple %d %d %d\n",values[0], size, offsetAdr);
        xSemaphoreTake(ESP32DMX.lxDataLock, portMAX_DELAY);
        for (int i = 0; i < pixCount; i++) {
          ESP32DMX.setSlot(offsetAdr+i*3,   _buffer[i].r);
          ESP32DMX.setSlot(offsetAdr+i*3+1, _buffer[i].g);
          ESP32DMX.setSlot(offsetAdr+i*3+2, _buffer[i].b);
        }
        // LOGF4("DMX set: %d - %d %d %d \n", offsetAdr, _buffer[0].r, _buffer[0].g, _buffer[0].b);
        xSemaphoreGive(ESP32DMX.lxDataLock);
      }
      return this;
    }


  private:

    bool outputOK = false;
    bool inputOK = false;
};

#endif
