#include "Scales.h"
#include "CONFIG.h"
#include "Events.h"
#include "Settings.h"
//--------------------------------------------------------------------------------------------------------------------------------------
bool ScaleFormattedData::operator==(const ScaleFormattedData& rhs)
{
    if(this == &rhs)
      return true;

  int8_t signThis = Value < 0 ? -1 : 1;
  int32_t rawThis = abs(Value)*100;
  rawThis += Fract;
  rawThis *= signThis;

  int8_t signRhs = rhs.Value < 0 ? -1 : 1;
  int32_t rawRhs = abs(rhs.Value)*100;
  rawRhs += rhs.Fract;
  rawRhs *= signRhs;

  return(rawThis == rawRhs);
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool ScaleFormattedData::operator!=(const ScaleFormattedData& rhs)
{
  return !(operator==(rhs));
}
//--------------------------------------------------------------------------------------------------------------------------------------
// Scale
//--------------------------------------------------------------------------------------------------------------------------------------
Scale::Scale(AxisKind _kind, uint16_t _eepromAddress, const char* _label, const char* _absButtonCaption, const char* _relButtonCaption, 
const char* _zeroButtonCaption, const char* _rstZeroButtonCaption, char _axis, uint8_t _dataPin,  bool _active)
{
   kind = _kind;
   eepromAddress = _eepromAddress;
   
   label = _label;
   absButtonCaption = _absButtonCaption;
   relButtonCaption = _relButtonCaption;
   zeroButtonCaption = _zeroButtonCaption;
   rstZeroButtonCaption = _rstZeroButtonCaption;

   zeroButtonIndex = -1;
   absButtonIndex = -1;

   isAbsFactorEnabled = false;
   absFactor = 0;

   isZeroFactorEnabled = false;
   zeroFactor = 0;


   dataPin = _dataPin;
   axis = _axis;
   rawData = NO_SCALE_DATA;
   axisY = -1;
   axisHeight = -1;
   dataXCoord = -1;
   

   active = _active;
   
   lastValueX = 5000;
   zeroFactorWantsToBeSaved = false;

   memset(FILLED_CHARS,-1,sizeof(FILLED_CHARS));
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::setup()
{
  //ТУТ НАСТРАИВАЕМ ПИНЫ
  pinMode(dataPin,INPUT);

  // читаем сохранённый ZERO-фактор
  DBG(F("Read saved ZERO factor for "));
  DBG(axis);
  DBG(F(" at address "));
  DBGLN(eepromAddress);
  
  if(Settings.read(eepromAddress,zeroFactor))
  {
    DBG(F("Saved ZERO factor for "));
    DBG(axis);
    DBG(F(" is: "));
    DBGLN(zeroFactor);

    isZeroFactorEnabled = (zeroFactor != 0);
  }
  else
  {
    DBG(F("No saved ZERO factor for "));
    DBGLN(axis);
  }

    #ifdef DEBUG_RANDOM_GENERATE_VALUES
      startValue = -abs(DEBUG_RANDOM_GENERATE_MAX);
    #endif  
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::update()
{
  if(zeroFactorWantsToBeSaved)
  {
    zeroFactorWantsToBeSaved = false;
    
    DBGLN(F("SAVE ZERO FACTOR !"));
    
    Settings.write(eepromAddress,zeroFactor);
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::switchZERO()
{
  if(!hasData())
    return;
    
  isZeroFactorEnabled = !isZeroFactorEnabled;
  
  if(isZeroFactorEnabled)
  {
    bool zFactorChanged = (zeroFactor != rawData);
    zeroFactor = rawData;

    if(zFactorChanged)
    {
      //Тут проблема: нас могут вызвать из прерывания, поэтому НЕЛЬЗЯ здесь писать в EEPROM!
      zeroFactorWantsToBeSaved = true; 
    }
        
  }
  else
  {
    if(zeroFactor != 0)
    {
      zeroFactor = 0;
      //Тут проблема: нас могут вызвать из прерывания, поэтому НЕЛЬЗЯ здесь писать в EEPROM!
      zeroFactorWantsToBeSaved = true;        
    }
    
  }  
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::switchABS()
{
  if(!hasData())
    return;
    
  isAbsFactorEnabled = !isAbsFactorEnabled;
  
  if(isAbsFactorEnabled)
  {
    absFactor = rawData;
  }
  else
  {
    absFactor = 0;
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::beginRead()
{
  if(!active)
    return;
      
  dataToRead = 0;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::endRead()
{
  if(!active)
    return;

  #ifdef DEBUG_RANDOM_GENERATE_VALUES
  
     startValue += DEBUG_RANDOM_GENERATE_STEP;
     
     if(startValue >= abs(DEBUG_RANDOM_GENERATE_MAX))
      startValue = -abs(DEBUG_RANDOM_GENERATE_MAX);
      
    dataToRead = startValue;
  #endif

  bool hasChanges = (dataToRead != rawData);
  rawData = dataToRead;
  dataToRead = 0;

  if(hasChanges)
  {
    Events.raise(this,EventScaleDataChanged,this);
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::readBit(int32_t bitNum, bool isLastBit)
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
        //TODO: Думаю, вот этой проверки делать не надо, поскольку ССЗБ, если датчик не подключил, да и подтяжка шины, по идее, должна быть к земле
        
        //if(dataToRead == 0xFFFFF) // в шине вообще все единицы !!!
        //{
        //  dataToRead = NO_SCALE_DATA;
        //}
        //else        
        //{        
          dataToRead |= ((int32_t)0x7FF << 21);
        //}

      // делаем проверку на минимальное и максимальное значение
      if(dataToRead < MIN_ALLOWED_SCALE_DATA)
        dataToRead = NO_SCALE_DATA;
        
      if(dataToRead > MAX_ALLOWED_SCALE_DATA)
        dataToRead = NO_SCALE_DATA;
        
        
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
ScaleFormattedData Scale::getData()
{
  ScaleFormattedData result;
  result.Value = NO_SCALE_DATA;
  result.Fract = 0;

  if(hasData())
  {
    int32_t temp = rawData;

    if(isZeroFactorEnabled)
    {
      temp -= zeroFactor;
    }
    
    if(isAbsFactorEnabled)
    {
      temp -= absFactor;
    }

    //TODO: ТУТ ФОРМАТИРУЕМ ДАННЫЕ, ПОКА СДЕЛАНО ПРОСТО ОТ БАЛДЫ, ТРЕБУЕТ ПРОВЕРКИ !!!
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
Scale* ScalesClass::getScale(AxisKind kind)
{
  size_t to = getCount();
  for(size_t i=0;i<to;i++)
  {
    Scale* scale = getScale(i);
    if(scale->getKind() == kind)
      return scale;
  }

  return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
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

    Scale* xData = new Scale(akX, ZERO_FACTOR_BASE_STORE_ADDRESS,
      #ifdef DISPLAY_COLON_AFTER_AXIS_NAME
      "06"
      #else
      "0"
      #endif
      
      ,X_SCALE_ABS_CAPTION,X_SCALE_REL_CAPTION,X_SCALE_ZERO_CAPTION,X_SCALE_RST_ZERO_CAPTION,'X',X_SCALE_DATA_PIN,
      
    #ifdef USE_X_SCALE
      true
    #else
      false
     #endif
      
    );
    xData->setup();
    data.push_back(xData);

    Scale* yData = new Scale(akY, ZERO_FACTOR_BASE_STORE_ADDRESS + 6,
      #ifdef DISPLAY_COLON_AFTER_AXIS_NAME
      "16"
      #else
      "1"
      #endif 
           
      ,Y_SCALE_ABS_CAPTION,Y_SCALE_REL_CAPTION,Y_SCALE_ZERO_CAPTION,Y_SCALE_RST_ZERO_CAPTION,'Y',Y_SCALE_DATA_PIN,
      
    #ifdef USE_Y_SCALE
      true
    #else
      false
     #endif
    
    );
    yData->setup();
    data.push_back(yData);

    Scale* zData = new Scale(akZ, ZERO_FACTOR_BASE_STORE_ADDRESS + 12,
      #ifdef DISPLAY_COLON_AFTER_AXIS_NAME
      "26"
      #else
      "2"
      #endif
      
      ,Z_SCALE_ABS_CAPTION,Z_SCALE_REL_CAPTION,Z_SCALE_ZERO_CAPTION,Z_SCALE_RST_ZERO_CAPTION,'Z',Z_SCALE_DATA_PIN,

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

    size_t cnt = data.size();    
    for(size_t i=0;i<cnt;i++)
    {
      data[i]->update();
    }

  if(millis() - updateTimer > SCALES_UPDATE_INTERVAL)
  {
    
    for(size_t i=0;i<cnt;i++)
    {
      if(data[i]->isActive())
        data[i]->beginRead();
    }

    for(int32_t bitNum=0;bitNum<21; bitNum++)
    {
        strobe();

        for(size_t i=0;i<cnt;i++)
        {
          if(data[i]->isActive())
            data[i]->readBit(bitNum, bitNum > 19);
        } // for

        delayMicroseconds(STROBE_DURATION);

    } // for

    for(size_t i=0;i<cnt;i++)
    {
      if(data[i]->isActive())
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

