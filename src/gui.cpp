#include "gui.h"
#include "CYD28_audio.h"
#include "CYD28_SD.h"
#include "CYD28_TouchscreenR.h"
#include <algorithm>

namespace
{
constexpr int kHeaderHeight = 24;
constexpr int kListRowHeight = 24;
constexpr int kButtonRows = 3;
constexpr int kButtonCols = 2;
constexpr uint8_t kVolumeStepPercent = 5;

String fileNameFromPath(const String &path)
{
    int slash = path.lastIndexOf('/');
    if (slash >= 0 && slash + 1 < path.length())
        return path.substring(slash + 1);
    return path;
}

String folderFromPath(const String &path)
{
    int slash = path.lastIndexOf('/');
    if (slash <= 0)
        return "/";
    return path.substring(0, slash);
}

String folderNameFromPath(const String &path)
{
    return fileNameFromPath(folderFromPath(path));
}

String joinPath(const String &folder, const String &name)
{
    if (folder == "/" || folder.isEmpty())
        return String("/") + name;

    return folder + "/" + name;
}

String formatTime(uint32_t sec)
{
    uint32_t minutes = sec / 60;
    uint32_t seconds = sec % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu:%02lu", static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
    return String(buf);
}

String formatRemainingTimeFrom(uint32_t totalSec, uint32_t currentSec)
{
    if (totalSec <= currentSec)
        return String("0:00");
    return formatTime(totalSec - currentSec);
}

bool isPlayableEntry(const FileEntry &entry)
{
    return !entry.directory;
}

String ellipsizeText(const String &text, size_t maxChars)
{
    if (text.length() <= maxChars)
        return text;

    if (maxChars <= 3)
        return text.substring(0, maxChars);

    return text.substring(0, maxChars - 3) + "...";
}
}

GUI::GUI(TFT_eSPI &display)
: tft(display)
{
    currentPath="/";
    selected=0;
    screen=0;
    firstVisible=0;
    fileChosen=false;
    statusMessage="Initializing...";
    nowPlayingPath="";
    metadataArtist="";
    metadataTrack="";
    playerArtistText="";
    playerTitleText="";
    playerDiscText="";
    playerCurrentSec=0;
    playerTotalSec=0;
    volumePercent=0;
    playerPaused=false;
    lastTapMs=0;
}

bool GUI::begin()
{
    sdcard.begin();
    
    Serial.begin(115200);
    Serial.println("init");
    return loadDirectory("/");
}

void GUI::refresh()
{
    loadDirectory(currentPath);
    Serial.println("refresh");
}

void GUI::setNowPlaying(const String &filePath)
{
    nowPlayingPath = filePath;
    metadataArtist = "";
    metadataTrack = "";
    playerCurrentSec = 0;
    playerTotalSec = 0;
    volumePercent = audioGetVolumePerCent();
    playerPaused = false;
    playerDiscText = folderNameFromPath(filePath);
    updatePlayerDisplayText();
}

void GUI::setMetadata(const String &artist, const String &track)
{
    if (!artist.isEmpty())
        metadataArtist = artist;
    if (!track.isEmpty())
        metadataTrack = track;

    updatePlayerDisplayText();
}

void GUI::setPlaybackTime(uint32_t currentSec, uint32_t totalSec)
{
    playerCurrentSec = currentSec;
    playerTotalSec = totalSec;
}

void GUI::setVolumePercent(uint8_t percent)
{
    volumePercent = percent > 100 ? 100 : percent;
}

void GUI::drawScreen(int screen)
{
    this->screen=screen;
    draw();
}

int GUI::currentScreen() const
{
    return screen;
}

bool GUI::loadDirectoryEntries(String path, std::vector<FileEntry> &outEntries) const
{
    outEntries.clear();

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory())
        return false;

    while (true)
    {
        File file = dir.openNextFile();
        if (!file)
            break;

        FileEntry e;
        String n = file.name();

        if (path != "/")
        {
            int p = n.lastIndexOf('/');
            if (p >= 0)
                n = n.substring(p + 1);
        }

        e.name = n;
        e.directory = file.isDirectory();
        e.size = file.size();
        outEntries.push_back(e);
        file.close();
    }

    dir.close();

    std::sort(outEntries.begin(), outEntries.end(),
    [](const FileEntry &a, const FileEntry &b)
    {
        if (a.directory != b.directory)
            return a.directory > b.directory;
        return a.name < b.name;
    });

    return true;
}

