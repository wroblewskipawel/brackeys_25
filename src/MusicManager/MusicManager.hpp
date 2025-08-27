#ifndef MUSICMANAGER_HPP
#define MUSICMANAGER_HPP

#include "miniaudio.h"
#include <array>
#include <string>

enum class SoundID {
    Coin = 0,
    Explosion,
    COUNT
};

class MusicManager {
    ma_engine engine{};
    std::array<ma_sound, static_cast<size_t>(SoundID::COUNT)> sounds{};
    std::array<bool, static_cast<size_t>(SoundID::COUNT)> loaded{};

public:
    MusicManager();
    ~MusicManager();
    void load(SoundID id, const std::string& filepath, bool loop = false);
    void play(SoundID id);
    void stop(SoundID id);
    void setVolume(SoundID id, float volume);
};

extern MusicManager gMusicManager;

#endif // MUSICMANAGER_HPP
