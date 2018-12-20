#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ScreenHAL.h"
#include "UTFT_Buttons_Rus.h"
#include "Events.h"
#include "CoreArray.h"
#include "CONFIG.h"
#include "Settings.h"
#include "Scales.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// главный экран
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


protected:
  
    virtual void doSetup(HalDC* hal);
    virtual void doUpdate(HalDC* hal);
    virtual void doDraw(HalDC* hal);

private:
    MainScreen();
    ~MainScreen();


    UTFT_Buttons_Rus* buttons;
    
    void drawGUI(HalDC* hal);

    void drawAxisData(HalDC* hal, ScaleData* scale);


    bool buttonsCreated;
    int xyzFontWidth, xyzFontHeight, screenWidth,screenHeight, xxlFontWidth, xxlFontHeight;

    String noDataString;
    int noDataStringWidth, fullDigitPlacesWidth;

    Vector<ScaleData*> wantsToDraw;
    void addToDrawQueue(ScaleData* dt);
    
  
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern MainScreen* Main;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------


