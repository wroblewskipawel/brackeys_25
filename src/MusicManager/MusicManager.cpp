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

    for (size_t i = 0; i < static_cast<size_t>(SoundID::COUNT); ++i) {
        load(static_cast<SoundID>(i), autoPaths[i], false);
    }
}

MusicManager::~MusicManager() {
    for (auto& soundOpt : sounds) {
        if (soundOpt.has_value()) {
            ma_sound_uninit(&soundOpt.value());
        }
    }
    ma_engine_uninit(&engine);
}

void MusicManager::load(SoundID id, const std::string& filepath, bool loop) {
    auto& soundOpt = sounds[static_cast<size_t>(id)];

    if (soundOpt.has_value()) {
        ma_sound_uninit(&soundOpt.value());
        soundOpt.reset();
    }

    soundOpt.emplace();
    if (ma_sound_init_from_file(&engine, filepath.c_str(), 0, nullptr, nullptr, &soundOpt.value()) != MA_SUCCESS) {
        soundOpt.reset();
        throw std::runtime_error("Failed to load sound: " + filepath);
    }

    ma_sound_set_looping(&soundOpt.value(), loop ? MA_TRUE : MA_FALSE);
}

void MusicManager::play(SoundID id) {
    if (auto& soundOpt = sounds[static_cast<size_t>(id)]; soundOpt.has_value()) {
        ma_sound_start(&soundOpt.value());
    }
}

void MusicManager::stop(SoundID id) {
    if (auto& soundOpt = sounds[static_cast<size_t>(id)]; soundOpt.has_value()) {
        ma_sound_stop(&soundOpt.value());
    }
}

void MusicManager::setVolume(SoundID id, float volume) {
    if (auto& soundOpt = sounds[static_cast<size_t>(id)]; soundOpt.has_value()) {
        ma_sound_set_volume(&soundOpt.value(), volume);
    }
}