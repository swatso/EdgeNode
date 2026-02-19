
// Collection of functions which allow access to the SPIFFS file system
#include "FileSystem.h"
//#include "SPIFFS.h"
#include <SPIFFS.h>                               	
#include "arduinoGlue.h"
#include "gpio.h"
#include "action.h"

// File paths to save input values permanently

const char* quietOffsetPath = "/quietOffset.txt";
const char* loudOffsetPath = "/loudOffset.txt";
const char* nodeConfigPath = "/config#.txt";
const char* trackConfigPath = "/trackConfig#.txt";
const char* actionConfigPath = "/actionConfig#.txt";
const char* servoConfigPath = "/servoConfig#.txt";

// Initialize SPIFFS
void setupSPIFFS() 
{
  if (!SPIFFS.begin(true)) 
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read a File from SPIFFS, if the file does not exist then a new one is created to store the default string (deft)
// Simply reads a single string from SPIFFS file
char* readFileC(fs::FS &fs, const char *path, const char *deft)
{
  Serial.printf("(readFileC): %s\r\n", path);
  vTaskDelay(20);
  static char fileContent[20];
  uint8_t i;
  for(i=0; i<20; i++)fileContent[i]='\0';     // clear the static buffer
  File file = fs.open(path);
  if(!file || file.isDirectory())
  {
    Serial.print("- failed to open file for reading, creating file and restoring default:");
    Serial.println(deft);
    for(i=0; deft[i]>0x1F;i++)fileContent[i]=deft[i];
    fileContent[i]='\0';
//Serial.println(fileContent);
    writeFile(SPIFFS, path, fileContent);
    return (&fileContent[0]);
  }
  // file is available, read it
  i=0;
  while(file.available())
  {
    fileContent[i]=file.read();
//        Serial.print("fileContent[i]:");
//      Serial.println(Serial.printf("%d \n",fileContent[i]));
    if(fileContent[i]>0x1F)i++;
    else
    {
      
      fileContent[i] = '\0';
//      Serial.print("i:");
//      Serial.println(i);
      while(file.available())file.read();
    }
  }
      int e = i;
//    Serial.println("ReadFileC");
//    for(i=0; i<=e; i++)Serial.printf("%d \n",fileContent[i]);
//    Serial.println("done");
  return(&fileContent[0]);
}

// Read a File from SPIFFS, if the file does not exist then a new one is created to store the default string (deft)
// Simply reads a single string from SPIFFS file
String readFile(fs::FS &fs, const char *path, const char *deft)
{
  Serial.printf("(readFile): %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory())
  {
    Serial.print("- failed to open file for reading, creating file and restoring default:");
    Serial.println(deft);
    writeFile(SPIFFS, path, deft);
    file.close();
    return (deft);
  }
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    Serial.print(fileContent);
    break;     
  }
  file.close();
  return fileContent;
}

// Write file to SPIFFS
// Creates a file and writes the string into  it
void writeFile(fs::FS &fs, const char * path, const char * message)
{
  Serial.printf("Writing char* to file: %s\r\n", path);
  Serial.println(message);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message))
  {
    Serial.println("- file written");
  } 
  else 
  {
    Serial.println("- write failed");
  }
  file.close();
}

