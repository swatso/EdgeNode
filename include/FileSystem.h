#ifndef FILESYSTEM_H
#define FILESYSTEM_H

//============ Includes ====================
#include "arduinoGlue.h"
#include "sound.h"

void writeMP3ConfigFile(fs::FS &fs);
void readMP3ConfigFile(fs::FS &fs);
void writeMP3TrackConfigFile(fs::FS &fs, uint8_t trackNo);
void readMP3TrackConfigFile(fs::FS &fs, uint8_t trackNo);
void writeActionConfigFile(fs::FS &fs, uint8_t number);
void readActionConfigFile(fs::FS &fs, uint8_t number);
char* readConfigItem(File file);

/*/
char* readFileC(fs::FS &fs, const char *path, const char *deft);
void writeFileC(fs::FS &fs, const char * path, const char * message);
void writeFileC(fs::FS &fs, const char * path, int value);
*/
    //============ Added by Convertor ==========
#endif // FILESYSTEM_H
