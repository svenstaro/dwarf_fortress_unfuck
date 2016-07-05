//some of this stuff is based on public domain code from nehe or opengl books over the years
//additions and modifications Copyright (c) 2008, Tarn Adams
//All rights reserved.  See game.cpp or license.txt for more information.

#ifndef ENABLER_H
#define ENABLER_H

#include "platform.h"

#include <map>
#include <vector>
#include <algorithm>
#include <utility>
#include <list>
#include <iostream>
#include <sstream>
#include <stack>
#include <queue>
#include <set>
#include <functional>

using std::vector;
using std::pair;
using std::map;
using std::set;
using std::list;
using std::stack;
using std::queue;

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#ifdef __APPLE__
# include <SDL_ttf/SDL_ttf.h>
# include <SDL_image/SDL_image.h>
#else
# include <SDL/SDL_ttf.h>
# include <SDL/SDL_image.h>
#endif

#include "GL/glew.h"

#include "basics.h"
#include "svector.h"
#include "endian.h"
#include "files.h"
#include "enabler_input.h"
#include "mail.hpp"

#define ENABLER

#ifndef BITS

#define BITS

#define BIT1 1
#define BIT2 2
#define BIT3 4
#define BIT4 8
#define BIT5 16
#define BIT6 32
#define BIT7 64
#define BIT8 128
#define BIT9 256
#define BIT10 512
#define BIT11 1024
#define BIT12 2048
#define BIT13 4096
#define BIT14 8192
#define BIT15 16384
#define BIT16 32768
#define BIT17 65536UL
#define BIT18 131072UL
#define BIT19 262144UL
#define BIT20 524288UL
#define BIT21 1048576UL
#define BIT22 2097152UL
#define BIT23 4194304UL
#define BIT24 8388608UL
#define BIT25 16777216UL
#define BIT26 33554432UL
#define BIT27 67108864UL
#define BIT28 134217728UL
#define BIT29 268435456UL
#define BIT30 536870912UL
#define BIT31 1073741824UL
#define BIT32 2147483648UL

#endif

#define GAME_TITLE_STRING "Dwarf Fortress"

class pstringst
{
 public:
  string dat;
};

class stringvectst
{
	public:
		svector<pstringst *> str;

		void add_string(const string &st)
			{
			pstringst *newp=new pstringst;
				newp->dat=st;
			str.push_back(newp);
			}

		long add_unique_string(const string &st)
			{
			long i;
			for(i=(long)str.size()-1;i>=0;i--)
				{
				if(str[i]->dat==st)return i;
				}
			add_string(st);
			return (long)str.size()-1;
			}

		void add_string(const char *st)
			{
			if(st!=NULL)
				{
				pstringst *newp=new pstringst;
					newp->dat=st;
				str.push_back(newp);
				}
			}

		void insert_string(long k,const string &st)
			{
			pstringst *newp=new pstringst;
				newp->dat=st;
			if(str.size()>k)str.insert(k,newp);
			else str.push_back(newp);
			}

		~stringvectst()
			{
			clean();
			}

		void clean()
			{
			while(str.size()>0)
				{
				delete str[0];
				str.erase(0);
				}
			}

		void read_file(file_compressorst &filecomp,long loadversion)
			{
			int32_t dummy;
			filecomp.read_file(dummy);
			str.resize(dummy);

			long s;
			for(s=0;s<dummy;s++)
				{
				str[s]=new pstringst;
				filecomp.read_file(str[s]->dat);
				}
			}
		void write_file(file_compressorst &filecomp)
			{
			int32_t dummy=(int32_t)str.size();
			filecomp.write_file(dummy);

			long s;
			for(s=0;s<dummy;s++)
				{
				filecomp.write_file(str[s]->dat);
				}
			}

		void copy_from(stringvectst &src)
			{
			clean();

			str.resize(src.str.size());

			long s;
			for(s=(long)src.str.size()-1;s>=0;s--)
				{
				str[s]=new pstringst;
					str[s]->dat=src.str[s]->dat;
				}
			}

		bool has_string(const string &st)
			{
			long i;
			for(i=(long)str.size()-1;i>=0;i--)
				{
				if(str[i]->dat==st)return true;
				}
			return false;
			}

		void remove_string(const string &st)
			{
			long i;
			for(i=(long)str.size()-1;i>=0;i--)
				{
				if(str[i]->dat==st)
					{
					delete str[i];
					str.erase(i);
					}
				}
			}

		void operator=(stringvectst &two);
};

