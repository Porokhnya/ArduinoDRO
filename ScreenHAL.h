#ifndef _UTFTMENU_H
#define _UTFTMENU_H
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "CONFIG.h"
#include "CoreArray.h"
#include "Events.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class HalDC;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern void ButtonPressed(int btn); // вызывается, если кнопка на экране нажата
extern void drawButtonsYield(); // вызывается после отрисовки каждой кнопки
extern int utf8_strlen(const String& str);
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if DISPLAY_USED == DISPLAY_TFT
#include <UTFT.h>
#else
  #error "Unsupported display!"
#endif

#include <URTouchCD.h>
#include <URTouch.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "UTFT_Buttons_Rus.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if DISPLAY_USED == DISPLAY_TFT

  #if defined (__arm__)
    #define READ_FONT_BYTE(x) font[(x)]  
  #elif defined(__AVR__)  
    #define READ_FONT_BYTE(x) pgm_read_byte(&(font[(x)]))  
  #endif
  
#else
  #error "Unsupported display!"
#endif  
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if DISPLAY_USED == DISPLAY_TFT

  typedef UTFT HalDCDescriptor;
  typedef uint16_t COLORTYPE;
  typedef uint8_t FONT_TYPE;
  extern FONT_TYPE BigRusFont[];               // какой шрифт используем
  extern FONT_TYPE SmallRusFont[];             // какой шрифт используем
  extern FONT_TYPE Various_Symbols_16x32[];    // какой шрифт используем
  extern FONT_TYPE Various_Symbols_32x32[];    // какой шрифт используем
  extern FONT_TYPE SevenSeg_XXXL_Num[];
  extern FONT_TYPE XYZFont[];
    
  #define SCREEN_SMALL_FONT SmallRusFont       // маленький шрифт
  #define SCREEN_BIG_FONT BigRusFont       // большой шрифт
  #define SCREEN_SYMBOL_FONT Various_Symbols_32x32  // символьный шрифт
  #define SCREEN_ORIENTATION  LANDSCAPE         // ориентация экрана горизонтальная
        
#else
  #error "Unsupported display!"
