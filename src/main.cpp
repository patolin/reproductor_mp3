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
#include <cstring>
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
portMUX_TYPE gMetadataMux = portMUX_INITIALIZER_UNLOCKED;
char gMetadataArtist[128] = {0};
char gMetadataTrack[128] = {0};
volatile bool gMetadataDirty = false;

void storeMetadataField(char *dest, size_t destSize, const String &value)
{
    size_t len = value.length();
    if (len >= destSize)
        len = destSize - 1;

    memcpy(dest, value.c_str(), len);
    dest[len] = '\0';
}

void setMetadataSnapshot(const String &artist, const String &track)
{
    portENTER_CRITICAL(&gMetadataMux);
    storeMetadataField(gMetadataArtist, sizeof(gMetadataArtist), artist);
    storeMetadataField(gMetadataTrack, sizeof(gMetadataTrack), track);
    gMetadataDirty = true;
    portEXIT_CRITICAL(&gMetadataMux);
}

void applyMetadataLine(const char *line)
{
    if (!line)
        return;

    String text(line);
    text.trim();

    if (text.startsWith("Artist:"))
    {
        char currentTrack[sizeof(gMetadataTrack)];
        portENTER_CRITICAL(&gMetadataMux);
        strncpy(currentTrack, gMetadataTrack, sizeof(currentTrack));
        currentTrack[sizeof(currentTrack) - 1] = '\0';
        portEXIT_CRITICAL(&gMetadataMux);

        setMetadataSnapshot(text.substring(7), String(currentTrack));
        return;
    }

    if (text.startsWith("Title:"))
    {
        char currentArtist[sizeof(gMetadataArtist)];
        portENTER_CRITICAL(&gMetadataMux);
        strncpy(currentArtist, gMetadataArtist, sizeof(currentArtist));
        currentArtist[sizeof(currentArtist) - 1] = '\0';
        portEXIT_CRITICAL(&gMetadataMux);

        setMetadataSnapshot(String(currentArtist), text.substring(6));
        return;
    }
}

bool touchPressed(CYD28_TS_Point &point)
{
    if (!touch.touched())
        return false;

    point = touch.getPointScaled();

    return true;
}

void audio_id3data(const char *info)
{
    applyMetadataLine(info);
}

void audio_showstreamtitle(const char *info)
{
    if (!info)
        return;

    String text(info);
    text.trim();

    int separator = text.indexOf(" - ");
    if (separator > 0)
    {
        setMetadataSnapshot(text.substring(0, separator), text.substring(separator + 3));
    }
    else
    {
        setMetadataSnapshot(String(), text);
    }
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

    if (gMetadataDirty)
    {
        char artist[sizeof(gMetadataArtist)];
        char track[sizeof(gMetadataTrack)];
        portENTER_CRITICAL(&gMetadataMux);
        strncpy(artist, gMetadataArtist, sizeof(artist));
        artist[sizeof(artist) - 1] = '\0';
        strncpy(track, gMetadataTrack, sizeof(track));
        track[sizeof(track) - 1] = '\0';
        gMetadataDirty = false;
        portEXIT_CRITICAL(&gMetadataMux);

        gui.setMetadata(String(artist), String(track));
        if (gui.currentScreen() == 1)
        {
            gui.drawScreen(1);
        }
    }

    if (gui.hasSelectedFile())
    {
        String selected = gui.selectedFile();
        Serial.println(selected);
        gui.setNowPlaying(selected);
        gui.drawScreen(1);

        char buf[256];
        selected.toCharArray(buf, 256);
        audioConnecttoSD(buf);
    }

    static uint32_t lastPlayerUiMs = 0;
    if (gui.currentScreen() == 1)
    {
        uint32_t now = millis();
        if (now - lastPlayerUiMs >= 250)
        {
            gui.setPlaybackTime(audio.getAudioCurrentTime(), audio.getAudioFileDuration());
            gui.drawScreen(1);
            lastPlayerUiMs = now;
        }
    }


}
