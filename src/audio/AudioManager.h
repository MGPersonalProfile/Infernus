#pragma once
#include "raylib.h"
#include <cstring>
#include <string>
#include <unordered_map>

// Centralized audio management with sound effect and music support.
// Sounds are loaded lazily from assets/audio/ directory.
// If audio files are missing, calls are silently ignored.
class AudioManager {
public:
  static AudioManager &getInstance() {
    static AudioManager instance;
    return instance;
  }

  void init() {
    InitAudioDevice();
    masterVolume = 0.7f;
    sfxVolume = 0.8f;
    musicVolume = 0.5f;
    SetMasterVolume(masterVolume);
  }

  void shutdown() {
    for (auto &pair : sounds)
      UnloadSound(pair.second);
    sounds.clear();

    if (currentMusic.ctxData != nullptr) {
      StopMusicStream(currentMusic);
      UnloadMusicStream(currentMusic);
    }

    CloseAudioDevice();
  }

  // Play a sound effect by name (e.g., "hit", "dash", "death")
  // Looks for assets/audio/sfx/<name>.wav
  void playSFX(const std::string &name) {
    if (!IsAudioDeviceReady())
      return;

    if (sounds.find(name) == sounds.end()) {
      std::string path = "assets/audio/sfx/" + name + ".wav";
      if (FileExists(path.c_str())) {
        sounds[name] = LoadSound(path.c_str());
        SetSoundVolume(sounds[name], sfxVolume);
      } else {
        // Try .ogg
        path = "assets/audio/sfx/" + name + ".ogg";
        if (FileExists(path.c_str())) {
          sounds[name] = LoadSound(path.c_str());
          SetSoundVolume(sounds[name], sfxVolume);
        } else {
          return; // No audio file found — silent
        }
      }
    }

    PlaySound(sounds[name]);
  }

  // Play background music
  void playMusic(const std::string &name) {
    if (!IsAudioDeviceReady())
      return;
    if (currentMusicName == name)
      return; // Already playing this track

    std::string path = "assets/audio/music/" + name + ".ogg";
    if (!FileExists(path.c_str())) {
      path = "assets/audio/music/" + name + ".wav";
      if (!FileExists(path.c_str()))
        return;
    }

    if (currentMusic.ctxData != nullptr) {
      StopMusicStream(currentMusic);
      UnloadMusicStream(currentMusic);
    }

    currentMusic = LoadMusicStream(path.c_str());
    SetMusicVolume(currentMusic, musicVolume);
    PlayMusicStream(currentMusic);
    currentMusicName = name;
  }

  // Must be called every frame to keep music streaming
  void update() {
    if (IsAudioDeviceReady() && currentMusic.ctxData != nullptr)
      UpdateMusicStream(currentMusic);
  }

  void stopMusic() {
    if (currentMusic.ctxData != nullptr) {
      StopMusicStream(currentMusic);
      UnloadMusicStream(currentMusic);
      memset(&currentMusic, 0, sizeof(currentMusic));
      currentMusicName.clear();
    }
  }

  void setMasterVolume(float vol) {
    masterVolume = vol;
    SetMasterVolume(vol);
  }
  float getSFXVolume() const { return sfxVolume; }
  float getMusicVolume() const { return musicVolume; }
  float getMasterVolume() const { return masterVolume; }
  void setSFXVolume(float vol) { sfxVolume = vol; }
  void setMusicVolume(float vol) {
    musicVolume = vol;
    if (currentMusic.ctxData != nullptr)
      SetMusicVolume(currentMusic, vol * musicDuck);
  }

  // Music ducking — multiply current music volume by `duck` (0..1).
  // Game uses this to lower music in combat (more enemies = more duck).
  // Smoothed internally so transitions are not abrupt.
  void setMusicDuckTarget(float duck) {
    if (duck < 0.2f) duck = 0.2f; // never silence completely
    if (duck > 1.0f) duck = 1.0f;
    musicDuckTarget = duck;
  }
  // Call from Game::update each frame to smoothly approach target duck.
  void updateMusicDuck(float deltaTime) {
    if (currentMusic.ctxData == nullptr) return;
    float diff = musicDuckTarget - musicDuck;
    float step = 1.5f * deltaTime; // ~0.66s to fully transition
    if (diff > step)       musicDuck += step;
    else if (diff < -step) musicDuck -= step;
    else                   musicDuck = musicDuckTarget;
    SetMusicVolume(currentMusic, musicVolume * musicDuck);
  }

private:
  AudioManager() = default;
  AudioManager(const AudioManager &) = delete;

  std::unordered_map<std::string, Sound> sounds;
  Music currentMusic = {};
  std::string currentMusicName;

  float masterVolume = 0.7f;
  float sfxVolume = 0.8f;
  float musicVolume = 0.5f;
  float musicDuck = 1.0f;       // current applied multiplier
  float musicDuckTarget = 1.0f; // smoothed toward by updateMusicDuck
};
