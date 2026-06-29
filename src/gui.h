#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <vector>

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

    void touch(uint16_t x, uint16_t y);

    void scroll(int delta);

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

    bool loadDirectory(String path);

    void drawHeader();

    void drawList();

    void drawPlayer();

    void drawFooter();

    void drawButtons();

    bool enterSelection();

    bool goBack();

    int visibleRows() const;

    void clampScrolling();
    int screen;
};