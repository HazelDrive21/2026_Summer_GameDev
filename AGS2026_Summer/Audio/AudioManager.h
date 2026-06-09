#pragma once
#include "SoundTable.h"

class AudioManager
{
public:
	static void CreateInstance(void) { if (instance_ == nullptr) { instance_ = new AudioManager(); } }
	static AudioManager* GetInstance(void) { return instance_; }
	static void DeleteInstance(void) { if (instance_ != nullptr) { delete instance_; instance_ = nullptr; } }

private:
	static AudioManager* instance_;
	AudioManager();
	~AudioManager();

	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;
	AudioManager(AudioManager&&) = delete;
	AudioManager& operator=(AudioManager&&) = delete;

public:
	void Init(void);
	void LoadSceneSound(LoadScene scene);
	void DeleteSceneSound(LoadScene scene);

	void PlayBGM(SoundID id);
	void StopBGM(void);
	void PlaySE(SoundID id);
	void DeleteAll(void);

	void PlaySELoop(SoundID id); // 🔥 SEをループ再生する
	void StopSE(SoundID id);     // 🔥 指定したSEを停止する
	bool IsPlaySE(SoundID id);   // 🔥 SEが現在再生中か調べる

	void SetBgmVolume(int volume);
	void SetSeVolume(int volume);
	void SetMasterVolume(int volume);

	int GetBgmVolume() const { return bgmVolume_; }
	int GetSeVolume() const { return seVolume_; }
	int GetMasterVolume() const { return masterVolume_; }

private:
	std::unordered_map<SoundID, int> handles_;
	SoundID currentBgm_;
	int bgmVolume_;
	int seVolume_;
	int masterVolume_;
};