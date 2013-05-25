#ifndef INIT_H
#define INIT_H

#include "enabler.h"
#ifdef __APPLE__
#include <SDL_ttf/SDL_ttf.h>
#else
#include <SDL/SDL_ttf.h>
#endif

enum ttf_flag {
  ttf_off, ttf_on, ttf_auto
};

class init_fontst
{
	public:
		long small_font_texpos[256];
		long large_font_texpos[256];
		long small_font_datapos[256];
		long large_font_datapos[256];
		float small_font_adjx;
		float small_font_adjy;
		float large_font_adjx;
		float large_font_adjy;
		long small_font_dispx;
		long small_font_dispy;
		long large_font_dispx;
		long large_font_dispy;
                ttf_flag use_ttf;
                int ttf_limit;
};

enum InitDisplayFlag
{
	INIT_DISPLAY_FLAG_USE_GRAPHICS,
	INIT_DISPLAY_FLAG_BLACK_SPACE,
	INIT_DISPLAY_FLAG_PARTIAL_PRINT,
	INIT_DISPLAY_FLAG_FRAME_BUFFER,
	INIT_DISPLAY_FLAG_SINGLE_BUFFER,
	INIT_DISPLAY_FLAG_ACCUM_BUFFER,
	INIT_DISPLAY_FLAG_VBO,
        INIT_DISPLAY_FLAG_2D,
        INIT_DISPLAY_FLAG_2DHW,
        INIT_DISPLAY_FLAG_2DASYNC,
	INIT_DISPLAY_FLAG_UNUSED_01_08,//
        INIT_DISPLAY_FLAG_TEXT,
        INIT_DISPLAY_FLAG_SHADER,
        INIT_DISPLAY_FLAG_NOT_RESIZABLE,
        INIT_DISPLAY_FLAG_ARB_SYNC,
	INIT_DISPLAY_FLAGNUM
};

enum InitDisplayWindow
{
	INIT_DISPLAY_WINDOW_TRUE,
	INIT_DISPLAY_WINDOW_FALSE,
	INIT_DISPLAY_WINDOW_PROMPT,
	INIT_DISPLAY_WINDOWNUM
};

class init_displayst
{
 public:
  flagarrayst flag;
  InitDisplayWindow windowed;

  int grid_x, grid_y; // The *current* display grid size, kept up to date

  int desired_fullscreen_width, desired_fullscreen_height;
  int desired_windowed_width, desired_windowed_height;

  
  char partial_print_count;
  
  init_displayst();
};

enum InitMediaFlag
{
	INIT_MEDIA_FLAG_SOUND_OFF,
	INIT_MEDIA_FLAG_INTRO_OFF,
	INIT_MEDIA_FLAG_COMPRESS_SAVES,
	INIT_MEDIA_FLAGNUM
};

class init_mediast
{
	public:
		flagarrayst flag;
		int32_t volume;

		init_mediast()
			{
			flag.set_size_on_flag_num(INIT_MEDIA_FLAGNUM);
			volume=255;
			}
};

enum InitInputFlag
{
	INIT_INPUT_FLAG_MOUSE_OFF,
	INIT_INPUT_FLAG_MOUSE_PICTURE,
	INIT_INPUT_FLAGNUM
};

class init_inputst
{
 public:
  long hold_time;
  long repeat_time;
  long macro_time;
  long pause_zoom_no_interface_ms;
  flagarrayst flag;
  long zoom_speed;
  int repeat_accel_start;
  int repeat_accel_limit;
  
  init_inputst()
    {
      hold_time=150;
      repeat_time=150;
      macro_time=75;
      pause_zoom_no_interface_ms=0;
      flag.set_size_on_flag_num(INIT_INPUT_FLAGNUM);
      zoom_speed = 10;
      repeat_accel_start = 10;
      repeat_accel_limit = 1;
    }
};

enum InitWindowFlag
{
	INIT_WINDOW_FLAG_TOPMOST,
	INIT_WINDOW_FLAG_VSYNC_ON,
	INIT_WINDOW_FLAG_VSYNC_OFF,
	INIT_WINDOW_FLAG_TEXTURE_LINEAR,
	INIT_WINDOW_FLAGNUM
};

class init_windowst
{
	public:
		flagarrayst flag;

		init_windowst()
			{
			flag.set_size_on_flag_num(INIT_WINDOW_FLAGNUM);
			}
};

class initst
{
	public:
		init_displayst display;
		init_mediast media;
		init_inputst input;
		init_fontst font;
		init_windowst window;

		void begin();
};

extern initst init;

#endif
