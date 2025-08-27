#define MINIAUDIO_IMPLEMENTATION
#include "MusicManager.hpp"
#include <stdexcept>

MusicManager gMusicManager;

MusicManager::MusicManager() {
    if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize audio engine.");
    }

    std::array<std::string, static_cast<size_t>(SoundID::COUNT)> autoPaths = {
        "assets/audio/coin.wav",
        "assets/audio/explosion.wav"
    };

    for (size_t i = 0; i < static_cast<size_t>(SoundID::COUNT); ++i)
        load(static_cast<SoundID>(i), autoPaths[i], false);
}

MusicManager::~MusicManager() {
    for (size_t i = 0; i < sounds.size(); i++) {
        if (loaded[i]) {
            ma_sound_uninit(&sounds[i]);
        }
    }
    ma_engine_uninit(&engine);
}

void MusicManager::load(SoundID id, const std::string& filepath, bool loop) {
    const auto index = static_cast<size_t>(id);
    if (loaded[index]) {
        ma_sound_uninit(&sounds[index]);
        loaded[index] = false;
    }

    if (ma_sound_init_from_file(&engine, filepath.c_str(), 0, nullptr, nullptr, &sounds[index]) != MA_SUCCESS) {
        throw std::runtime_error("Failed to load sound: " + filepath);
    }

    ma_sound_set_looping(&sounds[index], loop ? MA_TRUE : MA_FALSE);
    loaded[index] = true;
}

void MusicManager::play(SoundID id) {
    if (const auto index = static_cast<size_t>(id); loaded[index]) {
        ma_sound_start(&sounds[index]);
    }
}

void MusicManager::stop(SoundID id) {
    if (const auto index = static_cast<size_t>(id); loaded[index]) {
        ma_sound_stop(&sounds[index]);
    }
}

void MusicManager::setVolume(SoundID id, float volume) {
    if (const auto index = static_cast<size_t>(id); loaded[index]) {
        ma_sound_set_volume(&sounds[index], volume);
    }
}