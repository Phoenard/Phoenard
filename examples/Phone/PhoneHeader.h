#include "Phoenard.h"

#ifndef _PHN_WIDGET_OS_HEADER_H_
#define _PHN_WIDGET_OS_HEADER_H_

#define PHONE_HEADER_SYNC_INTERVAL    20000
#define PHONE_HEADER_UPDATE_INTERVAL  500
#define PHONE_HEADER_BIG_HEIGHT       27

class PHN_PhoneHeader : public PHN_Widget {
 public:
  PHN_PhoneHeader(void);
  virtual void update(void);
  virtual void draw(void);
  void setUpdating(boolean updating);
  void setBigHeader(boolean big);
  void refreshInfo();
  void setNavigation(const char* prevText, const char* nextText);
  void showStatus(const char* statusText);
  boolean isNavigating();
  boolean isAccepted();
  boolean isCancelled();
  int getSignalLines(int cnt);
 private:
  void drawHeader();
  void drawSidebars();
  void drawBackground();
 
  PHN_Button cancelBtn;
  PHN_Button acceptBtn;
  boolean bigHeader;
  boolean enableUpdates;
  Date date;
  char provider[30];
  int regStatus;
  boolean cardInserted;
  float battery;
  int signal;
  long lastSync;
  long lastUpdate;
  int updateIdx;
};

#endif
