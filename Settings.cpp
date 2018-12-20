#include "Settings.h"
#include "Memory.h"
//--------------------------------------------------------------------------------------------------------------------------------
SettingsClass Settings;
//--------------------------------------------------------------------------------------------------------------------------------
SettingsClass::SettingsClass()
{
}
//--------------------------------------------------------------------------------------------------------------------------------
uint8_t SettingsClass::read8(uint16_t address, uint8_t defaultVal)
{
  Value8 v;
  uint8_t* iter = (uint8_t*)&v;
  for(size_t i=0;i<sizeof(Value8);i++)
  {
    *iter++ = MemRead(address++);
  }

  if(!(v.h1 == SETT_HEADER1 && v.h2 == SETT_HEADER2))
    return defaultVal; 

   return v.value;
}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::write8(uint16_t address, uint8_t val)
{
  Value8 v;
  v.value = val;
  uint8_t* iter = (uint8_t*)&v;
  for(size_t i=0;i<sizeof(Value8);i++)
  {
    MemWrite(address++,*iter++);
  }
}
//--------------------------------------------------------------------------------------------------------------------------------
uint16_t SettingsClass::read16(uint16_t address, uint16_t defaultVal)
{
  Value16 v;
  uint8_t* iter = (uint8_t*)&v;
  for(size_t i=0;i<sizeof(Value16);i++)
  {
    *iter++ = MemRead(address++);
  }

  if(!(v.h1 == SETT_HEADER1 && v.h2 == SETT_HEADER2))
    return defaultVal; 

   return v.value;
}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::write16(uint16_t address, uint16_t val)
{
  Value16 v;
  v.value = val;
  uint8_t* iter = (uint8_t*)&v;
  for(size_t i=0;i<sizeof(Value16);i++)
  {
    MemWrite(address++,*iter++);
  }
}
//--------------------------------------------------------------------------------------------------------------------------------
uint32_t SettingsClass::read32(uint16_t address, uint32_t defaultVal)
{
  Value32 v;
  uint8_t* iter = (uint8_t*)&v;
  for(size_t i=0;i<sizeof(Value32);i++)
  {
    *iter++ = MemRead(address++);
  }

  if(!(v.h1 == SETT_HEADER1 && v.h2 == SETT_HEADER2))
    return defaultVal; 

   return v.value;
}
//--------------------------------------------------------------------------------------------------------------------------------
void SettingsClass::write32(uint16_t address, uint32_t val)
{
  Value32 v;
  v.value = val;
  uint8_t* iter = (uint8_t*)&v;
  for(size_t i=0;i<sizeof(Value32);i++)
  {
    MemWrite(address++,*iter++);
  }
}
//--------------------------------------------------------------------------------------------------------------------------------

