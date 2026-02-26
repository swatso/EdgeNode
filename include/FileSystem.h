#ifndef FILESYSTEM_H
#define FILESYSTEM_H

//============ Includes ====================
#include "sound.h"
#include <Arduino.h>
#include <SPIFFS.h>
void setupSPIFFS();
void writeMP3ConfigFile(fs::FS &fs);
void readMP3ConfigFile(fs::FS &fs);
void writeMP3TrackConfigFile(fs::FS &fs, uint8_t trackNo);
void readMP3TrackConfigFile(fs::FS &fs, uint8_t trackNo);
void writeActionConfigFile(fs::FS &fs, uint8_t number);
void readActionConfigFile(fs::FS &fs, uint8_t number);
char* readConfigItem(File file);
int readServoPosition(fs::FS &fs, uint8_t bitNo);
void writeServoPosition(fs::FS &fs, uint8_t bitNo, int position);
void readConfigFile(fs::FS &fs, uint8_t bit);
void writeConfigFile(fs::FS &fs, uint8_t bit);
void writeFile(fs::FS &fs, const char * path, const char * message);
void writeFile(fs::FS &fs, const char * path, int value);
char* readFileC(fs::FS &fs, const char *path, const char *deft);

/*/
char* readFileC(fs::FS &fs, const char *path, const char *deft);
void writeFileC(fs::FS &fs, const char * path, const char * message);
void writeFileC(fs::FS &fs, const char * path, int value);
*/
    //============ Added by Convertor ==========
#endif // FILESYSTEM_H
