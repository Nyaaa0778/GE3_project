#pragma once

#include "SoundManager.h"
#include <string>

struct SoundData {
  WAVEFORMATEX wfex;
  BYTE *pBuffer;
  unsigned int bufferSize;
};

class Sound {
public:
  Sound();
  ~Sound();

  bool LoadWav(const std::string &filepath);
  void Play(SoundManager *soundManager);
  void Unload();

private:
  SoundData soundData_{};
  IXAudio2SourceVoice *sourceVoice_ = nullptr;
};
