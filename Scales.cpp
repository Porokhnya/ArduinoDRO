#include "Scales.h"
#include "CONFIG.h"
#include "Events.h"
//--------------------------------------------------------------------------------------------------------------------------------------
// ScaleData
//--------------------------------------------------------------------------------------------------------------------------------------
ScaleData::ScaleData(const char* _label, const char* _zeroButtonCaption, char _axis, uint8_t _dataPin,  bool _active)
{
   label = _label;
   zeroButtonCaption = _zeroButtonCaption;
   dataPin = _dataPin;
   axis = _axis;
   rawData = NO_SCALE_DATA;
   zeroButtonIndex = -1;
   axisY = -1;
   axisHeight = -1;
   dataXCoord = -1;
   isZeroed = false;
   zeroFactor = 0;

   active = _active;
   
   lastValueX = 5000;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void ScaleData::setup()
{
  //ТУТ НАСТРАИВАЕМ ПИНЫ
  pinMode(dataPin,INPUT);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void ScaleData::zeroAxis()
{
  if(!hasData())
    return;
    
  //обнуляем значение оси
  isZeroed = !isZeroed;
  if(isZeroed)
  {
    zeroFactor = rawData;
    DBG(F("Zero factor set to: "));
    DBGLN(zeroFactor);
  }
  else
  {
    DBGLN(F("Zero factor cleared!"));
    zeroFactor = 0;
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
void ScaleData::beginRead()
{
if(!active)
    return;
      
  dataToRead = 0;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void ScaleData::endRead()
{
  if(!active)
    return;

  bool hasChanges = (dataToRead != rawData);
  rawData = dataToRead;
  dataToRead = 0;

  if(hasChanges)
  {
    Events.raise(this,EventScaleDataChanged,this);
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
void ScaleData::readBit(int32_t bitNum, bool isLastBit)
{
  if(!active)
    return;

  int32_t readed = digitalRead(dataPin);
  
  if(!isLastBit)
  {
    dataToRead |= ((readed == BIT_IS_SET_LEVEL ? 1 : 0) << bitNum);
  }
  else
  {    

  /*
    DBG("dataPin=");
    DBG(dataPin);
    DBG("; RAW value=");
    Serial.print(dataToRead,HEX);
    DBG("; mask=");
    Serial.println(((int32_t)0x7FF << 21),HEX);
   */
     if(readed == BIT_IS_SET_LEVEL)
     {
        if(dataToRead == 0xFFFFF) // в шине вообще все единицы !!!
        {
          dataToRead = NO_SCALE_DATA;
        }
        else
        {
          dataToRead |= ((int32_t)0x7FF << 21);
        }
     }

    /*
    DBG("dataPin=");
    DBG(dataPin);
    DBG("; value=");
    Serial.println(dataToRead,HEX);
    */
    
    
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
ScaleFormattedData ScaleData::getData()
{
  ScaleFormattedData result;
  result.Value = NO_SCALE_DATA;
  result.Fract = 0;

  if(hasData())
  {
    //TODO: ТУТ ФОРМАТИРУЕМ ДАННЫЕ, ПОКА СДЕЛАНО ПРОСТО ОТ БАЛДЫ, ТРЕБУЕТ ПРОВЕРКИ !!!
    int32_t temp = rawData;
    if(isZeroed)
    {
      temp -= zeroFactor;
    }

    result.Value = temp/100;
    result.Fract = abs(temp%100);
  }
  
  return result;
}
//--------------------------------------------------------------------------------------------------------------------------------------
// ScalesClass
//--------------------------------------------------------------------------------------------------------------------------------------
ScalesClass Scales;
//--------------------------------------------------------------------------------------------------------------------------------------
ScalesClass::ScalesClass()
{
  updateTimer = 0;
}
//--------------------------------------------------------------------------------------------------------------------------------------
ScalesClass::~ScalesClass()
{
  for(size_t i=0;i<data.size();i++)
  {
    delete data[i]; 
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
void ScalesClass::strobe()
{
  digitalWrite(SCALE_CLOCK_PIN, STROBE_HIGH_LEVEL);

  delayMicroseconds(STROBE_DURATION);
  
  digitalWrite(SCALE_CLOCK_PIN, !STROBE_HIGH_LEVEL);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void ScalesClass::setup()
{

    // настраиваем пин строба
    pinMode(SCALE_CLOCK_PIN,OUTPUT);

    ScaleData* xData = new ScaleData("06",X_SCALE_ZERO_CAPTION,'X',X_SCALE_DATA_PIN,

    #ifdef USE_X_SCALE
      true
    #else
      false
     #endif
      
    );
    xData->setup();
    data.push_back(xData);

    ScaleData* yData = new ScaleData("16",Y_SCALE_ZERO_CAPTION,'Y',Y_SCALE_DATA_PIN,
    #ifdef USE_Y_SCALE
      true
    #else
      false
     #endif
    
    );
    yData->setup();
    data.push_back(yData);

    ScaleData* zData = new ScaleData("26",Z_SCALE_ZERO_CAPTION,'Z',Z_SCALE_DATA_PIN,

    #ifdef USE_Z_SCALE
      true
    #else
      false
     #endif
    
    );
    zData->setup();
    data.push_back(zData);

}
//--------------------------------------------------------------------------------------------------------------------------------------
void ScalesClass::update()
{
  if(millis() - updateTimer > SCALES_UPDATE_INTERVAL)
  {
    size_t cnt = data.size();
    
    for(size_t i=0;i<cnt;i++)
    {
      data[i]->beginRead();
    }

    for(int32_t bitNum=0;bitNum<21; bitNum++)
    {
        strobe();

        for(size_t i=0;i<cnt;i++)
        {
          data[i]->readBit(bitNum, bitNum > 19);
        } // for

    } // for

    for(size_t i=0;i<cnt;i++)
    {
      data[i]->endRead();
    }

    #ifdef DUMP_SCALE_DATA_TO_SERIAL
    
      for(size_t i=0;i<cnt;i++)
      {
        if(i>0)
          Serial.print(';');
          
        Serial.print(data[i]->getAxis());
        if(data[i]->hasData())
          Serial.print(data[i]->getRawData());
        else
          Serial.print('_');
      }
      Serial.println();
          
    #endif // DUMP_SCALE_DATA_TO_SERIAL

    updateTimer = millis();
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------

