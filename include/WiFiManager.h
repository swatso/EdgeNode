#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <cstring>
#include <Arduino.h>

unsigned long WiFiGetCommsUptime();                             
String WiFiGetIPAddress();                                      
String WiFiGetSSID();                                           
int WiFiGetRSSI(); 

#endif // WIFIMANAGER_H
