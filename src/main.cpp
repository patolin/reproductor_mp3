/**
 * @file main.cpp
 * @author Piotr Zapart (www.hexefx.com)
 * @brief 	demo project for the CYD 2.8" board (ESP32 + ILI9341 display)
 * 
 * 	Libraries:
 * 		CYD28_LDR - onboard LDR for light intensite measurement
 * 		CYD28_RGBled - onboard RGB led, or just R if PSRAM mod is perfromed
 * 		CYD28_SD - sd card library and filesystem
 * 		CYD28_display - dual buffered display driver configured for LVGL v8
 * 		CYD28_Touchscreen - bit banged SPI driver for the onboard touch screen controller
 * 		CYD_Audio - audio library optimized for internal 8bit DAC, based on ESP32-audioI2S by Wolle (schreibfaul1)
 * 		lvgl - v8.3.9 
 * 		SimpleCLI - console command via UART
 * 		TFT_eSPI - used for low level dislpay access.
 * 		WifiManager - added as lib dependency in platformio.ini
 * 
 * 		--- Audio ---
 * 		Audio is runing as a separate task on core 0. Use queues to communicate with it. 
 * 		See CYD28_audio.h/cpp and gui.h/cpp files  
 * 
 * @version 1.
 * @date 2023-08-25
 */
#include <Arduino.h>
#include "SPI.h"
#include "CYD28_RGBled.h"
#include "CYD28_SD.h"
#include "CYD28_audio.h"
#include "CYD28_TouchscreenR.h"
#include "console.h"
#include <TFT_eSPI.h>
#include "gui.h"

TFT_eSPI tft;
CYD28_TouchR touch(320, 240);
GUI gui(tft);

RGBLED led;

uint32_t tNow, tLast;

bool touchPressed(CYD28_TS_Point &point)
{
    if (!touch.touched())
        return false;

    point = touch.getPointScaled();

    return true;
}

void setup()
{
	Serial.begin(115200);
#ifndef DUSE_BACKLIGHT_MOD
	pinMode(21, OUTPUT);		// turn on the display backlight
	digitalWrite(21, HIGH);
#endif
    sdcard.begin();
    tft.init();
    tft.setRotation(0);
    touch.begin();
    touch.setRotation(0);

    gui.begin();
    gui.draw();
	console_init();
	delay(1500);

	//display.begin(CYD28_DISPLAY_ROT_PORT0);
	
	audioInit();

	
}

void loop()
{
	//display.loop();
    console_process();
	// Serial.print("ok!");
    
    
    // Touch controller
    static bool touchWasPressed = false;
    CYD28_TS_Point point;

    bool touched = touchPressed(point);
    if (touched && !touchWasPressed)
    {
        gui.touch(point);
    }
    touchWasPressed = touched;

    if (gui.hasSelectedFile())
    {
        Serial.println(gui.selectedFile());
        gui.drawScreen(1);

        char buf[256];
        gui.selectedFile().toCharArray(buf, 256);
        audioConnecttoSD(buf);
    }


}
