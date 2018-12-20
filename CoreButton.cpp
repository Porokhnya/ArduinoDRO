//--------------------------------------------------------------------------------------------------
#include "CoreButton.h"
//--------------------------------------------------------------------------------------------------
Button::Button()
{
  buttonPin = 0;
  pullUp = true;

  reset();
}
//--------------------------------------------------------------------------------------------------
void Button::begin(uint8_t _pin, bool _pullup)
{
   buttonPin = _pin;
   pullUp = _pullup;
  
  if(pullUp)
    pinMode(buttonPin, INPUT_PULLUP); // подтягиваем к питанию, если попросили

  reset();
}
//--------------------------------------------------------------------------------------------------
void Button::reset()
{

  state.atLeastOneStateChangesFound = false;

  state.click_down      = false;
  state.click_up        = false;
  state.doubleclick     = false;
  state.timer           = false;
  state.retention       = false;

  state.clickCounter  =     0;
  
  state.lastBounce  =      false;
  state.lastDoubleClick =  false;
  state.lastTimer  =        false;
  state.lastRetention  =    false;
  
  lastMillis  =      millis();

  state.lastButtonState  = readButtonState(buttonPin);
}
//--------------------------------------------------------------------------------------------------
void Button::update()
{

  // обновляем внутреннее состояние
  bool curBounce  = false;
  bool curDoubleClick = false;
  bool curTimer  = false;
  bool curRetention  = false;

  // сбрасываем все флаги
  state.click_down  = false;
  state.click_up    = false;
  state.doubleclick = false;
  state.timer       = false;
  state.retention   = false;

  uint32_t curMillis = millis();
  uint32_t millisDelta = curMillis - lastMillis;
  uint8_t curButtonState = readButtonState(buttonPin); // читаем текущее состояние

  // в зависимости от значения подтяжки выясняем состояние кнопки - нажата или нет
  bool isButtonPressed = pullUp ? !curButtonState : curButtonState;

 if (curButtonState != state.lastButtonState) // состояние изменилось
  {
    state.atLeastOneStateChangesFound = true; // было хотя бы одно изменение в состоянии (нужно для того, чтобы не было события "clicked", когда кнопку не нажимали ни разу)
    state.lastButtonState = curButtonState; // сохраняем его
    lastMillis = curMillis; // и время последнего обновления
  }

  if (millisDelta > BOUNCE_INTERVAL)  // надо проверить на дребезг
    curBounce = true;

  if (millisDelta > DOUBLECLICK_INTERVAL) // надо проверить на даблклик
    curDoubleClick = true;

  if (curDoubleClick != state.lastDoubleClick) // состояние даблклика с момента последней проверки изменилось
  {
    state.lastDoubleClick = curDoubleClick; // сохраняем текущее
    if (state.lastDoubleClick) // проверяем - если кнопка не нажата, то сбрасываем счётчик нажатий 
      state.clickCounter = 0;
  }

  if (curBounce != state.lastBounce) // состояние проверки дребезга изменилось
  {
    state.lastBounce = curBounce; // сохраняем текущее

    if (isButtonPressed && curBounce) // если кнопка была нажата в момент последнего замера и сейчас - значит, дребезг прошёл и мы можем сохранять состояние
    {
      state.click_down = true; // выставляем флаг, что кнопка нажата

      ++state.clickCounter; // увеличиваем счётчик кликов
      
      if (state.clickCounter == 2) // если кликнули два раза
      {
        state.clickCounter = 0;  // сбрасываем счётчик кликов
        state.doubleclick = true; // и выставляем флаг двойного нажатия
        
      }
    }
    state.click_up = !isButtonPressed && state.lastBounce && state.atLeastOneStateChangesFound; // кнопка отпущена тогда, когда последний замер и текущий - равны 1 (пин подтянут к питанию!), и был хотя бы один клик на кнопке
  }

  if (millisDelta > INACTIVITY_INTERVAL) // пора проверять неактивность
    curTimer = true;
    
  if (curTimer != state.lastTimer) // состояние неактивности изменилось с момента последнего замера?
  {
    state.lastTimer = curTimer; // сохраняем текущее
    state.timer = !isButtonPressed && state.lastTimer && state.atLeastOneStateChangesFound; // кнопка неактивна тогда, когда не была нажата с момента последнего опроса этого состояния

  }

  if (millisDelta > RETENTION_INTERVAL) // пора проверять удержание
    curRetention = true;

  if (curRetention != state.lastRetention) // если состояние изменилось
  {
    state.lastRetention = curRetention; // сохраняем его
    state.retention = isButtonPressed && state.lastRetention && state.atLeastOneStateChangesFound; // и считаем кнопку удерживаемой, когда она нажата сейчас и была нажата до этого
    
  }
  
}
//--------------------------------------------------------------------------------------------------