class flagarrayst
{
	public:
		flagarrayst()
			{
			slotnum=0;
			array=NULL;
			}
		~flagarrayst()
			{
			if(array!=NULL)delete[] array;
			array=NULL;
			slotnum=0;
			}

		void set_size_on_flag_num(long flagnum)
			{
			if(flagnum<=0)return;

			set_size(((flagnum-1)>>3)+1);
			}

		void set_size(long newsize)
			{
			if(newsize<=0)return;

			if(array!=NULL)delete[] array;
			array=new unsigned char[newsize];
			memset(array,0,sizeof(unsigned char)*newsize);

			slotnum=newsize;
			}

		void clear_all()
			{
			if(slotnum<=0)return;

			if(array!=NULL)memset(array,0,sizeof(unsigned char)*slotnum);
			}

		void copy_from(flagarrayst &src)
			{
			clear_all();

			if(src.slotnum>0)
				{
				set_size(src.slotnum);
				memmove(array,src.array,sizeof(unsigned char)*slotnum);
				}
			}

		bool has_flag(long checkflag)
			{
			if(checkflag<0)return false;
			long slot=checkflag>>3;
			return (slot>=0&&slot<slotnum&&((array[slot] & (1<<(checkflag&7)))!=0));
			}

		void add_flag(long checkflag)
			{
			if(checkflag<0)return;
			long slot=checkflag>>3;
			if(slot>=0&&slot<slotnum)array[slot]|=(1<<(checkflag&7));
			}

		void toggle_flag(long checkflag)
			{
			if(checkflag<0)return;
			long slot=checkflag>>3;
			if(slot>=0&&slot<slotnum)array[slot]^=(1<<(checkflag&7));
			}

		void remove_flag(long checkflag)
			{
			if(checkflag<0)return;
			long slot=checkflag>>3;
			if(slot>=0&&slot<slotnum)array[slot]&=~(1<<(checkflag&7));
			}

		void write_file(file_compressorst &filecomp)
			{
			filecomp.write_file(slotnum);
			if(slotnum>0)
				{
				long ind;
				for(ind=0;ind<slotnum;ind++)filecomp.write_file(array[ind]);
				}
			}

		void read_file(file_compressorst &filecomp,long loadversion)
			{
			int32_t newsl;
			filecomp.read_file(newsl);
			if(newsl>0)
				{
				//AVOID UNNECESSARY DELETE/NEW
				if(array!=NULL&&slotnum!=newsl)
					{
					delete[] array;
					array=new unsigned char[newsl];
					}
				if(array==NULL)array=new unsigned char[newsl];

				long ind;
				for(ind=0;ind<newsl;ind++)filecomp.read_file(array[ind]);

				slotnum=newsl;
				}
			else if(array!=NULL)
				{
				delete[] array;
				array=NULL;

				slotnum=0;
				}
			}

	private:
		unsigned char *array;
		int32_t slotnum;
};

#ifdef ENABLER

#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_YELLOW 6
#define COLOR_WHITE	7

enum ColorData
  {
    COLOR_DATA_WHITE_R,
    COLOR_DATA_WHITE_G,
    COLOR_DATA_WHITE_B,
    COLOR_DATA_RED_R,
    COLOR_DATA_RED_G,
    COLOR_DATA_RED_B,
    COLOR_DATA_GREEN_R,
    COLOR_DATA_GREEN_G,
    COLOR_DATA_GREEN_B,
    COLOR_DATA_BLUE_R,
    COLOR_DATA_BLUE_G,
    COLOR_DATA_BLUE_B,
    COLOR_DATA_YELLOW_R,
    COLOR_DATA_YELLOW_G,
    COLOR_DATA_YELLOW_B,
    COLOR_DATA_MAGENTA_R,
    COLOR_DATA_MAGENTA_G,
    COLOR_DATA_MAGENTA_B,
    COLOR_DATA_CYAN_R,
    COLOR_DATA_CYAN_G,
    COLOR_DATA_CYAN_B,
    COLOR_DATANUM
  };

#define TILEFLAG_DEAD BIT1
#define TILEFLAG_ROTATE BIT2
#define TILEFLAG_PIXRECT BIT3
#define TILEFLAG_HORFLIP BIT4
#define TILEFLAG_VERFLIP BIT5
#define TILEFLAG_LINE BIT6
#define TILEFLAG_RECT BIT7
#define TILEFLAG_BUFFER_DRAW BIT8
#define TILEFLAG_MODEL_PERSPECTIVE BIT9
#define TILEFLAG_MODEL_ORTHO BIT10
#define TILEFLAG_MODEL_TRANSLATE BIT11
#define TILEFLAG_LINE_3D BIT12

