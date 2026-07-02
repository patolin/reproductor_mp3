#include "app_state.h"
#include <cstring>

bool AppState::readState(FileSystemService &fileSystem, String &trackPath, uint8_t &volumePercent)
{
    trackPath = "";
    volumePercent = kDefaultVolumePercent;

    String text;
    if (!fileSystem.readTextFile(kStatePath, text))
        return false;

    int start = 0;
    while (start < text.length())
    {
        int end = text.indexOf('\n', start);
        String line = (end >= 0) ? text.substring(start, end) : text.substring(start);
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

        if (end < 0)
            break;
        start = end + 1;
    }

    return true;
}

bool AppState::writeState(FileSystemService &fileSystem, const String &trackPath, uint8_t volumePercent)
{
    if (volumePercent > 100)
        volumePercent = 100;

    String text;
    text.reserve(trackPath.length() + 32);
    text += kKeyTrack;
    text += '=';
    text += trackPath;
    text += '\n';
    text += kKeyVolume;
    text += '=';
    text += String(volumePercent);
    text += '\n';

    fileSystem.removeFile(kTempPath);

    if (!fileSystem.writeTextFile(kTempPath, text))
        return false;

    fileSystem.removeFile(kStatePath);

    if (!fileSystem.renameFile(kTempPath, kStatePath))
    {
        fileSystem.removeFile(kTempPath);
        return false;
    }

    return true;
}

bool AppState::loadSession(FileSystemService &fileSystem, String &trackPath, uint8_t &volumePercent)
{
    return readState(fileSystem, trackPath, volumePercent);
}

bool AppState::saveSession(FileSystemService &fileSystem, const String &trackPath, uint8_t volumePercent)
{
    return writeState(fileSystem, trackPath, volumePercent);
}
