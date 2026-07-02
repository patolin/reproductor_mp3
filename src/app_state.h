#pragma once

#include <Arduino.h>
#include "app_services.h"

class AppState
{
public:
    bool loadSession(FileSystemService &fileSystem, String &trackPath, uint8_t &volumePercent);

    bool saveSession(FileSystemService &fileSystem, const String &trackPath, uint8_t volumePercent);

private:
    static constexpr const char *kStatePath = "/player_state.txt";
    static constexpr const char *kTempPath = "/player_state.tmp";
    static constexpr const char *kKeyTrack = "track";
    static constexpr const char *kKeyVolume = "volume";
    static constexpr uint8_t kDefaultVolumePercent = 21;

    bool readState(FileSystemService &fileSystem, String &trackPath, uint8_t &volumePercent);
    bool writeState(FileSystemService &fileSystem, const String &trackPath, uint8_t volumePercent);
};