#define TRIMAX 9999

enum render_phase {
  setup, // 0
  complete,
  phase_count
};

class texture_bo {
  GLuint bo, tbo;
 public:
  texture_bo() { bo = tbo = 0; }
  void reset() {
    if (bo) {
      glDeleteBuffers(1, &bo);
      glDeleteTextures(1, &tbo);
      bo = tbo = 0;
      printGLError();
    }
  }
  void buffer(GLvoid *ptr, GLsizeiptr sz) {
    if (bo) reset();
    glGenBuffersARB(1, &bo);
    glGenTextures(1, &tbo);
    glBindBufferARB(GL_TEXTURE_BUFFER_ARB, bo);
    glBufferDataARB(GL_TEXTURE_BUFFER_ARB, sz, ptr, GL_STATIC_DRAW_ARB);
    printGLError();
  }
  void bind(GLenum texture_unit, GLenum type) {
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_BUFFER_ARB, tbo);
    glTexBufferARB(GL_TEXTURE_BUFFER_ARB, type, bo);
    printGLError();
  }
  GLuint texnum() { return tbo; }
};


class shader {
  string filename;
  std::ostringstream lines;
 public:
  std::ostringstream header;
  void load(const string &filename) {
    this->filename = filename;
    std::ifstream file(filename.c_str());
    string version;
    getline(file, version);
    header << version << std::endl;
    while (file.good()) {
      string line;
      getline(file, line);
      lines << line << std::endl;
    }
    file.close();
  }
  GLuint upload(GLenum type) {
    GLuint shader = glCreateShader(type);
    string lines_done = lines.str(), header_done = header.str();
    const char *ptrs[3];
    ptrs[0] = header_done.c_str();
    ptrs[1] = "#line 1 0\n";
    ptrs[2] = lines_done.c_str();
    glShaderSource(shader, 3, ptrs, NULL);
    glCompileShader(shader);
    // Let's see if this compiled correctly..
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) { // ..no. Check the compilation log.
      GLint log_size;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
      //errorlog << filename << " preprocessed source:" << std::endl;
      std::cerr << filename << " preprocessed source:" << std::endl;
      //errorlog << header_done << "#line 1 0\n" << lines_done;
      std::cerr << header_done << "#line 1 0\n" << lines_done;
      //errorlog << filename << " shader compilation log (" << log_size << "):" << std::endl;
      std::cerr << filename << " shader compilation log (" << log_size << "):" << std::endl;
      char *buf = new char[log_size];
      glGetShaderInfoLog(shader, log_size, NULL, buf);
      //errorlog << buf << std::endl;
      std::cerr << buf << std::endl;
      //errorlog.flush();
      delete[] buf;
      MessageBox(NULL, "Shader compilation failed; details in errorlog.txt", "Critical error", MB_OK);
      abort();
    }
    printGLError();
    return shader;
  }
};


class text_info_elementst
{
 public:
  virtual string get_string()
  {
    string empty;
    return empty;
  }
  virtual long get_long()
  {
    return 0;
  }

  virtual ~text_info_elementst(){}
};

class text_info_element_stringst : public text_info_elementst
{
 public:
  virtual string get_string()
  {
    return str;
  }
  text_info_element_stringst(const string &newstr)
    {
      str=newstr;
    }

 protected:
  string str;
};

class text_info_element_longst : public text_info_elementst
{
 public:
  virtual long get_long()
  {
    return val;
  }
  text_info_element_longst(long nval)
    {
      val=nval;
    }

 protected:
  long val;
};

class text_infost
{
 public:
  svector<text_info_elementst *> element;

  void clean()
  {
    while(element.size()>0)
      {
	delete element[0];
	element.erase(0);
      }
  }

  string get_string(int e)
  {
    if(e<0||e>=element.size())
      {
	string empty;
	return empty;
      }
    if(element[e]==NULL)
      {
	string empty;
	return empty;
      }
    return element[e]->get_string();
  }

  long get_long(int e)
  {
    if(e<0||e>=element.size())
      {
	return 0;
      }
    if(element[e]==NULL)
      {
	return 0;
      }
    return element[e]->get_long();
  }

  ~text_infost()
    {
      clean();
    }
};

class text_system_file_infost
{
 public:
  long index;
  string filename;

  static text_system_file_infost *add_file_info(const string &newf,long newi,char newft)
  {
    return new text_system_file_infost(newf,newi,newft);
  }