void GUI::updatePlayerDisplayText()
{
    playerDiscText = folderNameFromPath(nowPlayingPath);

    if (!metadataArtist.isEmpty() && !metadataTrack.isEmpty())
    {
        playerArtistText = metadataArtist;
        playerTitleText = metadataTrack;
        return;
    }

    if (!metadataTrack.isEmpty())
    {
        playerArtistText = folderFromPath(nowPlayingPath);
        playerTitleText = metadataTrack;
        return;
    }

    if (!metadataArtist.isEmpty())
    {
        playerArtistText = metadataArtist;
        playerTitleText = fileNameFromPath(nowPlayingPath);
        return;
    }

    if (!nowPlayingPath.isEmpty())
    {
        playerArtistText = folderFromPath(nowPlayingPath);
        playerTitleText = fileNameFromPath(nowPlayingPath);
        return;
    }

    playerArtistText = "No track";
    playerTitleText = "selected";
    playerDiscText = "";
}

String GUI::ellipsize(const String &text, size_t maxChars) const
{
    return ellipsizeText(text, maxChars);
}

int GUI::visibleRows() const
{
    int buttonAreaHeight = tft.height() / 2;
    int listHeight = tft.height() - kHeaderHeight - buttonAreaHeight;
    if (listHeight <= 0)
        return 1;

    int rows = listHeight / kListRowHeight;
    return rows > 0 ? rows : 1;
}

bool GUI::playPreviousTrack()
{
    if (entries.empty())
        return false;

    String currentFile = nowPlayingPath.isEmpty() ? chosenFile : nowPlayingPath;
    int currentIndex = findEntryIndexByName(fileNameFromPath(currentFile));
    if (currentIndex < 0)
        currentIndex = selected;

    for (int i = currentIndex - 1; i >= 0; --i)
    {
        if (isPlayableEntry(entries[i]))
            return queueTrackPath(joinPath(currentPath, entries[i].name));
    }

    String siblingTrack;
    if (siblingFolderTrack(currentPath, -1, siblingTrack))
        return queueTrackPath(siblingTrack);

    return false;
}

bool GUI::playNextTrack()
{
    if (entries.empty())
        return false;

    String currentFile = nowPlayingPath.isEmpty() ? chosenFile : nowPlayingPath;
    int currentIndex = findEntryIndexByName(fileNameFromPath(currentFile));
    if (currentIndex < 0)
        currentIndex = selected;

    for (size_t i = static_cast<size_t>(currentIndex + 1); i < entries.size(); ++i)
    {
        if (isPlayableEntry(entries[i]))
            return queueTrackPath(joinPath(currentPath, entries[i].name));
    }

    String siblingTrack;
    if (siblingFolderTrack(currentPath, 1, siblingTrack))
        return queueTrackPath(siblingTrack);

    return false;
}

bool GUI::loadDirectory(String path)
{
    entries.clear();
    statusMessage="";

    if (SD.cardType() == CARD_NONE)
    {
        sdcard.begin();
    }

    if(!loadDirectoryEntries(path, entries))
    {
        statusMessage="No SD card or invalid folder";
        currentPath=path;
        selected=0;
        firstVisible=0;
        return false;
    }

    currentPath=path;
    selected=0;
    firstVisible=0;
    statusMessage = entries.empty() ? "Empty folder" : "";
    Serial.println("Current path: "+currentPath + " entries: "+String(entries.size()));
    return true;
}

void GUI::draw()
{
    tft.fillScreen(TFT_BLACK);

    drawHeader();
    Serial.println("screen: "+String(screen));
    switch (screen) {
        case 0:
            drawList();
            drawButtons();
            break;
        case 1:     
            drawPlayer();
    }
}

void GUI::drawHeader()
{
    tft.fillRect(0,0,tft.width(),kHeaderHeight,TFT_BLUE);

    tft.setTextFont(1);
    tft.setTextColor(TFT_WHITE,TFT_BLUE);
    String headerText = (screen == 1) ? String("PLAYER") : currentPath;
    tft.drawString(headerText,4,4);

    if (screen == 1)
    {
        int width = tft.width();
        int barX = 88;
        int barY = 8;
        int barW = width - barX - 6;
        int barH = 8;

        tft.drawRect(barX, barY, barW, barH, TFT_WHITE);
        int fillW = (barW - 2) * volumePercent / 100;
        if (fillW > 0)
            tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, TFT_ORANGE);
    }

    if(screen == 0 && !statusMessage.isEmpty())
    {
        tft.setTextFont(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_YELLOW, TFT_BLUE);
        tft.drawString(statusMessage, 4, 12);
    }
}