// Write single integer to SPIFFS
void writeFile(fs::FS &fs, const char * path, int value)
{
  Serial.printf("Writing integer to file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(value))
  {
    Serial.println("- file written");
  } 
  else 
  {
    Serial.println("- write failed");
  }
  file.close();
}

void writeMP3ConfigFile(fs::FS &fs)
{
  Serial.println("Writing MP3 config file");
  File file = fs.open("/mp3Config.txt", FILE_WRITE);
  if(!file)
  {
    Serial.println("- failed to open mp3 config file for writing");
    return;
  }
  file.println(mp3.currentTrack);
  file.println(mp3.currentVolume);
  file.println(mp3.manualTrim);
  file.println(mp3.autoTrim);
  file.close();
}

void readMP3ConfigFile(fs::FS &fs)
{
  Serial.println("Reading MP3 config file");
  File file = fs.open("/mp3Config.txt", FILE_READ);
  if(!file)
  {
    Serial.println("- failed to open mp3 config file for reading");
    return;
  }
  if(file.size() == 0)
  {
    // config file is empty, so just leave the hardcoded defaults
    file.close();
    Serial.println("No File");
    return;
  }
  mp3.currentTrack = std::stoi(readConfigItem(file),NULL,10);
  mp3.currentVolume = std::stoi(readConfigItem(file),NULL,10);
  mp3.manualTrim = std::stoi(readConfigItem(file),NULL,10);
  mp3.autoTrim = std::stoi(readConfigItem(file),NULL,10);
  file.close();
}

void writeMP3TrackConfigFile(fs::FS &fs, uint8_t trackNo)
{
  Serial.println("Writing MP3 Track config file");

  vTaskDelay(100);
  char path[30];
  uint8_t i,j;
  for(i=0, j=0; trackConfigPath[i] != 0; i++)
  {
    if(trackConfigPath[i] != HASH)path[j++] = trackConfigPath[i];
    else
    {
      char buffer[3];
      sprintf(buffer, "%02d", trackNo);
      path[j++]=buffer[0];
      path[j++]=buffer[1];
    }
  }
  path[j]=0;
  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    Serial.println("- failed to open mp3 track config file for writing");
    return;
  }
  file.println(mp3.track[trackNo].name);
  file.println(mp3.track[trackNo].volume);
  file.println(mp3.track[trackNo].duration);
  file.println(mp3.track[trackNo].enableLocal);
  file.println(mp3.track[trackNo].enableRemote);
  file.close();
}

void readMP3TrackConfigFile(fs::FS &fs, uint8_t trackNo)
{
  Serial.print("Reading MP3 Track config file: ");
  Serial.println(trackNo);
  vTaskDelay(10);
  char path[30];
  uint8_t i,j;
  for(i=0, j=0; trackConfigPath[i] != 0; i++)
  {
    if(trackConfigPath[i] != HASH)path[j++] = trackConfigPath[i];
    else
    {
      char buffer[3];
      sprintf(buffer, "%02d", trackNo);
      path[j++]=buffer[0];
      path[j++]=buffer[1];
    }
  }
  path[j]=0;
  File file = fs.open(path, FILE_READ);
  if(!file)
  {
    Serial.println("- failed to open mp3 track config file for reading");
    return;
  }
  if(file.size() == 0)
  {
    // config file is empty, so just leave the hardcoded defaults
    file.close();
    Serial.println("No File");
    return;
  }
  char buffer[20];
  int l = file.readBytesUntil('\n',buffer,sizeof(buffer));
  buffer[l-1]='\0';
  for(i=0; (buffer[i] != '\0')&& (i<19); i++)mp3.track[trackNo].name[i] = buffer[i];
  mp3.track[trackNo].name[i]='\0';
  mp3.track[trackNo].volume = std::stoi(readConfigItem(file),NULL,10);
  mp3.track[trackNo].duration = std::stoi(readConfigItem(file),NULL,10);
  mp3.track[trackNo].enableLocal = std::stoi(readConfigItem(file),NULL,10);
  mp3.track[trackNo].enableRemote = std::stoi(readConfigItem(file),NULL,10);

  Serial.print("name: ");
  Serial.println(mp3.track[trackNo].name);
  Serial.print("volume: ");
  Serial.println(mp3.track[trackNo].volume);
  Serial.print("duration: ");
  Serial.println(mp3.track[trackNo].duration);
  Serial.print("enableLocal: "); 
  Serial.println(mp3.track[trackNo].enableLocal);
  Serial.print("enableRemote: ");
  Serial.println(mp3.track[trackNo].enableRemote);

  file.close();
}

void writeServoPosition(fs::FS &fs, uint8_t bitNo, int position)
{
  Serial.print("Writing Servo config file:");
  Serial.println(bitNo);

  vTaskDelay(100);
  char path[30];
  uint8_t i,j;
  for(i=0, j=0; servoConfigPath[i] != 0; i++)
  {
    if(servoConfigPath[i] != HASH)path[j++] = servoConfigPath[i];
    else
    {
      char buffer[3];
      sprintf(buffer, "%02d", bitNo);
      path[j++]=buffer[0];
      path[j++]=buffer[1];
    }
  }
  path[j]=0;
  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    Serial.println("- failed to open servo config file for writing");
    return;
  }
  file.println(position);
  file.close();
}

