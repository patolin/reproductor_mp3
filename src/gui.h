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

    void setVolumePercent(uint8_t percent);

    bool hasSelectedFile();

    String selectedFile();

    void drawScreen(int screen); // 0: file list, 1: audio player

private:

    TFT_eSPI &tft;
    TFT_eSprite playerSprite;
    TFT_eSPI *canvas;
    bool playerSpriteReady;
    bool playerSpriteAttempted;

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
    String playerArtistText;
    String playerTitleText;
    String playerDiscText;
    uint32_t playerCurrentSec;
    uint32_t playerTotalSec;
    uint8_t volumePercent;
    bool playerPaused;

    bool loadDirectory(String path);

    bool loadDirectoryEntries(String path, std::vector<FileEntry> &outEntries) const;

    int findEntryIndexByName(const String &name) const;

    bool queueTrackPath(const String &path);

    bool firstPlayableTrackInFolder(const String &folder, String &outPath) const;

    bool siblingFolderTrack(const String &currentFolder, int direction, String &outPath) const;

    void drawHeader();

    void drawList();

    void drawPlayer();

    void drawPlayerControls();

    void drawProgressBar();

    void drawButtons();

    bool enterSelection();

    bool goBack();

    bool playPreviousTrack();

    bool playNextTrack();

    int visibleRows() const;

    void clampScrolling();

    void updatePlayerDisplayText();
    bool ensurePlayerSprite();

    String ellipsize(const String &text, size_t maxChars) const;

    String formatRemainingTime() const;

    int buttonIndexFromPoint(const CYD28_TS_Point &point) const;

    bool isInButtonArea(const CYD28_TS_Point &point) const;

    void handleButton(int buttonIndex);

    uint32_t lastTapMs;

    int screen;
};
