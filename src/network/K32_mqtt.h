/*
  K32_mqtt.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_mqtt_h
#define K32_mqtt_h

#include "system/K32_intercom.h"

#define CONFIG_ASYNC_TCP_RUNNING_CORE 0
#include <AsyncMqttClient.h>

#define MQTT_SUBS_SLOTS 16

typedef void (*mqttcbPtr)(char *payload, size_t length);

struct mqttconf
{
  const char *broker;
  int beatInterval;
  int beaconInterval;

};

struct mqttsub
{
  const char* topic;
  int qos;
  mqttcbPtr callback;
};


class K32_mqtt {
  public:
    K32_mqtt(K32_intercom *intercom);
    void start(mqttconf conf);
    void publish(const char *topic, const char *payload = (const char *)nullptr, uint8_t qos=1, bool retain=false);

    void subscribe(mqttsub sub);

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
    

    mqttconf conf;
    mqttsub subscriptions[MQTT_SUBS_SLOTS];
    int subscount = 0;

    K32_intercom *intercom;
};

#endif
