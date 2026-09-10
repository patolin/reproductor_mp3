#include "app_services_impl.h"

#include "CYD28_SD.h"
#include "CYD28_audio.h"
#include <SD.h>
#include <algorithm>
#include <ff.h>

namespace
{
class AppFileSystemService final : public FileSystemService
{
public:
    void begin() override
    {
        sdcard.begin();
    }

    bool exists(const String &path) const override
    {
        return SD.exists(path.c_str());
    }

    bool loadDirectoryEntries(const String &path, std::vector<FileEntry> &outEntries) const override
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

            FileEntry entry;
            String name = file.name();

            int separator = name.lastIndexOf('/');
            if (separator >= 0)
                name = name.substring(separator + 1);

            String lowerName = name;
            lowerName.toLowerCase();
            const bool directory = file.isDirectory();
            if (name.startsWith(".") ||
                lowerName == "system volume information" ||
                lowerName == "$recycle.bin" ||
                lowerName == "recycler" ||
                lowerName == "recycled" ||
                (!directory && !lowerName.endsWith(".mp3")))
            {
                file.close();
                continue;
            }

            // SD is the application's first and only mounted FAT volume (drive 0).
            // Arduino File does not expose the FAT hidden/system attributes.
            String fatPath = "0:" + path;
            if (!fatPath.endsWith("/"))
                fatPath += '/';
            fatPath += name;
            FILINFO info;
            if (f_stat(fatPath.c_str(), &info) != FR_OK ||
                (info.fattrib & (AM_HID | AM_SYS)))
            {
                file.close();
                continue;
            }

            entry.name = name;
            entry.directory = directory;
            entry.size = file.size();
            outEntries.push_back(entry);
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

    bool readTextFile(const String &path, String &outText) const override
    {
        outText = "";

        File file = SD.open(path, FILE_READ);
        if (!file)
            return false;

        while (file.available())
        {
            outText += static_cast<char>(file.read());
        }

        file.close();
        return true;
    }

    bool writeTextFile(const String &path, const String &text) override
    {
        File file = SD.open(path, FILE_WRITE);
        if (!file)
            return false;

        bool ok = file.print(text);
        file.flush();
        file.close();
        return ok;
    }

    bool removeFile(const String &path) override
    {
        return SD.remove(path.c_str());
    }

    bool renameFile(const String &from, const String &to) override
    {
        return SD.rename(from.c_str(), to.c_str());
    }
};

class AppAudioService final : public AudioService
{
public:
    uint8_t maxVolume() const override
    {
        return audio.maxVolume();
    }

    uint8_t volumePercent() const override
    {
        return audioGetVolumePerCent();
    }

    uint32_t rms() const override
    {
        return audioGetRMS();
    }

    uint32_t currentTime() override
    {
        return audio.getAudioCurrentTime();
    }

    uint32_t duration() override
    {
        return audio.getAudioFileDuration();
    }

    void setVolumeRaw(uint8_t rawVolume) override
    {
        audioSetVolume(rawVolume);
    }

    void pauseResume() override
    {
        audioPauseResume();
    }

    void stopSong() override
    {
        audioStopSong();
    }

    bool connectToSD(const char *filename) override
    {
        return audioConnecttoSD(filename);
    }
};

AppFileSystemService gFileSystemService;
AppAudioService gAudioService;
}

FileSystemService &fileSystemService = gFileSystemService;
AudioService &audioService = gAudioService;
