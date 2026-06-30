#include "gui.h"
#include "CYD28_SD.h"
#include "CYD28_TouchscreenR.h"
#include <algorithm>

namespace
{
constexpr int kHeaderHeight = 24;
constexpr int kListRowHeight = 24;
constexpr int kButtonRows = 3;
constexpr int kButtonCols = 2;
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
void GUI::drawScreen(int screen)
{
    this->screen=screen;
    draw();
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

bool GUI::loadDirectory(String path)
{
    entries.clear();
    statusMessage="";

    if (SD.cardType() == CARD_NONE)
    {
        sdcard.begin();
    }

    File dir=SD.open(path);

    if(!dir || !dir.isDirectory())
    {
        statusMessage="No SD card or invalid folder";
        currentPath=path;
        selected=0;
        firstVisible=0;
        return false;
    }

    while(true)
    {
        File file=dir.openNextFile();

        if(!file)
            break;

        FileEntry e;

        String n=file.name();

        if(path!="/")
        {
            int p=n.lastIndexOf('/');
            if(p>=0)
                n=n.substring(p+1);
        }

        e.name=n;
        e.directory=file.isDirectory();
        e.size=file.size();

        entries.push_back(e);

        file.close();
    }

    dir.close();

    std::sort(entries.begin(),entries.end(),
    [](const FileEntry&a,const FileEntry&b)
    {
        if(a.directory!=b.directory)
            return a.directory>b.directory;

        return a.name<b.name;
    });

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
    tft.drawString(currentPath,4,4);

    if(!statusMessage.isEmpty())
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
    tft.fillRect(0,kHeaderHeight,tft.width(),tft.height()-kHeaderHeight,TFT_BLACK);

    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("Tocando:", tft.width() / 2, 90, 2);
    tft.drawCentreString(chosenFile, tft.width() / 2, 125, 2);
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
        case 6:
            enterSelection();
            draw();
            break;
        case 5:
            scroll(1);
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
