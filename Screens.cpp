//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "Screens.h"
#include "CONFIG.h"
#include "Buzzer.h"
#include "Scales.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// железные кнопки оси X
#if defined(USE_X_ABS_HARDWARE_BUTTON) && defined(USE_X_SCALE)
Button xAbsHardwareButton;
#endif

#if defined(USE_X_ZERO_HARDWARE_BUTTON) && defined(USE_X_SCALE)
Button xZeroHardwareButton;
#endif

// железные кнопки оси Y
#if defined(USE_Y_ABS_HARDWARE_BUTTON) && defined(USE_Y_SCALE)
Button yAbsHardwareButton;
#endif

#if defined(USE_Y_ZERO_HARDWARE_BUTTON) && defined(USE_Y_SCALE)
Button yZeroHardwareButton;
#endif

// железные кнопки оси Z
#if defined(USE_Z_ABS_HARDWARE_BUTTON) && defined(USE_Z_SCALE)
Button zAbsHardwareButton;
#endif

#if defined(USE_Z_ZERO_HARDWARE_BUTTON) && defined(USE_Z_SCALE)
Button zZeroHardwareButton;
#endif    
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// MainScreen
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MainScreen* Main = NULL;        
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MainScreen::MainScreen() : AbstractHALScreen()
{
  Main = this;
  buttons = new UTFT_Buttons_Rus(Screen.getUTFT(),
   Screen.getTouch()
  ,20   
  
  );
  buttons->setTextFont(SCREEN_BIG_FONT);
  buttons->setSymbolFont(Various_Symbols_32x32);
  buttons->setButtonColors(BUTTON_COLORS);

  buttonsCreated = false;

  #ifdef DEFAULT_IS_INCH
  measureMode = mmInch;
  #else
  measureMode = mmMM;
  #endif

  xMultiplier = yMultiplier = zMultiplier = 1;
  xRadDiaButton = yRadDiaButton = zRadDiaButton = infoButton = -1;
  wantRedrawDot = false;
  drawCalled = false;

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MainScreen::~MainScreen()
{
  delete buttons;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::onDeactivate()
{
  // станем неактивными
  drawCalled = false;

  // просим все оси перерисоваться при следующей активации экрана
  size_t total = Scales.getCount();
  for(size_t i=0;i<total;i++)
  {
    Scale* scale = Scales.getScale(i);
    memset(scale->FILLED_CHARS,-1,sizeof(scale->FILLED_CHARS));
  }  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::onActivate()
{
  // мы активизируемся
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::onButtonPressed(int btn)
{
  DBG(F("BUTTON PRESSED: "));
  DBGLN(btn);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::onEvent(Event event, void* param)
{
  if(!isActive())
    return;

    if(EventScaleDataChanged == event)
    {
      //DBGLN(F("EventScaleDataChanged"));
      Scale* scale = (Scale*) param;
      addToDrawQueue(scale);
    }
 
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::addToDrawQueue(Scale* scale)
{
  size_t to = wantsToDraw.size();
  for(size_t i=0;i<to;i++)
  {
    if(wantsToDraw[i].scale == scale)
      return;
  }

  ScaleDrawData dt; 
  dt.scale = scale; 
  dt.scaleData = scale->getRawData();
  
  wantsToDraw.push_back(dt);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::doSetup(HalDC* hal)
{

    xyzFontWidth = hal->getFontWidth(XYZFont);
    xyzFontHeight = hal->getFontHeight(XYZFont);

    screenWidth = hal->getScreenWidth();
    screenHeight = hal->getScreenHeight();
    
    xxlFontWidth = hal->getFontWidth(SevenSeg_XXXL_Num);
    xxlFontHeight = hal->getFontHeight(SevenSeg_XXXL_Num);

    uint8_t stored;
    if(Settings.read(RAD_DIA_BASE_STORE_ADDRESS,stored))
    {
      xMultiplier = stored;
      DBG(F("xMultiplier stored = "));
      DBGLN(xMultiplier);
    }
    if(Settings.read(RAD_DIA_BASE_STORE_ADDRESS+3,stored))
    {
      yMultiplier = stored;
      DBG(F("yMultiplier stored = "));
      DBGLN(yMultiplier);
    }
    if(Settings.read(RAD_DIA_BASE_STORE_ADDRESS+6,stored))
    {
      zMultiplier = stored;
      DBG(F("zMultiplier stored = "));
      DBGLN(zMultiplier);
    }

    #ifndef USE_X_RAD_DIA_BUTTON
      xMultiplier = 1;
    #endif

    #ifndef USE_Y_RAD_DIA_BUTTON
      yMultiplier = 1;
    #endif

    #ifndef USE_Z_RAD_DIA_BUTTON
      zMultiplier = 1;
    #endif
    
    

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::doUpdate(HalDC* hal)
{
  if(!isActive() || !drawCalled)
    return;

  // переименовываем кнопки, если надо
  size_t toRelabel = relabelQueue.size();
  if(toRelabel)
  {
    for(size_t i=0;i<toRelabel;i++)
    {
      RelabelQueueItem* it = &(relabelQueue[i]);
      buttons->relabelButton(it->btn,it->newLabel,true);
    }

    relabelQueue.empty();
  }

  // перерисовываем показания, если надо
  size_t sz = wantsToDraw.size();
  if(sz)
  {
      for(size_t i=0;i<sz;i++)
      {
        if(wantRedrawDot)
          drawDot(hal,wantsToDraw[i].scale);
          
        drawAxisData(hal,wantsToDraw[i].scale,wantsToDraw[i].scaleData);
      }
      wantsToDraw.empty();
      wantRedrawDot = false;

  } // if(sz)

  // проверяем на изменение значения у весов с последним отрисованным, при необходимости - перерисовываем
  sz = scaleLastDrawedData.size();
  for(size_t i=0;i<sz;i++)
  {

      Scale* scale = scaleLastDrawedData[i].scale;
      int32_t scaleLastData = scaleLastDrawedData[i].scaleData;
      int32_t scaleNowData = scale->getRawData();
      
      if(scaleNowData != scaleLastData)
      {
        ScaleDrawData needRedrawRecord;
        needRedrawRecord.scale = scale;
        needRedrawRecord.scaleData = scaleNowData;
        
        wantsToDraw.push_back(needRedrawRecord);
        
      //  DBG(F("CATCHED CHANGES AT AXIS "));
      //  DBGLN(scaleLastDrawedData[i].scale->getAxis());
      }
  }
  scaleLastDrawedData.empty();
  
    // проверяем экранные кнопки
    int clicked_button = buttons->checkButtons(ButtonPressed);
    if(clicked_button != -1)
    {
      // сообщаем, что у нас нажата кнопка
      hal->notifyAction(this);

      if(infoButton == clicked_button)
      {
        DBGLN(F("SHOW INFO BOX!"));
        Vector<const char*> lines;
        lines.push_back("Arduino DRO project.");
        lines.push_back("");
        lines.push_back("Автор: Порохня Дмитрий.");
        lines.push_back("Email: spywarrior@gmail.com.");
        lines.push_back("");
        lines.push_back("(c) 2018-2019");

        MessageBox->show("ИНФОРМАЦИЯ", lines,this);
      }
      
      #ifdef USE_MM_INCH_SWITCH
      else
      if(mmInchButton == clicked_button)
      {
        
        DBGLN(F("SWITCH MM/INCH!"));

        wantRedrawDot = true;
        
        if(measureMode == mmMM)
          measureMode = mmInch;
        else
          measureMode = mmMM;

        buttons->relabelButton(mmInchButton,measureMode == mmMM ? INCH_CAPTION : MM_CAPTION, true);
        redrawMeasureUnits(hal);

        // тут просим все оси перерисоваться, поскольку мы поменяли единицы измерения
        size_t total = Scales.getCount();
        for(size_t i=0;i<total;i++)
        {
          Scale* scale = Scales.getScale(i);
          //TODO: ТУТ ПОД ВОПРОСОМ - РАСКОММЕНТИРОВАТЬ ИЛИ НЕТ СЛЕДУЮЩУЮ СТРОЧКУ!!!
          memset(scale->FILLED_CHARS,-1,sizeof(scale->FILLED_CHARS));
          addToDrawQueue(scale);
        }

        
      } // else if(mmInchButton == clicked_button)
      #endif // USE_MM_INCH_SWITCH
      
      #if defined(USE_X_RAD_DIA_BUTTON)  && defined(USE_X_SCALE)
      else if(xRadDiaButton == clicked_button)
      {
        xMultiplier = xMultiplier == 1 ? 2 : 1;

        DBG(F("X multiplier: "));
        DBGLN(xMultiplier);

        Settings.write(RAD_DIA_BASE_STORE_ADDRESS,xMultiplier);

        if(xMultiplier == 1)
        {
          buttons->setButtonBackColor(xRadDiaButton,X_RAD_DIA_BUTTON_COLOR);
          buttons->setButtonFontColor(xRadDiaButton,X_RAD_DIA_BUTTON_FONT_COLOR);
          buttons->relabelButton(xRadDiaButton,X_RAD_CAPTION, true);                    
        }
        else
        {
          buttons->setButtonBackColor(xRadDiaButton,X_RAD_DIA_BUTTON_DIA_SELECTED_BUTTON_COLOR);
          buttons->setButtonFontColor(xRadDiaButton,X_RAD_DIA_BUTTON_DIA_SELECTED_FONT_COLOR);
          buttons->relabelButton(xRadDiaButton,X_DIA_CAPTION, true);
        }

        // просим ось X перерисоваться
        Scale* scale = Scales.getScale(akX);
        if(scale && scale->hasData())
          addToDrawQueue(scale);
          
      }
      #endif // USE_X_RAD_DIA_BUTTON
      
      #if defined(USE_Y_RAD_DIA_BUTTON)  && defined(USE_Y_SCALE)
      else if(yRadDiaButton == clicked_button)
      {
        yMultiplier = yMultiplier == 1 ? 2 : 1;

        DBG(F("Y multiplier: "));
        DBGLN(yMultiplier);

        Settings.write(RAD_DIA_BASE_STORE_ADDRESS+3,yMultiplier);
        
        if(yMultiplier == 1)
        {
          buttons->setButtonBackColor(yRadDiaButton,Y_RAD_DIA_BUTTON_COLOR);
          buttons->setButtonFontColor(yRadDiaButton,Y_RAD_DIA_BUTTON_FONT_COLOR);
          buttons->relabelButton(yRadDiaButton,Y_RAD_CAPTION, true);
          
        }
        else
        {
          buttons->setButtonBackColor(yRadDiaButton,Y_RAD_DIA_BUTTON_DIA_SELECTED_BUTTON_COLOR);
          buttons->setButtonFontColor(yRadDiaButton,Y_RAD_DIA_BUTTON_DIA_SELECTED_FONT_COLOR);
          buttons->relabelButton(yRadDiaButton,Y_DIA_CAPTION, true);          
        }        
        // просим ось Y перерисоваться
        Scale* scale = Scales.getScale(akY);
        if(scale && scale->hasData())
          addToDrawQueue(scale);

      }
      #endif // USE_Y_RAD_DIA_BUTTON 
           
      #if defined(USE_Z_RAD_DIA_BUTTON)  && defined(USE_Z_SCALE)
      else if(zRadDiaButton == clicked_button)
      {
        zMultiplier = zMultiplier == 1 ? 2 : 1;

        DBG(F("Z multiplier: "));
        DBGLN(zMultiplier);

        Settings.write(RAD_DIA_BASE_STORE_ADDRESS+6,zMultiplier);
        
        if(zMultiplier == 1)
        {
          buttons->setButtonBackColor(zRadDiaButton,Z_RAD_DIA_BUTTON_COLOR);
          buttons->setButtonFontColor(zRadDiaButton,Z_RAD_DIA_BUTTON_FONT_COLOR);
          buttons->relabelButton(zRadDiaButton,Z_RAD_CAPTION, true);
          
        }
        else
        {
          buttons->setButtonBackColor(zRadDiaButton,Z_RAD_DIA_BUTTON_DIA_SELECTED_BUTTON_COLOR);
          buttons->setButtonFontColor(zRadDiaButton,Z_RAD_DIA_BUTTON_DIA_SELECTED_FONT_COLOR);
          buttons->relabelButton(zRadDiaButton,Z_DIA_CAPTION, true);          
        }
        // просим ось Z перерисоваться
        Scale* scale = Scales.getScale(akZ);
        if(scale && scale->hasData())
          addToDrawQueue(scale);

      }
      #endif // USE_Z_RAD_DIA_BUTTON   
         
      else
      {        
          size_t total = Scales.getCount();
          for(size_t i=0;i<total;i++)
          {
            Scale* scale = Scales.getScale(i);
            if(scale->isActive())
            {
              if((scale->getAbsButtonIndex() == clicked_button)) // кнопка ABS
              {
                DBG(scale->getAxis());
                DBG(F(" CLICKED: "));
                DBGLN(scale->getAbsButtonCaption());
                switchABS(scale);
                break;
              }
              else
              if((scale->getZeroButtonIndex() == clicked_button)) // кнопка ZERO
              {
                DBG(scale->getAxis());
                DBG(F(" CLICKED: "));
                DBGLN(scale->getZeroButtonCaption());
                switchZERO(scale);
                break;
              }
              
            } // if(scale->isActive())
          } // for
          
      } // else

    } // if(clicked_button != -1)
    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t MainScreen::getMultiplier(Scale* scale)
{
  if(!scale)
    return 1;
    
  AxisKind kind = scale->getKind();
  switch(kind)
  {
    case akX:
    return xMultiplier;
    
    case akY:
    return yMultiplier;

    case akZ:
    return zMultiplier;
    
  }

  return 1;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::switchZERO(Scale* scale)
{
  if(!scale)
    return;
  
  ScaleFormattedData oldData = scale->getData(measureMode,getMultiplier(scale));
  scale->switchZERO();            
  if(scale->getData(measureMode,getMultiplier(scale)) != oldData)
  {
    // сбрасываем последние известные значения для оси
    memset(scale->FILLED_CHARS,-1,sizeof(scale->FILLED_CHARS));
    addToDrawQueue(scale);
  }

  RelabelQueueItem qi;
  qi.btn = scale->getZeroButtonIndex();
  
  if(scale->inZEROMode())
  {
    qi.newLabel = scale->getZeroButtonCaption();
  }
  else
  {
    qi.newLabel = scale->getRstZeroButtonCaption();
  }     

  relabelQueue.push_back(qi);
      
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::switchABS(Scale* scale)
{
  if(!scale)
    return;
  
  ScaleFormattedData oldData = scale->getData(measureMode,getMultiplier(scale));
  scale->switchABS();            
  if(scale->getData(measureMode,getMultiplier(scale)) != oldData)
  {
    // сбрасываем последние известные значения для оси
    memset(scale->FILLED_CHARS,-1,sizeof(scale->FILLED_CHARS));
    addToDrawQueue(scale);
  }

  RelabelQueueItem qi;
  qi.btn = scale->getAbsButtonIndex();

  if(scale->inABSMode())
  {
    //buttons->relabelButton(scale->getAbsButtonIndex(),scale->getAbsButtonCaption(),true);
    qi.newLabel = scale->getAbsButtonCaption();
  }
  else
  {
    //buttons->relabelButton(scale->getAbsButtonIndex(),scale->getRelButtonCaption(),true);              
    qi.newLabel = scale->getRelButtonCaption();
  }

  relabelQueue.push_back(qi);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::drawAxisData(HalDC* hal, Scale* scale, int32_t scaleData)
{
  ScaleDrawData dt; dt.scale = scale;
  dt.scaleData = scaleData;
  scaleLastDrawedData.push_back(dt);
  
  #if defined(_DEBUG) && defined(DEBUG_COMPUTE_DRAW_TIME)
    uint32_t startTime = millis();
  #endif

  FONT_TYPE* oldFont = hal->getFont();
  COLORTYPE oldColor = hal->getColor();
  COLORTYPE oldBackColor = hal->getBackColor();

  int axisY = scale->getY();
  int axisX = scale->getDataXCoord();

  hal->setBackColor(SCREEN_BACK_COLOR);
  hal->setFont(XYZFont);

  if(scaleData == NO_SCALE_DATA)
  {

    // нет данных с датчика
    // сбрасываем последние известные значения для оси
    memset(scale->FILLED_CHARS,-1,sizeof(scale->FILLED_CHARS));

    axisX -= (DIGITS_PLACES*xyzFontWidth + XYZ_FONT_DOT_WIDTH);
    int leftBorder = axisX;

    hal->setColor(scale->isActive() ? AXIS_NO_DATA_COLOR : INACTIVE_AXIS_COLOR);
     
    // рисуем целые
    int countWholePart = measureMode == mmMM ? DIGITS_PLACES - MM_RESOLUTION : DIGITS_PLACES - INCH_RESOLUTION;
    for(int i=0;i<countWholePart;i++)
    {
      hal->print("3",axisX,axisY); // строка вида "-" в используемом шрифте
      axisX += xyzFontWidth;
    } // for

    // рисуем точку
    drawDot(hal,scale);

    // рисуем дробные
    axisX += XYZ_FONT_DOT_WIDTH;

    int countFractPart = DIGITS_PLACES - countWholePart;
    for(int i=0;i<countFractPart;i++)
    {
      hal->print("3",axisX,axisY); // строка вида "-" в используемом шрифте
      axisX += xyzFontWidth;
    } // for

    // говорим, что якобы заняли последним значением все разряды, чтобы когда данные появятся - перерисовать всю шкалу
    scale->setLastValueX(leftBorder);

  } // if no data from scale
  else
  {

    // есть данные с датчика
    hal->setColor(AXIS_FRACT_VALUE_COLOR);
    hal->setFont(SevenSeg_XXXL_Num);
    
    ScaleFormattedData formattedData = scale->getData(measureMode, getMultiplier(scale), scaleData);
    static char curDigit[2] = {0};

    // выводим показания с датчика
    int countOfFractDigits = measureMode == mmMM ? MM_RESOLUTION : INCH_RESOLUTION;
    int strWidth = countOfFractDigits * xxlFontWidth;
    
    // дробные, N разрядов

    axisX -= strWidth;

    // отрисовываем сами дробные, их N штук
    uint32_t fractDelimiter = pow(10,countOfFractDigits-1);

    int fractPos = sizeof(scale->FILLED_CHARS) - countOfFractDigits;
  
    for(int i=0;i<countOfFractDigits;i++)
    {
      char lastDigit = scale->FILLED_CHARS[fractPos];
      uint32_t fractVal = formattedData.Fract/fractDelimiter % 10;
      
      formattedData.Fract -= fractVal*fractDelimiter;
      fractDelimiter /= 10;

      curDigit[0] = fractVal + '0';
      if(curDigit[0] != lastDigit)
      {
        // запоминаем
        scale->FILLED_CHARS[fractPos] = curDigit[0];

        // перерисовываем
        hal->print(curDigit,axisX,axisY);        
      }

      axisX += xyzFontWidth;
      fractPos++;

    } // for
    
    // перемещаемся перед точкой
    axisX -= (XYZ_FONT_DOT_WIDTH + strWidth);


    // целые
    
    hal->setColor(AXIS_WHOLE_VALUE_COLOR);
    
    // тут мы стоим за последним знаком целого значения.
    // нам надо отобразить все значащие разряды, и знак "минус", если значение отрицательное.
    // при этом разряд перерисовывается только тогда, когда он изменился с момента последней перерисовки.
    // у нас есть DIGITS_PLACES мест под разряды, плюс одно место - под опциональный минус.
        
    int32_t delimiter = 1;    
    int digitPos = sizeof(scale->FILLED_CHARS) - (countOfFractDigits+1); // указатель на текущий разряд
    uint32_t absVal = abs(formattedData.Value);

    // определяем разрядность числа
    int countDigits = formattedData.Value == 0 ? 1 : 0;
    uint32_t valCpy = absVal;
    while(valCpy != 0)
    {
      countDigits++;
      valCpy /= 10;
    }
    
    for(int z=0;z<countDigits;z++)
    {
      curDigit[0] = (absVal/delimiter % 10) + '0';
      delimiter *= 10;
      
      // перемещаемся на позицию цифры
      axisX -= xyzFontWidth;

      // проверяем, изменилась ли цифра разряда?
      if(scale->FILLED_CHARS[digitPos] != curDigit[0])
      {

        // запоминаем
        scale->FILLED_CHARS[digitPos] = curDigit[0];

        // перерисовываем
        hal->print(curDigit,axisX,axisY);

      }

      digitPos--;
      
    } // for
    

    // прошли по всем цифрам, теперь смотрим, не надо ли нарисовать минус
    if(formattedData.Value < 0)
    {
      axisX -= xyzFontWidth;
      
      if(scale->FILLED_CHARS[digitPos] != '-')
      {
        scale->FILLED_CHARS[digitPos] = '-';

        curDigit[0] = '3'; // символ "-" в используемом шрифте
        hal->setFont(XYZFont);
        hal->print(curDigit,axisX,axisY);
      }
    }

    // обнуляем все разряды слева
    while(digitPos > 0)
    {
      scale->FILLED_CHARS[digitPos--] = -1;
    }

    // теперь выясняем - если длина последних данных была больше, чем текущих - то закрашиваем прямоугольник слева
    int32_t axisLastValueX = scale->getLastValueX();  
    scale->setLastValueX(axisX);

    if(axisLastValueX < axisX)
    {
          
      // прошлое значение было левее!
      hal->setColor(SCREEN_BACK_COLOR);
      hal->fillRect(axisLastValueX, axisY,axisX,axisY + scale->getHeight());
    }    
    
    
  } // else

  hal->setFont(oldFont);
  hal->setColor(oldColor);
  hal->setBackColor(oldBackColor);


  #if defined(_DEBUG) && defined(DEBUG_COMPUTE_DRAW_TIME)
    uint32_t elapsed = millis() - startTime;

    DBG(scale->getAxis());
    DBG(F(" draw time (ms): "));
    DBGLN(elapsed);
  
  #endif

  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::redrawMeasureUnits(HalDC* hal)
{
  size_t total = Scales.getCount();
  
  for(size_t i=0;i<total;i++)
  {
    Scale* scale = Scales.getScale(i);
    // теперь рисуем единицы измерения
    hal->setColor(scale->isActive() ? AXIS_UNIT_COLOR : INACTIVE_AXIS_COLOR);
    hal->print(measureMode == mmMM ? "7" : "8",scale->getDataXCoord(),scale->getY());
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::drawGUI(HalDC* hal)
{
  if(!isActive())
    return;


    // рисуем подписи к осям
    hal->setBackColor(SCREEN_BACK_COLOR);
    hal->setFont(XYZFont);    
    

    // для начала выясняем, сколько у нас всего осей
    size_t total = Scales.getCount();

    // теперь выясняем, какую высоту займет одна из осей
    uint16_t axisHeight = xyzFontHeight;

    // теперь выясняем, сколько высоты займут все оси
    ////uint16_t totalHeight = axisHeight*total;

    // теперь выясняем, с какой позиции рисовать оси
    int curX = MAIN_SCREEN_LABELS_X_OFFSET;
    int curY = MAIN_SCREEN_LABELS_Y_OFFSET;//////(screenHeight - totalHeight - MAIN_SCREEN_AXIS_V_SPACING*(total-1)) / 2;


    if(!buttonsCreated)
    {
        int brfWidth = hal->getFontWidth(BigRusFont);       
        int cbLeft = MAIN_SCREEN_LABELS_Y_OFFSET;
        int cbTop = screenHeight - MAIN_SCREEN_LABELS_Y_OFFSET - MAIN_SCREEN_BOTTOM_BUTTONS_HEIGHT;
        
        #ifdef USE_MM_INCH_SWITCH
        // создаём кнопку mm/inch
        int mmInchButtonWidth = max(hal->print(MM_CAPTION,0,0,0,true),hal->print(INCH_CAPTION,0,0,0,true))*brfWidth + MAIN_SCREEN_BUTTON_TEXT_PADDING*2;
        mmInchButton = buttons->addButton(cbLeft,cbTop,mmInchButtonWidth,MAIN_SCREEN_BOTTOM_BUTTONS_HEIGHT,measureMode == mmMM ? INCH_CAPTION : MM_CAPTION);
        buttons->setButtonBackColor(mmInchButton,MM_INCH_SWITCH_BUTTON_COLOR);
        buttons->setButtonFontColor(mmInchButton,MM_INCH_SWITCH_BUTTON_FONT_COLOR);
        cbLeft += mmInchButtonWidth + MAIN_SCREEN_BUTTON_H_SPACING;
        #endif // USE_MM_INCH_SWITCH

    
        int xRadDiaButtonWidth = max(hal->print(X_RAD_CAPTION,0,0,0,true),hal->print(X_DIA_CAPTION,0,0,0,true))*brfWidth + MAIN_SCREEN_BUTTON_TEXT_PADDING*2;
        xRadDiaButton = buttons->addButton(cbLeft,cbTop,xRadDiaButtonWidth,MAIN_SCREEN_BOTTOM_BUTTONS_HEIGHT,xMultiplier == 2 ? X_DIA_CAPTION : X_RAD_CAPTION);
    #if defined(USE_X_RAD_DIA_BUTTON) && defined(USE_X_SCALE)
        buttons->setButtonBackColor(xRadDiaButton,xMultiplier == 2 ? X_RAD_DIA_BUTTON_DIA_SELECTED_BUTTON_COLOR : X_RAD_DIA_BUTTON_COLOR);
        buttons->setButtonFontColor(xRadDiaButton,xMultiplier == 2 ? X_RAD_DIA_BUTTON_DIA_SELECTED_FONT_COLOR : X_RAD_DIA_BUTTON_FONT_COLOR);
    #else
        buttons->disableButton(xRadDiaButton);
        buttons->setButtonBackColor(xRadDiaButton,SCREEN_BACK_COLOR);
    #endif
        cbLeft += xRadDiaButtonWidth + MAIN_SCREEN_BUTTON_H_SPACING;

        int yRadDiaButtonWidth = max(hal->print(Y_RAD_CAPTION,0,0,0,true),hal->print(Y_DIA_CAPTION,0,0,0,true))*brfWidth + MAIN_SCREEN_BUTTON_TEXT_PADDING*2;
        yRadDiaButton = buttons->addButton(cbLeft,cbTop,yRadDiaButtonWidth,MAIN_SCREEN_BOTTOM_BUTTONS_HEIGHT,yMultiplier == 2 ? Y_DIA_CAPTION : Y_RAD_CAPTION);
    #if defined(USE_Y_RAD_DIA_BUTTON) && defined(USE_Y_SCALE)
        buttons->setButtonBackColor(yRadDiaButton,yMultiplier == 2 ? Y_RAD_DIA_BUTTON_DIA_SELECTED_BUTTON_COLOR : Y_RAD_DIA_BUTTON_COLOR);
        buttons->setButtonFontColor(yRadDiaButton,yMultiplier == 2 ? Y_RAD_DIA_BUTTON_DIA_SELECTED_FONT_COLOR : Y_RAD_DIA_BUTTON_FONT_COLOR);
    #else        
        buttons->disableButton(yRadDiaButton);
        buttons->setButtonBackColor(yRadDiaButton,SCREEN_BACK_COLOR);
    #endif
        cbLeft += yRadDiaButtonWidth + MAIN_SCREEN_BUTTON_H_SPACING;

        int zRadDiaButtonWidth = max(hal->print(Z_RAD_CAPTION,0,0,0,true),hal->print(Z_DIA_CAPTION,0,0,0,true))*brfWidth + MAIN_SCREEN_BUTTON_TEXT_PADDING*2;
        zRadDiaButton = buttons->addButton(cbLeft,cbTop,zRadDiaButtonWidth,MAIN_SCREEN_BOTTOM_BUTTONS_HEIGHT,zMultiplier == 2 ? Z_DIA_CAPTION : Z_RAD_CAPTION);
    #if defined(USE_Z_RAD_DIA_BUTTON)  && defined(USE_Z_SCALE)
        buttons->setButtonBackColor(zRadDiaButton,zMultiplier == 2 ? Z_RAD_DIA_BUTTON_DIA_SELECTED_BUTTON_COLOR : Z_RAD_DIA_BUTTON_COLOR);
        buttons->setButtonFontColor(zRadDiaButton,zMultiplier == 2 ? Z_RAD_DIA_BUTTON_DIA_SELECTED_FONT_COLOR : Z_RAD_DIA_BUTTON_FONT_COLOR);
    #else
        buttons->disableButton(zRadDiaButton);
        buttons->setButtonBackColor(zRadDiaButton,SCREEN_BACK_COLOR);
    #endif
        cbLeft += zRadDiaButtonWidth + MAIN_SCREEN_BUTTON_H_SPACING;

        int infoButtonWidth = INFO_BUTTON_WIDTH;
        infoButton = buttons->addButton(screenWidth - infoButtonWidth - MAIN_SCREEN_LABELS_Y_OFFSET,cbTop,infoButtonWidth,MAIN_SCREEN_BOTTOM_BUTTONS_HEIGHT,"[", BUTTON_SYMBOL);
        buttons->setButtonBackColor(infoButton,INFO_BUTTON_COLOR);
        buttons->setButtonFontColor(infoButton,INFO_BUTTON_FONT_COLOR);
        
        
          
    } // !buttonsCreated


    // теперь рисуем подписи к осям
    for(size_t i=0;i<total;i++)
    {
        Scale* scale = Scales.getScale(i);

        hal->setColor(scale->isActive() ? AXIS_LABEL_COLOR : INACTIVE_AXIS_COLOR);
        hal->print(scale->getLabel(),curX,curY);

        if(!buttonsCreated) // кнопки не созданы, создаём
        {
          
          scale->setY(curY);
          scale->setHeight(axisHeight);
          
          // создаём кнопки оси
          const char* axisAbsButtonCaption = scale->getAbsButtonCaption();
          const char* axisZeroButtonCaption = scale->inZEROMode() ? scale->getZeroButtonCaption() : scale->getRstZeroButtonCaption();
          
          hal->setFont(SCREEN_BIG_FONT);
          int btnFontWidth = hal->getFontWidth(SCREEN_BIG_FONT);

          // ширина кнопки ABS, по максимальной длине возможных текстов на кнопке
          int absBtnWidth = max(hal->print(scale->getAbsButtonCaption(),0,0,0,true),hal->print(scale->getRelButtonCaption(),0,0,0,true))*btnFontWidth + MAIN_SCREEN_BUTTON_TEXT_PADDING*2;
          // ширина кнопки ZERO, по максимальной длине возможных текстов на кнопке
          int zeroBtnWidth = max(hal->print(scale->getZeroButtonCaption(),0,0,0,true),hal->print(scale->getRstZeroButtonCaption(),0,0,0,true))*btnFontWidth + MAIN_SCREEN_BUTTON_TEXT_PADDING*2;

          // текущий Y
          int btnY = curY + (axisHeight - MAIN_SCREEN_BUTTON_HEIGHT)/2;
          // текущий X
          int btnX = screenWidth - (absBtnWidth + zeroBtnWidth + MAIN_SCREEN_BUTTON_H_SPACING + MAIN_SCREEN_BUTTON_RIGHT_SPACING);
          int dataX = btnX;

          int absBtnIdx = buttons->addButton(btnX,btnY,absBtnWidth,MAIN_SCREEN_BUTTON_HEIGHT,axisAbsButtonCaption);
          scale->setAbsButtonIndex(absBtnIdx);

          btnX += absBtnWidth + MAIN_SCREEN_BUTTON_H_SPACING;
          int zeroBtnIdx = buttons->addButton(btnX,btnY,zeroBtnWidth,MAIN_SCREEN_BUTTON_HEIGHT,axisZeroButtonCaption);
          scale->setZeroButtonIndex(zeroBtnIdx);

          if(!scale->isActive()) // выключаем кнопки для неактивной оси
          {
            buttons->disableButton(absBtnIdx);
            buttons->setButtonBackColor(absBtnIdx,SCREEN_BACK_COLOR);

            buttons->disableButton(zeroBtnIdx);
            buttons->setButtonBackColor(zeroBtnIdx,SCREEN_BACK_COLOR);
            
          }

          scale->setDataXCoord(dataX - MAIN_SCREEN_DATA_X_OFFSET - xyzFontWidth);

          hal->setFont(XYZFont);     
        
        } // if(!buttonsCreated)

        // теперь рисуем единицы измерения
        hal->setColor(scale->isActive() ? AXIS_UNIT_COLOR : INACTIVE_AXIS_COLOR);
        hal->print(measureMode == mmInch ? "8" : "7" ,scale->getDataXCoord(),curY);

        // рисуем точку
          drawDot(hal,scale);

        // добавляем в очередь на отрисовку
        addToDrawQueue(scale);

        curY += axisHeight + MAIN_SCREEN_AXIS_V_SPACING;        
        
    } // for

  buttonsCreated = true;
  
  buttons->drawButtons(drawButtonsYield);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::drawDot(HalDC* hal, Scale* scale)
{
  hal->setColor(scale->isActive() ? AXIS_NO_DATA_COLOR : INACTIVE_AXIS_COLOR);

  hal->setFont(XYZFont);
  hal->print(
    #ifdef USE_COMMA_INSTEAD_OF_DOT 
    "5"
    #else
    "4"
    #endif
    ,scale->getDataXCoord() - xyzFontWidth*(measureMode == mmMM ? MM_RESOLUTION : INCH_RESOLUTION) - XYZ_FONT_DOT_WIDTH,scale->getY()); // строка "." в используемом шрифте  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::doDraw(HalDC* hal)
{
   hal->clearScreen();

   // тут отрисовка текущего состояния
   drawGUI(hal);

   hal->updateDisplay();
   drawCalled = true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