int readServoPosition(fs::FS &fs, uint8_t bitNo)
{
  Serial.print("Reading servo config file: ");
  Serial.println(bitNo);
  vTaskDelay(10);
  char path[30];
  uint8_t i,j;
  for(i=0, j=0; servoConfigPath[i] != 0; i++)
  {
    if(servoConfigPath[i] != HASH)path[j++] = servoConfigPath[i];
    else
    {
      char buffer[3];
      sprintf(buffer, "%02d", bitNo);
      path[j++]=buffer[0];
      path[j++]=buffer[1];
    }
  }
  path[j]=0;
  File file = fs.open(path, FILE_READ);
  if(!file)
  {
    Serial.println("- failed to open servo config file for reading");
    return(-1);
  }
  if(file.size() == 0)
  {
    // config file is empty, so just leave the hardcoded defaults
    file.close();
    Serial.println("No File");
    return(-1);
  }
  int position = std::stoi(readConfigItem(file),NULL,10);
Serial.print("position: ");
Serial.println(position);
  file.close();
  return position;
}

void writeActionConfigFile(fs::FS &fs, uint8_t number)
{
  Serial.print("Writing Action config file:");
  Serial.println(number);

  vTaskDelay(100);
  char path[30];
  uint8_t i,j;
  for(i=0, j=0; actionConfigPath[i] != 0; i++)
  {
    if(actionConfigPath[i] != HASH)path[j++] = actionConfigPath[i];
    else
    {
      char buffer[3];
      sprintf(buffer, "%02d", number);
      path[j++]=buffer[0];
      path[j++]=buffer[1];
    }
  }
  path[j]=0;
  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    Serial.println("- failed to action config file for writing");
    return;
  }
  file.println(action[number].number);
  file.println(action[number].enableLocal);
  file.println(action[number].enableRemote);
  file.close();
}

void readActionConfigFile(fs::FS &fs, uint8_t number)
{
  Serial.print("Reading action config file: ");
  Serial.println(number);
  vTaskDelay(10);
  char path[30];
  uint8_t i,j;
  for(i=0, j=0; actionConfigPath[i] != 0; i++)
  {
    if(actionConfigPath[i] != HASH)path[j++] = actionConfigPath[i];
    else
    {
      char buffer[3];
      sprintf(buffer, "%02d", number);
      path[j++]=buffer[0];
      path[j++]=buffer[1];
    }
  }
  path[j]=0;
  File file = fs.open(path, FILE_READ);
  if(!file)
  {
    Serial.println("- failed to open action config file for reading");
    return;
  }
  if(file.size() == 0)
  {
    // config file is empty, so just leave the hardcoded defaults
    file.close();
    Serial.println("No File");
    return;
  }
  action[number].number = std::stoi(readConfigItem(file),NULL,10);
  action[number].enableLocal = std::stoi(readConfigItem(file),NULL,10);
  action[number].enableRemote = std::stoi(readConfigItem(file),NULL,10);

  Serial.print("number: ");
  Serial.println(action[number].number);
  Serial.print("enableLocal: "); 
  Serial.println(action[number].enableLocal);
  Serial.print("enableRemote: ");
  Serial.println(action[number].enableRemote);

  file.close();
}

// Write gpio bit configuration to SPIFFS
// There is a separate config file for each gpio Bit
void writeConfigFile(fs::FS &fs, uint8_t bit)
{
  Serial.println("Writing gpio bit config file");
  vTaskDelay(10);
  char path[30];
  uint8_t i,j;
  for(i=0, j=0; nodeConfigPath[i] != 0; i++)
  {
    if(nodeConfigPath[i] != HASH)path[j++] = nodeConfigPath[i];
    else
    {
      char buffer[3];
      sprintf(buffer, "%02d", bit);
      path[j++]=buffer[0];
      path[j++]=buffer[1];
    }
  }
  path[j]=0;

Serial.println(path);

  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    Serial.println("- failed to open config file for writing");
    return;
  }
  file.println(gpio[bit].name);
  file.println(gpio[bit].type);
  file.println(gpio[bit].preset0);
  file.println(gpio[bit].preset1);
  file.println(gpio[bit].preset2);
  file.println(gpio[bit].rate);
  file.println(gpio[bit].getEasingType());
  file.println(gpio[bit].enableRemote);
  file.println(gpio[bit].enableLocal);
  file.println(gpio[bit].getPublishRate());
  file.close();

  Serial.println(gpio[bit].type);
  Serial.println(gpio[bit].preset0);
  Serial.println(gpio[bit].preset1);
  Serial.println(gpio[bit].preset2);
  Serial.println(gpio[bit].rate);
  Serial.println(gpio[bit].getEasingType()); 
  Serial.println(gpio[bit].enableRemote);
  Serial.println(gpio[bit].enableLocal);
  Serial.println(gpio[bit].getPublishRate());
}

