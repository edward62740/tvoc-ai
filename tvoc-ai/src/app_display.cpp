#include "app_display.h"
#include "img_bmps.h"

AppDisplay::AppDisplay(TFT_eSPI *tft, uint8_t lcd_bl)
{
    this->tft = tft;
    this->lcd_bl = lcd_bl;
}

AppDisplay::~AppDisplay()
{
}

void AppDisplay::init()
{
    this->tft->init();
    this->tft->initDMA();
    this->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    this->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    this->tft->setTextColor(TFT_WHITE);
    this->tft->println("scheduler started...");
    for (int i = 0; i < 255; i++)
    {
        analogWrite(this->lcd_bl, i);
        vTaskDelay(3);
    }
}

void AppDisplay::drawNoEth()
{
    this->tft->pushImage(0, 0, 240, 240, img_no_eth);

    this->tft->setFreeFont(&FreeSans12pt7b);
    this->tft->setTextColor(TFT_ORANGE);
    this->tft->setCursor(10, 230);
    this->tft->print("LINK DOWN");
    this->tft->setCursor(180, 230);
    this->tft->print("0  M");
}

void AppDisplay::drawLinkUp()
{
    this->tft->pushImage(0, 0, 240, 240, img_net_up);

    this->tft->setFreeFont(&FreeSans12pt7b);
    this->tft->setTextColor(TFT_GREEN);
    this->tft->setCursor(10, 230);
    this->tft->print("LINK UP");
    this->tft->setCursor(170, 230);
    this->tft->print("100M");
}

bool AppDisplay::drawData(std::map<char *, float> SensorResultMap)
{
    float iaq = SensorResultMap["IAQ"];
    if (iaq <= 1.9)
    {
        this->tft->fillRectVGradient(0, 0, 240, 240, TFT_DARKGREEN, TFT_GREEN);
    }
    else if (iaq <= 2.9)
    {
        this->tft->fillRectVGradient(0, 0, 240, 240, TFT_GREEN, TFT_GREENYELLOW);
    }
    else if (iaq <= 3.9)
    {
        this->tft->fillRectVGradient(0, 0, 240, 240, TFT_GREENYELLOW, TFT_YELLOW);
    }
    else if (iaq <= 4.9)
    {
        this->tft->fillRectVGradient(0, 0, 240, 240, TFT_YELLOW, TFT_ORANGE);
    }
    else
    {
        this->tft->fillRectVGradient(0, 0, 240, 240, TFT_ORANGE, TFT_RED);
    }
    this->tft->setFreeFont(&FreeSerifBold18pt7b);
    this->tft->setTextColor(TFT_WHITE);
    this->tft->setCursor(10, 40);
    this->tft->print("IAQ: ");
    this->tft->print(SensorResultMap["IAQ"]);
    this->tft->setCursor(10, 70);
    this->tft->print("CO2: ");
    this->tft->print(SensorResultMap["CO2"]);
    this->tft->setCursor(10, 100);
    this->tft->print("TVOC: ");
    this->tft->print(SensorResultMap["TVOC"]);
    this->tft->setCursor(10, 130);
    this->tft->print("EtOH: ");
    this->tft->print(SensorResultMap["EtOH"]);
    this->tft->setCursor(10, 160);
    this->tft->print("logRcda: ");
    this->tft->print(SensorResultMap["logRcda"]);
    this->tft->setCursor(10, 190);
    this->tft->print(SensorResultMap["MeasCount"]);

    this->tft->setFreeFont(&FreeSans12pt7b);
    this->tft->setTextColor(TFT_GREEN);
    this->tft->setCursor(10, 230);
    this->tft->print("LINK UP");
    this->tft->setCursor(170, 230);
    this->tft->print("100M");

    return true;
}
