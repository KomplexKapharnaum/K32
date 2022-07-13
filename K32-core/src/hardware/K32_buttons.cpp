/*
  K32_buttons.cpp
  Created by Thomas BOHL, march 2022.
  Released under GPL v3.0
*/

#include "K32_version.h"
#include "K32_buttons.h"

#include <ESPmDNS.h>


/*
 *   PUBLIC
 */

K32_buttons::K32_buttons(K32* k32) : K32_plugin("btns", k32) {

    this->lock = xSemaphoreCreateMutex();

    for(int k=0; k<BTNS_SLOTS; k++) this->watchPins[k] = 0;

    xTaskCreate( this->watch,          // function
                  "btns_watch",         // server name
                  1000,              // stack memory
                  (void*)this,        // args
                  0,                  // priority
                  NULL              // handler
                  );                // core 
}

void K32_buttons::add(int pin, String name) 
{   
    for(int k=0; k<=BTNS_SLOTS; k++) {
        if (k==BTNS_SLOTS) {LOG("BTNS: no more slots.."); return;}   // No more slots

        if (this->watchPins[k]==0) 
        {
            pinMode(pin, INPUT_PULLUP);
            this->watchValues[k] = HIGH;
            this->watchDirty[k] = 0;
            this->watchNames[k]= name;
            this->watchPins[k] = pin;
            break;
        }
    }
}

void K32_buttons::add(int pin) {
    this->add(pin, String(pin));
}


void K32_buttons::command(Orderz* order) {
  // TODO: receive commands ?
}


/*
 *   PRIVATE
 */

void K32_buttons::watch( void * parameter ) {
    K32_buttons* that = (K32_buttons*) parameter;
    TickType_t xFrequency = pdMS_TO_TICKS(10);

    while(true) 
    { 
        for(int k=0; k<BTNS_SLOTS; k++) 
        {
            if (that->watchPins[k]>0) {
                bool value = digitalRead(that->watchPins[k]);
                if (that->watchValues[k] != value) 
                {
                    if (that->watchDirty[k] > DEBOUNCE_COUNT) 
                    {
                            if (value == LOW) {
                                that->emit( "btn/"+that->watchNames[k] );
                                that->emit( "btn/"+that->watchNames[k]+"-on" );
                            }
                            else that->emit( "btn/"+that->watchNames[k]+"-off" );

                            that->watchValues[k] = value;
                            that->watchDirty[k] = 0;
                    }
                    else that->watchDirty[k] += 1;
                }
                else that->watchDirty[k] = 0;
            }
        }

      vTaskDelay( xFrequency );
    }

    vTaskDelete(NULL);
}

