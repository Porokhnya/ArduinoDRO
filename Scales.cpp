#include "Scales.h"
#include "CONFIG.h"
#include "Events.h"
//--------------------------------------------------------------------------------------------------------------------------------------
// ScaleData
//--------------------------------------------------------------------------------------------------------------------------------------
ScaleData::ScaleData(const char* _label, const char* _zeroButtonCaption, uint8_t _dataPin, uint8_t _clockPin, bool _active)
{
   label = _label;
   zeroButtonCaption = _zeroButtonCaption;
   dataPin = _dataPin;
   clockPin = _clockPin;
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
  //TODO: ТУТ НАСТРАИВАЕМ ПИНЫ!! 
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
void ScaleData::update()
{
  if(!active)
    return;
    
  int32_t lastData = rawData;
  
  //TODO: ТУТ АКТУАЛЬНОЕ ОБНОВЛЕНИЕ ДАННЫХ!!!
  rawData = random(-10000,100000);

  // проверяем - если что-то изменилось - то постим событие
  if(lastData != rawData)
  {
    Events.raise(this,EventScaleDataChanged,this);
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
    //TODO: ТУТ ФОРМАТИРУЕМ ДАННЫЕ, ПОКА СДЕЛАНО ПРОСТО ОТ БАЛДЫ !!!
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
void ScalesClass::setup()
{

    ScaleData* xData = new ScaleData("06",X_SCALE_ZERO_CAPTION,X_SCALE_DATA_PIN,X_SCALE_CLOCK_PIN,

    #ifdef USE_X_SCALE
      true
    #else
      false
     #endif
      
    );
    xData->setup();
    data.push_back(xData);

    ScaleData* yData = new ScaleData("16",Y_SCALE_ZERO_CAPTION,Y_SCALE_DATA_PIN,Y_SCALE_CLOCK_PIN,
    #ifdef USE_Y_SCALE
      true
    #else
      false
     #endif
    
    );
    yData->setup();
    data.push_back(yData);

    ScaleData* zData = new ScaleData("26",Z_SCALE_ZERO_CAPTION,Z_SCALE_DATA_PIN,Z_SCALE_CLOCK_PIN,

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
    for(size_t i=0;i<data.size();i++)
    {
      data[i]->update();
    }

    updateTimer = millis();
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------

