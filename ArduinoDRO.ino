#include "CONFIG.h"
#include "Events.h"
#include "ScreenHAL.h"
#include "Memory.h"
#include <Wire.h>
#include "Screens.h"
#include "Buzzer.h"
#include <malloc.h>
#include "Settings.h"
#include "Scales.h"
#include "DueTimer.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t screenIdleTimer = 0;
bool canCallYeld = false;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if (EEPROM_USED_MEMORY != EEPROM_BUILTIN)
void resetI2C(uint8_t sclPin, uint8_t sdaPin)
{
  pinMode(sdaPin, OUTPUT);
  digitalWrite(sdaPin,HIGH);
  pinMode(sclPin,OUTPUT);
  
  for(uint8_t i=0;i<10;i++) // Send NACK signal
  {
    digitalWrite(sclPin,HIGH);
    delayMicroseconds(5);
    digitalWrite(sclPin,LOW);
    delayMicroseconds(5);   
  }

  // Send STOP signal
  digitalWrite(sdaPin,LOW);
  delayMicroseconds(5);
  digitalWrite(sclPin,HIGH);
  delayMicroseconds(2);
  digitalWrite(sdaPin,HIGH);
  delayMicroseconds(2);
  
  
  pinMode(sclPin,INPUT);   
  pinMode(sdaPin,INPUT);   
}
#endif
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void screenAction(AbstractHALScreen* screen)
{
   // какое-то действие на экране произошло.
   // тут просто сбрасываем таймер ничегонеделанья.
   screenIdleTimer = millis();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef USE_EXTERNAL_WATCHDOG
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
  WAIT_FOR_TRIGGERED,
  WAIT_FOR_NORMAL 
} ExternalWatchdogState;

typedef struct
{
  uint16_t timer;
  ExternalWatchdogState state;
} ExternalWatchdogSettings;

