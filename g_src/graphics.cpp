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
#include <zlib.h>
#include <cassert>

#include "svector.h"
#include "ttf_manager.hpp"

/*
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
*/

#include "random.h"

#include "endian.h"

#include "files.h"

#include "enabler.h"

#include "textlines.h"

#include "find_files.h"

#include "basics.h"

#include "g_basics.h"

#include "texture_handler.h"

#include "graphics.h"

#include "music_and_sound_g.h"

#include "init.h"

#include "interface.h"

#ifdef __APPLE__
#define _XOPEN_SOURCE_EXTENDED
#endif

#include "curses.h"

using namespace std;


#pragma comment( lib, "opengl32.lib" )			// Search For OpenGL32.lib While Linking
#pragma comment( lib, "glu32.lib" )				// Search For GLu32.lib While Linking

extern enablerst enabler;
extern texture_handlerst texture;
graphicst gps;
extern interfacest gview;

extern string errorlog_prefix;

void process_object_lines(textlinesst &lines,string &chktype,string &graphics_dir);

// Add, then increment to the (possible) PBO alignment requirement
static void align(size_t &sz, off_t inc) {
  sz += inc;
  while (sz%64) sz++; // So.. tired.. FIXME.
}

void graphicst::resize(int x, int y)  {
  dimx = x; dimy = y;
  init.display.grid_x = x;
  init.display.grid_y = y;
  setclipping(0, x-1, 0, y-1);
  force_full_display_count++;
  screen_limit = screen + dimx * dimy * 4;
}

void graphicst::addcoloredst(const char *str,const char *colorstr)
{
  const int slen = strlen(str);
  int s;
  for(s=0; s < slen && screenx < init.display.grid_x; s++)
    {
      if(screenx<0)
        {
          s-=screenx;
          screenx=0;
          if (s >= slen) break;
        }
      
      changecolor((colorstr[s] & 7),((colorstr[s] & 56))>>3,((colorstr[s] & 64))>>6);
      addchar(str[s]);
    }
}

static list<ttf_id> ttfstr;

static void abbreviate_string_helper_hackaroundmissingcode(string &str, int len) {
       if(str.length()>=2)
		{
		if((str[0]=='A'||str[0]=='a')&&
			str[1]==' ')
			{
			str.erase(str.begin()+1);
			str.erase(str.begin());

			if(str.length()<=len)return;
			}

		if(str.length()>=3)
			{
			if((str[0]=='A'||str[0]=='a')&&
				(str[1]=='N'||str[1]=='n')&&
				str[2]==' ')
				{
				str.erase(str.begin()+2);
				str.erase(str.begin()+1);
				str.erase(str.begin());

				if(str.length()<=len)return;
				}

			if(str.length()>=4)
				{
				if((str[0]=='T'||str[0]=='t')&&
					(str[1]=='H'||str[1]=='h')&&
					(str[2]=='E'||str[2]=='e')&&
					str[3]==' ')
					{
					str.erase(str.begin()+3);
					str.erase(str.begin()+2);
					str.erase(str.begin()+1);
					str.erase(str.begin());

					if(str.length()<=len)return;
					}
				}
			}
		}

	int32_t l;
	for(l=(int32_t)str.length()-1;l>=1;l--)
		{
		if(str[l-1]==' ')continue;

		if(str[l]=='a'||
			str[l]=='e'||
			str[l]=='i'||
			str[l]=='o'||
			str[l]=='u'||
			str[l]=='A'||
			str[l]=='E'||
			str[l]=='I'||
			str[l]=='O'||
			str[l]=='U')
			{
			str.erase(str.begin()+l);
			if(str.length()<=len)return;
			}
		}

	if(str.length()>len)str.resize(len);
}


static void abbreviate_string_hackaroundmissingcode(string &str, int32_t len)
{
  if (ttf_manager.ttf_active()) {
    // We'll need to use TTF-aware text shrinking.
    while (ttf_manager.size_text(str) > len)
      abbreviate_string_helper_hackaroundmissingcode(str, str.length() - 1);
  } else if(str.length()>len){
    // 1 letter = 1 tile.
    abbreviate_string_helper_hackaroundmissingcode(str, len);
  }
}


