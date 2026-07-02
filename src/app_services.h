#pragma once

#include <Arduino.h>
#include <vector>

struct FileEntry
{
    String name;
    bool directory;
    uint32_t size;
};

class FileSystemService
{
public:
    virtual ~FileSystemService() = default;

    virtual void begin() = 0;
    virtual bool exists(const String &path) const = 0;
    virtual bool loadDirectoryEntries(const String &path, std::vector<FileEntry> &outEntries) const = 0;
    virtual bool readTextFile(const String &path, String &outText) const = 0;
    virtual bool writeTextFile(const String &path, const String &text) = 0;
    virtual bool removeFile(const String &path) = 0;
    virtual bool renameFile(const String &from, const String &to) = 0;
};

class AudioService
{
public:
    virtual ~AudioService() = default;

    virtual uint8_t maxVolume() const = 0;
    virtual uint8_t volumePercent() const = 0;
    virtual uint32_t rms() const = 0;
    virtual uint32_t currentTime() = 0;
    virtual uint32_t duration() = 0;
    virtual void setVolumeRaw(uint8_t rawVolume) = 0;
    virtual void pauseResume() = 0;
    virtual void stopSong() = 0;
    virtual bool connectToSD(const char *filename) = 0;
};
