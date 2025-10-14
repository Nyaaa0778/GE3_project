#include "Sound.h"
#include <cassert>
#include <cstring>
#include <fstream>

/// <summary>
/// 音声データを読み込む
/// </summary>
/// <param name="filename"></param>
/// <returns></returns>
SoundData SoundLoadWave(const char *filename) {
  std::ifstream file(filename, std::ios::binary);
  assert(file.is_open());

  struct ChunkHeader {
    char id[4];
    int32_t size;
  };

  struct RiffHeader {
    ChunkHeader chunk;
    char type[4];
  };

  struct FormatChunk {
    ChunkHeader chunk;
    WAVEFORMATEX fmt;
  };

  // RIFF
  RiffHeader riff;
  file.read(reinterpret_cast<char *>(&riff), sizeof(riff));
  assert(strncmp(riff.chunk.id, "RIFF", 4) == 0);
  assert(strncmp(riff.type, "WAVE", 4) == 0);

  // fmt
  FormatChunk format{};
  file.read(reinterpret_cast<char *>(&format.chunk), sizeof(format.chunk));
  assert(strncmp(format.chunk.id, "fmt ", 4) == 0);
  size_t fmtReadSize = std::min<size_t>(format.chunk.size, sizeof(format.fmt));
  file.read(reinterpret_cast<char *>(&format.fmt), fmtReadSize);

  if (format.chunk.size > fmtReadSize) {
    file.seekg(format.chunk.size - fmtReadSize, std::ios_base::cur);
  }

  ChunkHeader chunk{};
  while (true) {
    file.read(reinterpret_cast<char *>(&chunk), sizeof(chunk));
    if (file.eof()) {
      assert(false && "data chunk not found");
    }
    if (strncmp(chunk.id, "data", 4) == 0)
      break;
    std::streamoff skipSize = chunk.size + (chunk.size % 2);
    file.seekg(skipSize, std::ios_base::cur);
  }

  char *pBuffer = new char[chunk.size];
  file.read(pBuffer, chunk.size);
  file.close();

  SoundData data;
  data.wfex = format.fmt;
  data.pBuffer = reinterpret_cast<BYTE *>(pBuffer);
  data.bufferSize = chunk.size;
  return data;
}

/// <summary>
/// 音声データの解放
/// </summary>
/// <param name="data"></param>
void SoundUnload(SoundData *data) {
  delete[] data->pBuffer;
  data->pBuffer = nullptr;
  data->bufferSize = 0;
  data->wfex = {};
}

Sound::Sound() = default;

Sound::~Sound() { Unload(); }

bool Sound::LoadWav(const std::string &filepath) {
  soundData_ = SoundLoadWave(filepath.c_str());
  return true;
}

/// <summary>
/// 音声再生
/// </summary>
/// <param name="manager"></param>
void Sound::Play(SoundManager *manager) {
  HRESULT result;
  result =
      manager->GetXAudio2()->CreateSourceVoice(&sourceVoice_, &soundData_.wfex);
  assert(SUCCEEDED(result));

  XAUDIO2_BUFFER buf{};
  buf.pAudioData = soundData_.pBuffer;
  buf.AudioBytes = soundData_.bufferSize;
  buf.Flags = XAUDIO2_END_OF_STREAM;

  result = sourceVoice_->SubmitSourceBuffer(&buf);
  assert(SUCCEEDED(result));
  result = sourceVoice_->Start();
  assert(SUCCEEDED(result));
}

void Sound::Unload() {
  if (sourceVoice_) {
    sourceVoice_->DestroyVoice();
    sourceVoice_ = nullptr;
  }
  SoundUnload(&soundData_);
}
