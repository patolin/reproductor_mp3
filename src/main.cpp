#include <Arduino.h>
#include <esp_sleep.h>
#include "SPI.h"
#include <cstring>
#include "CYD28_RGBled.h"
#include "CYD28_audio.h"
#include "CYD28_TouchscreenR.h"
#include "console.h"
#include <TFT_eSPI.h>
#include "app_services_impl.h"
#include "app_state.h"
#include "gui.h"
#include "ui_text.h"

TFT_eSPI tft;
CYD28_TouchR touch(320, 240);

RGBLED led;
AppState appState;

uint32_t tNow, tLast;
portMUX_TYPE gMetadataMux = portMUX_INITIALIZER_UNLOCKED;
char gMetadataArtist[128] = {0};
char gMetadataTrack[128] = {0};
volatile bool gMetadataDirty = false;
String gRestoredTrackPath;
uint8_t gRestoredVolumePercent = 21;
volatile bool gAutoNextPending = false;

constexpr uint32_t kUiIdleTimeoutMs = 30000;
constexpr uint32_t kWakeHoldMs = 5000;
constexpr uint32_t kStandbyPromptMs = 1500;
constexpr int kBacklightPin = 21;
constexpr int kBootButtonPin = 0;

bool gUiSleeping = false;
uint32_t gLastUiActivityMs = 0;
bool gBootButtonWasPressed = false;
bool gTouchWasPressed = false;
RTC_DATA_ATTR bool gWakeHoldRequired = false;

GUI gui(tft, fileSystemService, audioService);

uint8_t percentToRawVolume(uint8_t percent)
{
    uint8_t maxVol = audioService.maxVolume();
    if (maxVol == 0)
        return percent > 100 ? 100 : percent;

    if (percent > 100)
        percent = 100;

    return static_cast<uint8_t>((percent * maxVol + 50U) / 100U);
}

void setBacklightEnabled(bool enabled)
{
#ifndef DUSE_BACKLIGHT_MOD
    digitalWrite(kBacklightPin, enabled ? HIGH : LOW);
#else
    (void)enabled;
#endif
}

void setUiSleeping(bool sleeping)
{
    if (gUiSleeping == sleeping)
        return;

    gUiSleeping = sleeping;

    if (sleeping)
    {
        setBacklightEnabled(false);
        return;
    }

    setBacklightEnabled(true);
    gui.drawScreen(gui.currentScreen());
}

void markUiActivity()
{
    gLastUiActivityMs = millis();

    if (gUiSleeping)
        setUiSleeping(false);
}

void showStandbyPrompt()
{
    const auto &ui = UIText::strings();

    setUiSleeping(false);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;
    int lineGap = 18;
    tft.drawCentreString(ui.standbyPromptLine1, centerX, centerY - lineGap / 2, 2);
    tft.drawCentreString(ui.standbyPromptLine2, centerX, centerY + lineGap / 2, 2);
    tft.setTextDatum(TL_DATUM);
    delay(kStandbyPromptMs);
}

void enterStandby(bool showPrompt = true)
{
    if (showPrompt)
        showStandbyPrompt();

    setUiSleeping(true);
    gTouchWasPressed = false;
    gWakeHoldRequired = true;
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(kBootButtonPin), 0);
    delay(50);
    esp_deep_sleep_start();
}

void enforceWakeHoldIfNeeded()
{
    esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
    if (!(gWakeHoldRequired && wakeCause == ESP_SLEEP_WAKEUP_EXT0))
        return;

    uint32_t holdStartMs = millis();
    while (millis() - holdStartMs < kWakeHoldMs)
    {
        if (digitalRead(kBootButtonPin) != LOW)
        {
            enterStandby(false);
        }
        delay(10);
    }

    gWakeHoldRequired = false;
}

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

void audio_eof_mp3(const char *info)
{
    (void)info;
    gAutoNextPending = true;
}