ExternalWatchdogSettings watchdogSettings;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif // USE_EXTERNAL_WATCHDOG
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef USE_EXTERNAL_WATCHDOG
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void updateExternalWatchdog()
{
  static unsigned long watchdogLastMillis = millis();
  unsigned long watchdogCurMillis = millis();

  uint16_t dt = watchdogCurMillis - watchdogLastMillis;
  watchdogLastMillis = watchdogCurMillis;

      watchdogSettings.timer += dt;
      switch(watchdogSettings.state)
      {
        case WAIT_FOR_TRIGGERED:
        {
          if(watchdogSettings.timer >= WATCHDOG_WORK_INTERVAL)
          {
            watchdogSettings.timer = 0;
            watchdogSettings.state = WAIT_FOR_NORMAL;
            digitalWrite(WATCHDOG_REBOOT_PIN, WATCHDOG_TRIGGERED_LEVEL);
          }
        }
        break;

        case WAIT_FOR_NORMAL:
        {
          if(watchdogSettings.timer >= WATCHDOG_PULSE_DURATION)
          {
            watchdogSettings.timer = 0;
            watchdogSettings.state = WAIT_FOR_TRIGGERED;
            digitalWrite(WATCHDOG_REBOOT_PIN, WATCHDOG_NORMAL_LEVEL);
          }          
        }
        break;
      }  
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif // USE_EXTERNAL_WATCHDOG
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int freeRam()
{
    struct mallinfo mi = mallinfo();
    char* heapend = _sbrk(0);
    register char* stack_ptr asm("sp");

    return (stack_ptr - heapend + mi.fordblks);  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void pollButtons()
{
    bool isMainScreenActive = (Screen.getActiveScreen() == Main);
  
    // обновляем железные кнопки оси X
    #if defined(USE_X_ABS_HARDWARE_BUTTON) && defined(USE_X_SCALE)
      xAbsHardwareButton.update();
      if(xAbsHardwareButton.isClicked() && isMainScreenActive)
      {
        #ifdef USE_BUZZER
          Buzzer.buzz();
        #endif        
        
        Scale* scale = Scales.getScale(akX);
        Main->switchABS(scale);   
      }
    #endif    

      #if defined(USE_X_ZERO_HARDWARE_BUTTON) && defined(USE_X_SCALE)
      xZeroHardwareButton.update();
      if(xZeroHardwareButton.isClicked() && isMainScreenActive)
      {
        #ifdef USE_BUZZER
          Buzzer.buzz();
        #endif        

        Scale* scale = Scales.getScale(akX);
        Main->switchZERO(scale);   
      }      
    #endif 

    // обновляем железные кнопки оси Y
    #if defined(USE_Y_ABS_HARDWARE_BUTTON) && defined(USE_Y_SCALE)
      yAbsHardwareButton.update();
      if(yAbsHardwareButton.isClicked() && isMainScreenActive)
      {
        #ifdef USE_BUZZER
          Buzzer.buzz();
        #endif        
        
        Scale* scale = Scales.getScale(akY);
        Main->switchABS(scale);   
      }      
    #endif    

      #if defined(USE_Y_ZERO_HARDWARE_BUTTON) && defined(USE_Y_SCALE)
      yZeroHardwareButton.update();
      if(yZeroHardwareButton.isClicked() && isMainScreenActive)
      {
        #ifdef USE_BUZZER
          Buzzer.buzz();
        #endif        
        
        Scale* scale = Scales.getScale(akY);
        Main->switchZERO(scale);   
      }         
    #endif 


    // обновляем железные кнопки оси Z
    #if defined(USE_Z_ABS_HARDWARE_BUTTON) && defined(USE_Z_SCALE)
      zAbsHardwareButton.update();
      if(zAbsHardwareButton.isClicked() && isMainScreenActive)
      {
        #ifdef USE_BUZZER
          Buzzer.buzz();
        #endif        
        
        Scale* scale = Scales.getScale(akZ);
        Main->switchABS(scale);   
      }         
    #endif    

      #if defined(USE_Z_ZERO_HARDWARE_BUTTON) && defined(USE_Z_SCALE)
      zZeroHardwareButton.update();
      if(zZeroHardwareButton.isClicked() && isMainScreenActive)
      {
        #ifdef USE_BUZZER
          Buzzer.buzz();
        #endif        
        
        Scale* scale = Scales.getScale(akZ);
        Main->switchZERO(scale);   
      }          
    #endif   
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup() 
{
  
  canCallYeld = false;

   #if (EEPROM_USED_MEMORY != EEPROM_BUILTIN)
      resetI2C(SCL, SDA);
      Wire.begin();
   #endif
  
  MemInit();

  #if defined(_DEBUG) || defined(DUMP_SCALE_DATA_TO_SERIAL)
  Serial.begin(SERIAL_SPEED);
  #endif

  #ifdef USE_BUZZER
    Buzzer.begin();
  #endif
  

  #ifdef USE_EXTERNAL_WATCHDOG
    pinMode(WATCHDOG_REBOOT_PIN,OUTPUT);
    digitalWrite(WATCHDOG_REBOOT_PIN,WATCHDOG_NORMAL_LEVEL);
    watchdogSettings.timer = 0;
    watchdogSettings.state = WAIT_FOR_TRIGGERED;
  #endif

  DBGLN(F("Init scales..."));
  Scales.setup();

  DBGLN(F("Init buttons..."));

  bool anyButtonFound = false;
  
    // настраиваем железные кнопки оси X
    #if defined(USE_X_ABS_HARDWARE_BUTTON) && defined(USE_X_SCALE)
      xAbsHardwareButton.begin(X_ABS_HARDWARE_BUTTON_PIN,(X_ABS_HARDWARE_BUTTON_TRIGGERED_LEVEL == LOW));
      anyButtonFound = true;
    #endif
    
    #if defined(USE_X_ZERO_HARDWARE_BUTTON) && defined(USE_X_SCALE)
      xZeroHardwareButton.begin(X_ZERO_HARDWARE_BUTTON_PIN,(X_ZERO_HARDWARE_BUTTON_TRIGGERED_LEVEL == LOW));
      anyButtonFound = true;
    #endif
    

    // настраиваем железные кнопки оси Y
    #if defined(USE_Y_ABS_HARDWARE_BUTTON) && defined(USE_Y_SCALE)
      yAbsHardwareButton.begin(Y_ABS_HARDWARE_BUTTON_PIN,(Y_ABS_HARDWARE_BUTTON_TRIGGERED_LEVEL == LOW));
      anyButtonFound = true;
    #endif
    
    #if defined(USE_Y_ZERO_HARDWARE_BUTTON) && defined(USE_Y_SCALE)
      yZeroHardwareButton.begin(Y_ZERO_HARDWARE_BUTTON_PIN,(Y_ZERO_HARDWARE_BUTTON_TRIGGERED_LEVEL == LOW));
      anyButtonFound = true;
    #endif

    // настраиваем железные кнопки оси Z
    #if defined(USE_Z_ABS_HARDWARE_BUTTON) && defined(USE_Z_SCALE)
      zAbsHardwareButton.begin(Z_ABS_HARDWARE_BUTTON_PIN,(Z_ABS_HARDWARE_BUTTON_TRIGGERED_LEVEL == LOW));
      anyButtonFound = true;
    #endif
    
    #if defined(USE_Z_ZERO_HARDWARE_BUTTON) && defined(USE_Z_SCALE)
      zZeroHardwareButton.begin(Z_ZERO_HARDWARE_BUTTON_PIN,(Z_ZERO_HARDWARE_BUTTON_TRIGGERED_LEVEL == LOW));
      anyButtonFound = true;
    #endif
    
  DBGLN(F("Init screen..."));
  Screen.setup();

  Events.setup();

  canCallYeld = true;


  DBGLN(F("Add main screen...")); 
  Screen.addScreen(MainScreen::create()); // добавляем стартовый экран
  DBGLN(F("Main screen added.")); 

  
  screenIdleTimer = millis();
  Screen.onAction(screenAction);

  #ifdef USE_BUZZER
    // пискнем пищалкой при старте
    Buzzer.buzz(BUZZER_CONTROLLER_STARTED_COUNT);    
  #endif

  Screen.switchToScreen(Main);

  if(anyButtonFound)
  {
    HARDWARE_BUTTONS_TIMER.attachInterrupt(pollButtons).setPeriod(CHECK_BUTTONS_INTERVAL).start();
  }

  DBGLN(F("Inited."));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() 
{
   #ifdef USE_EXTERNAL_WATCHDOG
     updateExternalWatchdog();
   #endif // USE_EXTERNAL_WATCHDOG

  Screen.update();  
  Scales.update();


 // проверяем, какой экран активен. Если активен главный экран - сбрасываем таймер ожидания. Иначе - проверяем, не истекло ли время ничегонеделанья.
  AbstractHALScreen* activeScreen = Screen.getActiveScreen();
  if(activeScreen == Main)
  {
    screenIdleTimer = millis();
  }
  else
  {
      if(millis() - screenIdleTimer > RESET_TO_MAIN_SCREEN_DELAY)
      {
        screenIdleTimer = millis();
        Screen.switchToScreen(Main);
      }
  } // else   

   // выводим раз в 10 секунд информацию о свободной памяти, для теста
   #ifdef _DEBUG
    static uint32_t aliveTimer = millis();
    if(millis() - aliveTimer > 10000)
    {
      DBG(F("RAM: "));
      DBGLN(freeRam());

      aliveTimer = millis();
    }
   #endif

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool nestedYield = false;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void yield()
{
  
  if(nestedYield || !canCallYeld)
    return;

  nestedYield = true;
  
   #ifdef USE_EXTERNAL_WATCHDOG
     updateExternalWatchdog();
   #endif // USE_EXTERNAL_WATCHDOG

   Scales.update();

   nestedYield = false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------


