#ifdef WIN32
//copyright (c) 2006 by tarn adams

#define SOUND_CHANNELNUM 16

#ifndef NO_FMOD

#include "../../fmod/inc/fmod.h"
#include "../../fmod/inc/fmod_errors.h"

#define MAXSONGNUM 1000
#define MAXSOUNDNUM 1000

#ifndef MUSICSOUND
#define MUSICSOUND

class musicsoundst
{
	public:
		int SoftChannelNumber;

		musicsoundst()
			{
			doing_forced=0;
			int s;
			for(s=0;s<MAXSONGNUM;s++)mod[s]=NULL;
			for(s=0;s<MAXSOUNDNUM;s++)samp[s]=NULL;
			}
		~musicsoundst()
			{
			deinitsound();
			}

		void startbackgroundmusic(int song);
		void stopbackgroundmusic();
		void playsound(int s,int channel=-1);
		void playsound(int s,int min_channel,int max_channel,int force_channel);
		void initsound();
		void deinitsound();
		void forcebackgroundmusic(int song,uint32_t time);
		void set_song(string &filename,int slot);
		void set_sound(string &filename,int slot,int pan=-1,int priority=0);
		void set_sound_params(int slot,int p1,int vol,int pan,int priority);
		void stop_sound(int channel=FSOUND_ALL);
		void set_master_volume(int32_t newvol);

	private:
		int song;
		char musicactive;
		char soundpriority;
		int soundplaying;

		char doing_forced;
		uint32_t forcesongtime;
		uint32_t forcesongstart;

		char on;

		FSOUND_SAMPLE *mod[MAXSONGNUM];
		FSOUND_SAMPLE *samp[MAXSOUNDNUM];
};
#endif
#endif
#else
#ifndef MUSIC_AND_SOUND_G
#define MUSIC_AND_SOUND_G

// Fmodex works well enough on windows/os x, but on some linux distributions it fails badly
#if defined(linux)
#include "music_and_sound_openal.h"
#else
#include "music_and_sound_fmodex.h"
#endif // unix

extern musicsoundst musicsound;

#endif
#endif