#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "music_and_sound_openal.h"
#include "music_and_sound_v.h"

#define ABORT(str) do { printf("%s: line %d: %s\n", __FILE__, __LINE__, str); abort(); } while(0);
static bool init_openal();
static bool init_sndfile();

using namespace std;

#define alPrintErrors() do { alPrintErrors_(__FILE__,__LINE__); } while(0);

static void alPrintErrors_(const char* file, int line) {
  ALenum err;
  while ((err = alGetError()) != AL_NO_ERROR) {
    printf("At %s: %d: ", file, line);
    switch (err) {
    case AL_INVALID_NAME: puts("AL_INVALID_NAME detected"); break;
    case AL_INVALID_ENUM: puts("AL_INVALID_ENUM detected"); break;
    case AL_INVALID_VALUE: puts("AL_INVALID_VALUE detected"); break;
    case AL_INVALID_OPERATION: puts("AL_INVALID_OPERATION detected"); break;
    case AL_OUT_OF_MEMORY: puts("AL_OUT_OF_MEMORY detected"); break;
    }
  }
}

bool musicsoundst::initsound() {
  if (functional) return true;

  // Load the libraries
  if (!init_openal()) {
    puts("Dynamically loading the OpenAL library failed, disabling sound");
    MessageBox(NULL, "Dynamically loading the OpenAL library failed, disabling sound", 0, 0);
    return false;
  }
  if (!init_sndfile()) {
    puts("Dynamically loading the sndfile library failed, disabling sound");
    MessageBox(NULL, "Dynamically loading the sndfile library failed, disabling sound", 0, 0);
    return false;
  }
  
  // Find out what devices we have available
  const char *devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
  if (!devices) {
    puts("No sound devices available. Sound disabled. OpenAL broken?");
    return false;
  }

  const char *firstdevice = devices;
  puts("Sound devices available:");
  while (*devices) {
    puts(devices);
    devices += strlen(devices) + 1;
  }
  printf("Picking %s. If your desired device was missing, make sure you have the appropriate 32-bit libraries installed. If you wanted a different device, configure ~/.openalrc appropriately.\n",
         firstdevice);

  // Create the context
  device = alcOpenDevice(firstdevice);
  if (!device)
    return false;

  const ALCint attrlist[] = { ALC_FREQUENCY, SOUND_FREQUENCY,
                              ALC_MONO_SOURCES, 0,
                              ALC_STEREO_SOURCES, SOUND_CHANNELNUM };
  context = alcCreateContext(device, attrlist);
  if (context) {
    puts("Perfect OpenAL context attributes GET");
    goto done;
  }
  context = alcCreateContext(device, NULL);
  if (context) {
    puts("Using OpenAL in compatibility mode");
    goto done;
  }
  alcCloseDevice(device);
  return false;

 done:
  if (ALC_FALSE == alcMakeContextCurrent(context)) {
    puts("alcMakeContextCurrent failed");
    return false;
  }
  functional = true;
  return true;
}

// int main() {
//   musicsound.initsound();
//   string str = "data/sound/song_title.ogg";
//   musicsound.set_song(str, 14);
//   musicsound.startbackgroundmusic(14);
//   sleep(9999);
//   exit(1);
// }

void musicsoundst::set_song(string &filename, slot slot) {
  if (!functional) return;

  // printf("%s requested in %d-%d\n", filename.c_str(), (int)slot.first, slot.second);
  if (!buffers.count(filename)) {
    // Song not already loaded. Load it.
    SF_INFO sfinfo;
    sfinfo.format = 0;
    SNDFILE *sf = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    if (!sf) {
      printf("%s not found, sound not loaded\n", filename.c_str());
      goto end;
    } 
    short *buffer = new short[sfinfo.channels * sfinfo.frames];
    sf_count_t frames_read = sf_readf_short(sf, buffer, sfinfo.frames);
    if (frames_read != sfinfo.frames)
      printf("%s: %d frames requested, %d frames read. Corrupted file?\n",
             filename.c_str(), (int)sfinfo.frames, (int)frames_read);
    sf_close(sf);
    // Construct openal buffer and load this
    ALuint albuf;
    alGenBuffers(1, &albuf);
    if (!alIsBuffer(albuf)) {
      puts("Constructing OpenAL buffer mysteriously failed!");
      goto end;
    }
    ALenum format;
    switch (sfinfo.channels) {
    case 1: format = AL_FORMAT_MONO16;
      break;
    case 2: format = AL_FORMAT_STEREO16;
      break;
    default:
      printf("%s: Unexpected number of channels: %d\n",
             filename.c_str(), (int)sfinfo.channels);
      goto end;
    }
    alBufferData(albuf, format, (ALvoid*)buffer,
                 sfinfo.channels * sfinfo.frames * 2, sfinfo.samplerate);
    alPrintErrors();
    delete[] buffer;

    // Create a source for this song
    ALuint source;
    alGenSources(1, &source);
    if (!alIsSource(source)) {
      puts("Constructing OpenAL source mysteriously failed!");
      goto end;
    }
    alSourceQueueBuffers(source, 1, &albuf);
    
    buffers[filename] = albuf;
    sources[filename] = source;
  }

  // Store the requested song in the requested slot.
  // Say, should this alter the playing song if that slot is already playing?
  slot_buffer[slot] = buffers[filename];
  slot_source[slot] = sources[filename];

  end:
    alPrintErrors();
}

void musicsoundst::set_song(string &filename, int slot) {
  set_song(filename, make_pair(true, slot));
}

void musicsoundst::set_master_volume(long newvol) {
  if (!functional) return;
  alListenerf(AL_GAIN, newvol / 255.0f);
}

