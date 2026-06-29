#include "gui.h"
#include "CYD28_SD.h"

GUI::GUI(TFT_eSPI &display)
: tft(display)
{
    currentPath="/";
    selected=0;
    screen=0;
    firstVisible=0;
    fileChosen=false;
    statusMessage="Initializing...";
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
    return 8;
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
            break;
        case 1:     
            drawPlayer();
    }
    drawButtons();
    
}

void GUI::drawHeader()
{
    tft.fillRect(0,0,320,20,TFT_BLUE);

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
    int y=24;

    int rows=visibleRows();

    for(int i=0;i<rows;i++)
    {
        int idx=firstVisible+i;

        if(idx>=entries.size())
            break;

        if(idx==selected)
            tft.fillRect(0,y,320,24,TFT_DARKGREEN);

        tft.setTextFont(1);
        tft.setTextColor(TFT_WHITE);

        String s;

        if(entries[idx].directory)
            s="[DIR] ";

        else
            s="      ";

        s+=entries[idx].name;

        tft.drawString(s,5,y+4);
        Serial.println("Drawing entry: "+s);
        y+=24;
    }
}

void GUI::drawPlayer()
{
    tft.fillRect(0,24,320,192,TFT_BLACK);

    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("Tocando: "+chosenFile,160,100,2);
}

void GUI::drawButtons()
{
    int y=216;

    tft.fillRect(0,y,320,24,TFT_DARKGREY);

    tft.setTextFont(2);
    tft.drawCentreString("/\\",40,y+4,2);
    tft.drawCentreString("ABR.",120,y+4,2);
    tft.drawCentreString("<=",200,y+4,2);
    tft.drawCentreString("ACT.",280,y+4,2);
}

void GUI::touch(uint16_t x,uint16_t y)
{
    if (screen == 1) {
        // If in player screen, any touch returns to file list
        screen = 0;
        draw();
        return;
    }

    if(y<20)
        return;

    if(y<216)
    {
        int row=(y-24)/24;

        int idx=firstVisible+row;

        if(idx<entries.size())
        {
            selected=idx;
            draw();

            enterSelection();
        }

        return;
    }

    if(x<80)
    {
        scroll(-1);
    }
    else if(x<160)
    {
        enterSelection();
    }
    else if(x<240)
    {
        goBack();
    }
    else
    {
        refresh();
    }

    draw();
}

void GUI::scroll(int delta)
{
    selected+=delta;

    if(selected<0)
        selected=0;

    if(selected>=entries.size())
        selected=entries.size()-1;

    clampScrolling();
}

void GUI::clampScrolling()
{
    if(selected<firstVisible)
        firstVisible=selected;

    if(selected>=firstVisible+visibleRows())
        firstVisible=selected-visibleRows()+1;
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