#endif  
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// абстрактный класс экрана
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class AbstractHALScreen
{
  public:

    void setup(HalDC* hal);
    void update(HalDC* hal);
    void draw(HalDC* hal);

    bool isActive() {return isActiveScreen; }
    void setActive(bool val){ isActiveScreen = val; }
  
    AbstractHALScreen();
    virtual ~AbstractHALScreen();

    // вызывается, когда переключаются на экран
    virtual void onActivate(){}

    // вызывается, когда экран становится неактивным
    virtual void onDeactivate() {}

    // вызывается, когда кнопка нажата
    virtual void onButtonPressed(int btn) {}
    
    virtual void onEvent(Event event, void* param) {};

  protected:
    
    virtual void doSetup(HalDC* hal) = 0;
    virtual void doUpdate(HalDC* hal) = 0;
    virtual void doDraw(HalDC* hal) = 0;

    private:
      bool isActiveScreen;
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef Vector<AbstractHALScreen*> HALScreensList; // список экранов
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// класс-менеджер работы с экраном
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef void (*OnScreenAction)(AbstractHALScreen* screen);
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class HalDC : public IEventSubscriber
{

public:
  HalDC();

  void setup();
  void update();

  virtual void onEvent(Event event, void* param);


  #if DISPLAY_USED == DISPLAY_TFT
    UTFT* getUTFT() {return halDCDescriptor; }
  #endif

  URTouch* getTouch() { return tftTouch; }  

  void addScreen(AbstractHALScreen* screen);

  AbstractHALScreen* getActiveScreen();
  void onAction(OnScreenAction handler) {on_action = handler;}

  // сообщает, что произведено какое-то действие на экране
  void notifyAction(AbstractHALScreen* screen);

  void switchToScreen(AbstractHALScreen* screen);
  void switchToScreen(unsigned int screenIndex);

  // HARDWARE HAL
  int print(const char* str,int x, int y, int deg=0, bool computeStringLengthOnly=false);

  void setFont(FONT_TYPE* font);
  FONT_TYPE* getFont();
  void setBackColor(COLORTYPE color);
  COLORTYPE  getBackColor();
  void setColor(COLORTYPE color);
  COLORTYPE  getColor();
  void fillScreen(COLORTYPE color);
  void  drawRect(int x1, int y1, int x2, int y2);
  void  drawRoundRect(int x1, int y1, int x2, int y2);
  void  fillRect(int x1, int y1, int x2, int y2);
  void  fillRoundRect(int x1, int y1, int x2, int y2);
  void clrRoundRect(int x1, int y1, int x2, int y2);
  uint16_t getFontWidth(FONT_TYPE* font);
  uint16_t getFontHeight(FONT_TYPE* font);

  void drawBitmap(int x, int y, int w, int h, unsigned int* bmp);

  uint16_t getScreenWidth();
  uint16_t getScreenHeight();

  void clearScreen();
  void updateDisplay();

  void backlight(bool bOn=true);
  

private:

  void initHAL();


#if DISPLAY_USED == DISPLAY_TFT
  int printTFT(const char* str,int x, int y, int deg=0, bool computeStringLengthOnly=false);
#endif

  URTouch* tftTouch;

  AbstractHALScreen* requestedToActiveScreen;
  int requestedToActiveScreenIndex;

  OnScreenAction on_action;
  
  HALScreensList screens;
  HalDCDescriptor* halDCDescriptor;
  int currentScreenIndex;
  
  FONT_TYPE* curFont;

  
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern HalDC Screen;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
  mbShow,
  mbConfirm,
  mbHalt
  
} MessageBoxType;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct MessageBoxResultSubscriber
{
  virtual void onMessageBoxResult(bool okPressed) = 0;
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define MB_SCREEN_BUTTON_HEIGHT 100
#define MB_SCREEN_BUTTON_X_OFFSET 10
#define MB_SCREEN_BUTTON_Y_OFFSET 10
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class MessageBoxScreen : public AbstractHALScreen
{
  public:

  static AbstractHALScreen* create();

  void confirm(const char* caption, Vector<const char*>& lines, AbstractHALScreen* okTarget, AbstractHALScreen* cancelTarget, MessageBoxResultSubscriber* sub=NULL);
  void show(const char* caption, Vector<const char*>& lines, AbstractHALScreen* okTarget, MessageBoxResultSubscriber* sub=NULL);
  
  void halt(const char* caption, Vector<const char*>& lines);
    
protected:

    virtual void doSetup(HalDC* dc);
    virtual void doUpdate(HalDC* dc);
    virtual void doDraw(HalDC* dc);

private:
      MessageBoxScreen();

      AbstractHALScreen* targetOkScreen;
      AbstractHALScreen* targetCancelScreen;
      Vector<const char*> lines;

      const char* caption;

      MessageBoxResultSubscriber* resultSubscriber;

      UTFT_Buttons_Rus* buttons;
      int yesButton, noButton;

      MessageBoxType boxType;

      void recreateButtons();

      #ifdef USE_STEPPER_RUN_DIODE
        uint32_t diodeTimer;
        bool diodeTimerActive;
        uint8_t diodeLevel;
      #endif
  
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern MessageBoxScreen* MessageBox;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
  ktFull, // полная клавиатура
  ktNumbers, // ввод только чисел и точки
  
} KeyboardType;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct KeyboardInputTarget
{
  virtual void onKeyboardInputResult(const String& input, bool okPressed) = 0;
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define KBD_BUTTONS_IN_ROW  10
#define KBD_BUTTON_WIDTH  70
#define KBD_BUTTON_HEIGHT  50
#define KBD_BUTTON_HEIGHT_BIG  100
#define KBD_SCREEN_BUTTON_X_OFFSET 10
#define KBD_SCREEN_BUTTON_Y_OFFSET 10
#define KBD_SPACING  9
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class KeyboardScreen : public AbstractHALScreen
{
  public:

  static AbstractHALScreen* create();

  void show(const String& val, int maxLen, KeyboardInputTarget* inputTarget, KeyboardType keyboardType=ktFull);

  ~KeyboardScreen();  
protected:

    virtual void doSetup(HalDC* dc);
    virtual void doUpdate(HalDC* dc);
    virtual void doDraw(HalDC* dc);

private:
      KeyboardScreen();

      KeyboardInputTarget* inputTarget;
      String inputVal;
      int maxLen;
      bool isRusInput;

      UTFT_Buttons_Rus* buttons;
      Vector<String*> captions;
      Vector<String*> altCaptions;

      int backspaceButton, okButton, cancelButton, switchButton, spaceButton;

      void createKeyboard(HalDC* dc);
      void drawValue(HalDC* dc, bool deleteCharAtCursor=false);
      void switchInput(bool redraw);

      void applyType(KeyboardType keyboardType);

      int cursorPos;
      void redrawCursor(HalDC* dc, bool erase);

      char utf8Bytes[7];
  
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern KeyboardScreen* Keyboard;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif
