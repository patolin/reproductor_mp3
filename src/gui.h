#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <vector>

class CYD28_TS_Point;

struct FileEntry
{
    String name;
    bool directory;
    uint32_t size;
};

class GUI
{
public:

    GUI(TFT_eSPI &display);

    bool begin();

    void draw();

    void refresh();

    void touch(const CYD28_TS_Point &point);

    void scroll(int delta);

    void setNowPlaying(const String &filePath);

    void setMetadata(const String &artist, const String &track);

    void setPlaybackTime(uint32_t currentSec, uint32_t totalSec);

    int currentScreen() const;

    bool hasSelectedFile();

    String selectedFile();

    void drawScreen(int screen); // 0: file list, 1: audio player

private:

    TFT_eSPI &tft;

    std::vector<FileEntry> entries;

    String currentPath;

    int selected;
    int firstVisible;

    bool fileChosen;
    String chosenFile;
    String statusMessage;
    String nowPlayingPath;
    String metadataArtist;
    String metadataTrack;
    String playerPrimaryText;
    uint32_t playerCurrentSec;
    uint32_t playerTotalSec;
    bool playerHasMetadata;

    bool loadDirectory(String path);

    void drawHeader();

    void drawList();

    void drawPlayer();

    void drawButtons();

    bool enterSelection();

    bool goBack();

    int visibleRows() const;

    void clampScrolling();

    void updatePlayerPrimaryText();

    String formatRemainingTime() const;

    int buttonIndexFromPoint(const CYD28_TS_Point &point) const;

    bool isInButtonArea(const CYD28_TS_Point &point) const;

    void handleButton(int buttonIndex);

    uint32_t lastTapMs;

    int screen;
};
