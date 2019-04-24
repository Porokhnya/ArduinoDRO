#include "Scales.h"
#include "CONFIG.h"
#include "Events.h"
#include "Settings.h"
//--------------------------------------------------------------------------------------------------------------------------------------
bool ScaleFormattedData::operator==(const ScaleFormattedData& rhs)
{
    if(this == &rhs)
      return true;

  return (this->Value == rhs.Value) && (this->Fract == rhs.Fract);
}
//--------------------------------------------------------------------------------------------------------------------------------------
bool ScaleFormattedData::operator!=(const ScaleFormattedData& rhs)
{
  return !(operator==(rhs));
}
//--------------------------------------------------------------------------------------------------------------------------------------
// Scale
//--------------------------------------------------------------------------------------------------------------------------------------
InterruptRecord scaleInterrupts[3] = {
  {NULL, Scale::int1},
  {NULL, Scale::int2},
  {NULL, Scale::int3},
};
//--------------------------------------------------------------------------------------------------------------------------------------
Scale::Scale(AxisKind _kind, uint16_t _eepromAddress, const char* _label, const char* _absButtonCaption, const char* _relButtonCaption, 
const char* _zeroButtonCaption, const char* _rstZeroButtonCaption, char _axis, uint8_t _dataPin,  uint8_t _clockPin, uint8_t _scaleType, bool _active)
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

    wantNextCaliperBit = false;
    caliperBitNumber = 0;
    caliperValueSign = 1;

   isAbsFactorEnabled = false;
   absFactor = 0;

   isZeroFactorEnabled = false;
   zeroFactor = 0;


   dataPin = _dataPin;
   clockPin = _clockPin;
   scaleType = _scaleType;
   
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
void Scale::int1()
{
  Scale::wantReadBit_CaliperProtocol(0);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::int2()
{
  Scale::wantReadBit_CaliperProtocol(1);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::int3()
{
  Scale::wantReadBit_CaliperProtocol(2);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::wantReadBit_CaliperProtocol(uint8_t recIndex)
{
  if(scaleInterrupts[recIndex].scale)
  {
    scaleInterrupts[recIndex].scale->wantNextBit_CaliperProtocol();
  }
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::wantNextBit_CaliperProtocol()
{
  //взводим флаг, что надо прочитать следующий бит, поскольку пойман спад фронта
  wantNextCaliperBit = true;
}
//--------------------------------------------------------------------------------------------------------------------------------------
uint8_t Scale::readNextBit_CaliperProtocol()
{
  uint8_t result = 0;

  // читаем 3 раза, чтобы точно попасть в уровень  
  for(uint8_t i=0;i<3;i++)
    result += digitalRead(dataPin);

  return result > 0 ? 1 : 0;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::setup()
{
  //ТУТ НАСТРАИВАЕМ ПИНЫ
  pinMode(dataPin,INPUT);

  switch(scaleType)
  {
    case PROTOCOL_21_BIT:
      pinMode(clockPin,OUTPUT);
    break;

    case PROTOCOL_CALIPER:
    {
      //pinMode(clockPin,INPUT);
      
      // настраиваем вектора прерываний
      uint8_t recIndex = 0;
      for(;recIndex < 3; recIndex++)
      {
        if(!scaleInterrupts[recIndex].scale) // нашли свободное место в таблице обработчиков
        {
          scaleInterrupts[recIndex].scale = this;
          attachInterrupt(digitalPinToInterrupt(clockPin),scaleInterrupts[recIndex].func,FALLING);
          break;
        }
      } // for
      
    }
    break;
  }

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
  if(active) // читаем данные только тогда, когда активны
  {
    switch(scaleType) // в зависимости от протокола - читаем так, или иначе
    {
      case PROTOCOL_21_BIT: // Чтение протокола 21-бит вызывается само через нужные интервалы, мы вручную тактируем и читаем, поэтому здесь ничего делать - не надо
      {
        // NOP
      }
      break; // PROTOCOL_21_BIT
  
      case PROTOCOL_CALIPER:
      {
          //TODO: здесь проверяем, надо ли читать следующий бит
          if(wantNextCaliperBit)
          {
            // произошло прерывание по спаду фронта на линии тактирования, нам надо прочитать линию данных.
            // первый бит - мы всегда игнорируем, в 21-м бите - знак (0 => +, 1 => -), в 24-м бите - единицы измерения (0 => mm, 1 => inch).
            // данные передаются младшим битом вперёд, в сотых долях миллиметра.          
            
            wantNextCaliperBit = false; // сначала сбрасываем флаг необходимости прочесть следующий бит
  
            DBG(F("Want to read caliper bit #"));
            DBGLN(caliperBitNumber);
            
            caliperBitNumber++;
  
            // прочитали значение бита
            uint8_t reading = readNextBit_CaliperProtocol();
  
            if(caliperBitNumber < 2) // первый бит, он всегда 1, мы его игнорируем, здесь просто подготавливаем переменные к дальнейшему чтению
            {
              DBG(F("FIRST BIT, VALUE="));
              DBGLN(reading);
              
              dataToRead = 0; // обнуляем результат, куда будем читать
              caliperValueSign = 1; // обнуляем знак
            }
            else
            {
               // следующие биты, читаем
               
               if(caliperBitNumber == 21) // бит знака
               {                 
                 if(reading)
                  caliperValueSign = -1;
                 else
                  caliperValueSign = 1;
  
                 DBG(F("SIGN BIT: "));
                 DBGLN(caliperValueSign);
                                 
               } // if(caliperBitNumber == 21)
               else
               if(caliperBitNumber == 24) // последний бит
               {
                  // последний бит, там единицы измерения, пока мы его тупо игнорируем, т.к. у нас линейка, а не штангенциркуль.
                  // но в любом случае - на этом бите надо сбрасывать счётчик бит, и сигнализировать об изменениях.
  
                  caliperBitNumber = 0; // обнуляем счётчик бит, в следующий раз читать начнём сначала
  
                  // рассчитываем окончательное значение c учётом знака
                  dataToRead *= caliperValueSign;
                  caliperValueSign = 1;
  
                  // выводим его для теста в Serial
                  DBG(F("LAST BIT, caliper value: "));
                  DBGLN(dataToRead);
  
                    bool hasChanges = (dataToRead != rawData);
                    rawData = dataToRead;
                    dataToRead = 0;
                  
                    if(hasChanges) // были изменения, сигнализируем
                    {
                      Events.raise(this,EventScaleDataChanged,this);
                    }                
                
               } // if(caliperBitNumber == 24)
               else
               {
                 // тут просто читаем биты и складываем их в чиселку, не забывая про LSB, т.е. младший бит - идёт в данных первым
                 dataToRead |= ( reading << (caliperBitNumber-1) );
               }
               
            } // else
          } // if(wantNextCaliperBit)
      }
      break; // PROTOCOL_CALIPER
      
    }  // switch
  } // if(active)


  // проверяем, не надо ли сохранить фактор нуля, делаем это в самом конце, потому что запись в EEPROM - мееедленная.
  // делаем это также только тогда, когда все данные с линейки прочитаны, чтобы не пропустить значащих бит.
  if(zeroFactorWantsToBeSaved && caliperBitNumber < 1)
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
void Scale::read()
{
  if(!active) // выключены, пустышка
    return;

  switch(scaleType) // в зависимости от протокола - читаем так, или иначе
  {
    case PROTOCOL_21_BIT: // читаем протокол 21-бит, НЕ ТЕСТИРОВАНО ЗА НЕИМЕНИЕМ ТАКОЙ ЛИНЕЙКИ !!!
    {
        beginRead_21BitProtocol();
        
        for(int32_t bitNum=0;bitNum<21; bitNum++)
            {
              strobe_21BitProtocol();
              readBit_21BitProtocol(bitNum, bitNum > 19);
              delayMicroseconds(STROBE_DURATION);        
            } // for      
            
        endRead_21BitProtocol();
    }
    break; // PROTOCOL_21_BIT

    case PROTOCOL_CALIPER: // ЧТЕНИЕ ПРОТОКОЛА КИТАЙСКИХ ШТАНГЕНОВ - ИДЁТ ПО ФАКТУ ПРЕРЫВАНИЯ ПО СПАДУ ФРОНТА
    {
        // NOP
    }
    break; // PROTOCOL_CALIPER
    
  }  // switch
      
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::strobe_21BitProtocol()
{
    digitalWrite(clockPin, STROBE_HIGH_LEVEL);
    delayMicroseconds(STROBE_DURATION);  
    digitalWrite(clockPin, !STROBE_HIGH_LEVEL);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::beginRead_21BitProtocol()
{      
  dataToRead = 0;
}
//--------------------------------------------------------------------------------------------------------------------------------------
void Scale::endRead_21BitProtocol()
{
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
void Scale::readBit_21BitProtocol(int32_t bitNum, bool isLastBit)
{
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
ScaleFormattedData Scale::getData(MeasureMode mode, uint8_t multiplier)
{
  ScaleFormattedData result;
  result.Value = NO_SCALE_DATA;
  result.Fract = 0;

  if(hasData())
  {
    int32_t temp = rawData;

    //TODO: УДАЛИТЬ !!!!
    // if(getKind() == akX)
    //  temp = 123456;

    if(isZeroFactorEnabled)
    {
      temp -= zeroFactor;
    }
    
    if(isAbsFactorEnabled)
    {
      temp -= absFactor;
    }

    // с линейки у нас всё лежит в сотых долях миллиметра
    float fTemp = temp;
    fTemp /= 100.0;

    if(mode == mmInch)
    {
      // переводим в дюймы
      fTemp /= 25.4;
    }

    // применяем мультипликатор
    fTemp *= multiplier;

    // у нас результат вычисления лежит в сотых долях.
    // его нужно перевести в требуемые доли

    int32_t computed = fTemp;
    result.Value = computed;

    // теперь дробную часть, с нужным кол-вом знаков
    uint32_t resolution = ( mode == mmMM ? pow(10,MM_RESOLUTION) : pow(10,INCH_RESOLUTION) );

    fTemp -= computed;
    fTemp *= resolution;

    result.Fract  = abs(fTemp);    
    
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
void ScalesClass::setup()
{
  
    Scale* xData = new Scale(akX, ZERO_FACTOR_BASE_STORE_ADDRESS,
      #ifdef DISPLAY_COLON_AFTER_AXIS_NAME
      "06"
      #else
      "0"
      #endif
      
      ,X_SCALE_ABS_CAPTION,X_SCALE_REL_CAPTION,X_SCALE_ZERO_CAPTION,X_SCALE_RST_ZERO_CAPTION,'X',X_SCALE_DATA_PIN,X_SCALE_CLOCK_PIN,X_SCALE_TYPE,
      
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
           
      ,Y_SCALE_ABS_CAPTION,Y_SCALE_REL_CAPTION,Y_SCALE_ZERO_CAPTION,Y_SCALE_RST_ZERO_CAPTION,'Y',Y_SCALE_DATA_PIN,Y_SCALE_CLOCK_PIN,Y_SCALE_TYPE,
      
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
      
      ,Z_SCALE_ABS_CAPTION,Z_SCALE_REL_CAPTION,Z_SCALE_ZERO_CAPTION,Z_SCALE_RST_ZERO_CAPTION,'Z',Z_SCALE_DATA_PIN,Z_SCALE_CLOCK_PIN,Z_SCALE_TYPE,

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
        data[i]->read();
    }


    // если попросили выводить сырые данные - выводим их в Serial раз в N времени
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
    
  } // if(millis() - updateTimer > SCALES_UPDATE_INTERVAL)
}
//--------------------------------------------------------------------------------------------------------------------------------------

