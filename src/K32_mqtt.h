/*
  K32_mqtt.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_mqtt_h
#define K32_mqtt_h

#include "K32_system.h"
#include "K32_wifi.h"
#include "K32_audio.h"
#include "K32_light.h"

#define CONFIG_ASYNC_TCP_RUNNING_CORE 0
#include <AsyncMqttClient.h>

struct mqttconf
{
  const char *broker;
  int beatInterval;
  int beaconInterval;
};

class K32_mqtt {
  public:
    K32_mqtt(K32_system *system, K32_wifi *wifi, K32_audio *audio, K32_light *light);
    void start(mqttconf conf);

  private:
    SemaphoreHandle_t lock;
    AsyncMqttClient* mqttClient;

    static void check(void * parameter);
    static void beat( void * parameter );
    static void beacon( void * parameter );

    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) ;

    void dispatch(char* topic, char* payload, size_t length);

    bool connected;
    bool noteOFF = true;

    mqttconf conf;

    K32_system *system;
    K32_wifi *wifi;
    K32_audio *audio;
    K32_light *light;
};

#endif