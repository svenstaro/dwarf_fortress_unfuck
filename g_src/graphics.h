#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <string>
#include <map>
#include <cassert>
using std::string;

#include "GL/glew.h"
#include "g_basics.h"
#include "platform.h"
#include "basics.h"

enum Texture
{
	TEXTURE_MOUSE,
	TEXTURENUM
};

/* screen array layout
 *
 *
 * X*Y tiles of 4 bytes each in column-major order (FIXME: This is inefficient! It should be row-major!)
 * For each tile, byte 0 is the character, 1 is foreground color, 2 is bacground, and 3 denotes bold.
 *
 * As there are only 8 different colors and bold is a boolean, this leaves a lot of free space. Therefore,
 * without involving the graphics arrays, out-of-gamut values can be used for designating TTF objects.
 *
 * This means setting the bold byte to all 1s (0xff), and then using the other three bytes as a designator.
 *
 * Time will tell whether this was a good idea or not.
 */

// So yeah, we store "type" in the previously bold byte. This means we have quite a lot of free types yet.
#define GRAPHICSTYPE_TTF 0xff
// This type denotes a tile that is covered by truetype, but is not the tile it starts on.
#define GRAPHICSTYPE_TTFCONT 0xfe


class graphicst
{
  int lookup_pair(std::pair<int,int> color);
  long calculate_old_fps();
	public:
		long screenx,screeny;
		char screenf,screenb;
		char screenbright;

		unsigned char *screen;
		long *screentexpos;
		char *screentexpos_addcolor;
		unsigned char *screentexpos_grayscale;
		unsigned char *screentexpos_cf;
		unsigned char *screentexpos_cbr;

                // Calling this is not enough in itself. You also need to call swap_front/back.
                void resize(int x, int y);
                                
		long clipx[2],clipy[2];
		long tex_pos[TEXTURENUM];

		long rect_id;

		LARGE_INTEGER print_time[100];
		long print_index;
		char display_frames;

		short force_full_display_count;

		char original_rect;

                int dimx, dimy;

		graphicst()
			{
			print_index=0;
			display_frames=0;
			rect_id=-1;
			force_full_display_count=4;
			original_rect=1;

                        screentexpos = NULL;
                        screentexpos_addcolor = NULL;
                        screentexpos_grayscale = NULL;
                        screentexpos_cf = NULL;
                        screentexpos_cbr = NULL;
                        screen = NULL;
                        }

                void locate(long y,long x)
                {
                  // No point in clamping here, addchar clamps too.
                  screenx=x;
                  screeny=y;
                }
                void changecolor(short f,short b,char bright)
                {
                  screenf=f;
                  screenb=b;
                  screenbright=bright;
                }
                void addchar(unsigned char c,char advance=1)
                {
                  /* assert (screen_limit == screen + dimy * dimx * 4); */
                  unsigned char *s = screen + screenx*dimy*4 + screeny*4;
                  if (s < screen_limit) {
	if(screenx>=clipx[0]&&screenx<=clipx[1]&&
		screeny>=clipy[0]&&screeny<=clipy[1])
		{
                    *s++ = c;
                    *s++ = screenf;
                    *s++ = screenb;
                    *s++ = screenbright;
                    screentexpos[screenx*dimy + screeny]=0;
	}
                  }
                  screenx += advance;
                }
                void addchar(unsigned int x, unsigned int y, unsigned char c,
                             unsigned char f, unsigned char b, unsigned char bright) {
                  /* assert (screen_limit == screen + dimy * dimx * 4); */
                  unsigned char *s = screen + x*dimy*4 + y*4;
                  if (s >= screen && s < screen_limit) {
	if(x>=clipx[0]&&x<=clipx[1]&&
		y>=clipy[0]&&y<=clipy[1])
		{
                    *s++ = c;
                    *s++ = f;
                    *s++ = b;
                    *s++ = bright;
	}
                  }
                }
		void addcoloredst(const char *str,const char *colorstr);
		void addst(const string &str, justification just = justify_left, int space=0);
		void erasescreen_clip();
		void erasescreen();
                void erasescreen_rect(int x1, int x2, int y1, int y2);
		void setclipping(long x1,long x2,long y1,long y2);

		void add_tile(long texp,char addcolor);
		void add_tile_grayscale(long texp,char cf,char cbr);

		void prepare_graphics(string &src_dir);

		void gray_out_rect(long sx,long ex,long sy,long ey)
                {
                  long x,y;
                  for(x=sx;x<=ex;x++)
                    {
                      for(y=sy;y<=ey;y++)
                        {
                          screen[x*dimy*4 + y*4 + 1]=0;
                          screen[x*dimy*4 + y*4 + 2]=7;
                          screen[x*dimy*4 + y*4 + 3]=0;
                        }
                    }
                }
		void dim_colors(long x,long y,char dim);

		void rain_color_square(long x,long y);
		void snow_color_square(long x,long y);
		void color_square(long x,long y,unsigned char f,unsigned char b,unsigned char br);

		long border_start_x(){return 1;}
		long border_start_y(){return 1;}
		long border_end_x(){return 78;}
		long border_end_y(){return 23;}
		long text_width(){return 1;}
		long text_height(){return 1;}
		long window_element_height(long minus,char border)
			{
			long height=25;
			if(border)height-=2;
			height-=text_height()*minus;
			return height;
			}

                int mouse_x, mouse_y;
		void get_mouse_text_coords(int32_t &mx, int32_t &my);
                void draw_border(int x1, int x2, int y1, int y2);

                // Instead of doing costly bounds-checking calculations, we cache the end
                // of the arrays..
                unsigned char *screen_limit;
};

extern graphicst gps;
// From graphics.cpp
void render_things();

// Locates some area of the screen with free space for writing
class gps_locator {
  int y, last_x;
public:
  gps_locator(int y, int x) {
    this->y = y;
    last_x = x;
  }
  bool is_free(int x) {
    unsigned char c = gps.screen[x*gps.dimy*4 + y*4];
    switch (c) {
    case 0:
    case 20:
    case 176:
    case 177:
    case 178:
    case 219:
    case 254:
    case 255:
      return true;
    default:
      return false;
    }
  }
  void operator()(int sz) {
    // First, check if our cached slot will still do
    bool ok = true;
    for (int x = last_x; x < last_x + sz; x++)
      if (!is_free(x)) {
        ok = false;
        break;
      }
    if (ok) {
      // Yeah, okay
      gps.locate(y, last_x);
    } else {
      // Not so okay. Find a new spot.
      int run = 0, x = 0;
      for (; x < gps.dimx; x++) {
        if (is_free(x))
          run++;
        else run = 0;
        if (run > sz + 2) { // We pad things a bit for cleanliness.
          ok = true;
          x -= sz + 1;
          break;
        }
      }
      if (ok) {
        // Found a new spot.
        last_x = x;
        gps.locate(y, x);
      } else {
        // Damn it.
        gps.locate(y, last_x);
      }
    }
  }
};

#endif
