/*
  K32_mqtt.cpp
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/

#include "Arduino.h"
#include "K32_mqtt.h"

WiFiClient espClient;

/*
 *   PUBLIC
 */

K32_mqtt::K32_mqtt(mqttconf conf, K32* engine) : conf(conf), engine(engine)
{ 
  this->lock = xSemaphoreCreateMutex();

  this->client = new WiFiClient();
  this->mqttc = new PubSubClient(espClient);
  this->mqttc->setServer(this->conf.broker, 1883);
  this->mqttc->setCallback( [this](char* topic, byte* payload, unsigned int length){
    this->dispatch(topic, payload, length);
  } );


  // LOOP client
  xTaskCreate( this->loop,          // function
                "mqtt_client",      // name
                10000,               // stack memory
                (void*)this,        // args
                5,                  // priority
                NULL);              // handler

  // BEAT 
  if (this->conf.beatInterval > 0)
      xTaskCreate( this->beat,          // function
                  "mqtt_beat",         // server name
                  2000,               // stack memory
                  (void*)this,        // args
                  1,                  // priority
                  NULL);              // handler
  
};

void K32_mqtt::reconnect() {
    
  if (this->mqttc->connected() && !this->engine->wifi->isConnected()) {
    this->mqttc->disconnect();
    LOG("MQTT: wifi lost.. disconnecting");
  }

  // Loop until we're reconnected
  while (!this->mqttc->connected()) {

    if (!this->engine->wifi->ping()) {
      // LOG("MQTT: wifi not available...");
      delay(500);
      continue;
    }

    LOG("MQTT: Attempting connection...");
    // Create a random client ID
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (this->mqttc->connect(clientId.c_str())) {
      LOG("MQTT: connected");

      String myChan = String(this->engine->settings->get("channel")); 
      String myID =String(this->engine->settings->get("id"));

      this->mqttc->subscribe( ("k32/c"+myChan+"/#").c_str(), 1);
      LOG("MQTT: subscribed to "+("k32/c"+myChan+"/#") );

      this->mqttc->subscribe( ("k32/e"+myID+"/#").c_str(), 1);
      LOG("MQTT: subscribed to "+("k32/e"+myID+"/#") );

      this->mqttc->subscribe( "k32/all/#", 1);
      LOG("MQTT: subscribed to k32/all/#");

    } 
    else {
      LOGINL("failed, rc=");
      LOGINL(this->mqttc->state());
      LOG(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }

}



// /*
//  *   PRIVATE
//  */

void K32_mqtt::loop(void * parameter) {
    K32_mqtt* that = (K32_mqtt*) parameter;
    TickType_t xFrequency = pdMS_TO_TICKS(10);
    String name = that->engine->wifi->nameDevice;

    while(true) { 
      that->reconnect();
      that->mqttc->loop();
      vTaskDelay( xFrequency );
    }
    vTaskDelete(NULL);
}

void K32_mqtt::beat( void * parameter ) {
    K32_mqtt* that = (K32_mqtt*) parameter;
    TickType_t xFrequency = pdMS_TO_TICKS(that->conf.beatInterval);

    // PUBLISH NOT WORKING !!!!!!!
    // while(true) { 
    //   if (that->mqttc->connected())
    //     if (that->mqttc->publish("k32/monitor/beat", "yo") ) LOG("MQTT: beat published");
    //     else LOG("MQTT: beat not published");
    //   vTaskDelay( xFrequency );
    // }

    vTaskDelete(NULL);
}

void splitString(char* data, char* separator, int index, char* result)
{   
    char input[strlen(data)];
    strcpy(input, data);

    char* command = strtok(input, separator);
    for (int k=0; k<index; k++)
      if (command != NULL) command = strtok(NULL, separator);
    
    if (command == NULL) strcpy(result, "");
    else strcpy(result, command);
}


void K32_mqtt::dispatch(char* topic, byte* payload, unsigned int length) {
  payload[length] = 0;

  // TOPIC: k32/all/[motor]   or   k32/c[X]/[motor]   or   k32/e[X]/[motor]

  LOGINL("MQTT: recv ");
  LOGINL(topic);
  LOG((char*)payload);

  // ENGINE
  char motor[16];
  splitString(topic, "/", 2, motor);

  if (strcmp(motor, "reset") == 0) {
    this->engine->stm32->reset();
  }

  else if (strcmp(motor, "shutdown") == 0) {
    this->engine->stm32->shutdown();
  }

  else if (strcmp(motor, "channel") == 0) {
    if (strcmp((char*)payload, "") == 0) return;
    byte chan = atoi((char*)payload);
    if (chan > 0) {
      this->engine->settings->set("channel", chan);
      delay(100);
      this->engine->stm32->reset();
    }
  }

  // OSC AUDIO
  else if (strcmp(motor, "audio") == 0) {

    char action[16];
    splitString(topic, "/", 3, action);

    // PLAY MEDIA
    if (strcmp(action, "play") == 0) {
      
      char path[128];
      splitString((char*)payload, "§", 0, path);
      if (strcmp(path, "") == 0) return;
      this->engine->audio->play( path );

      char volume[5];
      splitString((char*)payload, "§", 1, volume);
      if (strcmp(volume, "") != 0)
        this->engine->audio->volume( atoi(volume) );

      char loop[5];
      splitString((char*)payload, "§", 2, loop);
      if (strcmp(loop, "") != 0)
        this->engine->audio->volume( atoi(loop) > 0 );

    }

    // SAMPLER NOTEON
    else if (strcmp(action, "noteon") == 0) {

      char bank[5];
      splitString((char*)payload, "§", 0, bank);
      char note[5];
      splitString((char*)payload, "§", 1, note);

      if (strcmp(bank, "") == 0 || strcmp(note, "") == 0) return;

      this->engine->sampler->bank( atoi(bank) );
      this->engine->audio->play( this->engine->sampler->path( atoi(note) ) );

      char velocity[5];
      splitString((char*)payload, "§", 2, velocity);
      if (strcmp(velocity, "") != 0)
        this->engine->audio->volume( atoi(velocity) ); 

      char loop[5];
      splitString((char*)payload, "§", 3, loop);
      if (strcmp(loop, "") != 0)
        this->engine->audio->loop( atoi(loop) > 0 );

    }

    // SAMPELR NOTEOFF 
    else if (strcmp(action, "noteoff") == 0) {
      char note[5];
      splitString((char*)payload, "§", 0, note);
      if (this->engine->audio->media() == this->engine->sampler->path( atoi(note) ))
          this->engine->audio->stop();
    }

    // STOP 
    else if (strcmp(action, "stop") == 0) {
      this->engine->audio->stop();
    }

    // VOLUME 
    else if (strcmp(action, "volume") == 0) {
      char volume[5];
      splitString((char*)payload, "§", 0, volume);
      this->engine->audio->volume( atoi(volume) );
    }

    // LOOP 
    else if (strcmp(action, "loop") == 0) {
      char loop[5];
      splitString((char*)payload, "§", 0, loop);
      this->engine->audio->loop( atoi(loop) > 0 );
    }

  }

  // MIDI RAW
  else if (strcmp(motor, "midi") == 0) {

    char val[16];
    splitString((char*)payload, "-", 0, val);
    byte event = atoi(val)/16;
    splitString((char*)payload, "-", 1, val);
    byte note = atoi(val);
    splitString((char*)payload, "-", 2, val);
    byte velo = atoi(val);

    // NOTE OFF
    if ( this->noteOFF && (event == 8 || (event == 9 && velo == 0)) ) {
      if (this->engine->audio->media() == this->engine->sampler->path(note))
          this->engine->audio->stop();
    }

    // NOTE ON
    else if (event == 9)
      this->engine->audio->play( this->engine->sampler->path(note), velo );
    
    // CC
    else if (event == 11) {
      
      // LOOP
      if (note == 1) this->engine->audio->loop( (velo > 63) );

      // NOTEOFF enable
      else if (note == 2) this->noteOFF = (velo < 63);

      // VOLUME
      else if (note == 7) this->engine->audio->volume(velo);

      // BANK SELECT
      // else if (note == 32) this->engine->sampler->bank(velo+1);

      // STOP ALL
      else if (note == 119 or note == 120) this->engine->audio->stop();
    }

  }

  // OSC LEDS
  else if (strcmp(motor, "leds") == 0) {

    char action[16];
    splitString(topic, "/", 3, action);

    // PYRAMID 
    if (strcmp(action, "pyramid") == 0) {
      
    }

    // ALL 
    else if (strcmp(action, "all") == 0) {
      
    }

    // STRIP 
    else if (strcmp(action, "strip") == 0) {
      
    }

    // PIXEL 
    else if (strcmp(action, "pixel") == 0) {
      
    }

    // PLAY ANIM 
    else if (strcmp(action, "play") == 0) {
      
      char anim_name[16];
      splitString((char*)payload, "§", 0, anim_name);
      K32_leds_anim* anim = this->engine->leds->anim( anim_name );
      LOGINL("MQTT: leds play "); LOGINL(anim_name);

      char val[128];
      byte inc = 1;
      splitString((char*)payload, "§", inc, val);
      while (strcmp(val, "") != 0 && (inc-1) < LEDS_PARAM_SLOTS) {
        LOGINL(" ");
        LOGINL(atoi(val));
        anim->setParam(inc-1, atoi(val));
        ++inc;
        splitString((char*)payload, "§", inc, val);
      }
      LOG("");

      this->engine->leds->play( anim );

    }

    // STOP 
    else if (strcmp(action, "stop") == 0 || strcmp(action, "blackout") == 0) {
      this->engine->leds->stop();
    }


  }

}

