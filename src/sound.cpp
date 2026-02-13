
/* MP3 Audio Player
 *  
 *  MP3 Player connected via serial port to ESP32
 *  The communication is transmit only, but the Software Serial Library requires recieve pins to be defined.
 *  So these are selected from the respricted I/O range of the ESP32 so as not to use up valuable outputs.
 *  The MP3 players can inform when the track is completed playing, but given the requirement for a small
 *  library of tracks it was simpler to record the time of each track in this module and then set a ticker
 *  timer to expire when the track would be complete.
 *  
 *  To add a new track to the system:
 *      copy MP3 sile into folder 01 of the SD card
 *      Name the file nnn.MP3   when nnn is the next number in the sequence
 *      eg 000.mp3 to 015.mp3
 * 
 *      filenames can optionally include descriptive text, which is ignored but useful for reference
 *      eg 000_bell.mp3, 001_horn.mp3, etc
 *
 *      the MP3 player object supports up to 16 tracks and needs to be configured with details of each track, including duration and volume level. This is done in the constructor of the mp3Player class in sound.cpp.
 *      
 *      sound[n].play() can now be added to code
 *            
 */

#include <SoftwareSerial.h>
#include <Ticker.h>
#include "sound.h"
#include "FileSystem.h"

SoftwareSerial channel1(34, 33);    // RX, TX

Ticker channel1Ticker;
Ticker volTicker;
mp3Player  mp3;

// MP3 player command bytes
# define Start_Byte 0x7E
# define Version_Byte 0xFF
# define Command_Length 0x06
# define End_Byte 0xEF
# define Acknowledge 0x00 //Returns info with command 0x41 [0x01: info, 0x00: no info]

int volPot = 0;                            // Raw value of the volume preset pot

void setupSound()
{
  // call this function at the end of setup() to initialise the MP3 player and volume control
  Serial.println("(setupSound)");
  channel1.begin(9600);
  analogReadResolution(12);       // set the resolution to 12 bits (0-4096)
  volPot = 0; 
  volTicker.attach_ms(5000, _getVolume);
}

mp3Player::mp3Player()
{
  currentTrack = -1;
  currentVolume = 0;
  for(int i=0; i<16; i++)
  {
    track[i].duration = 10000; // default duration of 10 seconds for each track, to be adjusted as needed
    track[i].volume = 20;       // default volume for each track, to be adjusted as needed
    track[i].enableLocal = true;
    track[i].enableRemote = true;
  }
}

void mp3Player::loadConfig()
{
  // call this function at the end of setup() to load the MP3 player configuration from SPIFFS
  readMP3ConfigFile(SPIFFS);
  for(int i=0; i<16; i++)readMP3TrackConfigFile(SPIFFS,i);
}

void mp3Player::play(uint8_t source,uint8_t fileNo, boolean Trepeat)
{
  // Plays the requested track if the source of the play command is allowed to play the track. This allows for local and remote control of the MP3 player, with the option to restrict certain tracks to local or remote control only.
  if((source==CMD_ANY)||((source==CMD_LOCAL)&&(track[fileNo].enableLocal==true))||((source==CMD_REMOTE)&&(track[fileNo].enableRemote==true)))
  {
Serial.print("(play) fileNo:");
Serial.println(fileNo);
    currentTrack = fileNo;
    repeat = Trepeat;
    _getVolume();
    execute_CMD(0x06,0,currentVolume);            // Set volume
    pause(500);
    execute_CMD(0x0F,01,fileNo);                  // Play track (from folder 01)
    channel1Ticker.attach_ms(track[fileNo].duration, _finished); 
  Serial.print("(_playSound) ");
  Serial.print("track:");
  Serial.print(fileNo);
  Serial.print(" volume:");
  Serial.println(currentVolume);
  Serial.print(" duration:");
  Serial.print(track[fileNo].duration/1000);
  Serial.println(" seconds");
  Serial.print(" repeat:");
  Serial.println(repeat);
  }
}

void mp3Player::stop(uint8_t source)
{
  // Stops the currently playing track if the source of the stop command is allowed to stop the track. This allows for local and remote control of the MP3 player, with the option to restrict certain tracks to local or remote control only.
  if((source==CMD_ANY)||((source==CMD_LOCAL)&&(track[currentTrack].enableLocal==true))||((source==CMD_REMOTE)&&(track[currentTrack].enableRemote==true)))
  {
    Serial.println("(stopSound)");
    execute_CMD(0x0E,0,0);
    currentTrack = -1;
    repeat = false;
    channel1Ticker.detach();
  }
}

void _finished()
{
  // Called at the end of playback of a track
  Serial.println("(trackFinished)");
  mp3.execute_CMD(0x0E,0,0);
  if(mp3.repeat == true)
  {
    // Playing track on a continuous loop, so restart it
    if(mp3.currentTrack > 0)mp3.play(CMD_LOCAL, mp3.currentTrack, true);
  }
  else 
  {
    mp3.currentTrack = 0;
    channel1Ticker.detach();
  }
}

void mp3Player::execute_CMD(byte CMD, byte Par1, byte Par2)
{
  // Excecute MP3 command - this does the talking with the MP3 player module by sending a 
  // command line of bytes over the serial connection. The command line includes a checksum to ensure the integrity of the command.
  // Calculate the checksum (2 bytes)
  word checksum = -(Version_Byte + Command_Length + CMD + Acknowledge + Par1 + Par2);
  // Build the command line
  byte Command_line[10] = { Start_Byte, Version_Byte, Command_Length, CMD, Acknowledge,
  Par1, Par2, highByte(checksum), lowByte(checksum), End_Byte};

  Serial.println("execute_CMD");

  //Send the command line to the module
  for (byte k=0; k<10; k++)
  {
    channel1.write( Command_line[k]);
  }
  Serial.println();
}

void _getVolume()
{
  // Calculates the current volume level and writes this to the MP3 play.
  // Should be run on a ticker at an appropriate speed
  // There are four volume sources which are added together
  // Each Track has a default volume level which is set in the track configuration and can be adjusted by the user.
  // The manual volume Potentiometer on the module is adjusted by the user and can be positive or negative.
  // A global ManumalTrim is calculated from the position of a potentiometer and can be adjusted by the user via the soundConfig web page 
  // A global AutoTrim is adjusted via the soundConfig web page and invoked via MQTT topic /iot/
  // This autoTrim is intended to allow JMRI to apply 'normal' or 'quiet' settings across all nodes with one flag

  // Current volumeLevel is simply the sume of the above elements
  int newVol;

  volPot = analogRead(VOLUME);   
  float analogValue = volPot*5;
  analogValue = analogValue / 4096;
  newVol = mp3.track[mp3.currentTrack].volume + analogValue + mp3.manualTrim;
  if(mp3.autoTrimEnabled == true)newVol += mp3.autoTrim;
  return;
}

void mp3Player::setVolume(int level)
{
//  Serial.println("(_setVolume)");
//  Serial.print("level:");
//  Serial.println(level);
//  Serial.println(level);
//  Serial.print("currentVol:");
//  Serial.println(currentVol);
  // adjusts current playback volume in the MP3 player
  if(level < 0)level = 0;

  if(currentVolume != level)
  {
    currentVolume = level;
    // write the volume to the MP3 player
    execute_CMD(0x06,0,level);                         // Adjust volume
    Serial.print("(_setVolume) Volume:"); 
    Serial.println(currentVolume);
  }
}

void mp3Player::pause(long mS)
{
  static unsigned long exitTime;
  exitTime = millis() + mS;
  while (millis() < exitTime)
  {
    // let other stuff run (we are on CPU1)
    yield();
  }
  return;
}


