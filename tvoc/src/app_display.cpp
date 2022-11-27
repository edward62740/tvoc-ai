#include "app_display.h"
#include "img_bmps.h"
#include "app_comms.h"

AppDisplay::AppDisplay(TFT_eSPI *tft, uint8_t lcd_bl)
{
    this->tft = tft;
    this->lcd_bl = lcd_bl;
}

AppDisplay::~AppDisplay()
{
    delete this;
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
    uint16_t bg_color = TFT_RED;

    //
    if (iaq <= 1.9)
    {
        this->_bg_color = TFT_DARKGREEN;
        if (this->prev["IAQ"] > 1.9)
            this->_bg_changed = true;
    }
    else if (iaq <= 2.9)
    {
        this->_bg_color = TFT_GREEN;
        if (this->prev["IAQ"] > 2.9 || this->prev["IAQ"] <= 1.9)
            this->_bg_changed = true;
    }
    else if (iaq <= 3.9)
    {
        this->_bg_color = TFT_YELLOW;
        if (this->prev["IAQ"] > 3.9 || this->prev["IAQ"] <= 2.9)
            this->_bg_changed = true;
    }
    else if (iaq <= 4.9)
    {
        this->_bg_color = TFT_ORANGE;
        if (this->prev["IAQ"] > 4.9 || this->prev["IAQ"] <= 3.9)
            this->_bg_changed = true;
    }
    if (this->_bg_changed)
    {
        this->tft->fillScreen(this->_bg_color);
        this->_bg_changed = false;
        this->tft->setCursor(10, 40);
        this->tft->setTextDatum(TL_DATUM);
        this->tft->setFreeFont(&FreeSansBold18pt7b);
        this->tft->setTextColor(TFT_MAGENTA, this->_bg_color);
        this->tft->print("IAQ ");
        this->tft->setCursor(10, 70);
        this->tft->print("CO2 ");
        this->tft->setCursor(10, 100);
        this->tft->print("TVOC ");
        this->tft->setCursor(10, 130);
        this->tft->print("EtOH ");
        this->tft->fillSmoothRoundRect(0, 185, 240, 55, 8, TFT_BLACK, this->_bg_color);
        this->tft->fillRect(0, 233, 240, 8, TFT_BLACK);
        this->tft->setFreeFont(&FreeSans12pt7b);
        this->tft->setTextColor(TFT_GREEN, TFT_BLACK);
        this->tft->setCursor(10, 233);
        this->tft->print("LINK UP");
        this->tft->setCursor(170, 233);
        this->tft->print("10 M");
        this->tft->setFreeFont(&FreeSans9pt7b);
        this->tft->setTextColor(TFT_PURPLE, TFT_BLACK);
        this->tft->setCursor(10, 205);
        this->tft->print(Ethernet.localIP());
        this->tft->drawFastHLine(15, 210, 205, TFT_DARKGREY);
    }
    this->tft->setFreeFont(&FreeSansBold18pt7b);
    this->tft->setTextColor(_bg_color);
    this->tft->setCursor(130, 40);
    if (this->prev["IAQ"] != SensorResultMap["IAQ"])
        this->tft->print(this->prev["IAQ"]);
    this->tft->setCursor(130, 70);
    if (this->prev["eCO2"] != SensorResultMap["eCO2"])
        this->tft->print(this->prev["eCO2"], 0);
    this->tft->setCursor(130, 100);
    if (this->prev["TVOC"] != SensorResultMap["TVOC"])
        this->tft->print(this->prev["TVOC"]);
    this->tft->setCursor(130, 130);
    if (this->prev["EtOH"] != SensorResultMap["EtOH"])
        this->tft->print(this->prev["EtOH"]);
    this->tft->setCursor(150, 205);
    this->tft->setFreeFont(&FreeSans9pt7b);
    this->tft->setTextColor(TFT_BLACK);
    this->tft->print(static_cast<uint32_t>(this->prev["MeasCount"]));

    prev = SensorResultMap;

    if (SensorResultMap["Valid"] != 0)
    {
        this->tft->setFreeFont(&FreeSansBold18pt7b);
        this->tft->setTextColor(TFT_BLACK, _bg_color);
        this->tft->setCursor(130, 40);
        this->tft->print(SensorResultMap["IAQ"]);
        this->tft->setCursor(130, 70);
        this->tft->print(SensorResultMap["eCO2"], 0);
        this->tft->setCursor(130, 100);
        this->tft->print(SensorResultMap["TVOC"]);
        this->tft->setCursor(130, 130);
        this->tft->print(SensorResultMap["EtOH"]);
        this->tft->setFreeFont(&FreeSansBold12pt7b);
        this->tft->setTextColor(_bg_color);
        this->tft->setCursor(60, 160);
        this->tft->print("Warmup...");
    }
    else
    {
        this->tft->setFreeFont(&FreeSansBold12pt7b);
        this->tft->setTextColor(TFT_ORANGE, _bg_color);
        this->tft->setCursor(60, 160);
        this->tft->print("Warmup...");
    }
    this->tft->setCursor(150, 205);
    this->tft->setFreeFont(&FreeSans9pt7b);
    this->tft->setTextColor(TFT_LIGHTGREY);
    this->tft->print(static_cast<uint32_t>(SensorResultMap["MeasCount"]));
    return true;
}