char* readConfigItem(File file)
{
  static char buffer[20];
  for(int i=0; i<20; i++)buffer[i]='\0';
  int l = file.readBytesUntil('\n',buffer,sizeof(buffer));
  Serial.println(l);
  //Serial.println(buffer[0]);
  //Serial.println(buffer[1]);
  //Serial.println(buffer[2]);

  buffer[l-1]='\0';
  Serial.printf("char0:%d ",buffer[0]);
  Serial.printf("char1:%d ",buffer[1]);
  Serial.printf("char2:%d ",buffer[2]);

  return(&buffer[0]);
}

// Read node configuration from SPIFFS
void readConfigFile(fs::FS &fs, uint8_t bit)
{
  Serial.print("Reading node config file for bit");
  Serial.println(bit);
  char path[30];
  uint8_t i,j;
  for(i=0, j=0; nodeConfigPath[i] != 0; i++)
  {
    if(nodeConfigPath[i] != HASH)path[j++] = nodeConfigPath[i];
    else
    {
      char buffer[3];
      sprintf(buffer, "%02d", bit);
      path[j++]=buffer[0];
      path[j++]=buffer[1];
    }
  }
  path[j]=0;
  File file = fs.open(path, FILE_READ);
  if(!file)
  {
    Serial.println("- failed to open config file for reading");
    return;
  }
  if(file.size() == 0)
  {
    // config file is empty, so just leave the hardcoded defaults
    file.close();
    Serial.println("No File");
    return;
  }
  Serial.println("a");
  char buffer[20];
  int l = file.readBytesUntil('\n',buffer,sizeof(buffer));
  buffer[l-1]='\0';
  for(i=0; (buffer[i] != '\0')&& (i<19); i++)gpio[bit].name[i] = buffer[i];
  gpio[bit].name[i]='\0';

//int xx = std::stoi(readConfigItem(file),NULL,10);
char* xx=readConfigItem(file);
Serial.print("ReadConfigFile Type=");
Serial.println(xx);

gpio[bit].setType((std::stoi(xx,NULL,10)));
Serial.println(std::stoi(xx,NULL,10));

//  gpio[bit].setType((int)(readConfigItem(file)));

  Serial.println("c");
  gpio[bit].preset0 = std::stoi(readConfigItem(file),NULL,10);
  gpio[bit].preset1 = std::stoi(readConfigItem(file),NULL,10);
  gpio[bit].preset2 = std::stoi(readConfigItem(file),NULL,10);
  gpio[bit].rate = std::stoi(readConfigItem(file),NULL,10);
  gpio[bit].setEasingType(std::stoi(readConfigItem(file),NULL,10));
  gpio[bit].enableRemote = std::stoi(readConfigItem(file),NULL,10);
  Serial.print("readCOnfigFile enableRemote: ");
  Serial.println(gpio[bit].enableRemote);
  gpio[bit].enableLocal = std::stoi(readConfigItem(file),NULL,10);
  gpio[bit].setPublishRate(std::stoi(readConfigItem(file),NULL,10));
  file.close();
  Serial.println("Done");
}

void  deleteFile(fs::FS &fs, const char * path)
{
  fs.remove(path);
}

char* hashPath(const char* path, uint8_t bit)
{
  // replaces the 'hash' in the given path with the single digit 
  // hexadecimal equivalent of the supplied bit
  static char newPath[30];
  uint8_t i,j;
  for(i=0, j=0; path[i] != 0; i++)
  {
    if(path[i] != HASH)newPath[j++] = path[i];
    else
    {
      char buffer[2];
      sprintf(buffer, "%01X", bit);
      newPath[j++]=buffer[0];
    }
  }
  newPath[j]=0;
  return(&newPath[0]);
}