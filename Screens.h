#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ScreenHAL.h"
#include "UTFT_Buttons_Rus.h"
#include "Events.h"
#include "CoreArray.h"
#include "CONFIG.h"
#include "Settings.h"
#include "Scales.h"
#include "CoreButton.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// главный экран
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// железные кнопки оси X
#if defined(USE_X_ABS_HARDWARE_BUTTON) && defined(USE_X_SCALE)
extern Button xAbsHardwareButton;
#endif

#if defined(USE_X_ZERO_HARDWARE_BUTTON) && defined(USE_X_SCALE)
extern Button xZeroHardwareButton;
#endif

// железные кнопки оси Y
#if defined(USE_Y_ABS_HARDWARE_BUTTON) && defined(USE_Y_SCALE)
extern Button yAbsHardwareButton;
#endif

#if defined(USE_Y_ZERO_HARDWARE_BUTTON) && defined(USE_Y_SCALE)
extern Button yZeroHardwareButton;
#endif

// железные кнопки оси Z
#if defined(USE_Z_ABS_HARDWARE_BUTTON) && defined(USE_Z_SCALE)
extern Button zAbsHardwareButton;
#endif

#if defined(USE_Z_ZERO_HARDWARE_BUTTON) && defined(USE_Z_SCALE)
extern Button zZeroHardwareButton;
#endif    
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
typedef struct
{
  int btn;
  const char* newLabel;
  
} RelabelQueueItem;
#pragma pack(pop)
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class MainScreen : public AbstractHALScreen
{
  public:

  static AbstractHALScreen* create()
  {
    return new MainScreen();
  }
  
   virtual void onActivate();
   virtual void onDeactivate();

   virtual void onEvent(Event event, void* param);

   virtual void onButtonPressed(int btn);

    void switchABS(Scale* scale);
    void switchZERO(Scale* scale);

protected:
  
    virtual void doSetup(HalDC* hal);
    virtual void doUpdate(HalDC* hal);
    virtual void doDraw(HalDC* hal);

private:
    MainScreen();
    ~MainScreen();


    UTFT_Buttons_Rus* buttons;
    
    void drawGUI(HalDC* hal);

    void drawAxisData(HalDC* hal, Scale* scale);


    bool buttonsCreated;
    int xyzFontWidth, xyzFontHeight, screenWidth,screenHeight, xxlFontWidth, xxlFontHeight;

    String noDataString;
    int noDataStringWidth, fullDigitPlacesWidth;

    Vector<Scale*> wantsToDraw;
    void addToDrawQueue(Scale* dt);

    #ifdef USE_MM_INCH_SWITCH
    int mmInchButton;
    #endif
    
    MeasureMode measureMode;
    void redrawMeasureUnits(HalDC* hal);

    uint8_t xMultiplier, yMultiplier, zMultiplier;
    uint8_t getMultiplier(Scale* scale);
    
    #if defined(USE_X_RAD_DIA_BUTTON) && defined(USE_X_SCALE)
    int xRadDiaButton;
    #endif
    #if defined(USE_Y_RAD_DIA_BUTTON) &&  defined(USE_Y_SCALE)
    int yRadDiaButton;
    #endif
    #if defined(USE_Z_RAD_DIA_BUTTON)  && defined(USE_Z_SCALE)
    int zRadDiaButton;
    #endif

    Vector<RelabelQueueItem> relabelQueue;
  
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern MainScreen* Main;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------


