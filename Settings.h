#pragma once
//--------------------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include "CONFIG.h"
#include "CoreArray.h"
#include "Memory.h"
#include "CoreButton.h"
//--------------------------------------------------------------------------------------------------------------------------------
class SettingsClass
{
  public:
    SettingsClass();

    template< typename T >
    bool read(uint16_t address, T& result)
    {
        if(MemRead(address++) != SETT_HEADER1)
          return false;
    
        if(MemRead(address++) != SETT_HEADER2)
          return false;
    
       uint8_t* ptr = (uint8_t*)&result;
    
       for(size_t i=0;i<sizeof(T);i++)
       {
        *ptr++ = MemRead(address++);
       }
    
      return true;      
    }

    template< typename T >
    void write(uint16_t address, T& val)
    {
        MemWrite(address++,SETT_HEADER1);
        MemWrite(address++,SETT_HEADER2);
      
        uint8_t* ptr = (uint8_t*)&val;
        
        for(size_t i=0;i<sizeof(T);i++)
          MemWrite(address++,*ptr++);      
    }

};
//--------------------------------------------------------------------------------------------------------------------------------
extern SettingsClass Settings;
