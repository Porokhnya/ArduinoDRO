#pragma once

#include <Arduino.h>
#include "CoreArray.h"
#include "CONFIG.h"
#include <limits.h>
//--------------------------------------------------------------------------------------------------------------------------------------
#define NO_SCALE_DATA INT_MIN // значение, которое устанавливается, если с датчика нет данных
//--------------------------------------------------------------------------------------------------------------------------------------
struct ScaleFormattedData
{
  int32_t Value;
  uint8_t Fract;

  bool operator==(const ScaleFormattedData& rhs);
  bool operator!=(const ScaleFormattedData& rhs);
  
};
//--------------------------------------------------------------------------------------------------------------------------------------
class ScalesClass;
//--------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
  akX,
  akY,
  akZ
  
} AxisKind;
//--------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
  mmMM,
  mmInch
  
} MeasureMode;
//--------------------------------------------------------------------------------------------------------------------------------------
class Scale
{
  public:

    Scale(AxisKind kind, uint16_t eepromAddress, const char* label, const char* absButtonCaption, const char* relButtonCaption,const char* zeroButtonCaption, const char* rstZeroButtonCaption, char axis, uint8_t dataPin, bool active);

    void setup();
    void update();

    AxisKind getKind() { return kind; }
   
    const char* getLabel() { return label; }

    bool hasData() { return active && (rawData != NO_SCALE_DATA); }
    ScaleFormattedData getData(MeasureMode mode, uint8_t multiplier);
    int32_t getRawData() { return rawData; }

    char getAxis() { return axis; }
    
    const char* getAbsButtonCaption() { return absButtonCaption; }
    const char* getRelButtonCaption() { return relButtonCaption; }
    const char* getZeroButtonCaption() { return zeroButtonCaption; }
    const char* getRstZeroButtonCaption() { return rstZeroButtonCaption; }
    
    void setAbsButtonIndex(int8_t idx) { absButtonIndex = idx; }
    int8_t getAbsButtonIndex() { return absButtonIndex; }

    void setZeroButtonIndex(int8_t idx) { zeroButtonIndex = idx; }
    int8_t getZeroButtonIndex() { return zeroButtonIndex; }    

    int32_t getLastValueX() { return lastValueX; }
    void setLastValueX(int32_t val) { lastValueX = val; }
    
    void switchABS();
    bool inABSMode() { return !isAbsFactorEnabled; }

    void switchZERO();
    bool inZEROMode() { return !isZeroFactorEnabled; }

    void setY(int val) { axisY = val; }
    int getY() { return axisY; }

    void setHeight(int val) { axisHeight = val; }
    int getHeight() { return axisHeight; }

    void setDataXCoord(int val) { dataXCoord = val; }
    int getDataXCoord() { return dataXCoord; }

    bool isActive() { return active; }

    char FILLED_CHARS[DIGIT_PLACES+3]; // последные известные значения разрядов

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
    const char* rstZeroButtonCaption;
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
    AxisKind kind;
    bool zeroFactorWantsToBeSaved;

    #ifdef DEBUG_RANDOM_GENERATE_VALUES
      int32_t startValue;
    #endif
    
  
};
//--------------------------------------------------------------------------------------------------------------------------------------
typedef Vector<Scale*> ScaleVec;
//--------------------------------------------------------------------------------------------------------------------------------------
class ScalesClass
{
private:
  ScaleVec data;

  uint32_t updateTimer;

  void strobe();
  
public:

  ScalesClass();
  ~ScalesClass();

  size_t getCount() { return data.size(); }
  Scale* getScale(size_t index) { return data[index]; }
  Scale* getScale(AxisKind kind);
  

  void setup();
  void update();

};
//--------------------------------------------------------------------------------------------------------------------------------------
extern ScalesClass Scales;
