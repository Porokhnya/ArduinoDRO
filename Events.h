#pragma once

#include <Arduino.h>
#include "CoreArray.h"
#include "CONFIG.h"
//--------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
  EventNOP, // "никакое" событие
  EventScaleDataChanged, // изменились данные с линейки
} Event;
//--------------------------------------------------------------------------------------------------------------------------------------
struct IEventSubscriber
{
  virtual void onEvent(Event event, void* param) = 0;
};
//--------------------------------------------------------------------------------------------------------------------------------------
typedef Vector<IEventSubscriber*> IEventSubscriberList;
typedef void* EventSender;
//--------------------------------------------------------------------------------------------------------------------------------------
class EventsClass
{
  private:

    IEventSubscriberList list;
    EventSender sender;
  
public:
  EventsClass();

  void setup();
  void raise(EventSender sender, Event event, void* param);
  void subscribe(IEventSubscriber* s);
  void unsubscribe(IEventSubscriber* s);

  EventSender getEventSender(){ return sender; }

};
//--------------------------------------------------------------------------------------------------------------------------------------
extern EventsClass Events;
