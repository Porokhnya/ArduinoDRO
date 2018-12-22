#pragma once

#include <Arduino.h>
#include "CoreArray.h"
#include "CONFIG.h"
#include <limits.h>
//--------------------------------------------------------------------------------------------------------------------------------------
#define NO_SCALE_DATA INT_MIN //int32_t(0xFFEFFFFF) // значение, которое выдаётся, когда в порту постоянно единица
//--------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  int32_t Value;
  uint8_t Fract;
} ScaleFormattedData;
//--------------------------------------------------------------------------------------------------------------------------------------
class ScalesClass;
//--------------------------------------------------------------------------------------------------------------------------------------
class ScaleData
{
  public:

    ScaleData(uint16_t eepromAddress, const char* label, const char* absButtonCaption, const char* relButtonCaption,const char* zeroButtonCaption, char axis, uint8_t dataPin, bool active);

    void setup();
    //void update();
    
    const char* getLabel() { return label; }

    bool hasData() { return active && (rawData != NO_SCALE_DATA); }
    ScaleFormattedData getData();
    int32_t getRawData() { return rawData; }

    char getAxis() { return axis; }
    
    const char* getAbsButtonCaption() { return absButtonCaption; }
    const char* getRelButtonCaption() { return relButtonCaption; }
    const char* getZeroButtonCaption() { return zeroButtonCaption; }
    
    void setAbsButtonIndex(int8_t idx) { absButtonIndex = idx; }
    int8_t getAbsButtonIndex() { return absButtonIndex; }

    void setZeroButtonIndex(int8_t idx) { zeroButtonIndex = idx; }
    int8_t getZeroButtonIndex() { return zeroButtonIndex; }    

    int32_t getLastValueX() { return lastValueX; }
    void setLastValueX(int32_t val) { lastValueX = val; }
    
    void switchABS();
    bool inABSMode() { return !isAbsFactorEnabled; }

    void switchZERO();
    bool inZEROMode() { return isZeroFactorEnabled; }

    void setY(int val) { axisY = val; }
    int getY() { return axisY; }

    void setHeight(int val) { axisHeight = val; }
    int getHeight() { return axisHeight; }

    void setDataXCoord(int val) { dataXCoord = val; }
    int getDataXCoord() { return dataXCoord; }

    bool isActive() { return active; }


    protected:

      friend class ScalesClass;
      void beginRead();
      void readBit(int32_t bitNum, bool isLastBit);
      void endRead();

  
  private:
  
    const char* label;
    const char* absButtonCaption;
    const char* relButtonCaption;
    const char* zeroButtonCaption;
    char axis;
    
    int8_t absButtonIndex;
    int8_t zeroButtonIndex;
    
    bool isZeroFactorEnabled;
    int32_t zeroFactor;

    bool isAbsFactorEnabled;
    int32_t absFactor;
    
    int32_t rawData, dataToRead;
    uint8_t dataPin;

    int axisY;
    int axisHeight;
    int dataXCoord;
    
    int32_t lastValueX;

    bool active;
    uint16_t eepromAddress;
  
};
//--------------------------------------------------------------------------------------------------------------------------------------
typedef Vector<ScaleData*> ScaleDataVec;
//--------------------------------------------------------------------------------------------------------------------------------------
class ScalesClass
{
private:
  ScaleDataVec data;

  uint32_t updateTimer;

  void strobe();
  
public:

  ScalesClass();
  ~ScalesClass();

  size_t getDataCount() { return data.size(); }
  ScaleData* getData(size_t index) { return data[index]; }

  void setup();
  void update();

};
//--------------------------------------------------------------------------------------------------------------------------------------
extern ScalesClass Scales;
