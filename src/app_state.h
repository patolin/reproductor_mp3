#pragma once

#include <Arduino.h>

class AppState
{
public:
    bool begin();

    bool loadSession(String &trackPath, uint8_t &volumePercent);

    bool saveSession(const String &trackPath, uint8_t volumePercent);

private:
    static constexpr const char *kStatePath = "/player_state.txt";
    static constexpr const char *kTempPath = "/player_state.tmp";
    static constexpr const char *kKeyTrack = "track";
    static constexpr const char *kKeyVolume = "volume";
    static constexpr uint8_t kDefaultVolumePercent = 21;

    bool readState(String &trackPath, uint8_t &volumePercent);
    bool writeState(const String &trackPath, uint8_t volumePercent);
};
