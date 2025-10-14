#include "SoundManager.h"

SoundManager::SoundManager() = default;

SoundManager::~SoundManager() { Finalize(); }

void SoundManager::Initialize() {
  HRESULT result;
  result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
  assert(SUCCEEDED(result));
  result = xAudio2_->CreateMasteringVoice(&masterVoice_);
  assert(SUCCEEDED(result));
}

void SoundManager::Finalize() {
  if (masterVoice_) {
    masterVoice_->DestroyVoice();
    masterVoice_ = nullptr;
  }
  xAudio2_.Reset();
}
