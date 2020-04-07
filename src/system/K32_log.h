/*
  KESP_LOG.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_log_h
#define K32_log_h

#include <Arduino.h>

#define KDEBUG

#ifdef KDEBUG
 #define LOGSETUP()                    Serial.begin(115200)
 #define LOGINL(x)                     Serial.print (x)
 #define LOGDEC(x)                     Serial.print (x, DEC)
 #define LOGHEX(x)                     Serial.print (x, HEX)
 #define LOGF(x, y)                    Serial.printf (x, y)
 #define LOGF2(x, y1, y2)              Serial.printf (x, y1, y2)
 #define LOGF3(x, y1, y2, y3)          Serial.printf (x, y1, y2, y3)
 #define LOGF4(x, y1, y2, y3, y4)      Serial.printf (x, y1, y2, y3, y4)
 #define LOGF5(x, y1, y2, y3, y4, y5)  Serial.printf (x, y1, y2, y3, y4, y5)
 #define LOG(x)                        Serial.println (x)
#else
 #define LOGSETUP()
 #define LOGINL(x)
 #define LOGDEC(x)
 #define LOGF(x, y)
 #define LOGF2(x, y1, y2)
 #define LOGF3(x, y1, y2, y3)
 #define LOGF4(x, y1, y2, y3, y4)
 #define LOG(x)
#endif


#endif