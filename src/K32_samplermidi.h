/*
  K32_samplermidi.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_samplermidi_h
#define K32_samplermidi_h

#include "Arduino.h"
#include "K32_log.h"

#define MIDI_MAX_BANK 17      //17
#define MIDI_MAX_NOTE 128     //128
#define MIDI_MAX_TITLE 14     // Filename length


class K32_samplermidi {
  public:
    K32_samplermidi();

    void scan();

    String path(int bank, int note);
    int size(byte bank, byte note);
    void remove(byte bank, byte note);

  private:
    SemaphoreHandle_t lock;
    static void task( void * parameter );

    char samples[MIDI_MAX_BANK][MIDI_MAX_NOTE][MIDI_MAX_TITLE];

    String pad3(int input);
};

#endif