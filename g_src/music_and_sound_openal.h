#ifndef MUSIC_AND_SOUND_OPENAL_H
#define MUSIC_AND_SOUND_OPENAL_H

#include <AL/al.h>
#include <AL/alc.h>

// HACKY HACKY HACK
// Fixes sndfile.h, until the bug is properly fixed
#include <stdio.h>
#include <sys/types.h>
#define _MSCVER
typedef int64_t __int64;
#include <sndfile.h>
#undef _MSCVER
// END HACKY HACKY HACK

#include <string>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <utility>

#define SOUND_CHANNELNUM 16

// Preferred mixer frequency. Should be the same as what the ogg files
// use, to avoid resampling.
#define SOUND_FREQUENCY 44100

// If the bool is false, a sound; otherwise a song
typedef std::pair<bool,int> slot;

class musicsoundst
{
 public:
  bool initsound(); // Returns false if it failed
  void update() {}
  void set_master_volume(long newvol);

  void set_song(std::string &filename, slot slot);
  void set_song(std::string &filename, int slot);
  void playsound(slot slot);
  void playsound(int slot); // Assumes sound

  void startbackgroundmusic(slot slot);
  void startbackgroundmusic(int slot); // Assumes song
  void stopbackgroundmusic();
  void stop_sound();
  void stop_sound(slot slot);
  void playsound(int s,int channel);
  void set_sound(std::string &filename,int slot,int pan=-1,int priority=0);
  void deinitsound();
                 
  // Deprecated:
  void forcebackgroundmusic(int slot, unsigned long time);
  void playsound(int s,int min_channel,int max_channel,int force_channel);
  void set_sound_params(int slot,int p1,int vol,int pan,int priority);

  musicsoundst() {
    functional = false;
    background_slot = slot(false,-1);
  }

  ~musicsoundst() {
    deinitsound();
  }
  
 private:
  bool functional;
  ALCdevice *device;
  ALCcontext *context;

  std::map<std::string,ALuint> buffers; // OpenAL buffers
  std::map<std::string,ALuint> sources; // And sources
  std::map<slot, ALuint> slot_buffer; // Mappings from DF slots to openal
  std::map<slot, ALuint> slot_source;

  slot background_slot; // Currently playing background music, or -1
};


#endif