void GUI::drawList()
{
    int width = tft.width();
    int y = kHeaderHeight;
    int buttonAreaHeight = tft.height() / 2;
    int listBottom = tft.height() - buttonAreaHeight;

    int rows=visibleRows();

    if(entries.empty())
    {
        tft.setTextFont(2);
        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.drawCentreString(statusMessage.isEmpty() ? "No entries" : statusMessage,
                             width / 2,
                             (listBottom + kHeaderHeight) / 2,
                             2);
        return;
    }

    for(int i=0;i<rows;i++)
    {
        int idx=firstVisible+i;

        if(idx>=entries.size())
            break;

        if (y >= listBottom)
            break;

        uint16_t rowColor = (idx == selected) ? TFT_DARKGREEN : TFT_BLACK;
        tft.fillRect(0,y,width,kListRowHeight,rowColor);
        tft.drawFastHLine(0, y + kListRowHeight - 1, width, TFT_DARKGREY);

        tft.setTextFont(1);
        tft.setTextColor(TFT_WHITE, rowColor);

        String s;

        if(entries[idx].directory)
            s="[DIR] ";

        else
            s="      ";

        s+=entries[idx].name;

        tft.drawString(s,5,y+4);
        Serial.println("Drawing entry: "+s);
        y+=kListRowHeight;
    }
}

void GUI::drawPlayer()
{
    int width = tft.width();
    int playerBottom = tft.height() / 2;
    tft.fillRect(0, kHeaderHeight, width, playerBottom - kHeaderHeight, TFT_BLACK);
    tft.drawFastHLine(0, playerBottom - 1, width, TFT_DARKGREY);

    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("Now Playing", width / 2, 34, 2);

    tft.setTextFont(1);
    tft.setTextColor(TFT_CYAN);
    tft.drawCentreString(ellipsize(playerArtistText, 36), width / 2, 52, 1);
    tft.drawCentreString(ellipsize(playerTitleText, 36), width / 2, 66, 1);
    if (!playerDiscText.isEmpty())
    {
        tft.setTextColor(TFT_LIGHTGREY);
        tft.drawCentreString(ellipsize(String("Disc: ") + playerDiscText, 36), width / 2, 80, 1);
    }

    tft.setTextFont(1);
    tft.setTextColor(TFT_WHITE);
    String timeLine = "Elapsed: ";
    timeLine += (playerTotalSec > 0) ? formatTime(playerCurrentSec) : String("--:--");
    timeLine += " - Total: ";
    timeLine += (playerTotalSec > 0) ? formatTime(playerTotalSec) : String("--:--");
    tft.drawCentreString(timeLine, width / 2, 96, 1);

    drawProgressBar();
    drawPlayerControls();
}

void GUI::drawProgressBar()
{
    int width = tft.width();
    int barX = 16;
    int barY = 111;
    int barW = width - 32;
    int barH = 8;
    uint32_t progressPercent = 0;

    if (playerTotalSec > 0 && playerCurrentSec <= playerTotalSec)
        progressPercent = (playerCurrentSec * 100UL) / playerTotalSec;

    tft.setTextFont(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawRect(barX, barY, barW, barH, TFT_WHITE);
    int fillW = (barW - 2) * progressPercent / 100;
    if (fillW > 0)
        tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, TFT_GREEN);
}

void GUI::drawPlayerControls()
{
    int width = tft.width();
    int buttonTop = tft.height() / 2;
    int buttonAreaHeight = tft.height() - buttonTop;
    int cellWidth = width / kButtonCols;
    int cellHeight = buttonAreaHeight / kButtonRows;

    struct ButtonLabel
    {
        const char *text;
        uint16_t color;
    };

    const ButtonLabel labels[6] = {
        {nullptr, TFT_DARKCYAN},
        {"VOL+", TFT_DARKGREEN},
        {"PREV", TFT_DARKCYAN},
        {"NEXT", TFT_ORANGE},
        {"STOP", TFT_RED},
        {"VOL-", TFT_DARKGREEN}
    };

    String pauseLabel = playerPaused ? "RESUME" : "PAUSE";
    uint16_t pauseColor = playerPaused ? TFT_MAROON : TFT_DARKCYAN;

    for(int row=0; row<kButtonRows; ++row)
    {
        for(int col=0; col<kButtonCols; ++col)
        {
            int index = row * kButtonCols + col;
            int x = col * cellWidth;
            int y = buttonTop + row * cellHeight;
            const char *text = labels[index].text;
            uint16_t color = labels[index].color;
            if (index == 0)
            {
                text = pauseLabel.c_str();
                color = pauseColor;
            }

            tft.fillRect(x, y, cellWidth, cellHeight, color);
            tft.drawRect(x, y, cellWidth, cellHeight, TFT_BLACK);
            if (text && text[0] != '\0')
            {
                tft.setTextColor(TFT_WHITE, color);
                tft.setTextFont(2);
                tft.drawCentreString(text, x + cellWidth / 2, y + (cellHeight / 2) - 8, 2);
            }
        }
    }
}

