/*
Portable ZX-Spectrum emulator.
Copyright (C) 2001-2012 SMT, Dexus, Alone Coder, deathsoft, djdron, scor

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef _PSP

#include <pspaudio.h>
#include <pspaudiolib.h>
#include "../platform.h"
#include "../../tools/sound_mixer.h"
#include "../../tools/options.h"

namespace xPlatform
{

static eSoundMixer sound_mixer;

static void AudioCallback(void* buf, unsigned int length, void* userdata)
{
	length *= 4; // translate samples to bytes
	if(length <= sound_mixer.Ready())
	{
		memcpy(buf, sound_mixer.Ptr(), length);
		sound_mixer.Use(length);
	}
	else
	{
		memcpy(buf, sound_mixer.Ptr(), sound_mixer.Ready());
		memset((byte*)buf + sound_mixer.Ready(), 0, length - sound_mixer.Ready());
		sound_mixer.Use(sound_mixer.Ready());
	}
}

void InitAudio()
{
	using namespace xOptions;
	struct eOptionBX : public eOptionB
	{
		void Unuse() { customizable = false; storeable = false; }
	};
	eOptionBX* o = (eOptionBX*)eOptionB::Find("sound");
	SAFE_CALL(o)->Unuse();
	o = (eOptionBX*)eOptionB::Find("volume");
	SAFE_CALL(o)->Unuse();

	pspAudioInit();
	pspAudioSetChannelCallback(0, AudioCallback, NULL);
}

void UpdateAudio()
{
	sound_mixer.Update();
	static bool audio_filled = false;
	bool audio_filled_new = sound_mixer.Ready() > 44100*2*2/50*7; // 7-frame data
	if(audio_filled != audio_filled_new)
	{
		audio_filled = audio_filled_new;
		Handler()->VideoPaused(audio_filled);
	}
}

void DoneAudio()
{
	pspAudioEnd();
}

}
//namespace xPlatform

#endif//_PSP