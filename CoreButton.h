#pragma once
//--------------------------------------------------------------------------------------------------
#include <Arduino.h>
//--------------------------------------------------------------------------------------------------
#define BOUNCE_INTERVAL  70 // длительность отслеживания дребезга.
#define DOUBLECLICK_INTERVAL 200 // длительность отслеживания двойного клика.
#define INACTIVITY_INTERVAL 5000 // длительность отслеживания неактивности.
#define RETENTION_INTERVAL 2000 // длительность отслеживания нажатия и удержания.
//--------------------------------------------------------------------------------------------------
typedef struct
{
  bool  lastBounce : 1;
  bool  lastTimer : 1;
  bool  lastRetention : 1;
  bool click_down : 1; // нажата?
  bool click_up : 1; // нажата и отпущена?
  bool doubleclick : 1; // два раза нажата и отпущена?
  bool timer : 1; // неактивна в течение установленного интервала?
  bool retention : 1; // нажата и удерживается в течение установленного интервала?  

  uint8_t  lastButtonState : 1;
  bool atLeastOneStateChangesFound : 1;
  bool lastDoubleClick : 1;
  uint8_t clickCounter : 5;

  
} ButtonState;
//--------------------------------------------------------------------------------------------------
class Button
{
 public:
  
    Button();

    void begin(uint8_t _pin, bool _pullup=true); // _pullup == false - подтяжка к земле, иначе - подтяжка к питанию
    void update(); // обновляем внутреннее состояние
    void reset(); // сбрасываем состояние на неизменившееся
    
    bool isPressed() { return state.click_down; } // нажата?
    bool isClicked() { return state.click_up; } // нажата и отпущена?
    bool isDoubleClicked() { return state.doubleclick; } // дважды нажата и отпущена?
    bool isInactive() { return state.timer; } // неактивна в течение какого-то времени?
    bool isRetention() { return state.retention; } // удерживается в течение какого-то времени?

 private:
  
  byte buttonPin; // пин, на котором висит кнопка
  bool pullUp;
  
  uint32_t lastMillis;

  ButtonState state;


 protected:   
    
    uint8_t readButtonState(uint8_t pin)
    {
      return digitalRead(pin);
    }
    
 };
//--------------------------------------------------------------------------------------------------

