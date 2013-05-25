//copyright (c) 2006 by tarn adams

#define SOUND_CHANNELNUM 16

#include <string>
#include <map>

#ifndef NO_FMOD

#include <fmod.hpp>
#include <fmod_errors.h>

/* The maximums can be no larger than the largest value
 * of a signed integer.
 */
#define MAXSONGNUM       1000
#define MAXSOUNDNUM      1000
#define FSOUND_STEREOPAN 0

struct fmodSound {
	FMOD::Sound *sound;
	FMOD::Channel *channel;
};



class musicsoundst
{
	public:
		enum linux_sound_system {
			ALSA,
			OSS,
			ESD,
		};
		
		int SoftChannelNumber;

		musicsoundst() : song(-1), system(NULL), masterchannelgroup(NULL), sound_system(ALSA)
		{
			int s;
			for (s = 0; s < MAXSONGNUM; s++) {
				mod[s].sound = NULL;
				mod[s].channel = NULL;
			}

			for (s = 0; s < MAXSOUNDNUM; s++) {
				samp[s].sound = NULL;
				samp[s].channel = NULL;
			}
		}
		~musicsoundst()
		{
			deinitsound();
		}

		void startbackgroundmusic(int new_song);
		void stopbackgroundmusic();
		void playsound(int s,int channel=-1);
		void playsound(int s,int min_channel,int max_channel,int force_channel);
		void initsound();
		void deinitsound();
		void set_song(string &filename,int slot);
		void set_sound(string &filename,int slot,int pan=-1,int priority=0);
		void set_sound_params(int slot,int p1,int vol,int pan,int priority);
		void stop_sound(int channel);
		void stop_sound() {
			masterchannelgroup->stop();
		}
		void set_master_volume(long newvol);
		void update() {
			if (!on) {
				return;
			}

			system->update();
		}
		
		void set_sound_system(musicsoundst::linux_sound_system system) {
			sound_system = system;
		}

	private:
		float oldval_to_volumefloat(int val);
		float oldval_to_panfloat(int val);
		int oldval_to_priority(int val);

		int song;
		char musicactive;
		char soundpriority;
		int soundplaying;

		char on;

		FMOD::System *system;
		FMOD::ChannelGroup *masterchannelgroup;
		fmodSound      mod[MAXSONGNUM];
		fmodSound      samp[MAXSOUNDNUM];

		musicsoundst::linux_sound_system sound_system;
};
#endif