void musicsoundst::playsound(slot slot) {
  if (!functional) return;
  // printf("%d requested\n", slot);
  if (!slot_source.count(slot)) {
    // printf("Slot %d-%d requested, but no song loaded\n", (int)slot.first, slot.second);
    return;
  }
  if (background_slot == slot) {
    puts("playsound called on background song, background song cancelled!?");
    background_slot = make_pair(false,-1);
  }
  alSourcei(slot_source[slot], AL_LOOPING, AL_FALSE);
  alSourcePlay(slot_source[slot]);
  alPrintErrors();
}

void musicsoundst::playsound(int slot) {
  playsound(make_pair(false,slot));
}

void musicsoundst::startbackgroundmusic(slot slot) {
  if (!functional) return;

  if (!slot_source.count(slot)) {
    // printf("Slot %d-%d requested, but no song loaded\n", (int)slot.first, slot.second);
    return;
  }

  if (background_slot == slot)
    return; // Verily, it is already playing
  stop_sound(background_slot);
  background_slot = slot;
  // printf("%d backgrounded\n", slot);

  alSourcei(slot_source[slot], AL_LOOPING, AL_TRUE);
  alSourcePlay(slot_source[slot]);
  alPrintErrors();
}

void musicsoundst::startbackgroundmusic(int slot) {
  startbackgroundmusic(make_pair(true,slot));
}

void musicsoundst::stopbackgroundmusic() {
  if (!functional) return;
  if (background_slot == make_pair(false,-1)) return;

  alSourceStop(slot_source[background_slot]);
}

void musicsoundst::stop_sound() {
  if (!functional) return;
  // Stop all playing sounds. Does this include background music?
  std::map<std::string,ALuint>::iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
    alSourceStop(it->second);
}

void musicsoundst::stop_sound(slot slot) {
  if (!functional) return;
  if (slot_source.count(slot) == 0) return;
  ALuint source = slot_source[slot];
  alSourceStop(source);
}

void musicsoundst::deinitsound() {
  if (!functional) return;

  std::map<std::string,ALuint>::iterator it;
  // Free all sources
  for (it = sources.begin(); it != sources.end(); ++it) {
    ALuint source = it->second;
    alDeleteSources(1, &source);
  }
  // Free all sample memory
  for (it = buffers.begin(); it != buffers.end(); ++it) {
    ALuint buffer = it->second;
    alDeleteBuffers(1, &buffer);
  }
  // Deinit OpenAL
  alcMakeContextCurrent(NULL);
  alcDestroyContext(context);
  alcCloseDevice(device);

  functional=false;
}

void musicsoundst::set_sound(string &filename, int slot, int pan, int priority) {
  if (!functional) return;
  set_song(filename, make_pair(false,slot));
}

// Deprecated stuff below

void musicsoundst::playsound(int s, int channel) {
  if (!functional) return;
  playsound(s);
}


//// OpenAL, ALC and sndfile stub ////

