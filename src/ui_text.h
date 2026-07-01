#pragma once

namespace UIText
{
enum class Language
{
    English,
    Spanish
};

struct Strings
{
    const char *screenTitlePlayer;
    const char *statusInitializing;
    const char *statusNoTrack;
    const char *statusSelected;
    const char *statusNoSdCardOrInvalidFolder;
    const char *statusEmptyFolder;
    const char *statusNoEntries;
    const char *playerNowPlaying;
    const char *playerDiscPrefix;
    const char *playerElapsedPrefix;
    const char *playerTotalPrefix;
    const char *timeZero;
    const char *timeUnknown;
    const char *ellipsis;
    const char *directoryPrefix;
    const char *filePrefix;
    const char *buttonPause;
    const char *buttonResume;
    const char *buttonVolUp;
    const char *buttonPrev;
    const char *buttonNext;
    const char *buttonStop;
    const char *buttonVolDown;
    const char *buttonUp;
    const char *buttonBack;
    const char *buttonDown;
    const char *buttonOpen;
};

void setLanguage(Language language);
Language currentLanguage();
const Strings &strings();
}