String GUI::formatRemainingTime() const
{
    return formatRemainingTimeFrom(playerTotalSec, playerCurrentSec);
}

int GUI::findEntryIndexByName(const String &name) const
{
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (!entries[i].directory && entries[i].name == name)
            return static_cast<int>(i);
    }
    return -1;
}

bool GUI::queueTrackPath(const String &path)
{
    String folder = folderFromPath(path);
    String fileName = fileNameFromPath(path);

    if (currentPath != folder)
    {
        if (!loadDirectory(folder))
            return false;
    }

    int idx = findEntryIndexByName(fileName);
    if (idx < 0)
        return false;

    selected = idx;
    clampScrolling();
    chosenFile = path;
    nowPlayingPath = path;
    metadataArtist = "";
    metadataTrack = "";
    playerCurrentSec = 0;
    playerTotalSec = 0;
    playerPaused = false;
    fileChosen = true;
    updatePlayerDisplayText();
    if (screen == 1)
        drawPlayer();
    return true;
}

bool GUI::firstPlayableTrackInFolder(const String &folder, String &outPath) const
{
    std::vector<FileEntry> tempEntries;
    if (!loadDirectoryEntries(folder, tempEntries))
        return false;

    for (const auto &entry : tempEntries)
    {
        if (isPlayableEntry(entry))
        {
            outPath = joinPath(folder, entry.name);
            return true;
        }
    }

    for (const auto &entry : tempEntries)
    {
        if (!entry.directory)
            continue;

        String nestedPath;
        if (firstPlayableTrackInFolder(joinPath(folder, entry.name), nestedPath))
        {
            outPath = nestedPath;
            return true;
        }
    }

    return false;
}

bool GUI::siblingFolderTrack(const String &currentFolder, int direction, String &outPath) const
{
    String folder = currentFolder;

    while (!folder.isEmpty() && folder != "/")
    {
        String parent = folderFromPath(folder);
        String currentName = fileNameFromPath(folder);

        std::vector<FileEntry> parentEntries;
        if (!loadDirectoryEntries(parent, parentEntries))
            return false;

        std::vector<String> folders;
        folders.reserve(parentEntries.size());
        for (const auto &entry : parentEntries)
        {
            if (entry.directory)
                folders.push_back(entry.name);
        }

        int currentIndex = -1;
        for (size_t i = 0; i < folders.size(); ++i)
        {
            if (folders[i] == currentName)
            {
                currentIndex = static_cast<int>(i);
                break;
            }
        }

        int siblingIndex = currentIndex + direction;
        if (currentIndex >= 0 && siblingIndex >= 0 && siblingIndex < static_cast<int>(folders.size()))
        {
            String siblingFolder = joinPath(parent, folders[siblingIndex]);
            return firstPlayableTrackInFolder(siblingFolder, outPath);
        }

        folder = parent;
    }

    return false;
}

void GUI::drawButtons()
{
    int width = tft.width();
    int buttonAreaHeight = tft.height() / 2;
    int buttonTop = tft.height() - buttonAreaHeight;
    int cellWidth = width / kButtonCols;
    int cellHeight = buttonAreaHeight / kButtonRows;

    struct ButtonLabel
    {
        const char *text;
        uint16_t color;
    };

    const ButtonLabel labels[6] = {
        {"UP", TFT_DARKGREEN},
        {"BACK", TFT_DARKCYAN},
        {"", TFT_DARKGREY},
        {"", TFT_DARKGREY},
        {"DOWN", TFT_DARKGREEN},
        {"OPEN", TFT_ORANGE}
    };

    for(int row=0; row<kButtonRows; ++row)
    {
        for(int col=0; col<kButtonCols; ++col)
        {
            int index = row * kButtonCols + col;
            int x = col * cellWidth;
            int y = buttonTop + row * cellHeight;

            tft.fillRect(x, y, cellWidth, cellHeight, labels[index].color);
            tft.drawRect(x, y, cellWidth, cellHeight, TFT_BLACK);
            if (labels[index].text[0] != '\0')
            {
                tft.setTextColor(TFT_WHITE, labels[index].color);
                tft.setTextFont(2);
                tft.drawCentreString(labels[index].text, x + cellWidth / 2, y + (cellHeight / 2) - 8, 2);
            }
        }
    }
}

