#ifndef NO_FMOD

#include "platform.h"
#include <string.h>
#include <math.h>
#include <iosfwd>
#include <iostream>
#include <ios>
#include <streambuf>
#include <istream>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <map>

#include "svector.h"

#ifdef WIN32

#ifndef INTEGER_TYPES
	#define INTEGER_TYPES
	typedef short int16_t;
	typedef int int32_t;
	typedef long long int64_t;
	typedef unsigned short uint16_t;
	typedef unsigned int uint32_t;
	typedef unsigned long long uint64_t;
#endif

typedef int32_t VIndex;
typedef int32_t Ordinal;

#endif

#include "random.h"

using std::string;
using std::map;
using std::pair;

#include "basics.h"
#include "endian.h"
#include "files.h"
#include "enabler.h"
#include "init.h"

#include "music_and_sound_fmodex.h"
#include "music_and_sound_v.h"

void musicsoundst::startbackgroundmusic(int new_song)
{	
	if (!on || new_song < 0 || new_song > MAXSONGNUM || mod[new_song].sound == NULL) {
		return;
	}

	if (song != new_song) {

		stopbackgroundmusic(); /* This is safe to call, even if song isn't valid. */

		song = new_song;
		FMOD_CHANNELINDEX cid = static_cast<FMOD_CHANNELINDEX>(0);
		system->playSound(cid, mod[song].sound, false, &mod[song].channel);
	}
}

void musicsoundst::stopbackgroundmusic()
{
	if (!on || song == -1) {
		return;
	}

	if (mod[song].channel != NULL) { 
		mod[song].channel->stop();
		mod[song].channel = NULL;
	}

	song = -1;
}

/* Set channel to less than 0 to have FMOD decide the channel for you. */
void musicsoundst::playsound(int s,int channel)
{
	if (!on || s < 0 || s > MAXSOUNDNUM || samp[s].sound == NULL) {
		return;
	}

	if (channel >= 0) {
		FMOD_CHANNELINDEX cid = static_cast<FMOD_CHANNELINDEX>(channel);
		system->playSound(cid, samp[s].sound, false, &samp[s].channel);
	} else {
		system->playSound(FMOD_CHANNEL_FREE, samp[s].sound, false, &samp[s].channel);
	}
}

// Toady:
/* Yeah, I started porting all that other crap, but it wasn't all applicable to ex
 * and besides which, you never call it anyway.
 */
void musicsoundst::playsound(int s, int min_channel, int max_channel, int force_channel)
{
	if (!on || s < 0 || s > MAXSOUNDNUM || samp[s].sound == NULL) {
		return;
	}

	playsound(s, force_channel);
}

// Prints a relevent error message to stderr.
inline void err(FMOD_RESULT r)
{
	std::cerr << "sound: failure: " << FMOD_ErrorString(r) << std::endl;
}

void musicsoundst::initsound()
{
	FMOD_RESULT result = FMOD::System_Create(&system);
	if (result != FMOD_OK) {
		err(result);
		on = 0;
		return;
	}

#if defined(linux)
	/* Set up the sound system. Default to ALSA. */
	switch (this->sound_system) {
	default:
	case ALSA:
		result = system->setOutput(FMOD_OUTPUTTYPE_ALSA);
		break;
	case OSS:
		result = system->setOutput(FMOD_OUTPUTTYPE_OSS);
		break;
	case ESD:
		result = system->setOutput(FMOD_OUTPUTTYPE_ESD);
		break;
	}
	if (result != FMOD_OK) {
		err(result);
		on = 0;
		return;
	}
#endif

	SoftChannelNumber = SOUND_CHANNELNUM;
	result = system->init(SoftChannelNumber, FMOD_INIT_NORMAL, NULL);
	if (result != FMOD_OK) {
		err(result);
		on = 0;
		return;
	}

	result = system->getMasterChannelGroup(&masterchannelgroup);
	if (result != FMOD_OK) {
		err(result);
		on = 0;
		return;
	}
	
	set_master_volume(init.media.volume);

	on = 1;
}

