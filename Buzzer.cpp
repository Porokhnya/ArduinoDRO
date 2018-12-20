#include "Buzzer.h"
#include "DueTimer.h"
//--------------------------------------------------------------------------------------------------------------------------------------
#ifdef USE_BUZZER
//--------------------------------------------------------------------------------------------------------------------------------------
BuzzerClass Buzzer;
//--------------------------------------------------------------------------------------------------------------------------------------
BuzzerClass::BuzzerClass()
{
}
//--------------------------------------------------------------------------------------------------------------------------------------
void BuzzerClass::begin()
{      
    pinMode(BUZZER_DRIVE_PIN,OUTPUT);
    digitalWrite(BUZZER_DRIVE_PIN,!BUZZER_ON);
    BUZZER_TIMER.attachInterrupt(buzzHandler);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void BuzzerClass::buzz(uint8_t _times, uint16_t durOn, uint16_t durOff)
{
  if(!_times) // ничего не передано для пищания
    return;
    
  currentLevel = BUZZER_ON;
  
  times = _times*2 - 1;    
  buzzLevel(currentLevel);

  durationOn = durOn;
  durationOff = durOff;
  
  double microsecs = durationOn;
  microsecs *= 1000;
  
  BUZZER_TIMER.setPeriod(microsecs);
  BUZZER_TIMER.start();
}
//--------------------------------------------------------------------------------------------------------------------------------------
void BuzzerClass::buzzLevel(uint8_t level)
{
   digitalWrite(BUZZER_DRIVE_PIN,level);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void BuzzerClass::buzzHandler()
{
  Buzzer.currentLevel = !Buzzer.currentLevel;


  if(Buzzer.times > 0)
    --Buzzer.times;

  Buzzer.buzzLevel(Buzzer.currentLevel);
  
  if(!Buzzer.times)
  {
    BUZZER_TIMER.stop();
  }
  else
  {
      double microsecs = Buzzer.durationOn;  
      if(!Buzzer.currentLevel)
      {
        microsecs = Buzzer.durationOff;
      }
    
      microsecs *= 1000;  
      BUZZER_TIMER.stop();
      BUZZER_TIMER.setPeriod(microsecs);    
      BUZZER_TIMER.start();
  }
  

}
//--------------------------------------------------------------------------------------------------------------------------------------
#endif // USE_BUZZER