static void (*_alEnable)( ALenum capability );
static void (*_alDisable)( ALenum capability );
static ALboolean (*_alIsEnabled)( ALenum capability );
static const ALchar* (*_alGetString)( ALenum param );
static void (*_alGetBooleanv)( ALenum param, ALboolean* data );
static void (*_alGetIntegerv)( ALenum param, ALint* data );
static void (*_alGetFloatv)( ALenum param, ALfloat* data );
static void (*_alGetDoublev)( ALenum param, ALdouble* data );
static ALboolean (*_alGetBoolean)( ALenum param );
static ALint (*_alGetInteger)( ALenum param );
static ALfloat (*_alGetFloat)( ALenum param );
static ALdouble (*_alGetDouble)( ALenum param );
static ALenum (*_alGetError)( void );
static ALboolean (*_alIsExtensionPresent)( const ALchar* extname );
static void* (*_alGetProcAddress)( const ALchar* fname );
static ALenum (*_alGetEnumValue)( const ALchar* ename );
static void (*_alListenerf)( ALenum param, ALfloat value );
static void (*_alListener3f)( ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
static void (*_alListenerfv)( ALenum param, const ALfloat* values );
static void (*_alListeneri)( ALenum param, ALint value );
static void (*_alListener3i)( ALenum param, ALint value1, ALint value2, ALint value3 );
static void (*_alListeneriv)( ALenum param, const ALint* values );
static void (*_alGetListenerf)( ALenum param, ALfloat* value );
static void (*_alGetListener3f)( ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3 );
static void (*_alGetListenerfv)( ALenum param, ALfloat* values );
static void (*_alGetListeneri)( ALenum param, ALint* value );
static void (*_alGetListener3i)( ALenum param, ALint *value1, ALint *value2, ALint *value3 );
static void (*_alGetListeneriv)( ALenum param, ALint* values );
static void (*_alGenSources)( ALsizei n, ALuint* sources );
static void (*_alDeleteSources)( ALsizei n, const ALuint* sources );
static ALboolean (*_alIsSource)( ALuint sid );
static void (*_alSourcef)( ALuint sid, ALenum param, ALfloat value );
static void (*_alSource3f)( ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
static void (*_alSourcefv)( ALuint sid, ALenum param, const ALfloat* values );
static void (*_alSourcei)( ALuint sid, ALenum param, ALint value );
static void (*_alSource3i)( ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3 );
static void (*_alSourceiv)( ALuint sid, ALenum param, const ALint* values );
static void (*_alGetSourcef)( ALuint sid, ALenum param, ALfloat* value );
static void (*_alGetSource3f)( ALuint sid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
static void (*_alGetSourcefv)( ALuint sid, ALenum param, ALfloat* values );
static void (*_alGetSourcei)( ALuint sid, ALenum param, ALint* value );
static void (*_alGetSource3i)( ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3);
static void (*_alGetSourceiv)( ALuint sid, ALenum param, ALint* values );
static void (*_alSourcePlayv)( ALsizei ns, const ALuint *sids );
static void (*_alSourceStopv)( ALsizei ns, const ALuint *sids );
static void (*_alSourceRewindv)( ALsizei ns, const ALuint *sids );
static void (*_alSourcePausev)( ALsizei ns, const ALuint *sids );
static void (*_alSourcePlay)( ALuint sid );
static void (*_alSourceStop)( ALuint sid );
static void (*_alSourceRewind)( ALuint sid );
static void (*_alSourcePause)( ALuint sid );
static void (*_alSourceQueueBuffers)( ALuint sid, ALsizei numEntries, const ALuint *bids );
static void (*_alSourceUnqueueBuffers)( ALuint sid, ALsizei numEntries, ALuint *bids );
static void (*_alGenBuffers)( ALsizei n, ALuint* buffers );
static void (*_alDeleteBuffers)( ALsizei n, const ALuint* buffers );
static ALboolean (*_alIsBuffer)( ALuint bid );
static void (*_alBufferData)( ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq );
static void (*_alBufferf)( ALuint bid, ALenum param, ALfloat value );
static void (*_alBuffer3f)( ALuint bid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
static void (*_alBufferfv)( ALuint bid, ALenum param, const ALfloat* values );
static void (*_alBufferi)( ALuint bid, ALenum param, ALint value );
static void (*_alBuffer3i)( ALuint bid, ALenum param, ALint value1, ALint value2, ALint value3 );
static void (*_alBufferiv)( ALuint bid, ALenum param, const ALint* values );
static void (*_alGetBufferf)( ALuint bid, ALenum param, ALfloat* value );
static void (*_alGetBuffer3f)( ALuint bid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
static void (*_alGetBufferfv)( ALuint bid, ALenum param, ALfloat* values );
static void (*_alGetBufferi)( ALuint bid, ALenum param, ALint* value );
static void (*_alGetBuffer3i)( ALuint bid, ALenum param, ALint* value1, ALint* value2, ALint* value3);
static void (*_alGetBufferiv)( ALuint bid, ALenum param, ALint* values );
static void (*_alDopplerFactor)( ALfloat value );
static void (*_alDopplerVelocity)( ALfloat value );
static void (*_alSpeedOfSound)( ALfloat value );
static void (*_alDistanceModel)( ALenum distanceModel );
static ALCcontext * (*_alcCreateContext)( ALCdevice *device, const ALCint* attrlist );
static ALCboolean (*_alcMakeContextCurrent)( ALCcontext *context );
static void (*_alcProcessContext)( ALCcontext *context );
static void (*_alcSuspendContext)( ALCcontext *context );
static void (*_alcDestroyContext)( ALCcontext *context );
static ALCcontext * (*_alcGetCurrentContext)( void );
static ALCdevice* (*_alcGetContextsDevice)( ALCcontext *context );
static ALCdevice * (*_alcOpenDevice)( const ALCchar *devicename );
static ALCboolean (*_alcCloseDevice)( ALCdevice *device );
static ALCenum (*_alcGetError)( ALCdevice *device );
static ALCboolean (*_alcIsExtensionPresent)( ALCdevice *device, const ALCchar *extname );
static void * (*_alcGetProcAddress)( ALCdevice *device, const ALCchar *funcname );
static ALCenum (*_alcGetEnumValue)( ALCdevice *device, const ALCchar *enumname );
static const ALCchar * (*_alcGetString)( ALCdevice *device, ALCenum param );
static void (*_alcGetIntegerv)( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data );
static ALCdevice* (*_alcCaptureOpenDevice)( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );
static ALCboolean (*_alcCaptureCloseDevice)( ALCdevice *device );
static void (*_alcCaptureStart)( ALCdevice *device );
static void (*_alcCaptureStop)( ALCdevice *device );
static void (*_alcCaptureSamples)( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );

static bool linkit(void **target, const char *symbol, void *handle) {
  *target = dlsym(handle, symbol);
  if (!*target) {
    printf("Failed to link %s\n", symbol);
    return false;
  }
  return true;
}

static bool init_openal() {
  void *handle = dlopen("libopenal.so", RTLD_LAZY);
  if (!handle) handle = dlopen("libopenal.so.1", RTLD_LAZY);
  if (!handle) return false;
  
  if (!linkit((void**)&_alEnable, "alEnable", handle)) return false;
  if (!linkit((void**)&_alDisable, "alDisable", handle)) return false;
  if (!linkit((void**)&_alIsEnabled, "alIsEnabled", handle)) return false;
  if (!linkit((void**)&_alGetString, "alGetString", handle)) return false;
  if (!linkit((void**)&_alGetBooleanv, "alGetBooleanv", handle)) return false;
  if (!linkit((void**)&_alGetIntegerv, "alGetIntegerv", handle)) return false;
  if (!linkit((void**)&_alGetFloatv, "alGetFloatv", handle)) return false;
  if (!linkit((void**)&_alGetDoublev, "alGetDoublev", handle)) return false;
  if (!linkit((void**)&_alGetBoolean, "alGetBoolean", handle)) return false;
  if (!linkit((void**)&_alGetInteger, "alGetInteger", handle)) return false;
  if (!linkit((void**)&_alGetFloat, "alGetFloat", handle)) return false;
  if (!linkit((void**)&_alGetDouble, "alGetDouble", handle)) return false;
  if (!linkit((void**)&_alGetError, "alGetError", handle)) return false;
  if (!linkit((void**)&_alIsExtensionPresent, "alIsExtensionPresent", handle)) return false;
  if (!linkit((void**)&_alGetProcAddress, "alGetProcAddress", handle)) return false;
  if (!linkit((void**)&_alGetEnumValue, "alGetEnumValue", handle)) return false;
  if (!linkit((void**)&_alListenerf, "alListenerf", handle)) return false;
  if (!linkit((void**)&_alListener3f, "alListener3f", handle)) return false;
  if (!linkit((void**)&_alListenerfv, "alListenerfv", handle)) return false;
  if (!linkit((void**)&_alListeneri, "alListeneri", handle)) return false;
  if (!linkit((void**)&_alListener3i, "alListener3i", handle)) return false;
  if (!linkit((void**)&_alListeneriv, "alListeneriv", handle)) return false;
  if (!linkit((void**)&_alGetListenerf, "alGetListenerf", handle)) return false;
  if (!linkit((void**)&_alGetListener3f, "alGetListener3f", handle)) return false;
  if (!linkit((void**)&_alGetListenerfv, "alGetListenerfv", handle)) return false;
  if (!linkit((void**)&_alGetListeneri, "alGetListeneri", handle)) return false;
  if (!linkit((void**)&_alGetListener3i, "alGetListener3i", handle)) return false;
  if (!linkit((void**)&_alGetListeneriv, "alGetListeneriv", handle)) return false;
  if (!linkit((void**)&_alGenSources, "alGenSources", handle)) return false;
  if (!linkit((void**)&_alDeleteSources, "alDeleteSources", handle)) return false;
  if (!linkit((void**)&_alIsSource, "alIsSource", handle)) return false;
  if (!linkit((void**)&_alSourcef, "alSourcef", handle)) return false;
  if (!linkit((void**)&_alSource3f, "alSource3f", handle)) return false;
  if (!linkit((void**)&_alSourcefv, "alSourcefv", handle)) return false;
  if (!linkit((void**)&_alSourcei, "alSourcei", handle)) return false;
  if (!linkit((void**)&_alSource3i, "alSource3i", handle)) return false;
  if (!linkit((void**)&_alSourceiv, "alSourceiv", handle)) return false;
  if (!linkit((void**)&_alGetSourcef, "alGetSourcef", handle)) return false;
  if (!linkit((void**)&_alGetSource3f, "alGetSource3f", handle)) return false;
  if (!linkit((void**)&_alGetSourcefv, "alGetSourcefv", handle)) return false;
  if (!linkit((void**)&_alGetSourcei, "alGetSourcei", handle)) return false;
  if (!linkit((void**)&_alGetSource3i, "alGetSource3i", handle)) return false;
  if (!linkit((void**)&_alGetSourceiv, "alGetSourceiv", handle)) return false;
  if (!linkit((void**)&_alSourcePlayv, "alSourcePlayv", handle)) return false;
  if (!linkit((void**)&_alSourceStopv, "alSourceStopv", handle)) return false;
  if (!linkit((void**)&_alSourceRewindv, "alSourceRewindv", handle)) return false;
  if (!linkit((void**)&_alSourcePausev, "alSourcePausev", handle)) return false;
  if (!linkit((void**)&_alSourcePlay, "alSourcePlay", handle)) return false;
  if (!linkit((void**)&_alSourceStop, "alSourceStop", handle)) return false;
  if (!linkit((void**)&_alSourceRewind, "alSourceRewind", handle)) return false;
  if (!linkit((void**)&_alSourcePause, "alSourcePause", handle)) return false;
  if (!linkit((void**)&_alSourceQueueBuffers, "alSourceQueueBuffers", handle)) return false;
  if (!linkit((void**)&_alSourceUnqueueBuffers, "alSourceUnqueueBuffers", handle)) return false;
  if (!linkit((void**)&_alGenBuffers, "alGenBuffers", handle)) return false;
  if (!linkit((void**)&_alDeleteBuffers, "alDeleteBuffers", handle)) return false;
  if (!linkit((void**)&_alIsBuffer, "alIsBuffer", handle)) return false;
  if (!linkit((void**)&_alBufferData, "alBufferData", handle)) return false;
  if (!linkit((void**)&_alBufferf, "alBufferf", handle)) return false;
  if (!linkit((void**)&_alBuffer3f, "alBuffer3f", handle)) return false;
  if (!linkit((void**)&_alBufferfv, "alBufferfv", handle)) return false;
  if (!linkit((void**)&_alBufferi, "alBufferi", handle)) return false;
  if (!linkit((void**)&_alBuffer3i, "alBuffer3i", handle)) return false;
  if (!linkit((void**)&_alBufferiv, "alBufferiv", handle)) return false;
  if (!linkit((void**)&_alGetBufferf, "alGetBufferf", handle)) return false;
  if (!linkit((void**)&_alGetBuffer3f, "alGetBuffer3f", handle)) return false;
  if (!linkit((void**)&_alGetBufferfv, "alGetBufferfv", handle)) return false;
  if (!linkit((void**)&_alGetBufferi, "alGetBufferi", handle)) return false;
  if (!linkit((void**)&_alGetBuffer3i, "alGetBuffer3i", handle)) return false;
  if (!linkit((void**)&_alGetBufferiv, "alGetBufferiv", handle)) return false;
  if (!linkit((void**)&_alDopplerFactor, "alDopplerFactor", handle)) return false;
  if (!linkit((void**)&_alDopplerVelocity, "alDopplerVelocity", handle)) return false;
  if (!linkit((void**)&_alSpeedOfSound, "alSpeedOfSound", handle)) return false;
  if (!linkit((void**)&_alDistanceModel, "alDistanceModel", handle)) return false;
  if (!linkit((void**)&_alcCreateContext, "alcCreateContext", handle)) return false;
  if (!linkit((void**)&_alcMakeContextCurrent, "alcMakeContextCurrent", handle)) return false;
  if (!linkit((void**)&_alcProcessContext, "alcProcessContext", handle)) return false;
  if (!linkit((void**)&_alcSuspendContext, "alcSuspendContext", handle)) return false;
  if (!linkit((void**)&_alcDestroyContext, "alcDestroyContext", handle)) return false;
  if (!linkit((void**)&_alcGetCurrentContext, "alcGetCurrentContext", handle)) return false;
  if (!linkit((void**)&_alcGetContextsDevice, "alcGetContextsDevice", handle)) return false;
  if (!linkit((void**)&_alcOpenDevice, "alcOpenDevice", handle)) return false;
  if (!linkit((void**)&_alcCloseDevice, "alcCloseDevice", handle)) return false;
  if (!linkit((void**)&_alcGetError, "alcGetError", handle)) return false;
  if (!linkit((void**)&_alcIsExtensionPresent, "alcIsExtensionPresent", handle)) return false;
  if (!linkit((void**)&_alcGetProcAddress, "alcGetProcAddress", handle)) return false;
  if (!linkit((void**)&_alcGetEnumValue, "alcGetEnumValue", handle)) return false;
  if (!linkit((void**)&_alcGetString, "alcGetString", handle)) return false;
  if (!linkit((void**)&_alcGetIntegerv, "alcGetIntegerv", handle)) return false;
  if (!linkit((void**)&_alcCaptureOpenDevice, "alcCaptureOpenDevice", handle)) return false;
  if (!linkit((void**)&_alcCaptureCloseDevice, "alcCaptureCloseDevice", handle)) return false;
  if (!linkit((void**)&_alcCaptureStart, "alcCaptureStart", handle)) return false;
  if (!linkit((void**)&_alcCaptureStop, "alcCaptureStop", handle)) return false;
  if (!linkit((void**)&_alcCaptureSamples, "alcCaptureSamples", handle)) return false;

  return true;
}


void alEnable( ALenum capability ) { _alEnable(capability); }
void alDisable( ALenum capability ) { _alDisable(capability); }
ALboolean alIsEnabled( ALenum capability ) { _alIsEnabled(capability); }
const ALchar* alGetString( ALenum param ) { return _alGetString(param); }
void alGetBooleanv( ALenum param, ALboolean* data ) { _alGetBooleanv(param, data); }
void alGetIntegerv( ALenum param, ALint* data ) { _alGetIntegerv(param, data); }
void alGetFloatv( ALenum param, ALfloat* data ) { _alGetFloatv(param, data); }
void alGetDoublev( ALenum param, ALdouble* data ) { _alGetDoublev(param, data); }
ALboolean alGetBoolean( ALenum param ) { return _alGetBoolean(param); }
ALint alGetInteger( ALenum param ) { return _alGetInteger(param); }
ALfloat alGetFloat( ALenum param ) { return _alGetFloat(param); }
ALdouble alGetDouble( ALenum param ) { return _alGetDouble(param); }
ALenum alGetError( void ) { _alGetError(); }
ALboolean alIsExtensionPresent( const ALchar* extname ) { return _alIsExtensionPresent(extname); }
void* alGetProcAddress( const ALchar* fname ) { return _alGetProcAddress(fname); }
ALenum alGetEnumValue( const ALchar* ename ) { return _alGetEnumValue(ename); }
void alListenerf( ALenum param, ALfloat value ) { return _alListenerf(param, value); }
void alListener3f( ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 ) { return _alListener3f(param, value1, value2, value3); }
void alListenerfv( ALenum param, const ALfloat* values ) { return _alListenerfv(param, values); }
void alListeneri( ALenum param, ALint value ) { return _alListeneri(param, value); }
void alListener3i( ALenum param, ALint value1, ALint value2, ALint value3 ) { return _alListener3i(param, value1, value2, value3); }
void alListeneriv( ALenum param, const ALint* values ) { return _alListeneriv(param, values); }
void alGetListenerf( ALenum param, ALfloat* value ) { return _alGetListenerf(param, value); }
void alGetListener3f( ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3 ) { return _alGetListener3f(param, value1, value2, value3); }
void alGetListenerfv( ALenum param, ALfloat* values ) { return _alGetListenerfv(param, values); }
void alGetListeneri( ALenum param, ALint* value ) { return _alGetListeneri(param, value); }
void alGetListener3i( ALenum param, ALint *value1, ALint *value2, ALint *value3 ) { return _alGetListener3i(param, value1, value2, value3); }
void alGetListeneriv( ALenum param, ALint* values ) { return _alGetListeneriv(param, values); }
void alGenSources( ALsizei n, ALuint* sources ) { return _alGenSources(n, sources); }
void alDeleteSources( ALsizei n, const ALuint* sources ) { return _alDeleteSources(n, sources); }
ALboolean alIsSource( ALuint sid ) { return _alIsSource(sid); }
void alSourcef( ALuint sid, ALenum param, ALfloat value ) { return _alSourcef(sid, param, value); }
void alSource3f( ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 ) { return _alSource3f(sid, param, value1, value2, value3); }
void alSourcefv( ALuint sid, ALenum param, const ALfloat* values ) { return _alSourcefv(sid, param, values); }
void alSourcei( ALuint sid, ALenum param, ALint value ) { return _alSourcei(sid, param, value); }
void alSource3i( ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3 ) { return _alSource3i(sid, param, value1, value2, value3); }
void alSourceiv( ALuint sid, ALenum param, const ALint* values ) { return _alSourceiv(sid, param, values); }
void alGetSourcef( ALuint sid, ALenum param, ALfloat* value ) { return _alGetSourcef(sid, param, value); }
void alGetSource3f( ALuint sid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3) { return _alGetSource3f(sid, param, value1, value2, value3); }
void alGetSourcefv( ALuint sid, ALenum param, ALfloat* values ) { return _alGetSourcefv(sid, param, values); }
void alGetSourcei( ALuint sid, ALenum param, ALint* value ) { return _alGetSourcei(sid, param, value); }
void alGetSource3i( ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3) { return _alGetSource3i(sid, param, value1, value2, value3); }
void alGetSourceiv( ALuint sid, ALenum param, ALint* values ) { return _alGetSourceiv(sid, param, values); }
void alSourcePlayv( ALsizei ns, const ALuint *sids ) { return _alSourcePlayv(ns, sids); }
void alSourceStopv( ALsizei ns, const ALuint *sids ) { return _alSourceStopv(ns, sids); }
void alSourceRewindv( ALsizei ns, const ALuint *sids ) { return _alSourceRewindv(ns, sids); }
void alSourcePausev( ALsizei ns, const ALuint *sids ) { return _alSourcePausev(ns, sids); }
void alSourcePlay( ALuint sid ) { return _alSourcePlay(sid); }
void alSourceStop( ALuint sid ) { return _alSourceStop(sid); }
void alSourceRewind( ALuint sid ) { return _alSourceRewind(sid); }
void alSourcePause( ALuint sid ) { return _alSourcePause(sid); }
void alSourceQueueBuffers( ALuint sid, ALsizei numEntries, const ALuint *bids ) { return _alSourceQueueBuffers(sid, numEntries, bids); }
void alSourceUnqueueBuffers( ALuint sid, ALsizei numEntries, ALuint *bids ) { return _alSourceUnqueueBuffers(sid, numEntries, bids); }
void alGenBuffers( ALsizei n, ALuint* buffers ) { return _alGenBuffers(n, buffers); }
void alDeleteBuffers( ALsizei n, const ALuint* buffers ) { return _alDeleteBuffers(n, buffers); }
ALboolean alIsBuffer( ALuint bid ) { return _alIsBuffer(bid); }
void alBufferData( ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq ) { return _alBufferData(bid, format, data, size, freq); }
void alBufferf( ALuint bid, ALenum param, ALfloat value ) { return _alBufferf(bid, param, value); }
void alBuffer3f( ALuint bid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 ) { return _alBuffer3f(bid, param, value1, value2, value3); }
void alBufferfv( ALuint bid, ALenum param, const ALfloat* values ) { return _alBufferfv(bid, param, values); }
void alBufferi( ALuint bid, ALenum param, ALint value ) { return _alBufferi(bid, param, value); }
void alBuffer3i( ALuint bid, ALenum param, ALint value1, ALint value2, ALint value3 ) { return _alBuffer3i(bid, param, value1, value2, value3); }
void alBufferiv( ALuint bid, ALenum param, const ALint* values ) { return _alBufferiv(bid, param, values); }
void alGetBufferf( ALuint bid, ALenum param, ALfloat* value ) { return _alGetBufferf(bid, param, value); }
void alGetBuffer3f( ALuint bid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3) { return _alGetBuffer3f(bid, param, value1, value2, value3); }
void alGetBufferfv( ALuint bid, ALenum param, ALfloat* values ) { return _alGetBufferfv(bid, param, values); }
void alGetBufferi( ALuint bid, ALenum param, ALint* value ) { return _alGetBufferi(bid, param, value); }
void alGetBuffer3i( ALuint bid, ALenum param, ALint* value1, ALint* value2, ALint* value3) { return _alGetBuffer3i(bid, param, value1, value2, value3); }
void alGetBufferiv( ALuint bid, ALenum param, ALint* values ) { return _alGetBufferiv(bid, param, values); }
void alDopplerFactor( ALfloat value ) { return _alDopplerFactor(value); }
void alDopplerVelocity( ALfloat value ) { return _alDopplerVelocity(value); }
void alSpeedOfSound( ALfloat value ) { return _alSpeedOfSound(value); }
void alDistanceModel( ALenum distanceModel ) { return _alDistanceModel(distanceModel); }
ALCcontext * alcCreateContext( ALCdevice *device, const ALCint* attrlist ) { return _alcCreateContext(device, attrlist); }
ALCboolean alcMakeContextCurrent( ALCcontext *context ) { return _alcMakeContextCurrent(context); }
void alcProcessContext( ALCcontext *context ) { return _alcProcessContext(context); }
void alcSuspendContext( ALCcontext *context ) { return _alcSuspendContext(context); }
void alcDestroyContext( ALCcontext *context ) { return _alcDestroyContext(context); }
ALCcontext * alcGetCurrentContext( void ) { return _alcGetCurrentContext(); }
ALCdevice* alcGetContextsDevice( ALCcontext *context ) { return _alcGetContextsDevice(context); }
ALCdevice * alcOpenDevice( const ALCchar *devicename ) { return _alcOpenDevice(devicename); }
ALCboolean alcCloseDevice( ALCdevice *device ) { return _alcCloseDevice(device); }
ALCenum alcGetError( ALCdevice *device ) { return _alcGetError(device); }
ALCboolean alcIsExtensionPresent( ALCdevice *device, const ALCchar *extname ) { return _alcIsExtensionPresent(device, extname); }
void * alcGetProcAddress( ALCdevice *device, const ALCchar *funcname ) { return _alcGetProcAddress(device, funcname); }
ALCenum alcGetEnumValue( ALCdevice *device, const ALCchar *enumname ) { return _alcGetEnumValue(device, enumname); }
const ALCchar * alcGetString( ALCdevice *device, ALCenum param ) { return _alcGetString(device, param); }
void alcGetIntegerv( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data ) { return _alcGetIntegerv(device, param, size, data); }
ALCdevice* alcCaptureOpenDevice( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize ) { return _alcCaptureOpenDevice(devicename, frequency, format, buffersize); }
ALCboolean alcCaptureCloseDevice( ALCdevice *device ) { return _alcCaptureCloseDevice(device); }
void alcCaptureStart( ALCdevice *device ) { return _alcCaptureStart(device); }
void alcCaptureStop( ALCdevice *device ) { return _alcCaptureStop(device); }
void alcCaptureSamples( ALCdevice *device, ALCvoid *buffer, ALCsizei samples ) { return _alcCaptureSamples(device, buffer, samples); }


static SNDFILE* (*_sf_open) (const char *path, int mode, SF_INFO *sfinfo);
static SNDFILE* (*_sf_open_fd) (int fd, int mode, SF_INFO *sfinfo, int close_desc);
static SNDFILE* (*_sf_open_virtual) (SF_VIRTUAL_IO *sfvirtual, int mode, SF_INFO *sfinfo, void *user_data);
static int (*_sf_error) (SNDFILE *sndfile);
static const char* (*_sf_strerror) (SNDFILE *sndfile);
static const char* (*_sf_error_number) (int errnum);
static int (*_sf_perror) (SNDFILE *sndfile);
static int (*_sf_error_str) (SNDFILE *sndfile, char* str, size_t len);
static int (*_sf_command) (SNDFILE *sndfile, int command, void *data, int datasize);
static int (*_sf_format_check) (const SF_INFO *info);
static sf_count_t (*_sf_seek) (SNDFILE *sndfile, sf_count_t frames, int whence);
static int (*_sf_set_string) (SNDFILE *sndfile, int str_type, const char* str);
static const char* (*_sf_get_string) (SNDFILE *sndfile, int str_type);
static const char * (*_sf_version_string) (void);
static sf_count_t (*_sf_read_raw) (SNDFILE *sndfile, void *ptr, sf_count_t bytes);
static sf_count_t (*_sf_write_raw) (SNDFILE *sndfile, const void *ptr, sf_count_t bytes);
static sf_count_t (*_sf_readf_short) (SNDFILE *sndfile, short *ptr, sf_count_t frames);
static sf_count_t (*_sf_writef_short) (SNDFILE *sndfile, const short *ptr, sf_count_t frames);
static sf_count_t (*_sf_readf_int) (SNDFILE *sndfile, int *ptr, sf_count_t frames);
static sf_count_t (*_sf_writef_int) (SNDFILE *sndfile, const int *ptr, sf_count_t frames);
static sf_count_t (*_sf_readf_float) (SNDFILE *sndfile, float *ptr, sf_count_t frames);
static sf_count_t (*_sf_writef_float) (SNDFILE *sndfile, const float *ptr, sf_count_t frames);
static sf_count_t (*_sf_readf_double) (SNDFILE *sndfile, double *ptr, sf_count_t frames);
static sf_count_t (*_sf_writef_double) (SNDFILE *sndfile, const double *ptr, sf_count_t frames);
static sf_count_t (*_sf_read_short) (SNDFILE *sndfile, short *ptr, sf_count_t items);
static sf_count_t (*_sf_write_short) (SNDFILE *sndfile, const short *ptr, sf_count_t items);
static sf_count_t (*_sf_read_int) (SNDFILE *sndfile, int *ptr, sf_count_t items);
static sf_count_t (*_sf_write_int) (SNDFILE *sndfile, const int *ptr, sf_count_t items);
static sf_count_t (*_sf_read_float) (SNDFILE *sndfile, float *ptr, sf_count_t items);
static sf_count_t (*_sf_write_float) (SNDFILE *sndfile, const float *ptr, sf_count_t items);
static sf_count_t (*_sf_read_double) (SNDFILE *sndfile, double *ptr, sf_count_t items);
static sf_count_t (*_sf_write_double) (SNDFILE *sndfile, const double *ptr, sf_count_t items);
static int (*_sf_close) (SNDFILE *sndfile);
static void (*_sf_write_sync) (SNDFILE *sndfile);

static bool init_sndfile() {
  void *handle = dlopen("libsndfile.so", RTLD_LAZY);
  if (!handle) handle = dlopen("libsndfile.so.1", RTLD_LAZY);
  if (!handle) return false;
  
  if (!linkit((void**)&_sf_open, "sf_open", handle)) return false;
  if (!linkit((void**)&_sf_open_fd, "sf_open_fd", handle)) return false;
  if (!linkit((void**)&_sf_open_virtual, "sf_open_virtual", handle)) return false;
  if (!linkit((void**)&_sf_error, "sf_error", handle)) return false;
  if (!linkit((void**)&_sf_strerror, "sf_strerror", handle)) return false;
  if (!linkit((void**)&_sf_error_number, "sf_error_number", handle)) return false;
  if (!linkit((void**)&_sf_perror, "sf_perror", handle)) return false;
  if (!linkit((void**)&_sf_error_str, "sf_error_str", handle)) return false;
  if (!linkit((void**)&_sf_command, "sf_command", handle)) return false;
  if (!linkit((void**)&_sf_format_check, "sf_format_check", handle)) return false;
  if (!linkit((void**)&_sf_seek, "sf_seek", handle)) return false;
  if (!linkit((void**)&_sf_set_string, "sf_set_string", handle)) return false;
  if (!linkit((void**)&_sf_get_string, "sf_get_string", handle)) return false;
  if (!linkit((void**)&_sf_version_string, "sf_version_string", handle)) return false;
  if (!linkit((void**)&_sf_read_raw, "sf_read_raw", handle)) return false;
  if (!linkit((void**)&_sf_write_raw, "sf_write_raw", handle)) return false;
  if (!linkit((void**)&_sf_readf_short, "sf_readf_short", handle)) return false;
  if (!linkit((void**)&_sf_writef_short, "sf_writef_short", handle)) return false;
  if (!linkit((void**)&_sf_readf_int, "sf_readf_int", handle)) return false;
  if (!linkit((void**)&_sf_writef_int, "sf_writef_int", handle)) return false;
  if (!linkit((void**)&_sf_readf_float, "sf_readf_float", handle)) return false;
  if (!linkit((void**)&_sf_writef_float, "sf_writef_float", handle)) return false;
  if (!linkit((void**)&_sf_readf_double, "sf_readf_double", handle)) return false;
  if (!linkit((void**)&_sf_writef_double, "sf_writef_double", handle)) return false;
  if (!linkit((void**)&_sf_read_short, "sf_read_short", handle)) return false;
  if (!linkit((void**)&_sf_write_short, "sf_write_short", handle)) return false;
  if (!linkit((void**)&_sf_read_int, "sf_read_int", handle)) return false;
  if (!linkit((void**)&_sf_write_int, "sf_write_int", handle)) return false;
  if (!linkit((void**)&_sf_read_float, "sf_read_float", handle)) return false;
  if (!linkit((void**)&_sf_write_float, "sf_write_float", handle)) return false;
  if (!linkit((void**)&_sf_read_double, "sf_read_double", handle)) return false;
  if (!linkit((void**)&_sf_write_double, "sf_write_double", handle)) return false;
  if (!linkit((void**)&_sf_close, "sf_close", handle)) return false;
  if (!linkit((void**)&_sf_write_sync, "sf_write_sync", handle)) return false;

  return true;
}


SNDFILE* sf_open (const char *path, int mode, SF_INFO *sfinfo) { return _sf_open(path, mode, sfinfo); }
SNDFILE* sf_open_fd (int fd, int mode, SF_INFO *sfinfo, int close_desc) { return _sf_open_fd(fd, mode, sfinfo, close_desc); }
SNDFILE* sf_open_virtual (SF_VIRTUAL_IO *sfvirtual, int mode, SF_INFO *sfinfo, void *user_data) { return _sf_open_virtual(sfvirtual, mode, sfinfo, user_data); }
int sf_error (SNDFILE *sndfile) { return _sf_error(sndfile); }
const char* sf_strerror (SNDFILE *sndfile) { return _sf_strerror(sndfile); }
const char* sf_error_number (int errnum) { return _sf_error_number(errnum); }
int sf_perror (SNDFILE *sndfile) { return _sf_perror(sndfile); }
int sf_error_str (SNDFILE *sndfile, char* str, size_t len) { return _sf_error_str(sndfile, str, len); }
int sf_command (SNDFILE *sndfile, int command, void *data, int datasize) { return _sf_command(sndfile, command, data, datasize); }
int sf_format_check (const SF_INFO *info) { return _sf_format_check(info); }
sf_count_t sf_seek (SNDFILE *sndfile, sf_count_t frames, int whence) { return _sf_seek(sndfile, frames, whence); }
int sf_set_string (SNDFILE *sndfile, int str_type, const char* str) { return _sf_set_string(sndfile, str_type, str); }
const char* sf_get_string (SNDFILE *sndfile, int str_type) { return _sf_get_string(sndfile, str_type); }
const char * sf_version_string (void) { return _sf_version_string(); }
sf_count_t sf_read_raw (SNDFILE *sndfile, void *ptr, sf_count_t bytes) { return _sf_read_raw(sndfile, ptr, bytes); }
sf_count_t sf_write_raw (SNDFILE *sndfile, const void *ptr, sf_count_t bytes) { return _sf_write_raw(sndfile, ptr, bytes); }
sf_count_t sf_readf_short (SNDFILE *sndfile, short *ptr, sf_count_t frames) { return _sf_readf_short(sndfile, ptr, frames); }
sf_count_t sf_writef_short (SNDFILE *sndfile, const short *ptr, sf_count_t frames) { return _sf_writef_short(sndfile, ptr, frames); }
sf_count_t sf_readf_int (SNDFILE *sndfile, int *ptr, sf_count_t frames) { return _sf_readf_int(sndfile, ptr, frames); }
sf_count_t sf_writef_int (SNDFILE *sndfile, const int *ptr, sf_count_t frames) { return _sf_writef_int(sndfile, ptr, frames); }
sf_count_t sf_readf_float (SNDFILE *sndfile, float *ptr, sf_count_t frames) { return _sf_readf_float(sndfile, ptr, frames); }
sf_count_t sf_writef_float (SNDFILE *sndfile, const float *ptr, sf_count_t frames) { return _sf_writef_float(sndfile, ptr, frames); }
sf_count_t sf_readf_double (SNDFILE *sndfile, double *ptr, sf_count_t frames) { return _sf_readf_double(sndfile, ptr, frames); }
sf_count_t sf_writef_double (SNDFILE *sndfile, const double *ptr, sf_count_t frames) { return _sf_writef_double(sndfile, ptr, frames); }
sf_count_t sf_read_short (SNDFILE *sndfile, short *ptr, sf_count_t items) { return _sf_read_short(sndfile, ptr, items); }
sf_count_t sf_write_short (SNDFILE *sndfile, const short *ptr, sf_count_t items) { return _sf_write_short(sndfile, ptr, items); }
sf_count_t sf_read_int (SNDFILE *sndfile, int *ptr, sf_count_t items) { return _sf_read_int(sndfile, ptr, items); }
sf_count_t sf_write_int (SNDFILE *sndfile, const int *ptr, sf_count_t items) { return _sf_write_int(sndfile, ptr, items); }
sf_count_t sf_read_float (SNDFILE *sndfile, float *ptr, sf_count_t items) { return _sf_read_float(sndfile, ptr, items); }
sf_count_t sf_write_float (SNDFILE *sndfile, const float *ptr, sf_count_t items) { return _sf_write_float(sndfile, ptr, items); }
sf_count_t sf_read_double (SNDFILE *sndfile, double *ptr, sf_count_t items) { return _sf_read_double(sndfile, ptr, items); }
sf_count_t sf_write_double (SNDFILE *sndfile, const double *ptr, sf_count_t items) { return _sf_write_double(sndfile, ptr, items); }
int sf_close (SNDFILE *sndfile) { return _sf_close(sndfile); }
void sf_write_sync (SNDFILE *sndfile) { return _sf_write_sync(sndfile); }