void graphicst::addst(const string &str_orig, justification just, int space)
{
  if (!str_orig.size())
    return;
  string str = str_orig;
  if (space)
    abbreviate_string_hackaroundmissingcode(str, space);
  if (just == not_truetype || !ttf_manager.ttf_active()) {
    int s;
    for(s=0;s<str.length()&&screenx<init.display.grid_x;s++)
      {
        if(screenx<0)
          {
            s-=screenx;
            screenx=0;
            if(s>=str.length())break;
          }
        
        addchar(str[s]);
      }
  } else {
    // Truetype
    if (str.size() > 2 && str[0] == ':' && str[1] == ' ')
      str[1] = '\t'; // EVIL HACK
    struct ttf_id id = {str, screenf, screenb, screenbright};
    ttfstr.push_back(id);
    // if (str.size() == 80) {
    //   cout << "(" << int(str[0]) << ") ";
    // }
    // cout << screeny << "," << str.size() << ":" << str;
    // if (just == justify_cont)
    //   cout << "|";
    // else
    //   cout << endl;
    if (just == justify_cont)
      return; // More later
    // This string is done. Time to render.
    ttf_details details = ttf_manager.get_handle(ttfstr, just);
    const int handle = details.handle;
    const int offset = details.offset;
    int width = details.width;
    const int ourx = screenx + offset;
    unsigned int * const s = ((unsigned int*)screen + ourx*dimy + screeny);
    if (s < (unsigned int*)screen_limit)
      s[0] = (((unsigned int)GRAPHICSTYPE_TTF) << 24) | handle;
    // Also set the other tiles this text covers, but don't write past the end.
    if (width + ourx >= dimx)
      width = dimx - ourx - 1;
    for (int x = 1; x < width; ++x)
      s[x * dimy] = (((unsigned int)GRAPHICSTYPE_TTFCONT) << 24) | handle;
    // Clean up, prepare for next string.
    screenx = ourx + width;
    ttfstr.clear();
  }
}

void graphicst::erasescreen_clip()
{
	changecolor(0,0,0);
	short x2,y2;
	for(x2=clipx[0];x2<=clipx[1];x2++)
		{
		for(y2=clipy[0];y2<=clipy[1];y2++)
			{
			locate(y2,x2);
			addchar(' ');
			}
		}
}

void graphicst::erasescreen_rect(int x1, int x2, int y1, int y2)
{ 
  changecolor(0,0,0);
  for (int x = x1; x <= x2; x++) {
    for (int y = y1; y <= y2; y++) {
      locate(y, x);
      addchar(' ');
    }
  }
}

void graphicst::erasescreen()
{
	memset(screen, 0, dimx*dimy*4);

	memset(screentexpos, 0, dimx*dimy*sizeof(long));
}

void graphicst::setclipping(long x1,long x2,long y1,long y2)
{
	if(x1<0)x1=0;
	if(x2>init.display.grid_x-1)x2=init.display.grid_x-1;
	if(y1<0)y1=0;
	if(y2>init.display.grid_y-1)y2=init.display.grid_y-1;

	clipx[0]=x1;
	clipx[1]=x2;
	clipy[0]=y1;
	clipy[1]=y2;
}