  void initialize_info();
  void get_text(text_infost &text);
  void get_specific_text(text_infost &text,long num);

 protected:
  char file_token;
  long number;

  text_system_file_infost(const string &newf,long newi,char newft)
    {
      filename=newf;
      file_token=newft;
      index=newi;
      number=0;
    }
};

class text_systemst
{
 public:
  void register_file_fixed(const string &file_name,int32_t index,char token,char initialize)
  {
    text_system_file_infost *tsfi=text_system_file_infost::add_file_info(file_name,index,token);
    if(initialize)tsfi->initialize_info();
    file_info.push_back(tsfi);
  }
  void register_file(const string &file_name,int32_t &index,char token,char initialize)
  {
    int32_t t;
    for(t=(int32_t)file_info.size()-1;t>=0;t--)
      {
	if(file_info[t]->filename==file_name)
	  {
	    //RESET CALLING INDEX AND BAIL IF THIS FILE IS ALREADY IN THE SYSTEM
	    index=file_info[t]->index;
	    return;
	  }
      }

    text_system_file_infost *tsfi=text_system_file_infost::add_file_info(file_name,index,token);
    if(initialize)tsfi->initialize_info();
    file_info.push_back(tsfi);
  }
  void initialize_system()
  {
    int32_t t;
    for(t=(int32_t)file_info.size()-1;t>=0;t--)file_info[t]->initialize_info();
  }
  void get_text(int32_t index,text_infost &text)
  {
    int32_t t;
    for(t=(int32_t)file_info.size()-1;t>=0;t--)
      {
	if(file_info[t]->index==index)
	  {
	    file_info[t]->get_text(text);
	    return;
	  }
      }
  }
  void get_text(const string &file_name,text_infost &text)
  {
    int32_t t;
    for(t=(int32_t)file_info.size()-1;t>=0;t--)
      {
	if(file_info[t]->filename==file_name)
	  {
	    file_info[t]->get_text(text);
	    return;
	  }
      }
  }
  void get_specific_text(int32_t index,text_infost &text,int32_t num)
  {
    int32_t t;
    for(t=(int32_t)file_info.size()-1;t>=0;t--)
      {
	if(file_info[t]->index==index)
	  {
	    file_info[t]->get_specific_text(text,num);
	    return;
	  }
      }
  }

  ~text_systemst()
    {
      while(file_info.size()>0)
	{
	  delete file_info[0];
	  file_info.erase(0);
	}
    }

 protected:
  svector<text_system_file_infost *> file_info;
};

class curses_text_boxst
{
	public:
		stringvectst text;

		void add_paragraph(stringvectst &src,int32_t para_width);
		void add_paragraph(const string &src,int32_t para_width);

		void read_file(file_compressorst &filecomp,int32_t loadversion)
			{
			text.read_file(filecomp,loadversion);
			}
		void write_file(file_compressorst &filecomp)
			{
			text.write_file(filecomp);
			}
		void clean()
			{
			text.clean();
			}
};

#define COPYTEXTUREFLAG_HORFLIP BIT1
#define COPYTEXTUREFLAG_VERFLIP BIT2

#define ENABLERFLAG_RENDER BIT1
#define ENABLERFLAG_MAXFPS BIT2

// GL texture positions
struct gl_texpos {
  GLfloat left, right, top, bottom;
};

// Covers every allowed permutation of text
struct ttf_id {
  std::string text;
  unsigned char fg, bg, bold;
  
  bool operator< (const ttf_id &other) const {
    if (fg != other.fg) return fg < other.fg;
    if (bg != other.bg) return bg < other.bg;
    if (bold != other.bold) return bold < other.bold;
    return text < other.text;
  }

  bool operator== (const ttf_id &other) const {
    return fg == other.fg && bg == other.bg && bold == other.bold && text == other.text;
  }
};

namespace std {
  template<> struct hash<ttf_id> {
    size_t operator()(ttf_id val) const {
      // Not the ideal hash function, but it'll do. And it's better than GCC's. id? Seriously?
      return hash<string>()(val.text) + val.fg + (val.bg << 4) + (val.bold << 8);
    }
  };
};