int GUI::buttonIndexFromPoint(const CYD28_TS_Point &point) const
{
    int buttonAreaHeight = tft.height() / 2;
    int buttonTop = tft.height() - buttonAreaHeight;

    if (point.y < buttonTop)
        return 0;

    int cellWidth = tft.width() / kButtonCols;
    int cellHeight = buttonAreaHeight / kButtonRows;
    int col = point.x / cellWidth;
    int row = (point.y - buttonTop) / cellHeight;

    if (col < 0 || col >= kButtonCols || row < 0 || row >= kButtonRows)
        return 0;

    return row * kButtonCols + col + 1;
}

bool GUI::isInButtonArea(const CYD28_TS_Point &point) const
{
    int buttonAreaHeight = tft.height() / 2;
    int buttonTop = tft.height() - buttonAreaHeight;
    return point.y >= buttonTop;
}

void GUI::handleButton(int buttonIndex)
{
    if (screen == 1)
    {
        switch(buttonIndex)
        {
            case 1:
                audioPauseResume();
                playerPaused = !playerPaused;
                drawPlayer();
                break;
            case 2:
            {
                uint8_t vol = volumePercent;
                if (vol < 100)
                    vol = (vol + kVolumeStepPercent > 100) ? 100 : (vol + kVolumeStepPercent);
                uint8_t maxVol = audio.maxVolume();
                uint8_t rawVol = maxVol ? (vol * maxVol + 50) / 100 : vol;
                audioSetVolume(rawVol);
                setVolumePercent(vol);
                drawPlayer();
                break;
            }
            case 3:
                playPreviousTrack();
                break;
            case 4:
                playNextTrack();
                break;
            case 5:
                audioStopSong();
                playerPaused = false;
                setPlaybackTime(0, 0);
                drawPlayer();
                break;
            case 6:
            {
                uint8_t vol = volumePercent;
                if (vol > 0)
                    vol = (vol < kVolumeStepPercent) ? 0 : (vol - kVolumeStepPercent);
                uint8_t maxVol = audio.maxVolume();
                uint8_t rawVol = maxVol ? (vol * maxVol + 50) / 100 : vol;
                audioSetVolume(rawVol);
                setVolumePercent(vol);
                drawPlayer();
                break;
            }
            default:
                break;
        }
        return;
    }

    switch(buttonIndex)
    {
        case 1:
            scroll(-1);
            draw();
            break;
        case 2:
            goBack();
            draw();
            break;
        case 5:
            scroll(1);
            draw();
            break;
        case 6:
            enterSelection();
            draw();
            break;
        default:
            break;
    }
}

void GUI::touch(const CYD28_TS_Point &point)
{
    uint32_t now = millis();
    if (now - lastTapMs < 150)
        return;

    lastTapMs = now;

    if (!isInButtonArea(point))
    {
        screen = (screen == 0) ? 1 : 0;
        draw();
        return;
    }

    int buttonIndex = buttonIndexFromPoint(point);
    if (buttonIndex == 0)
        return;

    handleButton(buttonIndex);
}

void GUI::scroll(int delta)
{
    if (entries.empty())
        return;

    selected+=delta;

    if(selected<0)
        selected=0;

    if(selected>=entries.size())
        selected=entries.size()-1;

    clampScrolling();
}

void GUI::clampScrolling()
{
    if (entries.empty())
    {
        selected = 0;
        firstVisible = 0;
        return;
    }

    int rows = visibleRows();

    if(selected<firstVisible)
        firstVisible=selected;

    if(selected>=firstVisible+rows)
        firstVisible=selected-rows+1;

    if (firstVisible < 0)
        firstVisible = 0;

    int maxFirstVisible = entries.size() - rows;
    if (maxFirstVisible < 0)
        maxFirstVisible = 0;

    if (firstVisible > maxFirstVisible)
        firstVisible = maxFirstVisible;
}

bool GUI::enterSelection()
{
    if(entries.empty())
        return false;

    auto &e=entries[selected];

    if(e.directory)
    {
        String p=currentPath;

        if(!p.endsWith("/"))
            p+="/";

        p+=e.name;

        loadDirectory(p);

        return true;
    }

    chosenFile=currentPath;

    if(!chosenFile.endsWith("/"))
        chosenFile+="/";

    chosenFile+=e.name;

    fileChosen=true;

    return true;
}

bool GUI::goBack()
{
    if(currentPath=="/")
        return false;

    int pos=currentPath.lastIndexOf('/');

    if(pos==0)
        currentPath="/";
    else
        currentPath=currentPath.substring(0,pos);

    return loadDirectory(currentPath);
}

bool GUI::hasSelectedFile()
{
    return fileChosen;
}

String GUI::selectedFile()
{
    fileChosen=false;
    return chosenFile;
}
