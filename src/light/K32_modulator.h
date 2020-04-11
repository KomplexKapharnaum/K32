/*
  K32_modulator.h
  Created by Thomas BOHL, March 2020.
  Released under GPL v3.0
*/
#ifndef K32_modulator_h
#define K32_modulator_h

#define MOD_PARAMS_SLOTS 8

#include "K32_anim.h"

//
// BASE MODULATOR
//
class K32_modulator
{
public:
  K32_modulator() {
    this->paramInUse = xSemaphoreCreateBinary();
    xSemaphoreGive(this->paramInUse);
  }

  K32_modulator(int *p, int size) {
    this->paramInUse = xSemaphoreCreateBinary();
    xSemaphoreGive(this->paramInUse);

    xSemaphoreTake(this->paramInUse, portMAX_DELAY);
    size = min(size, MOD_PARAMS_SLOTS);
    for (int k = 0; k < size; k++) this->params[k] = p[k];
    xSemaphoreGive(this->paramInUse);
  }


  // get/set name
  String name() { return this->_name; }
  void name(String n) { this->_name = n; }

  K32_modulator* at(int slot)
  {
    if (slot < ANIM_DATA_SLOTS) this->dataslot[slot] = true;
    return this;
  }

  K32_modulator* play()
  {
    if (!this->isRunning) {
      this->trigger();
      if (this->dataslot >= 0) LOGF2("ANIM: %s modulate param %i \n", this->name().c_str(), this->dataslot);

      this->freezeTime = 0;
      this->isRunning = true;
    }
    return this;
  }

  K32_modulator* trigger() 
  {
    this->triggerTime = millis();
    this->_fresh = true;
    return this;
  }

  K32_modulator* pause()
  {
    this->freezeTime = millis();
    return this;
  }

  K32_modulator* stop()
  {
    this->isRunning = false;
    return this;
  }

  // Execute modulation function
  bool run(int *animData)
  { 
    bool didChange = false;

    if (this->isRunning)
    {
      // Get Modulator value
      xSemaphoreTake(this->paramInUse, portMAX_DELAY);
      uint8_t val = this->value();
      xSemaphoreGive(this->paramInUse);

      // Apply modulation to dataslots, value of 255 will not do anything
      if (val < 255) {
        for (int s=0; s<ANIM_DATA_SLOTS; s++)
          if (this->dataslot[s]) animData[s] = scale16by8(animData[s], val);
      }

      // Did animator produced a different result than last call ?
      didChange = (this->_lastProducedData != val);
      this->_lastProducedData = val;

    }
    return didChange;
  }

  // 8Bit Direct value : Defined in SubClass ! 
  virtual uint8_t value()    { return 255; }

  // change one Params
  K32_modulator *param(int k, int value)
  {
    if (k < MOD_PARAMS_SLOTS)
    {
      // xSemaphoreTake(this->paramInUse, portMAX_DELAY);
      this->params[k] = value;
      // xSemaphoreGive(this->paramInUse);
    }
    return this;
  }

  // set special params
  K32_modulator *mini(int m) {
    this->_mini = m;
    return this;
  }
  K32_modulator *maxi(int m) {
    this->_maxi = m;
    return this;
  }
  K32_modulator *period(int p) {
    this->_period = p;
    return this;
  }
  K32_modulator *phase(int p) {
    this->_phase = p;
    return this;
  }

  // TOOLS
  //

  // get special params
  int mini() { return this->_mini; }
  int maxi() { return this->_maxi; }
  int amplitude() { return this->_maxi - this->_mini; }
  int period() { return max(1, this->_period); }
  int phase() { return this->_phase; }

  virtual int time() { return (this->freezeTime > 0) ? this->freezeTime : millis(); }
  float progress() { return 1.0 * (time() % period()) / period(); }
  int periodCount() { return time() / period(); }

  bool fresh() {
    bool r = this->_fresh;
    this->_fresh = false;
    return r;
  }
  

protected:

  int *anim_data;
  int params[MOD_PARAMS_SLOTS];

  // common params
  int _period = 1000;
  int _phase = 0;
  int _mini = 0;
  int _maxi = 255;

  // time refs
  unsigned long freezeTime = 0;
  unsigned long triggerTime = 0;
  bool _fresh = false; 
  

private:

  SemaphoreHandle_t paramInUse;
  String _name = "?";
  bool isRunning = false;

  bool dataslot[ANIM_DATA_SLOTS];
  int _lastProducedData = 0;
};


//  PERIODIC MODULATORS (LFO)
//
class K32_modulator_periodic : public K32_modulator {
  public:

    // Time is shifted with phase (0->360°) * period()
    virtual int time() 
    {
      return K32_modulator::time() - ((this->_phase % 360) * this->_period) / 360;
    }

};


//  TRIGGER MODULATORS
//
class K32_modulator_trigger : public K32_modulator {
  public:

    // Time is referenced to triggerTime (play)
    // Time is shifted with phase as fixed delay
    virtual int time() 
    {
      return K32_modulator::time() - this->triggerTime - this->_phase;
    }

};




#endif