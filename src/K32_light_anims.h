/*
  K32_light_anims.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_light_anims_h
#define K32_light_anims_h

#define LEDS_ANIMS_SLOTS  16
#define LEDS_PARAM_SLOTS  16

#include "K32_leds.h"

//
// NOTE: WHEN ADDING A NEW ANIM, REGISTER IT IN THE ANIMATOR DOWN THERE !
//

class K32_light_anim {
  public:
    K32_light_anim() {
      this->critical = xSemaphoreCreateMutex();
    }

    virtual String name () { return ""; };
    virtual bool loop ( K32_leds* leds ) { return false; };
    
    void init() { this->startTime = millis(); }
    void setParam(int k, int value) { if (k < LEDS_PARAM_SLOTS) this->params[k] = value; } 
    void lock() { xSemaphoreTake(this->critical, portMAX_DELAY); }
    void unlock() { xSemaphoreGive(this->critical); }
    
    int params[LEDS_PARAM_SLOTS];
    SemaphoreHandle_t critical;

  protected:
    unsigned long startTime = 0;
};


//
// TEST
//
class K32_light_anim_test : public K32_light_anim {
  public:
    K32_light_anim_test() {
      this->params[0] = 150;    // intensity
      this->params[1] = 2000;   // initial delay
      this->params[2] = 200;    // step duration
    }
    
    String name () { return "test"; }
    
    void init() {}

    bool loop ( K32_leds* leds ){
    
      int wait = this->params[2];

      delay(this->params[1]);
      LOG("LEDS: testing..");

      this->lock();
      leds->blackout();

      leds->setPixel(-1, 0, this->params[0], 0, 0);
      leds->setPixel(-1, 1, this->params[0], 0, 0);
      leds->setPixel(-1, 2, this->params[0], 0, 0);
      leds->show();
      this->unlock();

      vTaskDelay(pdMS_TO_TICKS(wait));
      this->lock();
      leds->setPixel(-1, 0, 0, this->params[0], 0);
      leds->setPixel(-1, 1, 0, this->params[0], 0);
      leds->setPixel(-1, 2, 0, this->params[0], 0);
      leds->show();
      this->unlock();

      vTaskDelay(pdMS_TO_TICKS(wait));

      this->lock();
      leds->setPixel(-1, 0, 0, 0, this->params[0]);
      leds->setPixel(-1, 1, 0, 0, this->params[0]);
      leds->setPixel(-1, 2, 0, 0, this->params[0]);
      leds->show();
      this->unlock();

      vTaskDelay(pdMS_TO_TICKS(wait));

      this->lock();
      leds->setPixel(-1, 0, 0, 0, 0, this->params[0]);
      leds->setPixel(-1, 1, 0, 0, 0, this->params[0]);
      leds->setPixel(-1, 2, 0, 0, 0, this->params[0]);
      leds->show();
      this->unlock();

      vTaskDelay(pdMS_TO_TICKS(wait));

      this->lock();
      leds->blackout();
      this->unlock();

      return false;    // DON'T LOOP !

    };
};

//
// SINUS
//
class K32_light_anim_sinus : public K32_light_anim {
  public:
    K32_light_anim_sinus() {
      this->params[0] = 2000; // period
      this->params[1] = 255;  // intensity max
      this->params[2] = 0;    // intensity min
      this->params[3] = 255;  // red
      this->params[4] = 255;  // green
      this->params[5] = 255;  // blue
      this->params[6] = 255;  // white
      this->params[7] = 0;    // duration (seconds) 0 = infinite
    }

    String name () { return "sinus"; }
    
    bool loop ( K32_leds* leds ){

      float factor = 0;
      unsigned long start = millis();
      unsigned long progress = 0;

      // LOG("LEDS sinus");

      while (progress <= this->params[0]) {

        factor = ((0.5f + 0.5f * sin( 2 * PI * progress / this->params[0] - 0.5f * PI ) ) * (this->params[1]-this->params[2]) + this->params[2]) / 255;

        this->lock();
        leds->setAll( (int)(this->params[3]*factor),  (int)(this->params[4]*factor),  (int)(this->params[5]*factor),  (int)(this->params[6]*factor));
        leds->show();
        this->unlock();

        vTaskDelay(pdMS_TO_TICKS(1));
        progress = millis() - start;
      }

      // ANIMATION Timeout
      if (this->params[7] > 0)
        if (millis() > (this->startTime+this->params[7]*1000)) {
          this->lock();
          leds->blackout();
          this->unlock();
          return false;
        }

      return true;    // LOOP !

    };
};


//
// STROBE
//
class K32_light_anim_strobe : public K32_light_anim {
  public:
    K32_light_anim_strobe() {
      this->params[0] = 2000; // period
      this->params[1] = 50;   // % ON
    }

    String name () { return "strobe"; }
    
    bool loop ( K32_leds* leds ){
        
      int intensity = 255;
      TickType_t xOn = max(1, (int)pdMS_TO_TICKS( (this->params[0]*this->params[1]/100) ));
      TickType_t xOff = max(1, (int)pdMS_TO_TICKS( this->params[0] - (this->params[0]*this->params[1]/100) ));
      
      this->lock();
      leds->setAll(intensity, intensity, intensity, intensity)->show();
      this->unlock();

      vTaskDelay( xOn );

      this->lock();
      leds->blackout();
      this->unlock();

      vTaskDelay( xOff );

      return true;    // LOOP !

    };
};


//
// HARDSTROBE
//
class K32_light_anim_hardstrobe : public K32_light_anim {
  public:
    K32_light_anim_hardstrobe() {
      this->params[0] = 2000; // period
      this->params[1] = 50;   // % ON

      this->nextEvent = 0;
      state = false;
    }

    String name () { return "hardstrobe"; }
    
    bool loop ( K32_leds* leds ){
      
      int intensity = 255;
      // LOGINL(millis()); LOGINL(" // "); LOG(this->nextEvent);  
      if (millis() > this->nextEvent) {
        // OFF -> ON
        if(!state) {
          this->nextEvent = millis() + (this->params[0]*this->params[1]/100) ;

          this->lock();
          leds->setAll(intensity, intensity, intensity, intensity)->show();
          this->unlock();
        }
        // ON -> OFF
        else {
          this->nextEvent = millis() + this->params[0] - (this->params[0]*this->params[1]/100);
          
          this->lock();
          leds->blackout();
          this->unlock();
        }
        state = !state;
      }

      return true;    // LOOP !

    };
    
    long nextEvent;
    bool state;
};


//
// ANIMATOR BOOK
//
class K32_light_animbook {
  public:
    K32_light_animbook() {

      //
      // REGISTER AVAILABLE ANIMS !
      //
      this->add( new K32_light_anim_test() );
      this->add( new K32_light_anim_sinus() );
      this->add( new K32_light_anim_strobe() );
      this->add( new K32_light_anim_hardstrobe() );

    }

    K32_light_anim* get( String name ) {
      for (int k=0; k<this->counter; k++)
        if (this->anims[k]->name() == name) {
          // LOGINL("LEDS: "); LOG(name);
          return this->anims[k];
        }
      LOGINL("ANIM: not found "); LOG(name);
      return new K32_light_anim();
    }


  private:
    K32_light_anim* anims[LEDS_ANIMS_SLOTS];
    int counter = 0;

    void add(K32_light_anim* anim) {
      if (this->counter >= LEDS_ANIMS_SLOTS) {
        LOG("ERROR: no more slot available to register new animation");
        return;
      }
      this->anims[ this->counter ] = anim;
      this->counter++;
    };
    
};


#endif