/*
  K32_osc.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_osc_h
#define K32_osc_h

#include "system/K32_system.h"
#include "network/K32_wifi.h"
#include "audio/K32_audio.h"
#include "light/K32_light.h"

#include <WiFi.h>
#include <WiFiUdp.h>

#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

struct oscconf
{
  int port_in;
  int port_out;
  int beatInterval;
  int beaconInterval;
};

class K32_osc {
  public:
    K32_osc(K32_system *system, K32_wifi *wifi, K32_audio *audio, K32_light *light);

    void start(oscconf conf);

    const char* id_path();
    const char* chan_path();

    OSCMessage status();

    void send(OSCMessage msg);

  private:
    SemaphoreHandle_t lock;
    static void server( void * parameter );
    static void beacon( void * parameter );
    static void beat( void * parameter );

    WiFiUDP* udp;         // must be protected with lock 
    WiFiUDP* sendSock;
    IPAddress linkedIP;

    oscconf conf;

    K32_system *system;
    K32_wifi *wifi;
    K32_audio *audio;
    K32_light *light;
};

// OSCMessage overload
class K32_oscmsg : public OSCMessage {
  public:
    K32_oscmsg(K32_osc* env);

    bool dispatch(const char * pattern, void (*callback)(K32_osc* env, K32_oscmsg &), int = 0);
    bool route(const char * pattern, void (*callback)(K32_osc* env, K32_oscmsg &, int), int = 0);
    String getStr(int position);

    K32_osc* env;
};

#endif