void musicsoundst::deinitsound()
{
	if (!on) {
		return;
	}

	int s;
	for (s = 0; s < MAXSONGNUM; s++) {
		if (mod[s].sound != NULL) {
			mod[s].sound->release();
			mod[s].sound = NULL;
			mod[s].channel = NULL;
		}
	}

	for (s = 0; s < MAXSOUNDNUM; s++) {
		if (samp[s].sound != NULL) {
			samp[s].sound->release();
			samp[s].sound = NULL;
			samp[s].channel = NULL;
		}
	}

	system->release();
	on = 0;
}



void musicsoundst::set_song(string &filename, int slot)
{
	if (!on || slot < 0 || slot > MAXSONGNUM) {
		return;
	}

	if (mod[slot].sound != NULL) {
		mod[slot].sound->release();
		mod[slot].sound = NULL;
		mod[slot].channel = NULL; 
	}

	FMOD_RESULT result = system->createSound(filename.c_str(), FMOD_DEFAULT, 0, &mod[slot].sound);
	if (result != FMOD_OK) {
		mod[slot].sound = NULL;
		mod[slot].channel = NULL;
		return;
	}
	
	mod[slot].sound->setMode(FMOD_LOOP_NORMAL);
}

void musicsoundst::set_sound(string &filename, int slot, int pan, int priority)
{
	if (!on || slot < 0 || slot > MAXSOUNDNUM) {
		return;
	}

	if (samp[slot].sound != NULL) {
		samp[slot].sound->release();
		samp[slot].sound = NULL;
		samp[slot].channel = NULL;
	}

	FMOD_RESULT result = system->createSound(filename.c_str(), FMOD_DEFAULT, 0, &samp[slot].sound);
	if (result != FMOD_OK) {
		samp[slot].sound = NULL;
		samp[slot].channel = NULL;
		return;
	}
}

void musicsoundst::set_sound_params(int slot, int p1, int vol, int pan, int priority)
{
	if (slot < 0 || slot > MAXSOUNDNUM) {
		return;
	} if (samp[slot].channel == NULL || on == 0) {
		return;
	}

	samp[slot].channel->setPan(oldval_to_panfloat(pan));
	samp[slot].channel->setVolume(oldval_to_volumefloat(vol));
	samp[slot].channel->setPriority(oldval_to_priority(priority));
	samp[slot].channel->setFrequency(static_cast<float>(p1));
}

void musicsoundst::stop_sound(int channel)
{
	FMOD::Channel* c;

	FMOD_RESULT result = system->getChannel((int)channel, &c);
	if (result != FMOD_OK) {
		return;
	}

	c->stop();
}


void musicsoundst::set_master_volume(long newvol)
{
	masterchannelgroup->setVolume(oldval_to_volumefloat(newvol));
}

// Converts old FMOD 3 0 - 255 volume values to FMOD Ex 0.0 <-> 1.0 float values.
float musicsoundst::oldval_to_volumefloat(int val) 
{
	if (val < 0) {
		return 0.0;
	} else if (val > 255) {
		return 1.0;
	}

	return static_cast<float>(val) / 255;
}

// Converts old FMOD 3 0 - 255 pan values to FMOD Ex -1.0 <-> 1.0 float values.
float musicsoundst::oldval_to_panfloat(int val)
{
	if (val < 0) { 
		return -1.0;
	} else if (val > 255) {
		return 1.0;
	}
	
	float n = static_cast<float>(val) / 255;
	return (n * 2) - 1.0;
}

// Converts old FMOD 3 0(lowest) - 255(highest) priority values to
// FmodEx 0(highest) - 256(lowest) priority values.
int musicsoundst::oldval_to_priority(int val)
{
	if (val < 0) {
		return 256;
	} else if (val > 255) {
		return 0;
	}

	return 255 - val;
}

#endif // NO_FMOD

