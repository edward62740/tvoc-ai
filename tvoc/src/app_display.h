#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H
#include "TFT_eSPI.h"
#include <map>
#include "STM32FreeRTOS.h"
#include "FreeRTOS.h"

class AppDisplay
{
public:
    AppDisplay(TFT_eSPI *tft, uint8_t lcd_bl);
    ~AppDisplay();
    void init();
    void drawNoEth();
    void drawLinkUp();
    bool drawData(std::map<const char *, float> SensorResultMap, bool link);
    bool _bg_changed = true;
private:
    TFT_eSPI *tft;
    uint8_t lcd_bl;
    uint16_t _bg_color = TFT_BLACK;
    std::map<const char *, float> prev;
    bool prev_link = false;
};

#endif // APP_DISPLAY_H