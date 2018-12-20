#pragma once
//--------------------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include "CONFIG.h"
#include "CoreArray.h"
#include "Memory.h"
#include "CoreButton.h"
#include <SdFat.h>
extern SdFat SD;
extern bool SD_INITED;
extern Button ControlButton;
//--------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
struct Value8
{
  uint8_t h1;
  uint8_t h2;
  uint8_t value;

  Value8() : h1(SETT_HEADER1), h2(SETT_HEADER2) {}

};
#pragma pack(pop)
//--------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
struct Value16
{
  uint8_t h1;
  uint8_t h2;
  uint16_t value;
  
  Value16() : h1(SETT_HEADER1), h2(SETT_HEADER2) {}

};
#pragma pack(pop)
//--------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
struct Value32
{
  uint8_t h1;
  uint8_t h2;
  uint32_t value;
  
  Value32() : h1(SETT_HEADER1), h2(SETT_HEADER2) {}

};
#pragma pack(pop)
//--------------------------------------------------------------------------------------------------------------------------------
class SettingsClass
{
  public:
    SettingsClass();


    uint8_t read8(uint16_t address, uint8_t defaultVal);
    void write8(uint16_t address, uint8_t val);
    
    uint16_t read16(uint16_t address, uint16_t defaultVal);
    void write16(uint16_t address, uint16_t val);

    uint32_t read32(uint16_t address, uint32_t defaultVal);
    void write32(uint16_t address, uint32_t val);


};
//--------------------------------------------------------------------------------------------------------------------------------
extern SettingsClass Settings;