// Being a texture catalog interface, with opengl, sdl and truetype capability
class textures
{
  friend class enablerst;
  friend class renderer_opengl;
 private:
  vector<SDL_Surface *> raws;
  bool uploaded;
  long add_texture(SDL_Surface*);
 protected:
  GLuint gl_catalog; // texture catalog gennum
  struct gl_texpos *gl_texpos; // Texture positions in the GL catalog, if any
 public:
  // Initialize state variables
  textures() {
    uploaded = false;
    gl_texpos = NULL;
  }
  ~textures() {
  	for (auto it = raws.cbegin(); it != raws.cend(); ++it)
		SDL_FreeSurface(*it);
}
  int textureCount() {
    return (int)raws.size();
  }
  // Upload in-memory textures to the GPU
  // When textures are uploaded, any alteration to a texture
  // is automatically reflected in the uploaded copy - eg. it's replaced.
  // This is very expensive in opengl mode. Don't do it often.
  void upload_textures();
  // Also, you really should try to remove uploaded textures before
  // deleting a window, in case of driver memory leaks.
  void remove_uploaded_textures();
  // Returns the most recent texture data
  SDL_Surface *get_texture_data(long pos);
  // Clone a texture
  long clone_texture(long src);
  // Remove all color, but not transparency
  void grayscale_texture(long pos);
  // Loads dimx*dimy textures from a file, assuming all tiles
  // are equally large and arranged in a grid
  // Texture positions are saved in row-major order to tex_pos
  // If convert_magenta is true and the file does not have built-in transparency,
  // any magenta (255,0,255 RGB) is converted to full transparency
  // The calculated size of individual tiles is saved to disp_x, disp_y
  void load_multi_pdim(const string &filename,long *tex_pos,long dimx,long dimy,
		       bool convert_magenta,
		       long *disp_x, long *disp_y);
  // Loads a single texture from a file, returning the handle
  long load(const string &filename, bool convert_magenta);
  // To delete a texture..
  void delete_texture(long pos);
};

struct tile {
  int x, y;
  long tex;
};

typedef struct {									// Window Creation Info
  char*				title;						// Window Title
  int					width;						// Width
  int					height;						// Height
  int					bitsPerPixel;				// Bits Per Pixel
  BOOL				isFullScreen;				// FullScreen?
} GL_WindowInit;									// GL_WindowInit

typedef struct {									// Contains Information Vital To A Window
  GL_WindowInit		init;						// Window Init
  BOOL				isVisible;				// Window Visible?
} GL_Window;								// GL_Window

enum zoom_commands { zoom_in, zoom_out, zoom_reset, zoom_fullscreen, zoom_resetgrid };


struct texture_fullid {
  int texpos;
  float r, g, b;
  float br, bg, bb;

  bool operator< (const struct texture_fullid &other) const {
    if (texpos != other.texpos) return texpos < other.texpos;
    if (r != other.r) return r < other.r;
    if (g != other.g) return g < other.g;
    if (b != other.b) return b < other.b;
    if (br != other.br) return br < other.br;
    if (bg != other.bg) return bg < other.bg;
    return bb < other.bb;
  }
};

typedef int texture_ttfid; // Just the texpos

class renderer {
  void cleanup_arrays();
 protected:
  unsigned char *screen;
  long *screentexpos;
  char *screentexpos_addcolor;
  unsigned char *screentexpos_grayscale;
  unsigned char *screentexpos_cf;
  unsigned char *screentexpos_cbr;
  // For partial printing:
  unsigned char *screen_old;
  long *screentexpos_old;
  char *screentexpos_addcolor_old;
  unsigned char *screentexpos_grayscale_old;
  unsigned char *screentexpos_cf_old;
  unsigned char *screentexpos_cbr_old;

  void gps_allocate(int x, int y);
  Either<texture_fullid,texture_ttfid> screen_to_texid(int x, int y);
 public:
  void display();
  virtual void update_tile(int x, int y) = 0;
  virtual void update_all() = 0;
  virtual void render() = 0;
  virtual void set_fullscreen() {} // Should read from enabler.is_fullscreen()
  virtual void zoom(zoom_commands cmd) {};
  virtual void resize(int w, int h) = 0;
  virtual void grid_resize(int w, int h) = 0;
  void swap_arrays();
  renderer() {
    screen = NULL;
    screentexpos = NULL;
    screentexpos_addcolor = NULL;
    screentexpos_grayscale = NULL;
    screentexpos_cf = NULL;
    screentexpos_cbr = NULL;
    screen_old = NULL;
    screentexpos_old = NULL;
    screentexpos_addcolor_old = NULL;
    screentexpos_grayscale_old = NULL;
    screentexpos_cf_old = NULL;
    screentexpos_cbr_old = NULL;
  }
  virtual ~renderer() {
    cleanup_arrays();
  }
  virtual bool get_mouse_coords(int &x, int &y) = 0;
  virtual bool uses_opengl() { return false; };
};

