//copyright (c) 2006 by tarn adams

#include <windows.h>
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

#include "svector.h"

#ifndef INTEGER_TYPES

#define INTEGER_TYPES

#ifdef WIN32
	typedef signed char int8_t;
	typedef short int16_t;
	typedef int int32_t;
	typedef long long int64_t;
	typedef unsigned char uint8_t;
	typedef unsigned short uint16_t;
	typedef unsigned int uint32_t;
	typedef unsigned long long uint64_t;
#endif

typedef int32_t VIndex;
typedef int32_t Ordinal;

#endif

#include "random.h"

using std::string;

#include "endian.h"

#include "files.h"

#include "basics.h"

#include "enabler.h"

#include "init.h"

#ifndef NO_FMOD

#include "music_and_sound_g.h"
#include "music_and_sound_v.h"

//MUSIC AND SOUND FUNCTIONS

//MUSIC
void musicsoundst::startbackgroundmusic(int new_song)
{
	if(!on)return;

	if(new_song!=song/*&&
		(!doing_forced||
		forcesongstart+forcesongtime>enabler.get_timer()||
		forcesongstart>enabler.get_timer())*/)
		{
		//doing_forced=0;
		song=new_song;

		FSOUND_PlaySound(0,mod[new_song]);
		}
}

void musicsoundst::forcebackgroundmusic(int new_song,uint32_t time)
{
	if(!on)return;

	if(new_song!=song)
		{
		if(song!=new_song)stopbackgroundmusic();
		song=new_song;

		FSOUND_PlaySound(0,mod[song]);
		}

	/*
	doing_forced=1;
	forcesongtime=time;
	forcesongstart=enabler.get_timer();
	*/
}

void musicsoundst::stopbackgroundmusic()
{
	if(!on)return;

	FSOUND_StopSound(0);
	song=-1;
}

//SOUND
void musicsoundst::playsound(int s,int channel)
{
	if(!on)return;

	if(channel>=0)
		{
		FSOUND_SAMPLE *smp;
		int smp_priority;
		int samp_priority;
		FSOUND_Sample_GetDefaults(samp[s],NULL,NULL,NULL,&samp_priority);
		smp=FSOUND_GetCurrentSample(channel);

		if(smp!=NULL)
			{
			FSOUND_Sample_GetDefaults(smp,NULL,NULL,NULL,&smp_priority);

			if(smp_priority>samp_priority)return;
			}

		FSOUND_PlaySound(channel,samp[s]);
		}
	else FSOUND_PlaySound(FSOUND_FREE,samp[s]);
}

void musicsoundst::playsound(int s,int min_channel,int max_channel,int force_channel)
{
	if(!on)return;

	if(min_channel!=-1&&max_channel!=-1)
		{
		FSOUND_SAMPLE *smp;
		int smp_priority;
		int samp_priority;
		FSOUND_Sample_GetDefaults(samp[s],NULL,NULL,NULL,&samp_priority);
		int32_t c;
		for(c=min_channel;c<=max_channel;c++)
			{
			smp=FSOUND_GetCurrentSample(c);

			if(smp!=NULL)
				{
				FSOUND_Sample_GetDefaults(smp,NULL,NULL,NULL,&smp_priority);

				if(smp_priority<samp_priority)
					{
					force_channel=c;
					break;
					}
				}
			else
				{
				force_channel=c;
				break;
				}
			}

		if(c<=max_channel)
			{
			if(force_channel>=0)FSOUND_PlaySound(force_channel,samp[s]);
			else return;
			}
		}
	else playsound(s,force_channel);
}

void musicsoundst::initsound()
{
	on=1;

	musicactive=0;
	soundplaying=-1;
	soundpriority=0;
	song=-1;
	forcesongtime=-1;

	FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
	FSOUND_SetDriver(0);

	//NOT USED CURRENTLY
	int num2d,num3d,total;
	FSOUND_GetNumHWChannels(&num2d,&num3d,&total);

	SoftChannelNumber=SOUND_CHANNELNUM;
	FSOUND_Init(44100, SoftChannelNumber, FSOUND_INIT_USEDEFAULTMIDISYNTH);
	FSOUND_SetSFXMasterVolume(init.media.volume);

	FSOUND_SetPriority(FSOUND_ALL,0);
}

void musicsoundst::set_master_volume(int32_t newvol)
{
	FSOUND_SetSFXMasterVolume(newvol);
}

void musicsoundst::deinitsound()
{
	if(!on)return;

	stopbackgroundmusic();

	int s;
	for(s=0;s<MAXSONGNUM;s++)
		{
		if(mod[s]!=NULL)
			{
			FSOUND_Sample_Free(mod[s]);
			mod[s]=NULL;
			}
		}

	for(s=0;s<MAXSOUNDNUM;s++)
		{
		if(samp[s]!=NULL)
			{
			FSOUND_Sample_Free(samp[s]);
			samp[s]=NULL;
			}
		}

	FSOUND_Close();
}

void musicsoundst::set_song(string &filename,int slot)
{
	if(!on)return;

	if(mod[slot]!=NULL)
		{
		FSOUND_Sample_Free(mod[slot]);
		mod[slot]=NULL;
		}

	if(SoftChannelNumber>0)mod[slot]=FSOUND_Sample_Load(FSOUND_UNMANAGED,filename.c_str(),FSOUND_16BITS | FSOUND_SIGNED | FSOUND_STEREO,0,0);
	else mod[slot]=FSOUND_Sample_Load(FSOUND_UNMANAGED,filename.c_str(),FSOUND_16BITS | FSOUND_SIGNED | FSOUND_STEREO | FSOUND_HW2D,0,0);

	FSOUND_Sample_SetMode(mod[slot],FSOUND_LOOP_NORMAL);
	FSOUND_Sample_SetDefaults(mod[slot],-1,-1,-1,255);
}

void musicsoundst::stop_sound(int channel)
{
	if(!on)return;

	FSOUND_StopSound(channel);
}

void musicsoundst::set_sound(string &filename,int slot,int pan,int priority)
{
	if(!on)return;

	if(samp[slot]!=NULL)
		{
		FSOUND_Sample_Free(samp[slot]);
		samp[slot]=NULL;
		}

	if(SoftChannelNumber>0)samp[slot]=FSOUND_Sample_Load(FSOUND_UNMANAGED,filename.c_str(),FSOUND_16BITS | FSOUND_SIGNED | FSOUND_STEREO,0,0);
	else samp[slot]=FSOUND_Sample_Load(FSOUND_UNMANAGED,filename.c_str(),FSOUND_16BITS | FSOUND_SIGNED | FSOUND_STEREO | FSOUND_HW2D,0,0);

	if(samp[slot]==NULL)
		{
		errorlog_string(filename);
		}

	FSOUND_Sample_SetMode(samp[slot],FSOUND_LOOP_OFF);
	FSOUND_Sample_SetDefaults(samp[slot],-1,-1,pan,priority);
}

void musicsoundst::set_sound_params(int slot,int p1,int vol,int pan,int priority)
{
	if(!on)return;

	FSOUND_Sample_SetDefaults(musicsound.samp[slot],p1,vol,pan,priority);
}

#endif