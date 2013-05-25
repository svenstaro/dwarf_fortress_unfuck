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