class enablerst : public enabler_inputst
{
  friend class initst;
  friend class renderer_2d_base;
  friend class renderer_2d;
  friend class renderer_opengl;
  friend class renderer_curses;

  bool fullscreen;
  stack<pair<int,int> > overridden_grid_sizes;

  class renderer *renderer;
  void eventLoop_SDL();
#ifdef CURSES
  void eventLoop_ncurses();
#endif
  
  // Framerate calculations
  int calculated_fps, calculated_gfps;
  queue<int> frame_timings, gframe_timings; // Milisecond lengths of the last few frames
  int frame_sum, gframe_sum;
  int frame_last, gframe_last; // SDL_GetTick returns
  void do_update_fps(queue<int> &q, int &sum, int &last, int &calc);

 public:
  void clear_fps();
 private:
  void update_fps();
  void update_gfps();

  // Frame timing calculations
  float fps, gfps;
  float fps_per_gfps;
  Uint32 last_tick;
  float outstanding_frames, outstanding_gframes;

  // Async rendering
  struct async_cmd {
    enum cmd_t { pause, start, render, inc, set_fps } cmd;
    int val; // If async_inc, number of extra frames to run. If set_fps, current value of fps.
    async_cmd() {}
    async_cmd(cmd_t c) { cmd = c; }
  };

  struct async_msg {
    enum msg_t { quit, complete, set_fps, set_gfps, push_resize, pop_resize, reset_textures } msg;
    union {
      int fps; // set_fps, set_gfps
      struct { // push_resize
        int x, y;
      };
    };
    async_msg() {}
    async_msg(msg_t m) { msg = m; }
  };
      
  unsigned int async_frames;      // Number of frames the async thread has been asked to run
  bool async_paused;
  Chan<async_cmd> async_tobox;    // Messages to the simulation thread
  Chan<async_msg> async_frombox;  // Messages from the simulation thread, and acknowledgements of those to
  Chan<zoom_commands> async_zoom; // Zoom commands (from the simulation thread)
  Chan<void> async_fromcomplete;  // Barrier for async_msg requests that require acknowledgement
 public:
  Uint32 renderer_threadid;
 private:

  void pause_async_loop();
  void async_wait();
  void unpause_async_loop() {
    struct async_cmd cmd;
    cmd.cmd = async_cmd::start;
    async_tobox.write(cmd);
  }
  
 public:

  string command_line;

  float ccolor[16][3]; // The curses-RGB mapping used for non-curses display modes
  
  enablerst();
  unsigned long flag; // ENABLERFLAG_RENDER, ENABLERFLAG_MAXFPS

  int loop(string cmdline);
  void async_loop();
  void do_frame();
  
  // Framerate interface
  void set_fps(int fps);
  void set_gfps(int gfps);
  int get_fps() { return (int)fps; }
  int get_gfps() { return (int)gfps; }
  int calculate_fps();  // Calculate the actual provided (G)FPS
  int calculate_gfps();

  // Mouse interface, such as it is
  char mouse_lbut,mouse_rbut,mouse_lbut_down,mouse_rbut_down,mouse_lbut_lift,mouse_rbut_lift;
  char tracking_on;   // Whether we're tracking the mouse or not

  // OpenGL state (wrappers)
  class textures textures; // Font/graphics texture catalog
  GLsync sync; // Rendering barrier
  void reset_textures() {
    async_frombox.write(async_msg(async_msg::reset_textures));
  }
  bool uses_opengl() {
    if (!renderer) return false;
    return renderer->uses_opengl();
  }
  
  // Grid-size interface
  void override_grid_size(int w, int h); // Pick a /particular/ grid-size
  void release_grid_size(); // Undoes override_grid_size
  void zoom_display(zoom_commands command);
  
  
  // Window management
  bool is_fullscreen() { return fullscreen; }
  void toggle_fullscreen() {
    fullscreen = !fullscreen;
    async_zoom.write(zoom_fullscreen);
  }

  // Conversations
  text_systemst text_system;

  // TOADY: MOVE THESE TO "FRAMERATE INTERFACE"
  MVar<int> simticks, gputicks;
  Uint32 clock; // An *approximation* of the current time for use in garbage collection thingies, updated every frame or so.
};
#endif

// Function prototypes for deep-DF calls
char beginroutine();
char mainloop();
void endroutine();

extern enablerst enabler;

#endif //ENABLER_H