void graphicst::dim_colors(long x,long y,char dim)
{
	if(x>=clipx[0]&&x<=clipx[1]&&
		y>=clipy[0]&&y<=clipy[1])
		{
		switch(dim)
			{
			case 4:
				switch(screen[x*dimy*4 + y*4 + 2])
					{
					case 4:
					case 5:
					case 6:
						screen[x*dimy*4 + y*4 + 2]=1;
						break;
					case 2:
					case 7:
						screen[x*dimy*4 + y*4 + 2]=3;
						break;
					}
				switch(screen[x*dimy*4 + y*4 + 1])
					{
					case 4:
					case 5:
					case 6:
						screen[x*dimy*4 + y*4 + 1]=1;
						break;
					case 2:
					case 7:
						screen[x*dimy*4 + y*4 + 1]=3;
						break;
					}
				if(screen[x*dimy*4 + y*4 + 1]==screen[x*dimy*4 + y*4 + 2])screen[x*dimy*4 + y*4 + 1]=0;
				screen[x*dimy*4 + y*4 + 3]=0;
				if(screen[x*dimy*4 + y*4 + 1]==0&&screen[x*dimy*4 + y*4 + 2]==0&&screen[x*dimy*4 + y*4 + 3]==0)screen[x*dimy*4 + y*4 + 3]=1;
				break;
			case 3:
				switch(screen[x*dimy*4 + y*4 + 2])
					{
					case 4:
					case 5:
						screen[x*dimy*4 + y*4 + 2]=6;
						break;
					case 2:
					case 7:
						screen[x*dimy*4 + y*4 + 2]=3;
						break;
					}
				switch(screen[x*dimy*4 + y*4 + 1])
					{
					case 1:
						screen[x*dimy*4 + y*4 + 3]=0;
						break;
					case 4:
					case 5:
						screen[x*dimy*4 + y*4 + 1]=6;
						break;
					case 2:
						screen[x*dimy*4 + y*4 + 1]=3;
						break;
					case 7:
						screen[x*dimy*4 + y*4 + 1]=3;
						break;
					}
				if(screen[x*dimy*4 + y*4 + 1]!=7)screen[x*dimy*4 + y*4 + 3]=0;
				if(screen[x*dimy*4 + y*4 + 1]==screen[x*dimy*4 + y*4 + 2]&&
					screen[x*dimy*4 + y*4 + 3]==0)screen[x*dimy*4 + y*4 + 1]=0;
				if(screen[x*dimy*4 + y*4 + 1]==0&&screen[x*dimy*4 + y*4 + 2]==0&&screen[x*dimy*4 + y*4 + 3]==0)screen[x*dimy*4 + y*4 + 3]=1;
				break;
			case 2:
				switch(screen[x*dimy*4 + y*4 + 2])
					{
					case 4:
					case 5:
						screen[x*dimy*4 + y*4 + 2]=6;
						break;
					}
				switch(screen[x*dimy*4 + y*4 + 1])
					{
					case 4:
					case 5:
						screen[x*dimy*4 + y*4 + 1]=6;
						break;
					}
				if(screen[x*dimy*4 + y*4 + 1]!=7)screen[x*dimy*4 + y*4 + 3]=0;
				if(screen[x*dimy*4 + y*4 + 1]==screen[x*dimy*4 + y*4 + 2]&&
					screen[x*dimy*4 + y*4 + 3]==0)screen[x*dimy*4 + y*4 + 1]=0;
				if(screen[x*dimy*4 + y*4 + 1]==0&&screen[x*dimy*4 + y*4 + 2]==0&&screen[x*dimy*4 + y*4 + 3]==0)screen[x*dimy*4 + y*4 + 3]=1;
				break;
			case 1:
				if(screen[x*dimy*4 + y*4 + 1]!=7)screen[x*dimy*4 + y*4 + 3]=0;
				if(screen[x*dimy*4 + y*4 + 1]==screen[x*dimy*4 + y*4 + 2]&&
					screen[x*dimy*4 + y*4 + 3]==0)screen[x*dimy*4 + y*4 + 1]=0;
				if(screen[x*dimy*4 + y*4 + 1]==0&&screen[x*dimy*4 + y*4 + 2]==0&&screen[x*dimy*4 + y*4 + 3]==0)screen[x*dimy*4 + y*4 + 3]=1;
				break;
			}
		}
}

void graphicst::rain_color_square(long x,long y)
{
	if(x>=clipx[0]&&x<=clipx[1]&&
		y>=clipy[0]&&y<=clipy[1])
		{
		screen[x*dimy*4 + y*4 + 1]=1;
		screen[x*dimy*4 + y*4 + 2]=0;
		screen[x*dimy*4 + y*4 + 3]=1;
		}
}

void graphicst::snow_color_square(long x,long y)
{
	if(x>=clipx[0]&&x<=clipx[1]&&
		y>=clipy[0]&&y<=clipy[1])
		{
		screen[x*dimy*4 + y*4 + 1]=7;
		screen[x*dimy*4 + y*4 + 2]=0;
		screen[x*dimy*4 + y*4 + 3]=1;
		}
}

void graphicst::color_square(long x,long y,unsigned char f,unsigned char b,unsigned char br)
{
	if(x>=clipx[0]&&x<=clipx[1]&&
		y>=clipy[0]&&y<=clipy[1])
		{
		screen[x*dimy*4 + y*4 + 1]=f;
		screen[x*dimy*4 + y*4 + 2]=b;
		screen[x*dimy*4 + y*4 + 3]=br;
		}
}

