//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "Screens.h"
#include "CONFIG.h"
#include "Buzzer.h"
#include "Scales.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawScreenCaption(HalDC* hal, const String& str) // рисуем заголовок экрана
{
  int screenWidth = hal->getScreenWidth();
  hal->setFont(SCREEN_BIG_FONT);
  int fontWidth = hal->getFontWidth(SCREEN_BIG_FONT);
  int fontHeight = hal->getFontHeight(SCREEN_BIG_FONT);
  int top = 10;

  // подложка под заголовок
  hal->setColor(VGA_NAVY);
  hal->fillRect(0, 0, screenWidth-1, top*2 + fontHeight);
  
  hal->setBackColor(VGA_NAVY);
  hal->setColor(VGA_WHITE); 
   
  int strLen = hal->print(str.c_str(),0,0,0,true);

  int left = (screenWidth - fontWidth*strLen)/2;

  hal->print(str.c_str(),left,top);  

  hal->setBackColor(SCREEN_BACK_COLOR);
  hal->setColor(SCREEN_TEXT_COLOR);
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// MainScreen
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MainScreen* Main = NULL;        
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MainScreen::MainScreen() : AbstractHALScreen()
{
  Main = this;
  buttons = new UTFT_Buttons_Rus(Screen.getUTFT(),

  #ifdef USE_TOUCH
    Screen.getTouch()
  #else
    NULL
  #endif
  ,10   
  
  );
  buttons->setTextFont(SCREEN_BIG_FONT);
  buttons->setButtonColors(BUTTON_COLORS);

/*
  xZeroButton = -1;
  yZeroButton = -1;
  zZeroButton = -1;
*/  
  buttonsCreated = false;
  

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
      ScaleData* scale = (ScaleData*) param;
      //drawAxisData(&Screen,scale);
      addToDrawQueue(scale);
    }
 
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::addToDrawQueue(ScaleData* dt)
{
  for(size_t i=0;i<wantsToDraw.size();i++)
  {
    if(wantsToDraw[i] == dt)
      return;
  }

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

    noDataString.reserve(DIGIT_PLACES);
    for(int i=0;i<DIGIT_PLACES;i++)
    {
      noDataString += '3';
    }
    // мы используем N позиций под целые, две - под дробные, плюс - точка.
    // точка имеет меньшую ширину
    noDataStringWidth = 2*xyzFontWidth + DIGIT_PLACES*xyzFontWidth + XYZ_FONT_DOT_WIDTH;
    fullDigitPlacesWidth = xyzFontWidth*DIGIT_PLACES;


}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::doUpdate(HalDC* hal)
{
  if(!isActive())
    return;


  size_t sz = wantsToDraw.size();
  if(sz)
  {
    for(size_t i=0;i<sz;i++)
    {
        drawAxisData(hal,wantsToDraw[i]);
    }
    wantsToDraw.empty();
  }


  #ifdef USE_TOUCH
  
    int clicked_button = buttons->checkButtons(ButtonPressed);
    if(clicked_button != -1)
    {
      // сообщаем, что у нас нажата кнопка
      hal->notifyAction(this);

      size_t total = Scales.getDataCount();
      for(size_t i=0;i<total;i++)
      {
        ScaleData* dt = Scales.getData(i);
        if(dt->isActive() && (dt->getZeroButtonIndex() == clicked_button))
        {
          DBG(F("CLICKED: "));
          DBGLN(dt->getZeroButtonCaption());
          dt->zeroAxis();
          drawAxisData(hal,dt);

          break;
        }
      } // for

    } // if(clicked_button != -1)
  
  #endif // USE_TOUCH

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::drawAxisData(HalDC* hal, ScaleData* scale)
{

 // DBGLN(F("drawAxisData"));

  FONT_TYPE* oldFont = hal->getFont();
  COLORTYPE oldColor = hal->getColor();
  COLORTYPE oldBackColor = hal->getBackColor();

  int axisY = scale->getY();
  int axisX = scale->getDataXCoord();


  hal->setBackColor(SCREEN_BACK_COLOR);
  hal->setFont(XYZFont);

  if(!scale->hasData())
  {

    // нет данных с датчика
    hal->setColor(scale->isActive() ? AXIS_NO_DATA_COLOR : INACTIVE_AXIS_COLOR);
    
    axisX -= noDataStringWidth;
    scale->setLastValueX(axisX);

    // теперь рисуем позиции до точки    
    hal->print(noDataString.c_str(),axisX,axisY); // строка вида "---" в используемом шрифте
    axisX += fullDigitPlacesWidth;

    // теперь точку
    hal->print("4",axisX,axisY); // строка "." в используемом шрифте
    axisX += XYZ_FONT_DOT_WIDTH;

    // теперь две позиции после запятой
    hal->print("33",axisX,axisY); // строка "--" в используемом шрифте


  } // if
  else
  {
    
    // есть данные с датчика
    hal->setColor(AXIS_VALUE_COLOR);
    hal->setFont(SevenSeg_XXXL_Num);
    ScaleFormattedData scaleData = scale->getData();

    // выводим показания с датчика
    // дробные
    String strToDraw;
    if(scaleData.Fract < 10)
      strToDraw += '0';
    strToDraw += scaleData.Fract;

    // рисуем точку
    int strWidth = strToDraw.length()*xxlFontWidth;
    axisX -= (strWidth + XYZ_FONT_DOT_WIDTH);
    
    hal->setFont(XYZFont);
    hal->print("4",axisX,axisY); // строка "." в используемом шрифте
    hal->setFont(SevenSeg_XXXL_Num);
    axisX += XYZ_FONT_DOT_WIDTH;
        
    hal->print(strToDraw.c_str(),axisX,axisY);    
    axisX -= XYZ_FONT_DOT_WIDTH;


    // целые
    strToDraw = abs(scaleData.Value);
    strWidth = strToDraw.length()*xxlFontWidth;
    axisX -= strWidth;
 
    hal->print(strToDraw.c_str(),axisX,axisY);

    // теперь знак, если он нужен
    if(scaleData.Value < 0)
    {
      hal->setFont(XYZFont);
      axisX -= xyzFontWidth;
      hal->print("3",axisX,axisY); // строка "-" в используемом шрифте
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
    size_t total = Scales.getDataCount();

    // теперь выясняем, какую высоту займет одна из осей
    uint16_t axisHeight = xyzFontHeight;

    // теперь выясняем, сколько высоты займут все оси
    uint16_t totalHeight = axisHeight*total;

    // теперь выясняем, с какой позиции рисовать оси
    int curX = MAIN_SCREEN_LABELS_X_OFFSET;
    int curY = (screenHeight - totalHeight - MAIN_SCREEN_AXIS_V_SPACING*(total-1)) / 2;

    // теперь рисуем подписи к осям
    for(size_t i=0;i<total;i++)
    {
        ScaleData* dt = Scales.getData(i);

        hal->setColor(dt->isActive() ? AXIS_LABEL_COLOR : INACTIVE_AXIS_COLOR);
        hal->print(dt->getLabel(),curX,curY);

        if(!buttonsCreated)
        {
          dt->setY(curY);
          dt->setHeight(axisHeight);
          
          // создаём кнопку оси
          const char* axisZeroButtonCaption = dt->getZeroButtonCaption();
          hal->setFont(SCREEN_BIG_FONT);
          int btnFontWidth = hal->getFontWidth(SCREEN_BIG_FONT);

          // ширина кнопки
          int btnWidth = hal->print(axisZeroButtonCaption,0,0,0,true)*btnFontWidth + MAIN_SCREEN_BUTTON_TEXT_PADDING*2;

          // текущий Y
          int btnY = curY + (axisHeight - MAIN_SCREEN_BUTTON_HEIGHT)/2;
          // текущий X
          int btnX = screenWidth - btnWidth - MAIN_SCREEN_BUTTON_H_SPACING;

          int addedBtn = buttons->addButton(btnX,btnY,btnWidth,MAIN_SCREEN_BUTTON_HEIGHT,axisZeroButtonCaption);
          dt->setZeroButtonIndex(addedBtn);

          if(!dt->isActive())
          {
            buttons->disableButton(addedBtn);
            buttons->setButtonBackColor(addedBtn,SCREEN_BACK_COLOR);
          }

          dt->setDataXCoord(btnX - MAIN_SCREEN_DATA_X_OFFSET - xyzFontWidth);

          hal->setFont(XYZFont); 
        
        } // if(!buttonsCreated)

        // теперь рисуем единицы измерения
        hal->setColor(dt->isActive() ? AXIS_UNIT_COLOR : INACTIVE_AXIS_COLOR);
        hal->print("7",dt->getDataXCoord(),curY); // пока тупо миллиметры отображаем

        //drawAxisData(hal,dt);
        addToDrawQueue(dt);

        curY += axisHeight + MAIN_SCREEN_AXIS_V_SPACING;        
        
    } // for

  buttonsCreated = true;
  
  buttons->drawButtons(drawButtonsYield);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MainScreen::doDraw(HalDC* hal)
{
   hal->clearScreen();

   // тут отрисовка текущего состояния
   drawGUI(hal);

   hal->updateDisplay();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

