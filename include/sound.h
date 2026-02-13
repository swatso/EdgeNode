#ifndef SOUND_H
#define SOUND_H

/*
  sound.h - Library for controlling the MP3 player and volume settings
*/

//============ Includes ====================
#include <Arduino.h>

void setupSound();                                           
void _finished();
void _getVolume();

#define CMD_ANY 0
#define CMD_LOCAL 1
#define CMD_REMOTE 2

#define VOLUME 35     // gpio pin for the volume preset potentiometer

// Soundtrack Object
// Encapsulates the data each a single soundtrack
struct soundTrack
{
    char name[20];            // Friendly name
    int duration;         // Soundtrack duration
    unsigned int volume;      // Soundtrack Volume
    bool enableLocal;
    bool enableRemote;
};

//object to control the MP3 player
class mp3Player
{
  public:
    uint8_t currentTrack;
    uint8_t currentVolume;
    soundTrack track[16];
    bool repeat = false;
    int8_t manualTrim = 0;                   // Volume offset
    int8_t autoTrim = 0;                     // Volume offest
    boolean autoTrimEnabled = false;         // Flag to indicate whether autoTrim is enabled or not
    mp3Player();
    void loadConfig();
    void play(uint8_t source,uint8_t fileNo, boolean repeat);
    void stop(uint8_t source);
    void setVolume(int level);
    void execute_CMD(uint8_t CMD, uint8_t Par1, uint8_t Par2);

  private:
    void pause(long mS);
};

extern mp3Player mp3;

#endif //SOUND_H