void graphicst::prepare_graphics(string &src_dir)
{
	if(!init.display.flag.has_flag(INIT_DISPLAY_FLAG_USE_GRAPHICS))return;

	texture.clean();

	//GET READY TO LOAD
	svector<char *> processfilename;
	long f;
	textlinesst setuplines;
	char str[400];

	//LOAD THE OBJECT FILES UP INTO MEMORY
		//MUST INSURE THAT THEY ARE LOADED IN THE PROPER ORDER, IN CASE THEY REFER TO EACH OTHER
	string chk=src_dir;
	chk+="graphics/graphics_*";
#ifdef WIN32
	chk+=".*";
#endif
	find_files_by_pattern_with_exception(chk.c_str(),processfilename,"text");

	string chktype="GRAPHICS";
	for(f=0;f<processfilename.size();f++)
		{
		strcpy(str,src_dir.c_str());
		strcat(str,"graphics/");
		strcat(str,processfilename[f]);
		setuplines.load_raw_to_lines(str);

		errorlog_prefix="*** Error(s) found in the file \"";
		errorlog_prefix+=str;
		errorlog_prefix+='\"';
		process_object_lines(setuplines,chktype,src_dir);
		errorlog_prefix.clear();


		delete[] processfilename[f];
		}
	processfilename.clear();

        enabler.reset_textures();
}

void graphicst::add_tile(long texp,char addcolor)
{
	if(screenx>=clipx[0]&&screenx<=clipx[1]&&
		screeny>=clipy[0]&&screeny<=clipy[1])
		{
		screentexpos[screenx*dimy + screeny]=texp;
		screentexpos_addcolor[screenx*dimy + screeny]=addcolor;
		screentexpos_grayscale[screenx*dimy + screeny]=0;
		}
}

void graphicst::add_tile_grayscale(long texp,char cf,char cbr)
{
	if(screenx>=clipx[0]&&screenx<=clipx[1]&&
		screeny>=clipy[0]&&screeny<=clipy[1])
		{
		screentexpos[screenx*dimy + screeny]=texp;
		screentexpos_addcolor[screenx*dimy + screeny]=0;
		screentexpos_grayscale[screenx*dimy + screeny]=1;
		screentexpos_cf[screenx*dimy + screeny]=cf;
		screentexpos_cbr[screenx*dimy + screeny]=cbr;
		}
}

void graphicst::draw_border(int x1, int x2, int y1, int y2) {
  // Upper and lower
  for (int x = x1; x <= x2; x++) {
    locate(y1, x);
    addchar(' ');
    locate(y2, x);
    addchar(' ');
  }
  // Left and right
  for (int y = y1+1; y < y2; y++) {
    locate(y, x1);
    addchar(' ');
    locate(y, x2);
    addchar(' ');
  }
}

void graphicst::get_mouse_text_coords(int32_t &mx, int32_t &my) {
  mx = mouse_x; my = mouse_y;
}

void render_things()
{
  //GRAB CURRENT SCREEN AT THE END OF THE LIST
  viewscreenst *currentscreen=&gview.view;
  while(currentscreen->child!=NULL)currentscreen=currentscreen->child;
  
  //NO INTERFACE LEFT, LEAVE
  if(currentscreen==&gview.view)return;
  
  if(currentscreen->breakdownlevel==INTERFACE_BREAKDOWN_NONE)
	{
	currentscreen->render();
	}
  else gps.erasescreen();

  // Render REC when recording macros. Definitely want this screen-specific. Or do we?
  const Time now = SDL_GetTicks();
  if (enabler.is_recording() && now % 1000 > 500) {
    gps.locate(0, 20);
    gps.changecolor(4,1,1);
    gps.addst("REC");
  }
  // Render PLAY when playing a macro
  if (enabler.is_macro_playing() && now % 1000 <= 500) {
    gps.locate(0,20);
    gps.changecolor(2,1,1);
    gps.addst("PLAY");
  }
  // Render # <i> when building a repetition prefix
  if (enabler.prefix_building()) {
    gps.locate(0,20);
    gps.changecolor(3,1,1);
    gps.addst("#" + enabler.prefix());
  }
  if (gps.display_frames) {
    ostringstream fps_stream;
    fps_stream << "FPS: " << setw(3) << enabler.calculate_fps() << " (" << enabler.calculate_gfps() << ")";
    string fps = fps_stream.str();
    gps.changecolor(7,3,1);
    static gps_locator fps_locator(0, 25);
    fps_locator(fps.size());
    gps.addst(fps);
  }
}
