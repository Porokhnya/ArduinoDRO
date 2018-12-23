#include "ScreenHAL.h"
#include "Buzzer.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int utf8_strlen(const String& str) // возвращает длину в символах строки в кодировке UTF-8
{
    
    int c,i,ix,q;
    for (q=0, i=0, ix=str.length(); i < ix; i++, q++)
    {
        c = (unsigned char) str[i];
        if      (c>=0   && c<=127) i+=0;
        else if ((c & 0xE0) == 0xC0) i+=1;
        else if ((c & 0xF0) == 0xE0) i+=2;
        else if ((c & 0xF8) == 0xF0) i+=3;
        //else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return 0; //invalid utf8
    }
    return q;
}
//--------------------------------------------------------------------------------------------------------------------------------
unsigned int utf8GetCharSize(unsigned char bt) 
{ 
  if (bt < 128) 
  return 1; 
  else if ((bt & 0xE0) == 0xC0) 
  return 2; 
  else if ((bt & 0xF0) == 0xE0) 
  return 3; 
  else if ((bt & 0xF8) == 0xF0) 
  return 4; 
  else if ((bt & 0xFC) == 0xF8) 
  return 5; 
  else if ((bt & 0xFE) == 0xFC) 
  return 6; 

 
  return 1; 
} 
//--------------------------------------------------------------------------------------------------------------------------------
void ButtonPressed(int btn) // вызывается, если кнопка на экране нажата
{
  if(btn != -1)
  {
    #ifdef USE_BUZZER
    Buzzer.buzz();
    #endif

    AbstractHALScreen* as = Screen.getActiveScreen();
    if(as)
    {
      as->onButtonPressed(btn);
    }
    
  }

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawButtonsYield() // вызывается после отрисовки каждой кнопки
{
  yield();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if DISPLAY_USED == DISPLAY_TFT
const uint8_t utf8_rus_charmap[] PROGMEM = {'A',128,'B',129,130,'E',131,132,133,134,135,'K',136,'M','H','O',137,'P','C','T',138,139,'X',140,141,
142,143,144,145,146,147,148,149,'a',150,151,152,153,'e',154,155,156,157,158,159,160,161,162,'o',163,'p','c',164,'y',165,'x',166,167,168,169,170,
171,172,173,174,175};
#endif
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractHALScreen::AbstractHALScreen()
{
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractHALScreen::~AbstractHALScreen()
{
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AbstractHALScreen::setup(HalDC* hal)
{
  // тут общие для всех классов настройки
  doSetup(hal); 
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AbstractHALScreen::update(HalDC* hal)
{
  if(isActive())
  {
      doUpdate(hal);
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AbstractHALScreen::draw(HalDC* hal)
{
  if(isActive())
  {
    doDraw(hal);    
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// HalDC
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
HalDC::HalDC()
{
  currentScreenIndex = -1;
  requestedToActiveScreen = NULL;
  requestedToActiveScreenIndex = -1;
  on_action = NULL;
  halDCDescriptor = NULL;
  curFont = NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::notifyAction(AbstractHALScreen* screen)
{
  if(on_action)
    on_action(screen);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::addScreen(AbstractHALScreen* screen)
{
  screen->setup(this);
  screens.push_back(screen);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::backlight(bool bOn)
{
  #ifdef USE_BACKLIGHT_PIN
    digitalWrite(BACKLIGHT_PIN, bOn ? BACKLIGHT_ON : !BACKLIGHT_ON);
  #endif // USE_BACKLIGHT_PIN
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::initHAL()
{
  //Тут инициализация/переинициализация дисплея
  
  #if DISPLAY_USED == DISPLAY_TFT

      #ifdef TFT_EXTRA_RESET
        pinMode(TFT_RST_PIN,OUTPUT);
        digitalWrite(TFT_RST_PIN,HIGH);
        delay(10);
        digitalWrite(TFT_RST_PIN,LOW);
        delay(10);
        digitalWrite(TFT_RST_PIN,HIGH);        
      #endif      
    
      #if DISPLAY_INIT_DELAY > 0
        delay(DISPLAY_INIT_DELAY);
      #endif

      halDCDescriptor->InitLCD(SCREEN_ORIENTATION);

      setBackColor(SCREEN_BACK_COLOR);
      setFont(SCREEN_SMALL_FONT);

      tftTouch->InitTouch(SCREEN_ORIENTATION);
      tftTouch->setPrecision(PREC_HI);  
            
	 #else
    #error "Unsupported display!"
  #endif  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::setup()
{

  // включаем подсветку

  #ifdef USE_BACKLIGHT_PIN
    pinMode(BACKLIGHT_PIN,OUTPUT);
    backlight(true);
  #endif // USE_BACKLIGHT_PIN

  //создание библиотеки для экрана

  #if DISPLAY_USED == DISPLAY_TFT  
    halDCDescriptor = new UTFT(TFT_MODEL,TFT_RS_PIN,TFT_WR_PIN,TFT_CS_PIN,TFT_RST_PIN);
  #else
    #error "Unsupported display!"
  #endif

  tftTouch = new URTouch(TFT_TOUCH_CLK_PIN,TFT_TOUCH_CS_PIN,TFT_TOUCH_DIN_PIN,TFT_TOUCH_DOUT_PIN,TFT_TOUCH_IRQ_PIN);

 
  // инициализируем дисплей
  initHAL();

  // добавляем экран мессадж-бокса
  addScreen(MessageBoxScreen::create());
  // добавляем экран клавиатуры
  addScreen(KeyboardScreen::create());

  Events.subscribe(this);
   
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::onEvent(Event event, void* param)
{
  AbstractHALScreen* a = getActiveScreen();
  if(a)
    a->onEvent(event,param);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::update()
{

  if(requestedToActiveScreen != NULL)
  {
    // попросили сделать экран активным
    AbstractHALScreen* screen = requestedToActiveScreen;
    currentScreenIndex = requestedToActiveScreenIndex;
    
    requestedToActiveScreen = NULL;
    
    screen->setActive(true);
    screen->onActivate();

    screen->update(this);
    screen->draw(this);
    
    return;
    
  } // if(requestedToActiveScreen != NULL)

  if(currentScreenIndex == -1) // ни разу не рисовали ещё ничего, исправляемся
  {
    if(screens.size())
     switchToScreen((unsigned int)0); // переключаемся на первый экран, если ещё ни разу не показали ничего     
  }

  if(currentScreenIndex == -1)
    return;

  // обновляем текущий экран
  AbstractHALScreen* currentScreen = screens[currentScreenIndex];
  currentScreen->update(this);
  
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractHALScreen* HalDC::getActiveScreen()
{
  if(currentScreenIndex < 0 || !screens.size())
    return NULL;

  return screens[currentScreenIndex];
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::switchToScreen(AbstractHALScreen* screen)
{

  if(requestedToActiveScreen != NULL) // ждём переключения на новый экран
  {
//    DBGLN(F("requestedToActiveScreen != NULL"));
    return;
  }

//  DBG(F("currentScreenIndex = "));
//  DBGLN(currentScreenIndex);
  
  if(currentScreenIndex > -1 && screens.size())
  {
     AbstractHALScreen* si = screens[currentScreenIndex];
     si->setActive(false);
     si->onDeactivate();
  }
  
  for(size_t i=0;i<screens.size();i++)
  {
    if(screens[i] == screen)
    {
      requestedToActiveScreen = screen;
      requestedToActiveScreenIndex = i;

      break;
    }
  }  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::switchToScreen(unsigned int screenIndex)
{
  if(screenIndex < screens.size())
      switchToScreen(screens[screenIndex]);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::drawBitmap(int x, int y, int w, int h, unsigned int* bmp)
{
  #if DISPLAY_USED == DISPLAY_TFT
   halDCDescriptor->drawBitmap(x,y,w,h,(uint16_t*)bmp);
  #else
    #error "Unsupported display!"
  #endif    
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::setFont(FONT_TYPE* font)
{
  curFont = font;
  
  #if DISPLAY_USED == DISPLAY_TFT
   halDCDescriptor->setFont(font);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
FONT_TYPE* HalDC::getFont()
{
  #if DISPLAY_USED == DISPLAY_TFT
    return halDCDescriptor->getFont();
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::clearScreen()
{
  #if DISPLAY_USED == DISPLAY_TFT
    halDCDescriptor->fillScr(SCREEN_BACK_COLOR);
  #else
    #error "Unsupported display!"
  #endif      
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::fillScreen(COLORTYPE color)
{
  #if DISPLAY_USED == DISPLAY_TFT
    halDCDescriptor->fillScr(color);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::setBackColor(COLORTYPE color)
{
  #if DISPLAY_USED == DISPLAY_TFT
    halDCDescriptor->setBackColor(color);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
COLORTYPE  HalDC::getBackColor()
{
  #if DISPLAY_USED == DISPLAY_TFT
    return halDCDescriptor->getBackColor();
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::setColor(COLORTYPE color)
{
  #if DISPLAY_USED == DISPLAY_TFT
    halDCDescriptor->setColor(color);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
COLORTYPE  HalDC::getColor()
{
  #if DISPLAY_USED == DISPLAY_TFT
  return halDCDescriptor->getColor();
  #else
    #error "Unsupported display!"
  #endif  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void  HalDC::drawRect(int x1, int y1, int x2, int y2)
{
  #if DISPLAY_USED == DISPLAY_TFT
    halDCDescriptor->drawRect(x1,y1,x2,y2);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void  HalDC::drawRoundRect(int x1, int y1, int x2, int y2)
{
  #if DISPLAY_USED == DISPLAY_TFT
    halDCDescriptor->drawRoundRect(x1,y1,x2,y2);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void  HalDC::fillRect(int x1, int y1, int x2, int y2)
{
  #if DISPLAY_USED == DISPLAY_TFT
    halDCDescriptor->fillRect(x1,y1,x2,y2);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void  HalDC::fillRoundRect(int x1, int y1, int x2, int y2)
{
  #if DISPLAY_USED == DISPLAY_TFT
    halDCDescriptor->fillRoundRect(x1,y1,x2,y2);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void  HalDC::clrRoundRect(int x1, int y1, int x2, int y2)
{
  #if DISPLAY_USED == DISPLAY_TFT
    //halDCDescriptor->fillRoundRect(x1,y1,x2,y2);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint16_t HalDC::getFontWidth(FONT_TYPE* font)
{
  #if DISPLAY_USED == DISPLAY_TFT    
    return READ_FONT_BYTE(0);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint16_t HalDC::getFontHeight(FONT_TYPE* font)
{
  #if DISPLAY_USED == DISPLAY_TFT
    return READ_FONT_BYTE(1); 
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint16_t HalDC::getScreenWidth()
{
  #if DISPLAY_USED == DISPLAY_TFT
    return halDCDescriptor->getDisplayXSize(); 
  #else
    #error "Unsupported display!"
  #endif     
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint16_t HalDC::getScreenHeight()
{
  #if DISPLAY_USED == DISPLAY_TFT
    return halDCDescriptor->getDisplayYSize(); 
  #else
    #error "Unsupported display!"
  #endif     
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HalDC::updateDisplay()
{
  #if DISPLAY_USED == DISPLAY_TFT
  #else
    #error "Unsupported display!"
  #endif      
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int HalDC::print(const char* st,int x, int y, int deg, bool computeStringLengthOnly)
{
  #if DISPLAY_USED == DISPLAY_TFT
    return printTFT(st,x,y,deg,computeStringLengthOnly);
  #else
    #error "Unsupported display!"
  #endif    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if DISPLAY_USED == DISPLAY_TFT
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int HalDC::printTFT(const char* st,int x, int y, int deg, bool computeStringLengthOnly)
{
    
 int stl, i;
  stl = strlen(st);

  if (halDCDescriptor->orient==PORTRAIT)
  {
    if (x==RIGHT) 
      x=(halDCDescriptor->disp_x_size+1)-(stl*halDCDescriptor->cfont.x_size);
  
    if (x==CENTER) 
      x=((halDCDescriptor->disp_x_size+1)-(stl*halDCDescriptor->cfont.x_size))/2;
  } 
  else 
  {
    if (x==RIGHT) 
      x=(halDCDescriptor->disp_y_size+1)-(stl*halDCDescriptor->cfont.x_size);
    
    if (x==CENTER) 
      x=((halDCDescriptor->disp_y_size+1)-(stl*halDCDescriptor->cfont.x_size))/2;
  }
  
  uint8_t utf_high_byte = 0;
  uint8_t ch, ch_pos = 0;
  
  for (i = 0; i < stl; i++) 
  {
    ch = st[i];
    
    if ( ch >= 128) 
    {
      if ( utf_high_byte == 0 && (ch ==0xD0 || ch == 0xD1)) 
      {
        utf_high_byte = ch;
        continue;
      } 
      else 
      {
        if ( utf_high_byte == 0xD0) 
        {
          if (ch == 0x81) 
          { //Ё
            ch = 6;
          } 
          else 
          {
            if(ch <= 0x95) 
            {
              ch -= 0x90;
            } 
            else if( ch < 0xB6)
            {
              ch -= (0x90 - 1);
            } 
            else 
            {
              ch -= (0x90 - 2);
            }
          }
          
          ch = pgm_read_byte((utf8_rus_charmap + ch));
        
        } 
        else if (utf_high_byte == 0xD1) 
        {
          if (ch == 0x91) 
          {//ё
            ch = 39;
          } 
          else 
          {
            ch -= 0x80;
            ch += 50;
          }
          
          ch = pgm_read_byte((utf8_rus_charmap + ch));
        }
        
        utf_high_byte = 0;
      }
    } 
    else 
    {
      utf_high_byte = 0;
    }
    

    if (deg==0) 
    {
      if(!computeStringLengthOnly)
        halDCDescriptor->printChar(ch, x + (ch_pos * (halDCDescriptor->cfont.x_size)), y);
    } 
    else 
    {
      if(!computeStringLengthOnly)
        halDCDescriptor->rotateChar(ch, x, y, ch_pos, deg);
    }
    ++ch_pos;
  } // for  

  return ch_pos;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif // DISPLAY_USED == DISPLAY_TFT
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
HalDC Screen;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MessageBoxScreen* MessageBox;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MessageBoxScreen::MessageBoxScreen() : AbstractHALScreen()
{
  targetOkScreen = NULL;
  targetCancelScreen = NULL;
  resultSubscriber = NULL;
  caption = NULL;

  buttons = new UTFT_Buttons_Rus(Screen.getUTFT(),
   Screen.getTouch()
  , 2
  );
  buttons->setTextFont(SCREEN_BIG_FONT);
  buttons->setButtonColors(BUTTON_COLORS);

  #ifdef USE_STEPPER_RUN_DIODE
    diodeTimerActive = false;
  #endif
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::doSetup(HalDC* dc)
{

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::doUpdate(HalDC* dc)
{
    // тут обновляем внутреннее состояние

  #ifdef USE_STEPPER_RUN_DIODE
    if(diodeTimerActive)
    {
      if(millis() - diodeTimer > 500)
      {
        diodeLevel = !diodeLevel;
        digitalWrite(STEPPER_RUN_DIODE_PIN,diodeLevel);
        diodeTimer = millis();    
      }
    }
  #endif

   //if(boxType == mbHalt)
   {
      dc->notifyAction(this);
   }

  
    int pressed_button = buttons->checkButtons(ButtonPressed);
    if(pressed_button != -1)
    {
      // сообщаем, что у нас нажата кнопка
      dc->notifyAction(this);
      
       if(pressed_button == noButton && targetCancelScreen)
       {
        if(resultSubscriber)
          resultSubscriber->onMessageBoxResult(false);
          
        dc->switchToScreen(targetCancelScreen);
       }
       else
       if(pressed_button == yesButton && targetOkScreen)
       {
          if(resultSubscriber)
            resultSubscriber->onMessageBoxResult(true);
            
            dc->switchToScreen(targetOkScreen);
       }
    
    } // if(pressed_button != -1)
  
    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::doDraw(HalDC* hal)
{
  hal->clearScreen();
  
  hal->setFont(SCREEN_BIG_FONT);
  
  uint8_t fontHeight = hal->getFontHeight(SCREEN_BIG_FONT);
  uint8_t fontWidth = hal->getFontWidth(SCREEN_BIG_FONT);
  
  int displayWidth = hal->getScreenWidth();
  int displayHeight = hal->getScreenHeight();
  
  int lineSpacing = 6; 
  int topOffset = 10;
  int curX = 0;
  int curY = topOffset;

  int lineLength = 0;

  // подложка под заголовок
  hal->setColor(VGA_NAVY);
  hal->fillRect(0, 0, displayWidth-1, topOffset*2 + fontHeight);
  
  if(caption)
  {
    hal->setBackColor(VGA_NAVY);
    hal->setColor(VGA_WHITE);
    lineLength = hal->print(caption,curX,curY,0,true);
    curX = (displayWidth - lineLength*fontWidth)/2; 
    hal->print(caption,curX,curY);
  }

  hal->setBackColor(SCREEN_BACK_COLOR);
  hal->setColor(SCREEN_TEXT_COLOR);

  curY = (displayHeight - MB_SCREEN_BUTTON_HEIGHT - (lines.size()*fontHeight + (lines.size()-1)*lineSpacing))/2;

  for(size_t i=0;i<lines.size();i++)
  {
    lineLength = hal->print(lines[i],curX,curY,0,true);
    curX = (displayWidth - lineLength*fontWidth)/2;    
    hal->print(lines[i],curX,curY);
    curY += fontHeight + lineSpacing;
  }


  buttons->drawButtons(drawButtonsYield);


  hal->updateDisplay();

  if(boxType == mbHalt)
  {
    Buzzer.buzz(1,BUZZER_HALT_DURATION);
  }

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::recreateButtons()
{
  buttons->deleteAllButtons();
  yesButton = -1;
  noButton = -1;

  HalDC* dc = &Screen;
  
  int screenWidth = dc->getScreenWidth();
  int screenHeight = dc->getScreenHeight();
  int buttonsWidth = 200;

  int numOfButtons = boxType == mbShow ? 1 : 2;

  int top = screenHeight - MB_SCREEN_BUTTON_HEIGHT - MB_SCREEN_BUTTON_Y_OFFSET;
  int left = (screenWidth - (buttonsWidth*numOfButtons + MB_SCREEN_BUTTON_X_OFFSET*(numOfButtons-1)))/2;
  
  yesButton = buttons->addButton(left, top, buttonsWidth, MB_SCREEN_BUTTON_HEIGHT, boxType == mbShow ? "OK" : "ДА");

  if(boxType == mbConfirm)
  {
    left += buttonsWidth + MB_SCREEN_BUTTON_Y_OFFSET;
    noButton = buttons->addButton(left, top, buttonsWidth, MB_SCREEN_BUTTON_HEIGHT, "НЕТ");  
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::halt(const char* _caption, Vector<const char*>& _lines)
{
  lines = _lines;
  caption = _caption;
  boxType = mbHalt;

  buttons->deleteAllButtons();
  yesButton = -1;
  noButton = -1;
    
  targetOkScreen = NULL;
  targetCancelScreen = NULL;
  resultSubscriber = NULL;  

  Screen.switchToScreen(this);


  #ifdef USE_STEPPER_RUN_DIODE
    pinMode(STEPPER_RUN_DIODE_PIN,OUTPUT);
    diodeLevel = !STEPPER_RUN_DIODE_ON;
    diodeTimerActive = true;
    diodeTimer = millis();    
  #endif
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::show(const char* _caption, Vector<const char*>& _lines, AbstractHALScreen* okTarget, MessageBoxResultSubscriber* sub)
{
  lines = _lines;
  caption = _caption;

  boxType = mbShow;
  recreateButtons();
    
  targetOkScreen = okTarget;
  targetCancelScreen = NULL;
  resultSubscriber = sub;

  Screen.switchToScreen(this);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MessageBoxScreen::confirm(const char* _caption, Vector<const char*>& _lines, AbstractHALScreen* okTarget, AbstractHALScreen* cancelTarget, MessageBoxResultSubscriber* sub)
{
  lines = _lines;
  caption = _caption;

  boxType = mbConfirm;
  recreateButtons();
  
  targetOkScreen = okTarget;
  targetCancelScreen = cancelTarget;
  resultSubscriber = sub;

  Screen.switchToScreen(this);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractHALScreen* MessageBoxScreen::create()
{
    MessageBox = new MessageBoxScreen();
    return MessageBox;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
KeyboardScreen* Keyboard;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
KeyboardScreen::KeyboardScreen() : AbstractHALScreen()
{
  inputTarget = NULL;
  maxLen = 20;
  isRusInput = true;

  buttons = new UTFT_Buttons_Rus(Screen.getUTFT(),
    Screen.getTouch()
  , 60
  );
  buttons->setTextFont(SCREEN_BIG_FONT);
  buttons->setButtonColors(BUTTON_COLORS);
  buttons->setSymbolFont(SCREEN_SYMBOL_FONT);

  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
KeyboardScreen::~KeyboardScreen()
{
  for(size_t i=0;i<captions.size();i++)
  {
    delete captions[i];
  }
  for(size_t i=0;i<altCaptions.size();i++)
  {
    delete altCaptions[i];
  }
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::switchInput(bool redraw)
{
  isRusInput = !isRusInput;
  Vector<String*>* pVec = isRusInput ? &captions : &altCaptions;

  // у нас кнопки изменяемой клавиатуры начинаются с индекса 10
  size_t startIdx = 10;

  for(size_t i=startIdx;i<pVec->size();i++)
  {
    buttons->relabelButton(i,(*pVec)[i]->c_str(),redraw);
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::doSetup(HalDC* dc)
{
  createKeyboard(dc);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::doUpdate(HalDC* dc)
{
    // тут обновляем внутреннее состояние
    // раз нас вызвали, то пока не нажмут кнопки - мы не выйдем, поэтому всегда сообщаем, что на экране что-то происходит
    dc->notifyAction(this);

    // мигаем курсором
    static uint32_t cursorTimer = millis();
    if(millis() - cursorTimer > 500)
    {
      static bool cursorVisible = true;
      cursorVisible = !cursorVisible;

      redrawCursor(dc,cursorVisible);

      cursorTimer = millis();
    }
    
    // проверяем на перемещение курсора
    URTouch* touch = dc->getTouch();
    
    if(touch->dataAvailable())
    {
      touch->read();
      int   touch_x = touch->getX();
      int   touch_y = touch->getY();

      // проверяем на попадание в прямоугольную область ввода текста
      int screenWidth = dc->getScreenWidth();
      
      if(touch_x >= KBD_SPACING && touch_x <= (screenWidth - KBD_SPACING) && touch_y >= KBD_SPACING && touch_y <= (KBD_SPACING + KBD_BUTTON_HEIGHT))
      {
        // кликнули на области ввода, ждём отпускания тача
        while (touch->dataAvailable()) { yield(); }
        
        int fontWidth = dc->getFontWidth(SCREEN_BIG_FONT);

        // вычисляем, на какой символ приходится клик тачем
        int symbolNum = touch_x/fontWidth - 1;
        
        if(symbolNum < 0)
          symbolNum = 0;
          
        int valLen = utf8_strlen(inputVal);

        if(symbolNum > valLen)
          symbolNum = valLen;

        redrawCursor(dc,true);
        cursorPos = symbolNum;
        redrawCursor(dc,false);
      }
    } // if (touch->dataAvailable())
  
    int pressed_button = buttons->checkButtons(ButtonPressed);
    if(pressed_button != -1)
    {
      
       if(pressed_button == backspaceButton)
       {
        // удалить последний введённый символ
        drawValue(dc,true);
       }
       else
       if(pressed_button == okButton)
       {
          // закрыть всё нафик
          if(inputTarget)
          {
            inputTarget->onKeyboardInputResult(inputVal,true);
            inputVal = "";
          }
       }
        else
       if(pressed_button == switchButton)
       {
          // переключить раскладку
          switchInput(true);
       }
       else
       if(pressed_button == cancelButton)
       {
          // закрыть всё нафик
          if(inputTarget)
          {
            inputTarget->onKeyboardInputResult(inputVal,false);
            inputVal = "";
          }
       }
       else
       {
         // одна из кнопок клавиатуры, добавляем её текст к буферу, но - в позицию курсора!!!
         int len = utf8_strlen(inputVal);
         const char* lbl = buttons->getLabel(pressed_button);
         String buff;
         
         if(!len) // пустая строка
         {
          buff = lbl;
         }
         else
         if(len < maxLen)
         {
            
            int len = utf8_strlen(inputVal);
            const char* ptr = inputVal.c_str();
            
            for(int i=0;i<len;i++)
            {
              unsigned char curChar = (unsigned char) *ptr;
              unsigned int charSz = utf8GetCharSize(curChar);
              for(byte k=0;k<charSz;k++) 
              {
                utf8Bytes[k] = *ptr++;
              }
              utf8Bytes[charSz] = '\0'; // добавляем завершающий 0
              
              if(i == cursorPos)
              {
                buff += lbl;
              }
              
              buff += utf8Bytes;
              
            } // for

            if(cursorPos >= len)
              buff += lbl;

         } // if(len < maxLen)
         
          inputVal = buff;
          
          drawValue(dc);

          redrawCursor(dc,true);
          cursorPos++;
          redrawCursor(dc,false);
          

         
       } // else одна из кнопок клавиатуры
    
    } // if(pressed_button != -1)
  
    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::doDraw(HalDC* hal)
{
  hal->clearScreen();
  

  buttons->drawButtons(drawButtonsYield);

  int screenWidth = hal->getScreenWidth();
  hal->setColor(VGA_GRAY);
  hal->drawRoundRect(KBD_SPACING, KBD_SPACING, screenWidth-KBD_SPACING, KBD_SPACING + KBD_BUTTON_HEIGHT);

  drawValue(hal);
  redrawCursor(hal,false);

  hal->updateDisplay();
}
//--------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::redrawCursor(HalDC* hal, bool erase)
{
  uint8_t fontHeight = hal->getFontHeight(SCREEN_BIG_FONT);
  uint8_t fontWidth = hal->getFontWidth(SCREEN_BIG_FONT);

  int top = KBD_SPACING + (KBD_BUTTON_HEIGHT - fontHeight)/2;
  int left = KBD_SPACING*2 + fontWidth*cursorPos;

  if(erase)
    hal->setColor(SCREEN_BACK_COLOR);
  else
    hal->setColor(SCREEN_TEXT_COLOR);
  
  hal->fillRect(left,top,left+1,top+fontHeight);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::drawValue(HalDC* hal, bool deleteCharAtCursor)
{
  if(!inputVal.length())
    return;

  if(deleteCharAtCursor)
  {
    // надо удалить символ слева от позиции курсора.

    String buff;
    int len = utf8_strlen(inputVal);
    const char* ptr = inputVal.c_str();
    
    for(int i=0;i<len;i++)
    {
      unsigned char curChar = (unsigned char) *ptr;
      unsigned int charSz = utf8GetCharSize(curChar);
      for(byte k=0;k<charSz;k++) 
      {
        utf8Bytes[k] = *ptr++;
      }
      utf8Bytes[charSz] = '\0'; // добавляем завершающий 0
      
      if(i != (cursorPos-1)) // игнорируем удаляемый символ
      {
        buff += utf8Bytes;
      }
      
    } // for
    
    buff += ' '; // маскируем последний символ для корректной перерисовки на экране
    inputVal = buff;

  }

  hal->setFont(SCREEN_BIG_FONT);
  
  uint8_t fontHeight = hal->getFontHeight(SCREEN_BIG_FONT);

  hal->setColor(SCREEN_TEXT_COLOR);
  hal->setBackColor(SCREEN_BACK_COLOR);

  int top = KBD_SPACING + (KBD_BUTTON_HEIGHT - fontHeight)/2;
  int left = KBD_SPACING*2;

  hal->print(inputVal.c_str(),left,top);

  if(deleteCharAtCursor)
  {
    // если надо удалить символ слева от позиции курсора, то в этом случае у нас последний символ - пробел, и мы его удаляем
    inputVal.remove(inputVal.length()-1,1);

    redrawCursor(hal,true);

    cursorPos--;
    if(cursorPos < 0)
      cursorPos = 0;

    redrawCursor(hal,false);
  }
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::createKeyboard(HalDC* dc)
{
  buttons->deleteAllButtons();

  int screenWidth = dc->getScreenWidth();
  int screenHeight = dc->getScreenHeight();  

  // создаём клавиатуру

  int colCounter = 0;
  int left = KBD_SPACING;
  int top = KBD_SPACING*2 + KBD_BUTTON_HEIGHT;

  // сперва у нас кнопки 0-9
  for(uint8_t i=0;i<10;i++)
  {
    char c = '0' + i;
    String* s = new String(c);
    captions.push_back(s);

    String* altS = new String(c);
    altCaptions.push_back(altS);    

    int addedBtn = buttons->addButton(left, top, KBD_BUTTON_WIDTH, KBD_BUTTON_HEIGHT, s->c_str());
    buttons->setButtonBackColor(addedBtn, VGA_GRAY);
    buttons->setButtonFontColor(addedBtn, VGA_BLACK);
    
    left += KBD_BUTTON_WIDTH + KBD_SPACING;
    colCounter++;
    if(colCounter >= KBD_BUTTONS_IN_ROW)
    {
      colCounter = 0;
      left = KBD_SPACING;
      top += KBD_SPACING + KBD_BUTTON_HEIGHT;
    }
  }
  // затем - А-Я
  const char* letters[] = {
    "А", "Б", "В", "Г", "Д", "Е",
    "Ж", "З", "И", "Й", "К", "Л",
    "М", "Н", "О", "П", "Р", "С",
    "Т", "У", "Ф", "Х", "Ц", "Ч",
    "Ш", "Щ", "Ъ", "Ы", "Ь", "Э",
    "Ю", "Я", NULL
  };

  const char* altLetters[] = {
    "A", "B", "C", "D", "E", "F",
    "G", "H", "I", "J", "K", "L",
    "M", "N", "O", "P", "Q", "R",
    "S", "T", "U", "V", "W", "X",
    "Y", "Z", ".", ",", ":", ";",
    "!", "?", NULL
  };  

  int lettersIterator = 0;
  while(letters[lettersIterator])
  {
    String* s = new String(letters[lettersIterator]);
    captions.push_back(s);

    String* altS = new String(altLetters[lettersIterator]);
    altCaptions.push_back(altS);

    buttons->addButton(left, top, KBD_BUTTON_WIDTH, KBD_BUTTON_HEIGHT, s->c_str());
    left += KBD_BUTTON_WIDTH + KBD_SPACING;
    colCounter++;
    if(colCounter >= KBD_BUTTONS_IN_ROW)
    {
      colCounter = 0;
      left = KBD_SPACING;
      top += KBD_SPACING + KBD_BUTTON_HEIGHT;
    } 

    lettersIterator++;
  }
  // затем - кнопка переключения ввода
    switchButton = buttons->addButton(left, top, KBD_BUTTON_WIDTH, KBD_BUTTON_HEIGHT, "q", BUTTON_SYMBOL);
    buttons->setButtonBackColor(switchButton, VGA_MAROON);
    buttons->setButtonFontColor(switchButton, VGA_WHITE);

    left += KBD_BUTTON_WIDTH + KBD_SPACING;
  
  // затем - пробел,
    spaceButton = buttons->addButton(left, top, KBD_BUTTON_WIDTH*5 + KBD_SPACING*4, KBD_BUTTON_HEIGHT, " ");
    buttons->setButtonBackColor(spaceButton, VGA_GRAY);
    buttons->setButtonFontColor(spaceButton, VGA_BLACK);
       
    left += KBD_BUTTON_WIDTH*5 + KBD_SPACING*5;
   
  // backspace, 
    backspaceButton = buttons->addButton(left, top, KBD_BUTTON_WIDTH*2 + KBD_SPACING, KBD_BUTTON_HEIGHT, ":", BUTTON_SYMBOL);
    buttons->setButtonBackColor(backspaceButton, VGA_MAROON);
    buttons->setButtonFontColor(backspaceButton, VGA_WHITE);

    left = KBD_SPACING;
    top = screenHeight - KBD_BUTTON_HEIGHT_BIG - KBD_SPACING;
   
  // OK,
    int okCancelButtonWidth = (screenWidth - KBD_SPACING*3)/2;
    okButton = buttons->addButton(left, top, okCancelButtonWidth, KBD_BUTTON_HEIGHT_BIG, "OK");
    left += okCancelButtonWidth + KBD_SPACING;
  
  // CANCEL
    cancelButton = buttons->addButton(left, top, okCancelButtonWidth, KBD_BUTTON_HEIGHT_BIG, "ОТМЕНА");

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::applyType(KeyboardType keyboardType)
{
  if(ktFull == keyboardType)
  {
    buttons->enableButton(spaceButton);
    buttons->enableButton(switchButton);

    // включаем все кнопки
    // у нас кнопки изменяемой клавиатуры начинаются с индекса 10
    size_t startIdx = 10;
  
    for(size_t i=startIdx;i<altCaptions.size();i++)
    {
      buttons->enableButton(i);
    }    

    isRusInput = false;
    switchInput(false);

    return;
  }

  if(ktNumbers == keyboardType)
  {
    buttons->disableButton(spaceButton);
    buttons->disableButton(switchButton);

    // выключаем все кнопки, кроме номеров и точки
    // у нас кнопки изменяемой клавиатуры начинаются с индекса 10
    size_t startIdx = 10;
  
    for(size_t i=startIdx;i<altCaptions.size();i++)
    {
      if(*(altCaptions[i]) != ".")
        buttons->disableButton(i);
    }        

    isRusInput = true;
    switchInput(false);

    return;
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void KeyboardScreen::show(const String& val, int ml, KeyboardInputTarget* it, KeyboardType keyboardType)
{
  inputVal = val;
  inputTarget = it;
  maxLen = ml;

  cursorPos = utf8_strlen(inputVal);

  applyType(keyboardType);

  Screen.switchToScreen(this);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AbstractHALScreen* KeyboardScreen::create()
{
    Keyboard = new KeyboardScreen();
    return Keyboard;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

