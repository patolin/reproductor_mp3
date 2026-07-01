#include "app_state.h"
#include <SD.h>
#include <cstring>

bool AppState::begin()
{
    return true;
}

bool AppState::readState(String &trackPath, uint8_t &volumePercent)
{
    trackPath = "";
    volumePercent = kDefaultVolumePercent;

    File file = SD.open(kStatePath, FILE_READ);
    if (!file)
        return false;

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.startsWith(String(kKeyTrack) + "="))
        {
            trackPath = line.substring(strlen(kKeyTrack) + 1);
        }
        else if (line.startsWith(String(kKeyVolume) + "="))
        {
            int value = line.substring(strlen(kKeyVolume) + 1).toInt();
            if (value < 0)
                value = 0;
            if (value > 100)
                value = 100;
            volumePercent = static_cast<uint8_t>(value);
        }
    }

    file.close();
    return true;
}

bool AppState::writeState(const String &trackPath, uint8_t volumePercent)
{
    if (volumePercent > 100)
        volumePercent = 100;

    SD.remove(kTempPath);

    File file = SD.open(kTempPath, FILE_WRITE);
    if (!file)
        return false;

    file.print(kKeyTrack);
    file.print('=');
    file.println(trackPath);
    file.print(kKeyVolume);
    file.print('=');
    file.println(volumePercent);
    file.flush();
    file.close();

    SD.remove(kStatePath);
    if (!SD.rename(kTempPath, kStatePath))
    {
        SD.remove(kTempPath);
        return false;
    }

    return true;
}

bool AppState::loadSession(String &trackPath, uint8_t &volumePercent)
{
    return readState(trackPath, volumePercent);
}

bool AppState::saveSession(const String &trackPath, uint8_t volumePercent)
{
    return writeState(trackPath, volumePercent);
}