void audio_eof_stream(const char *info)
{
    (void)info;
}

void setup()
{
	Serial.begin(115200);
    UIText::setLanguage(UIText::Language::Spanish);
#ifndef DUSE_BACKLIGHT_MOD
    pinMode(kBacklightPin, OUTPUT);		// turn on the display backlight
	setBacklightEnabled(true);
#endif
    pinMode(kBootButtonPin, INPUT_PULLUP);
    enforceWakeHoldIfNeeded();
    fileSystemService.begin();
    appState.loadSession(fileSystemService, gRestoredTrackPath, gRestoredVolumePercent);
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

    audioService.setVolumeRaw(percentToRawVolume(gRestoredVolumePercent));
    gRestoredVolumePercent = audioService.volumePercent();
    gui.setVolumePercent(gRestoredVolumePercent);
    gLastUiActivityMs = millis();

    if (!gRestoredTrackPath.isEmpty() && fileSystemService.exists(gRestoredTrackPath))
    {
        if (gui.restoreTrackPath(gRestoredTrackPath))
        {
            gui.drawScreen(1);

            char buf[256];
            gRestoredTrackPath.toCharArray(buf, sizeof(buf));
            if (!audioService.connectToSD(buf))
            {
                gui.setPlaybackStopped();
                gui.refresh();
                gui.drawScreen(0);
            }
            else
            {
                gui.setNowPlaying(gRestoredTrackPath);
                gui.drawScreen(1);
            }
        }
    }

    gBootButtonWasPressed = (digitalRead(kBootButtonPin) == LOW);
}

void loop()
{
	//display.loop();
    console_process();
	// Serial.print("ok!");

    bool bootButtonPressed = (digitalRead(kBootButtonPin) == LOW);
    bool bootButtonEdge = bootButtonPressed && !gBootButtonWasPressed;

    if (bootButtonEdge)
    {
        if (gUiSleeping)
        {
            markUiActivity();
            gTouchWasPressed = false;
        }
        else if (gui.isPlaybackActive())
        {
            setUiSleeping(true);
        }
        else
        {
            enterStandby();
        }
    }

    if (gAutoNextPending)
    {
        gAutoNextPending = false;
        if (!gui.advanceToNextTrack())
            gui.setPlaybackStopped();
    }

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

    if (!gUiSleeping)
    {
        // Touch controller
        CYD28_TS_Point point;

        bool touched = touchPressed(point);
        if (touched && !gTouchWasPressed)
        {
            markUiActivity();
            gui.touch(point);
        }
        gTouchWasPressed = touched;
    }
    else
    {
        gTouchWasPressed = false;
    }

    if (gui.hasSelectedFile())
    {
        String selected = gui.selectedFile();
        Serial.println(selected);
        if (fileSystemService.exists(selected))
        {
            gui.setNowPlaying(selected);
            gui.drawScreen(1);

            char buf[256];
            selected.toCharArray(buf, sizeof(buf));
            if (audioService.connectToSD(buf))
            {
                appState.saveSession(fileSystemService, selected, gui.currentVolumePercent());
            }
            else
            {
                gui.setPlaybackStopped();
                gui.refresh();
                gui.drawScreen(0);
            }
        }
        else
        {
            gui.setPlaybackStopped();
            gui.refresh();
            gui.drawScreen(0);
        }
    }

    static uint32_t lastPlayerUiMs = 0;
    if (gui.currentScreen() == 1)
    {
        uint32_t now = millis();
        if (now - lastPlayerUiMs >= 80)
        {
            gui.setPlaybackTime(audioService.currentTime(), audioService.duration());
            gui.drawScreen(1);
            lastPlayerUiMs = now;
        }
    }

    if (millis() - gLastUiActivityMs >= kUiIdleTimeoutMs)
    {
        if (gui.isPlaybackActive())
            setUiSleeping(true);
        else
            enterStandby();
    }

    gBootButtonWasPressed = bootButtonPressed;
}
