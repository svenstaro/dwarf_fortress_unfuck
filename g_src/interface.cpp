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

#include "svector.h"
using std::string;

#include "endian.h"

#include "files.h"

#include "enabler.h"

#include "textlines.h"

#include "find_files.h"

#include "basics.h"

#include "g_basics.h"

#include "music_and_sound_g.h"

#include "graphics.h"

#include "init.h"

#include "keybindings.h"
#include "interface.h"
#include "KeybindingScreen.h"
#include "ttf_manager.hpp"

#include <list>
#include <set>

void dwarf_end_announcements();
void dwarf_remove_screen();
void dwarf_option_screen();
void drawborder(const char *str,char style,const char *colorstr);


inline void CHECK_ERR(int err, const char* msg)
{
	if (err != Z_OK)
		{
		MessageBox(NULL, "One of the compressed files on disk has errors in it.  Restore from backup if you are able.", 0, 0);
		exit(1);
		}
}

using std::fstream;
using std::ios;
using std::list;
using std::set;

extern interfacest gview;
extern enablerst enabler;
extern graphicst gps;
extern initst init;
#ifndef NO_FMOD
extern musicsoundst musicsound;
#endif

extern GameMode gamemode;
extern GameType gametype;

extern int32_t movie_version;




void viewscreen_movieplayerst::help()
{
	if(is_playing)return;

	viewscreenst::help();
}

void interfacest::finish_movie()
{
	supermovie_on=0;
	currentblocksize=0;
	nextfilepos=0;
	supermovie_pos=0;
	viewscreen_movieplayerst::create(INTERFACE_PUSH_AT_BACK);
}

void interfacest::use_movie_input()
{
	if(supermovie_on)handlemovie(1);
	finish_movie();
}

viewscreen_movieplayerst *viewscreen_movieplayerst::create(char pushtype,viewscreenst *scr)
{
	viewscreen_movieplayerst *newv=new viewscreen_movieplayerst();
	gview.addscreen(newv,pushtype,scr);

	return newv;
}

void viewscreen_movieplayerst::force_play(const string &file)
{
	force_file=file;
	is_forced_play=1;
}

void viewscreen_movieplayerst::logic()
{
	enabler.flag&=~ENABLERFLAG_MAXFPS;

	enabler.flag|=ENABLERFLAG_RENDER;

	if(!force_file.empty()&&!is_playing&&!quit_if_no_play&&is_forced_play)
		{
		is_playing=1;
		quit_if_no_play=1;
		gview.movie_file=force_file;
		gview.supermovie_on=0;
		gview.currentblocksize=0;
		gview.nextfilepos=0;
		gview.supermovie_pos=0;
		maxmoviepos=0;
		}

	if(!is_playing&&quit_if_no_play)
		{
		breakdownlevel=INTERFACE_BREAKDOWN_STOPSCREEN;
		return;
		}

	//LOAD A MOVIE BUFFER BY BUFFER
	if(is_playing)
		{
		if(gview.supermovie_pos>=MOVIEBUFFSIZE||gview.currentblocksize==0)
			{
			gview.read_movie_chunk(maxmoviepos,is_playing);
			}

		if(is_playing)
			{
			int half_frame_size=init.display.grid_x*init.display.grid_y;

#ifndef NO_FMOD
			//PLAY ANY RELEVANT SOUNDS
			if(gview.supermovie_delaystep==gview.supermovie_delayrate)
				{
				int fr=gview.supermovie_pos/(half_frame_size*2);
				if(fr>=0&&fr<200)
					{
					int c,sd;
					for(c=0;c<16;c++)
						{
						sd=gview.supermovie_sound_time[fr][c];
						if(sd>=0&&sd<gview.supermovie_sound.str.size())
							{
							musicsound.playsound(sd,c);
							}
						}
					}
				}
#endif

			//PRINT THE NEXT FRAME AND ADVANCE POSITION
			short x2,y2;
			for(x2=0;x2<init.display.grid_x;x2++)
				{
				for(y2=0;y2<init.display.grid_y;y2++)
					{
					gview.supermovie_pos++;
					}
				}
			if(gview.supermovie_delaystep==0)
				{
				gview.supermovie_pos+=half_frame_size;
				gview.supermovie_delaystep=gview.supermovie_delayrate;
				}
			else
				{
				gview.supermovie_pos-=half_frame_size;//RETURN TO LAST FRAME
				gview.supermovie_delaystep--;
				}

			//DONE
			if(gview.supermovie_pos>=maxmoviepos&&
				maxmoviepos+half_frame_size*2<MOVIEBUFFSIZE)
				{
				is_playing=0;
				//NOTE: THIS CAUSES IT TO LOSE THE LAST FRAME DUE TO CLEARING
				}
			}
		}
}

void viewscreen_movieplayerst::render()
{
	if(breakdownlevel!=INTERFACE_BREAKDOWN_NONE)return;
	
	if(!is_playing&&is_forced_play)return;

	if(!quit_if_no_play)
		{
		if(editing)drawborder(NULL);
		else drawborder("  Moving Records  ");
		}

	//LOAD A MOVIE BUFFER BY BUFFER
	if(is_playing)
		{
		if(gview.currentblocksize>0)
			{
			int32_t half_frame_size=init.display.grid_x*init.display.grid_y;

			//PRINT THE NEXT FRAME AND ADVANCE POSITION
			drawborder(NULL,-1);
		
			int32_t curp=gview.supermovie_pos;
				//HANG ON THE LAST FRAME TO AVOID POSSIBLE OVERRUNS
			if(gview.supermovie_pos>=MOVIEBUFFSIZE-half_frame_size*2)
				{
				curp=MOVIEBUFFSIZE-half_frame_size*4;
				}
			short x2,y2;
			for(x2=0;x2<init.display.grid_x;x2++)
				{
				for(y2=0;y2<init.display.grid_y;y2++,++curp)
					{
					gps.locate(y2,x2);

					gps.changecolor((gview.supermoviebuffer[curp+half_frame_size] & 7),
						(gview.supermoviebuffer[curp+half_frame_size] & 56)>>3,
						(gview.supermoviebuffer[curp+half_frame_size] & 64));

					gps.addchar(gview.supermoviebuffer[curp]);
					}
				}
			}
		}
	else if(loading)
		{
		int scroll=selfile/21*21;
		int l;
		for(l=scroll;l<filelist.size() && l<scroll+21;l++)
			{
			if(l==selfile)gps.changecolor(7,0,1);
			else gps.changecolor(7,0,0);
			gps.locate(l-scroll+2,2);
			gps.addst(filelist[l]);
			}
		}
#ifdef DEBUG_MOVIE_EDIT
	else if(editing)
		{
		if(editing_menu)
			{
			int tx,ty;
			unsigned char c=0;
			for(ty=0;ty<16;ty++)
				{
				for(tx=0;tx<16;tx++)
					{
					gps.locate(ty,tx);
					gps.changecolor(editing_screenf,editing_screenb,editing_screenbright);
					gps.addchar(c);
					c++;
					}
				}
			gps.locate(18,0);
			gps.changecolor(editing_screenf,editing_screenb,editing_screenbright);
			gps.addchar(editing_char);
			for(ty=0;ty<16;ty++)
				{
				for(tx=0;tx<8;tx++)
					{
					gps.locate(ty,tx+16);
					gps.changecolor(ty%8,tx,ty/8);
					gps.addchar('A');
					}
				}

			gps.changecolor(7,0,1);
			gps.locate(20,0);
			string str;
			gps.addst("1/100 sec per frame: ");convert_long_to_string(gview.supermovie_delayrate,str);
			gps.addst(str);

			int scroll=(editing_selected_sound/25)*25;
			int e;
			for(e=scroll;e<scroll+25&&e<gview.supermovie_sound.str.size();e++)
				{
				if(e==editing_selected_sound)gps.changecolor(7,0,1);
				else gps.changecolor(7,0,0);
				gps.locate(e-scroll,26);
				gps.addst(gview.supermovie_sound.str[e]->dat);
				}

			int frame=gview.supermovie_pos/4000,sd;
			for(e=0;e<SOUND_CHANNELNUM;e++)
				{
				gps.changecolor(2,0,1);
				gps.locate(e-scroll,52);
				sd=gview.supermovie_sound_time[frame][e];
				if(sd>=0&&sd<gview.supermovie_sound.str.size())gps.addst(gview.supermovie_sound.str[sd]->dat);
				else
					{
					gps.addst("-----------------");
					}
				gps.changecolor(4,0,1);
				gps.locate(e-scroll,(init.display.grid_x-1));
				gps.addst("X");
				}
			}
		else
			{
			drawborder(NULL,-1);

			int curp=gview.supermovie_pos;
			int x2,y2;
			for(x2=0;x2<80;x2++)
				{
				for(y2=0;y2<25;y2++)
					{
					gps.locate(y2,x2);

					gps.changecolor((gview.supermoviebuffer[curp+2000] & 7),
						(gview.supermoviebuffer[curp+2000] & 56)>>3,
						(gview.supermoviebuffer[curp+2000] & 64));

					gps.addchar(gview.supermoviebuffer[curp]);

					curp++;
					}
				}

			if(enabler.mouse_y<150)gps.locate(24,0);
			else gps.locate(0,0);
			gps.changecolor(2,0,1);
			gps.addst("Frame: ");
			string num;
			convert_long_to_string(gview.supermovie_pos/4000+1,num);
			gps.addst(num);

			if(enabler.mouse_y<150)gps.locate(24,20);
			else gps.locate(0,20);
			gps.changecolor(3,0,1);
			gps.addst("Copy From: ");
			convert_long_to_string(editing_copy_from/4000+1,num);
			gps.addst(num);

			if(enabler.mouse_y<150)gps.locate(24,40);
			else gps.locate(0,40);
			gps.changecolor(4,0,1);
			gps.addst("Ends At: ");
			convert_long_to_string(end_frame_pos/4000+1,num);
			gps.addst(num);

			if(enabler.mouse_y<150)gps.locate(24,60);
			else gps.locate(0,60);
			int sx,sy;
			gps.get_mouse_text_coords(sx,sy);
			gps.changecolor(7,0,1);
			gps.addst("(");
			convert_long_to_string(sx,num);
			gps.addst(num);
			gps.addst(",");
			convert_long_to_string(sy,num);
			gps.addst(num);
			gps.addst(")");
			}
		}
#endif
	else
		{
		gps.changecolor(7,0,1);
		gps.locate(2,2);
		gview.print_interface_token(INTERFACEKEY_MOVIE_RECORD);
		gps.addst(": Start recording (active record is erased, stops when you return here)");
		gps.locate(3,2);
		gview.print_interface_token(INTERFACEKEY_MOVIE_PLAY);
		gps.addst(": Play the active moving record");
		gps.locate(4,2);
		gview.print_interface_token(INTERFACEKEY_MOVIE_SAVE);
		gps.addst(": Save the active moving record (you will be prompted for a name)");
		gps.locate(5,2);
		gview.print_interface_token(INTERFACEKEY_MOVIE_LOAD);
		gps.addst(": Load a moving record");

#ifdef DEBUG_MOVIE_EDIT
		gps.locate(7,2);
		gps.addst("E: Edit");
#endif

		if(saving)
			{
			gps.locate(10,2);
			gps.addst("Name: ");
			gps.addst(savename);
			}
		}
}

void viewscreen_movieplayerst::feed(std::set<InterfaceKey> &events)
{
	if(events.count(INTERFACEKEY_LEAVESCREEN))
		{
		events.clear();

		if(is_playing)
			{
			is_playing=0;
                        enabler.release_grid_size();
                        if (gview.original_fps)
                          enabler.set_fps(gview.original_fps);
			gview.supermovie_on=0;
			gview.currentblocksize=0;
			gview.nextfilepos=0;
			gview.supermovie_pos=0;
			maxmoviepos=0;

#ifndef NO_FMOD
			musicsound.stop_sound();
#endif
			}
		else if(saving)saving=0;
		else if(loading)loading=0;
#ifdef DEBUG_MOVIE_EDIT
		else if(editing)editing=0;
#endif
		else
			{
			is_playing=0;
                        enabler.release_grid_size();
                        if (gview.original_fps)
                          enabler.set_fps(gview.original_fps);
			gview.supermovie_on=0;
			gview.currentblocksize=0;
			gview.nextfilepos=0;
			gview.supermovie_pos=0;
			maxmoviepos=0;

			breakdownlevel=INTERFACE_BREAKDOWN_STOPSCREEN;
			return;
			}
		}
	else if(saving)
		{
		standardstringentry(savename,39,STRINGENTRY_LETTERS|STRINGENTRY_SPACE|STRINGENTRY_NUMBERS|STRINGENTRY_SYMBOLS,events);

		if(events.count(INTERFACEKEY_SELECT))
			{
			string filename;
			filename="data/movies/";
			filename+=savename;
			filename+=".cmv";

			copy_file(gview.movie_file,filename);
			saving=0;
			}
		}
	else if(loading)
		{
		if(events.count(INTERFACEKEY_SELECT))
			{
			string filename;
			filename="data/movies/";
			filename+=filelist[selfile];

			if(filename!=gview.movie_file)
				{
				copy_file(filename,gview.movie_file);
				}
			loading=0;
			}

		standardscrolling(events,selfile,0,filelist.size()-1,21);
		}
#ifdef DEBUG_MOVIE_EDIT
	else if(editing)
		{
		char entering=0;

		if(editing_menu)
			{
			if(enabler.mouse_lbut&&enabler.tracking_on)
				{
				int sx,sy;
				gps.get_mouse_text_coords(sx,sy);

				if(sx>=0&&sx<80&&sy>=0&&sy<25)
					{
					if(sx>=0&&sx<16&&sy>=0&&sy<16)
						{
						editing_char=sx+sy*16;
						}
					if(sx>=16&&sx<24&&sy>=0&&sy<16)
						{
						editing_screenf=sy%8;
						editing_screenb=sx-16;
						editing_screenbright=sy/8;
						}
					if(sx>=26&&sx<=51&&sy>=0&&sy<SOUND_CHANNELNUM)
						{
						editing_selected_sound=sy;
						}
					int frame=gview.supermovie_pos/4000;
					if(sx>=52&&sx<=78&&sy>=0&&sy<SOUND_CHANNELNUM)
						{
						gview.supermovie_sound_time[frame][sy]=editing_selected_sound;
						}
					if(sx==(init.display.grid_x-1)&&sy>=0&&sy<SOUND_CHANNELNUM)
						{
						gview.supermovie_sound_time[frame][sy]=-1;
						}
					}

				enabler.mouse_lbut=0;
				}

			if(enabler.mouse_rbut&&enabler.tracking_on)
				{
				editing_menu=0;
				enabler.mouse_rbut=0;
				}

			if(editing_selected_sound>=0&&editing_selected_sound<gview.supermovie_sound.str.size())
				{
				if(gview.c== '%')
					{
					delete gview.supermovie_sound.str[editing_selected_sound];
					gview.supermovie_sound.str.erase(editing_selected_sound);
					}
				else
					{
					standardstringentry(events,gview.supermovie_sound.str[editing_selected_sound]->dat,26,STRINGENTRY_LETTERS|STRINGENTRY_SPACE|STRINGENTRY_NUMBERS|STRINGENTRY_SYMBOLS);
					entering=1;
					}
				}
			else
				{
				if(gview.c== '#')gview.supermovie_sound.add_string("new_sound");
				if(gview.c== '+')gview.supermovie_delayrate++;
				if(gview.c== '-')gview.supermovie_delayrate--;
				if(gview.c== 'T')text_mode=1-text_mode;
				}
			if(gview.supermovie_delayrate<0)gview.supermovie_delayrate=0;
			if(gview.supermovie_delayrate>10)gview.supermovie_delayrate=10;
			}
		else
			{
			if(text_mode)
				{
				if(gview.c!=0)
					{
					int sx,sy;
					gps.get_mouse_text_coords(sx,sy);

					if(sx>=0&&sx<80&&sy>=0&&sy<25)
						{
						int curpos=gview.supermovie_pos+sy+sx*25;
						gview.supermoviebuffer[curpos]=gview.c;
						gview.supermoviebuffer[curpos+2000]=(editing_screenf&7)+((editing_screenb&7)<<3);
						if(editing_screenbright)gview.supermoviebuffer[curpos+2000]+=64;
						}
					}
				}
			else
				{
				if(gview.c== 'a')
					{
					int x2,y2;
					for(x2=0;x2<80;x2++)
						{
						for(y2=0;y2<25;y2++)
							{
							if(x2>0)
								{
								gview.supermoviebuffer[gview.supermovie_pos+y2+(x2-1)*25]=gview.supermoviebuffer[gview.supermovie_pos+y2+x2*25];
								gview.supermoviebuffer[gview.supermovie_pos+y2+(x2-1)*25+2000]=gview.supermoviebuffer[gview.supermovie_pos+y2+x2*25+2000];
								}
							if(x2==(init.display.grid_x-1))gview.supermoviebuffer[gview.supermovie_pos+y2+x2*25]=0;
							}
						}
					}
				if(gview.c== 'd')
					{
					int x2,y2;
					for(x2=(init.display.grid_x-1);x2>=0;x2--)
						{
						for(y2=0;y2<(init.display.grid_y-1);y2++)
							{
							if(x2<(init.display.grid_x-1))
								{
								gview.supermoviebuffer[gview.supermovie_pos+y2+(x2+1)*25]=gview.supermoviebuffer[gview.supermovie_pos+y2+x2*25];
								gview.supermoviebuffer[gview.supermovie_pos+y2+(x2+1)*25+2000]=gview.supermoviebuffer[gview.supermovie_pos+y2+x2*25+2000];
								}
							if(x2==0)gview.supermoviebuffer[gview.supermovie_pos+y2+x2*25]=0;
							}
						}
					}
				if(gview.c== 'E')end_frame_pos=gview.supermovie_pos;
				if(gview.c== 'c')editing_copy_from=gview.supermovie_pos;
				if(gview.c== 'p')
					{
					int i;
					for(i=0;i<4000;i++)
						{
						gview.supermoviebuffer[gview.supermovie_pos+i]=gview.supermoviebuffer[editing_copy_from+i];
						}
					}
				if(gview.c== '+')gview.supermovie_pos+=4000;
				if(gview.c== '-')gview.supermovie_pos-=4000;
				if(gview.c== '/')gview.supermovie_pos-=40000;
				if(gview.c== '*')gview.supermovie_pos+=40000;
				if(gview.supermovie_pos<0)gview.supermovie_pos=0;
				if(gview.supermovie_pos>=MOVIEBUFFSIZE)gview.supermovie_pos=MOVIEBUFFSIZE-4000;
				}

			if(enabler.mouse_lbut&&enabler.tracking_on)
				{
				int sx,sy;
				gps.get_mouse_text_coords(sx,sy);

				if(sx>=0&&sx<80&&sy>=0&&sy<25)
					{
					int curpos=gview.supermovie_pos+sy+sx*25;
					gview.supermoviebuffer[curpos]=editing_char;
					gview.supermoviebuffer[curpos+2000]=(editing_screenf&7)+((editing_screenb&7)<<3);
					if(editing_screenbright)gview.supermoviebuffer[curpos+2000]+=64;
					}
				}
			if(enabler.mouse_rbut&&enabler.tracking_on)
				{
				editing_menu=1;
				enabler.mouse_rbut=0;
				}
			}

		if(!entering&&gview.c== 'S')
			{
			int opos=gview.supermovie_pos;
			gview.first_movie_write=1;
			gview.supermovie_pos=end_frame_pos+4000;

			gview.write_movie_chunk();

			gview.supermovie_pos=opos;
			}
		}
#endif
	else
		{
		if(is_playing)
			{
			}
		else
			{
#ifdef DEBUG_MOVIE_EDIT
			if(gview.c== 'E')
				{
				editing=1;
				gview.supermovie_pos=0;
				}
#endif

			if(events.count(INTERFACEKEY_MOVIE_RECORD))
				{
				//TURN ON THE MOVIE RECORDER
				is_playing=0;
                                enabler.release_grid_size();
                                if (gview.original_fps)
                                  enabler.set_fps(gview.original_fps);
				gview.supermovie_on=1;
				gview.currentblocksize=0;
				gview.nextfilepos=0;
				gview.supermovie_pos=0;
				gview.supermovie_delayrate=0;
				gview.first_movie_write=1;
				maxmoviepos=0;

				breakdownlevel=INTERFACE_BREAKDOWN_STOPSCREEN;
				}
			if(events.count(INTERFACEKEY_MOVIE_PLAY))
				{
				is_playing=1;
				gview.supermovie_on=0;
				gview.currentblocksize=0;
				gview.nextfilepos=0;
				gview.supermovie_pos=0;
				maxmoviepos=0;
				}
			if(events.count(INTERFACEKEY_MOVIE_SAVE))
				{
				savename.erase();
				saving=1;
				}
			if(events.count(INTERFACEKEY_MOVIE_LOAD))
				{
				selfile=0;

				clearfilelist();

				find_files_by_pattern("data/movies/*.cmv",filelist);

				if(filelist.size()>0)loading=1;
				}
			}
		}
}

void viewscreen_movieplayerst::clearfilelist()
{
	int f;
	for(f=0;f<filelist.size();f++)delete[] filelist[f];
	filelist.clear();
}

viewscreen_movieplayerst::viewscreen_movieplayerst()
{
	force_file.erase();
	gview.movie_file="data/movies/last_record.cmv";
	is_playing=0;
        enabler.release_grid_size();
        if (gview.original_fps)
          enabler.set_fps(gview.original_fps);
	is_forced_play=0;
	quit_if_no_play=0;
	gview.supermovie_on=0;
	gview.currentblocksize=0;
	gview.nextfilepos=0;
	gview.supermovie_pos=0;
	maxmoviepos=0;
	saving=0;
	loading=0;
	editing=0;
	text_mode=0;
	editing_copy_from=0;
	editing_char=219;
	editing_screenf=7;
	editing_screenb=0;
	editing_screenbright=0;
	editing_menu=0;
	editing_selected_sound=0;
	end_frame_pos=0;
	gview.supermovie_sound.clean();
#ifndef NO_FMOD
	int i,c;
	for(i=0;i<200;i++)
		{
		for(c=0;c<SOUND_CHANNELNUM;c++)gview.supermovie_sound_time[i][c]=-1;
		}
#endif
}

interfacest::interfacest()
{
        original_fps = 0;
	shutdown_interface_for_ms=0;
	shutdown_interface_tickcount=0;
	flag=0;
	supermovie_on=0;
	supermovie_pos=0;
	supermovie_delayrate=0;
}

interfacest::~interfacest()
{
	//GO AHEAD
	while(view.child!=NULL)
		{
		removescreen(view.child);
		}
}

void interfacest::addscreen(viewscreenst *scr,char pushtype,viewscreenst *relate)
{
	gps.force_full_display_count+=2;

	switch(pushtype)
		{
		case INTERFACE_PUSH_AS_PARENT:insertscreen_as_parent(scr,relate);break;
		case INTERFACE_PUSH_AS_CHILD:insertscreen_as_child(scr,relate);break;
		case INTERFACE_PUSH_AT_FRONT:insertscreen_at_front(scr);break;
		default:insertscreen_at_back(scr);break;
		}

	//WHENEVER A SCREEN IS ADDED, END ANNOUNCEMENTS
	if(gamemode==GAMEMODE_DWARF)dwarf_end_announcements();
}

void interfacest::insertscreen_as_parent(viewscreenst *scr,viewscreenst *child)
{
	if(child==NULL)
		{
		insertscreen_at_back(scr);
		return;
		}

	scr->child=child;
	scr->parent=child->parent;

	if(scr->parent!=NULL)scr->parent->child=scr;
	child->parent=scr;
}

void interfacest::insertscreen_as_child(viewscreenst *scr,viewscreenst *parent)
{
	if(parent==NULL)
		{
		insertscreen_at_back(scr);
		return;
		}

	scr->child=parent->child;
	scr->parent=parent;

	if(scr->child!=NULL)scr->child->parent=scr;
	parent->child=scr;
}

void interfacest::insertscreen_at_back(viewscreenst *scr)
{
	//GRAB CURRENT SCREEN AT THE END OF THE LIST
	viewscreenst *currentscreen=&view;
	while(currentscreen->child!=NULL)currentscreen=currentscreen->child;

	//PUT IT ON TO THE BACK SCREEN
	insertscreen_as_child(scr,currentscreen);
}

void interfacest::insertscreen_at_front(viewscreenst *scr)
{
	//PUT IT ON TO THE BASE
	insertscreen_as_child(scr,&view);
}

viewscreenst *interfacest::grab_lastscreen() {
  viewscreenst *currentscreen = &view;
  while (currentscreen->child) currentscreen = currentscreen->child;
  return currentscreen;
}

char interfacest::loop() {
  //NO INTERFACE LEFT, QUIT
  if(view.child==0)return 1;

  //GRAB CURRENT SCREEN AT THE END OF THE LIST
  viewscreenst *currentscreen = grab_lastscreen();
  //MOVE SCREENS BACK
  switch(currentscreen->breakdownlevel) {
  case INTERFACE_BREAKDOWN_NONE: {
    
    currentscreen->logic();

	if(currentscreen->movies_okay())
		{
		//HANDLE MOVIES
		handlemovie(0);
		}

    const Time now = SDL_GetTicks();
    // Process as much input as possible. Some screens can't handle multiple input events
    // per logic call (retain_nonzero_input, and any alteration to the window setup
    // requires us to stop until the next logic call.
    for (;;) {
      if (currentscreen->child || currentscreen->breakdownlevel != INTERFACE_BREAKDOWN_NONE)
        break; // Some previous input or logic had the effect of switching screens

      if (flag & INTERFACEFLAG_RETAIN_NONZERO_INPUT) {
        flag&=~INTERFACEFLAG_RETAIN_NONZERO_INPUT;
        break;
      } else {
        set<InterfaceKey> era = enabler.get_input(now);
        if (era.size() == 0) {
          if(enabler.mouse_lbut || enabler.mouse_rbut) currentscreen->feed(era);
          break;
        }
        
        if (era.count(INTERFACEKEY_OPTIONS)&&!currentscreen->key_conflict(INTERFACEKEY_OPTIONS)) {
          //PEEL BACK ALL SCREENS TO THE CURRENT OPTION SCREEN IF THERE IS ONE
          //UNLESS THERE IS A BLOCKING SCREEN LIKE THE REGION MAKER
          viewscreenst *opscreen=&view;
          while(opscreen!=NULL) {
            if(opscreen->is_option_screen()) {
              opscreen->option_key_pressed=1;
              while(opscreen->child!=NULL) {
                if(opscreen->child->is_option_screen()==2) {
                  opscreen->child->option_key_pressed=1;
                  opscreen->option_key_pressed=0;
                  break;
                }
                removescreen(opscreen->child);
              }
              break;
            }
            opscreen = opscreen->child;
          }
          //NEED A NEW OPTIONS SCREEN?
          if(opscreen==NULL) dwarf_option_screen();

          era.clear();
          continue;
        }
        //DO MOVIE COMMANDS
        if (era.count(INTERFACEKEY_MOVIES)&&!currentscreen->key_conflict(INTERFACEKEY_MOVIES))
          if(currentscreen->movies_okay()) use_movie_input();
        if (era.count(INTERFACEKEY_HELP)&&!currentscreen->key_conflict(INTERFACEKEY_HELP))
          currentscreen->help();
        // Prefix commands
        // Most prefix commands we don't want to touch game management commands,
        // i.e. what's in here. Macro playback is a notable exception.
        if (era.count(INTERFACEKEY_PREFIX))
          enabler.prefix_toggle();
        int repeats = 1;  // If this input ends a prefix command, we'll want to repeat it.
        if (enabler.prefix_building()) {
          // TODO: OMGWTFBBQ
          char c = 0;
          if (era.count(INTERFACEKEY_STRING_A048)) c = '0';
          else if (era.count(INTERFACEKEY_STRING_A049)) c = '1';
          else if (era.count(INTERFACEKEY_STRING_A050)) c = '2';
          else if (era.count(INTERFACEKEY_STRING_A051)) c = '3';
          else if (era.count(INTERFACEKEY_STRING_A052)) c = '4';
          else if (era.count(INTERFACEKEY_STRING_A053)) c = '5';
          else if (era.count(INTERFACEKEY_STRING_A054)) c = '6';
          else if (era.count(INTERFACEKEY_STRING_A055)) c = '7';
          else if (era.count(INTERFACEKEY_STRING_A056)) c = '8';
          else if (era.count(INTERFACEKEY_STRING_A057)) c = '9';

          if (c) {
            enabler.prefix_add_digit(c);
            era.clear();
          } else {
            repeats = enabler.prefix_end();
          }
        }
        // TTF toggle
        if (era.count(INTERFACEKEY_TOGGLE_TTF)) {
          if (init.font.use_ttf == ttf_auto) {
            // Do whatever produces a visible result.
            if (ttf_manager.ttf_active())
              init.font.use_ttf = ttf_off;
            else
              init.font.use_ttf = ttf_on;
          } else if (init.font.use_ttf == ttf_on) {
            init.font.use_ttf = ttf_off;
          } else {
            init.font.use_ttf = ttf_on;
          }
          gps.force_full_display_count++;
        }
        // Zoom commands
        if (era.count(INTERFACEKEY_ZOOM_IN))
          enabler.zoom_display(zoom_in);
        if (era.count(INTERFACEKEY_ZOOM_OUT))
          enabler.zoom_display(zoom_out);
        if (era.count(INTERFACEKEY_ZOOM_RESET))
          enabler.zoom_display(zoom_reset);
        // Macro commands
        if (era.count(INTERFACEKEY_RECORD_MACRO)) {
          if (enabler.is_recording())
            enabler.record_stop();
          else
            enabler.record_input();
        }
        if (era.count(INTERFACEKEY_PLAY_MACRO)) {
          for (int i = 0; i < repeats; i++)
            enabler.play_macro();
        }
        if (era.count(INTERFACEKEY_SAVE_MACRO))
          gview.addscreen(new MacroScreenSave(), INTERFACE_PUSH_AT_BACK, NULL);
        if (era.count(INTERFACEKEY_LOAD_MACRO))
          gview.addscreen(new MacroScreenLoad(), INTERFACE_PUSH_AT_BACK, NULL);
        // Feed input
        for (int i = 0; i < repeats; i++)
          currentscreen->feed(era);
        if (era.count(INTERFACEKEY_TOGGLE_FULLSCREEN)) {
          enabler.toggle_fullscreen();
        }
        if (era.count(INTERFACEKEY_FPS_UP)) {
          int fps = enabler.get_fps();
          enabler.set_fps(fps + (fps+9)/10);
          enabler.clear_fps();
        }
        if (era.count(INTERFACEKEY_FPS_DOWN)) {
          int fps = enabler.get_fps();
          enabler.set_fps(fps - (fps+8)/10);
          enabler.clear_fps();
        }
      }
    }
    break;
  } // case INTERFACE_BREAKDOWN_NONE
    
  case INTERFACE_BREAKDOWN_QUIT:
    {
      handlemovie(1);
      return 1;
    }
  case INTERFACE_BREAKDOWN_STOPSCREEN:
    if(currentscreen->movies_okay())
      {
        //HANDLE MOVIES
        handlemovie(0);
      }
    
    removescreen(currentscreen);
    break;
  case INTERFACE_BREAKDOWN_TOFIRST:
    if(currentscreen->movies_okay())
      {
        //HANDLE MOVIES
        handlemovie(0);
      }
    
    remove_to_first();
    break;
  }
  
  return 0;
}

void interfacest::remove_to_first()
{
	//GRAB LAST SCREEN AT THE END OF THE LIST
	viewscreenst *lastscreen=&view;
	while(lastscreen->child!=NULL)lastscreen=lastscreen->child;

	//NO INTERFACE LEFT
	if(lastscreen==&view)return;

	//GO AHEAD
	while(lastscreen->parent!=&view)
		{
		viewscreenst *par=lastscreen->parent;
		removescreen(lastscreen);
		lastscreen=par;
		}
}

void interfacest::removescreen(viewscreenst *scr)
{
	//THE MINIMAP IS EXPENSIVE, SO WE REFRESH IT WHENEVER INTERFACE GETS IN THE WAY
	if(gamemode==GAMEMODE_DWARF)dwarf_remove_screen();

	//FIX LINKS
	if(scr->parent!=NULL)scr->parent->child=scr->child;
	if(scr->child!=NULL)scr->child->parent=scr->parent;

	//WASTE SCREEN
	delete scr;
}

int interfacest::write_movie_chunk()
{
	int inputsize=supermovie_pos;
	if(inputsize>MOVIEBUFFSIZE)inputsize=MOVIEBUFFSIZE;

	//DUMP CURRENT BUFFER INTO A COMPRESSION STREAM
	z_stream c_stream;
	int err;

	c_stream.zalloc = (alloc_func)0;
	c_stream.zfree = (free_func)0;
	c_stream.opaque = (voidpf)0;

	err = deflateInit(&c_stream, 9);
	CHECK_ERR(err, "deflateInit");

	c_stream.next_out = (Bytef*)supermoviebuffer_comp;
	c_stream.avail_out = COMPMOVIEBUFFSIZE;

	c_stream.next_in  = (Bytef*)supermoviebuffer;
	c_stream.avail_in  = inputsize;

	while (c_stream.total_in != inputsize && c_stream.total_out < COMPMOVIEBUFFSIZE) {
		//c_stream.avail_in = c_stream.avail_out = 1; // force small buffers
		err = deflate(&c_stream, Z_NO_FLUSH);
		CHECK_ERR(err, "deflate");
		}

	// Finish the stream, still forcing small buffers:
	for (;;) {
		err = deflate(&c_stream, Z_FINISH);
		if (err == Z_STREAM_END) break;
		CHECK_ERR(err, "deflate");
		}

	err = deflateEnd(&c_stream);
	CHECK_ERR(err, "deflateEnd");

	int length=0;

	if(c_stream.total_out>0)
		{
		if(first_movie_write)
			{
			//GET RID OF ANY EXISTING MOVIES IF THIS IS THE FIRST TIME THROUGH
			unlink(movie_file.c_str());
			}

		//OPEN UP THE MOVIE FILE AND APPEND
		std::fstream f;
		f.open(movie_file.c_str(), fstream::out | fstream::binary | fstream::app);

		if(f.is_open())
			{
			//WRITE A HEADER
			if(first_movie_write)
				{
				int swp_l=byteswap(movie_version);
				f.write((const char *)&swp_l,sizeof(int));


				cursesmovie_headerst cmh;
					cmh.dimx=init.display.grid_x;
					cmh.dimy=init.display.grid_y;
					cmh.delayrate=supermovie_delayrate;
					cmh.dimx=byteswap(cmh.dimx);
					cmh.dimy=byteswap(cmh.dimy);
					cmh.delayrate=byteswap(cmh.delayrate);
				f.write((const char *)&cmh,sizeof(cursesmovie_headerst));

				int32_t s=gview.supermovie_sound.str.size();
				s=byteswap(s);
				f.write((const char *)&s,sizeof(int32_t));
				char buf[50];
				for(s=0;s<gview.supermovie_sound.str.size();s++)
					{
					strcpy(buf,gview.supermovie_sound.str[s]->dat.c_str());
					f.write(buf,sizeof(char)*50);
					}

				int i1,i2;
				for(i1=0;i1<200;i1++)
					{
					for(i2=0;i2<SOUND_CHANNELNUM;i2++)
						{
#ifndef NO_FMOD
						swp_l=byteswap(gview.supermovie_sound_time[i1][i2]);
#else
                        swp_l=-1;
#endif
						f.write((const char *)&swp_l,sizeof(int));
						}
					}
				}

			//WRITE IT
			int compsize=byteswap(c_stream.total_out);
			f.write((const char *)&compsize,sizeof(int));
			f.write((const char *)supermoviebuffer_comp,c_stream.total_out);

			f.seekg(0,ios::beg);
			int beg=f.tellg();
			f.seekg(0,ios::end);
			int end=f.tellg();
			length=end-beg;

			f.close();
			}
		else supermovie_on=0;

		first_movie_write=0;
		}

	return length;
}

void interfacest::read_movie_chunk(int &maxmoviepos,char &is_playing)
{
	//OPEN UP THE MOVIE FILE AND MOVE TO CORRECT POSITION
	std::fstream f;
	f.open(movie_file.c_str(), fstream::in | fstream::binary);

	if(f.is_open())
		{
		f.seekg(0,ios::beg);
		int beg=f.tellg();
		f.seekg(0,ios::end);
		int end=f.tellg();
		int file_size=end-beg;

		if(gview.nextfilepos<file_size)
			{
			f.seekg(gview.nextfilepos,ios::beg);

			//LOAD THE HEADER
			char fail=0;
			if(gview.nextfilepos==0)
				{
				int loadversion;
				f.read((char *)&loadversion,sizeof(int));
				loadversion=byteswap(loadversion);

				if(loadversion>movie_version)fail=1;

				cursesmovie_headerst cmh;
				f.read((char *)&cmh,sizeof(cursesmovie_headerst));
                                cmh.dimx=byteswap(cmh.dimx);
                                cmh.dimy=byteswap(cmh.dimy);
                                cmh.delayrate=byteswap(cmh.delayrate);

                                enabler.override_grid_size(cmh.dimx, cmh.dimy);
                                if (!gview.original_fps)
                                  gview.original_fps = enabler.get_fps();
                                enabler.set_fps(100);

				gview.supermovie_delayrate=cmh.delayrate;
				gview.supermovie_delaystep=cmh.delayrate;

				gview.supermovie_sound.clean();
				if(loadversion>=10001)
					{
					int num;
					f.read((char *)&num,sizeof(int));
					num=byteswap(num);
					gview.nextfilepos+=sizeof(int);
					char buf[50];
					int s;
					for(s=0;s<num;s++)
						{
						f.read(buf,sizeof(char)*50);
						string str=buf;
						gview.supermovie_sound.add_string(str);
						gview.nextfilepos+=sizeof(char)*50;
						}

					int i1,i2,swp_l;
					for(i1=0;i1<200;i1++)
						{
						for(i2=0;i2<SOUND_CHANNELNUM;i2++)
							{
							f.read((char *)&swp_l,sizeof(int));
#ifndef NO_FMOD
							gview.supermovie_sound_time[i1][i2]=byteswap(swp_l);
#endif
							}
						}

					gview.nextfilepos+=sizeof(int)*200*SOUND_CHANNELNUM;
					}
				else
					{
#ifndef NO_FMOD
					int i,c;
					for(i=0;i<200;i++)
						{
						for(c=0;c<SOUND_CHANNELNUM;c++)gview.supermovie_sound_time[i][c]=-1;
						}
#endif
					}

				gview.nextfilepos+=sizeof(int)+sizeof(cursesmovie_headerst);

#ifndef NO_FMOD
				//HANDLE SOUND LOADING
				int s;
				for(s=0;s<gview.supermovie_sound.str.size();s++)
					{
					string filename="data/sound/";
					filename+=gview.supermovie_sound.str[s]->dat;
					filename+=".ogg";
					
					musicsound.set_sound(filename,s);
					}
#endif
				}

			if(!fail)
				{
				//READ IT
				f.read((char *)&gview.currentblocksize,sizeof(int));
				gview.currentblocksize=byteswap(gview.currentblocksize);
				f.read((char *)gview.supermoviebuffer_comp,gview.currentblocksize);

				gview.nextfilepos+=gview.currentblocksize+sizeof(int);

				//UNCOMPRESS IT
				z_stream d_stream; // decompression stream

				d_stream.zalloc = (alloc_func)0;
				d_stream.zfree = (free_func)0;
				d_stream.opaque = (voidpf)0;

				d_stream.next_in  = (Bytef*)gview.supermoviebuffer_comp;
				d_stream.avail_in = gview.currentblocksize;

				int err = inflateInit(&d_stream);
				CHECK_ERR(err, "inflateInit");

				d_stream.next_out = gview.supermoviebuffer;
				d_stream.avail_out = MOVIEBUFFSIZE;

				while (d_stream.total_out < MOVIEBUFFSIZE && d_stream.total_in < gview.currentblocksize) {
					//d_stream.avail_in = d_stream.avail_out = 1; // force small buffers
					err = inflate(&d_stream, Z_NO_FLUSH);
					if (err == Z_STREAM_END) break;
					CHECK_ERR(err, "inflate");
					}

				err = inflateEnd(&d_stream);
				CHECK_ERR(err, "inflateEnd");

				gview.supermovie_pos=0;
				maxmoviepos=d_stream.total_out;
				}
			else
				{
				is_playing=0;
                                enabler.release_grid_size();
                                if (gview.original_fps)
                                  enabler.set_fps(gview.original_fps);
				}
			}
		else
			{
			is_playing=0;
                        enabler.release_grid_size();
                        if (gview.original_fps)
                          enabler.set_fps(gview.original_fps);
			}

		f.close();
		}
	else
		{
		is_playing=0;
                enabler.release_grid_size();
                if (gview.original_fps)
                  enabler.set_fps(gview.original_fps);
		}
}

void interfacest::handlemovie(char flushall)
{
	//SAVE A MOVIE FRAME INTO THE CURRENT MOVIE BUFFER
	if(supermovie_on==1)
		{
		if(supermovie_delaystep>0&&!flushall)supermovie_delaystep--;
		else
			{
			if(!flushall)supermovie_delaystep=supermovie_delayrate;

			if(!flushall||supermovie_delaystep==0)
				{
				//SAVING CHARACTERS, THEN COLORS
				short x2,y2;
				for(x2=0;x2<init.display.grid_x;x2++)
					{
					for(y2=0;y2<init.display.grid_y;y2++)
						{
						supermoviebuffer[supermovie_pos]=gps.screen[x2*gps.dimy*4 + y2*4 + 0];

						supermovie_pos++;
						}
					}
				char frame_col;
				for(x2=0;x2<init.display.grid_x;x2++)
					{
					for(y2=0;y2<init.display.grid_y;y2++)
						{
						frame_col=gps.screen[x2*gps.dimy*4 + y2*4 + 1];
						frame_col|=(gps.screen[x2*gps.dimy*4 + y2*4 + 2]<<3);
						if(gps.screen[x2*gps.dimy*4 + y2*4 + 3])frame_col|=64;
						supermoviebuffer[supermovie_pos]=frame_col;

						supermovie_pos++;
						}
					}
				}

			int frame_size=init.display.grid_x*init.display.grid_y*2;
			if(supermovie_pos+frame_size>=MOVIEBUFFSIZE||flushall)
				{
				int length=write_movie_chunk();

				if(length>5000000)
					{
					finish_movie();
					}
				else supermovie_pos=0;
				}
			}
		}
}

void interfacest::print_interface_token(InterfaceKey key,justification just)
{
	short o_screenf=gps.screenf,o_screenb=gps.screenb,o_screenbright=gps.screenbright;
	gps.changecolor(2,0,1);
        string tok = enabler.GetKeyDisplay(key);
	gps.addst(tok,just);
	gps.changecolor(o_screenf,o_screenb,o_screenbright);
}

char standardstringentry(char *str,int maxlen,unsigned int flag,std::set<InterfaceKey> &events)
{
	string str2;
	str2=str;
	char ret=standardstringentry(str2,maxlen,flag,events);
	strcpy(str,str2.c_str());
	return ret;
}

char standardstringentry(string &str,int maxlen,unsigned int flag,std::set<InterfaceKey> &events)
{
	unsigned char entry=255;
	if(flag & STRINGENTRY_LETTERS)
		{
		if(events.count(INTERFACEKEY_STRING_A097))entry='a';
		if(events.count(INTERFACEKEY_STRING_A098))entry='b';
		if(events.count(INTERFACEKEY_STRING_A099))entry='c';
		if(events.count(INTERFACEKEY_STRING_A100))entry='d';
		if(events.count(INTERFACEKEY_STRING_A101))entry='e';
		if(events.count(INTERFACEKEY_STRING_A102))entry='f';
		if(events.count(INTERFACEKEY_STRING_A103))entry='g';
		if(events.count(INTERFACEKEY_STRING_A104))entry='h';
		if(events.count(INTERFACEKEY_STRING_A105))entry='i';
		if(events.count(INTERFACEKEY_STRING_A106))entry='j';
		if(events.count(INTERFACEKEY_STRING_A107))entry='k';
		if(events.count(INTERFACEKEY_STRING_A108))entry='l';
		if(events.count(INTERFACEKEY_STRING_A109))entry='m';
		if(events.count(INTERFACEKEY_STRING_A110))entry='n';
		if(events.count(INTERFACEKEY_STRING_A111))entry='o';
		if(events.count(INTERFACEKEY_STRING_A112))entry='p';
		if(events.count(INTERFACEKEY_STRING_A113))entry='q';
		if(events.count(INTERFACEKEY_STRING_A114))entry='r';
		if(events.count(INTERFACEKEY_STRING_A115))entry='s';
		if(events.count(INTERFACEKEY_STRING_A116))entry='t';
		if(events.count(INTERFACEKEY_STRING_A117))entry='u';
		if(events.count(INTERFACEKEY_STRING_A118))entry='v';
		if(events.count(INTERFACEKEY_STRING_A119))entry='w';
		if(events.count(INTERFACEKEY_STRING_A120))entry='x';
		if(events.count(INTERFACEKEY_STRING_A121))entry='y';
		if(events.count(INTERFACEKEY_STRING_A122))entry='z';
		if(events.count(INTERFACEKEY_STRING_A065))entry='A';
		if(events.count(INTERFACEKEY_STRING_A066))entry='B';
		if(events.count(INTERFACEKEY_STRING_A067))entry='C';
		if(events.count(INTERFACEKEY_STRING_A068))entry='D';
		if(events.count(INTERFACEKEY_STRING_A069))entry='E';
		if(events.count(INTERFACEKEY_STRING_A070))entry='F';
		if(events.count(INTERFACEKEY_STRING_A071))entry='G';
		if(events.count(INTERFACEKEY_STRING_A072))entry='H';
		if(events.count(INTERFACEKEY_STRING_A073))entry='I';
		if(events.count(INTERFACEKEY_STRING_A074))entry='J';
		if(events.count(INTERFACEKEY_STRING_A075))entry='K';
		if(events.count(INTERFACEKEY_STRING_A076))entry='L';
		if(events.count(INTERFACEKEY_STRING_A077))entry='M';
		if(events.count(INTERFACEKEY_STRING_A078))entry='N';
		if(events.count(INTERFACEKEY_STRING_A079))entry='O';
		if(events.count(INTERFACEKEY_STRING_A080))entry='P';
		if(events.count(INTERFACEKEY_STRING_A081))entry='Q';
		if(events.count(INTERFACEKEY_STRING_A082))entry='R';
		if(events.count(INTERFACEKEY_STRING_A083))entry='S';
		if(events.count(INTERFACEKEY_STRING_A084))entry='T';
		if(events.count(INTERFACEKEY_STRING_A085))entry='U';
		if(events.count(INTERFACEKEY_STRING_A086))entry='V';
		if(events.count(INTERFACEKEY_STRING_A087))entry='W';
		if(events.count(INTERFACEKEY_STRING_A088))entry='X';
		if(events.count(INTERFACEKEY_STRING_A089))entry='Y';
		if(events.count(INTERFACEKEY_STRING_A090))entry='Z';
		}
	if(flag & STRINGENTRY_SPACE)
		{
		if(events.count(INTERFACEKEY_STRING_A032))entry=' ';
		}
	if(events.count(INTERFACEKEY_STRING_A000))entry='\x0';
	if(flag & STRINGENTRY_NUMBERS)
		{
		if(events.count(INTERFACEKEY_STRING_A048))entry='0';
		if(events.count(INTERFACEKEY_STRING_A049))entry='1';
		if(events.count(INTERFACEKEY_STRING_A050))entry='2';
		if(events.count(INTERFACEKEY_STRING_A051))entry='3';
		if(events.count(INTERFACEKEY_STRING_A052))entry='4';
		if(events.count(INTERFACEKEY_STRING_A053))entry='5';
		if(events.count(INTERFACEKEY_STRING_A054))entry='6';
		if(events.count(INTERFACEKEY_STRING_A055))entry='7';
		if(events.count(INTERFACEKEY_STRING_A056))entry='8';
		if(events.count(INTERFACEKEY_STRING_A057))entry='9';
		}
	if(flag & STRINGENTRY_SYMBOLS)
		{
		if(events.count(INTERFACEKEY_STRING_A000))entry=0;
		if(events.count(INTERFACEKEY_STRING_A001))entry=1;
		if(events.count(INTERFACEKEY_STRING_A002))entry=2;
		if(events.count(INTERFACEKEY_STRING_A003))entry=3;
		if(events.count(INTERFACEKEY_STRING_A004))entry=4;
		if(events.count(INTERFACEKEY_STRING_A005))entry=5;
		if(events.count(INTERFACEKEY_STRING_A006))entry=6;
		if(events.count(INTERFACEKEY_STRING_A007))entry=7;
		if(events.count(INTERFACEKEY_STRING_A008))entry=8;
		if(events.count(INTERFACEKEY_STRING_A009))entry=9;
		if(events.count(INTERFACEKEY_STRING_A010))entry=10;
		if(events.count(INTERFACEKEY_STRING_A011))entry=11;
		if(events.count(INTERFACEKEY_STRING_A012))entry=12;
		if(events.count(INTERFACEKEY_STRING_A013))entry=13;
		if(events.count(INTERFACEKEY_STRING_A014))entry=14;
		if(events.count(INTERFACEKEY_STRING_A015))entry=15;
		if(events.count(INTERFACEKEY_STRING_A016))entry=16;
		if(events.count(INTERFACEKEY_STRING_A017))entry=17;
		if(events.count(INTERFACEKEY_STRING_A018))entry=18;
		if(events.count(INTERFACEKEY_STRING_A019))entry=19;
		if(events.count(INTERFACEKEY_STRING_A020))entry=20;
		if(events.count(INTERFACEKEY_STRING_A021))entry=21;
		if(events.count(INTERFACEKEY_STRING_A022))entry=22;
		if(events.count(INTERFACEKEY_STRING_A023))entry=23;
		if(events.count(INTERFACEKEY_STRING_A024))entry=24;
		if(events.count(INTERFACEKEY_STRING_A025))entry=25;
		if(events.count(INTERFACEKEY_STRING_A026))entry=26;
		if(events.count(INTERFACEKEY_STRING_A027))entry=27;
		if(events.count(INTERFACEKEY_STRING_A028))entry=28;
		if(events.count(INTERFACEKEY_STRING_A029))entry=29;
		if(events.count(INTERFACEKEY_STRING_A030))entry=30;
		if(events.count(INTERFACEKEY_STRING_A031))entry=31;
		if(events.count(INTERFACEKEY_STRING_A032))entry=32;
		if(events.count(INTERFACEKEY_STRING_A033))entry=33;
		if(events.count(INTERFACEKEY_STRING_A034))entry=34;
		if(events.count(INTERFACEKEY_STRING_A035))entry=35;
		if(events.count(INTERFACEKEY_STRING_A036))entry=36;
		if(events.count(INTERFACEKEY_STRING_A037))entry=37;
		if(events.count(INTERFACEKEY_STRING_A038))entry=38;
		if(events.count(INTERFACEKEY_STRING_A039))entry=39;
		if(events.count(INTERFACEKEY_STRING_A040))entry=40;
		if(events.count(INTERFACEKEY_STRING_A041))entry=41;
		if(events.count(INTERFACEKEY_STRING_A042))entry=42;
		if(events.count(INTERFACEKEY_STRING_A043))entry=43;
		if(events.count(INTERFACEKEY_STRING_A044))entry=44;
		if(events.count(INTERFACEKEY_STRING_A045))entry=45;
		if(events.count(INTERFACEKEY_STRING_A046))entry=46;
		if(events.count(INTERFACEKEY_STRING_A047))entry=47;
		if(events.count(INTERFACEKEY_STRING_A048))entry=48;
		if(events.count(INTERFACEKEY_STRING_A049))entry=49;
		if(events.count(INTERFACEKEY_STRING_A050))entry=50;
		if(events.count(INTERFACEKEY_STRING_A051))entry=51;
		if(events.count(INTERFACEKEY_STRING_A052))entry=52;
		if(events.count(INTERFACEKEY_STRING_A053))entry=53;
		if(events.count(INTERFACEKEY_STRING_A054))entry=54;
		if(events.count(INTERFACEKEY_STRING_A055))entry=55;
		if(events.count(INTERFACEKEY_STRING_A056))entry=56;
		if(events.count(INTERFACEKEY_STRING_A057))entry=57;
		if(events.count(INTERFACEKEY_STRING_A058))entry=58;
		if(events.count(INTERFACEKEY_STRING_A059))entry=59;
		if(events.count(INTERFACEKEY_STRING_A060))entry=60;
		if(events.count(INTERFACEKEY_STRING_A061))entry=61;
		if(events.count(INTERFACEKEY_STRING_A062))entry=62;
		if(events.count(INTERFACEKEY_STRING_A063))entry=63;
		if(events.count(INTERFACEKEY_STRING_A064))entry=64;
		if(events.count(INTERFACEKEY_STRING_A065))entry=65;
		if(events.count(INTERFACEKEY_STRING_A066))entry=66;
		if(events.count(INTERFACEKEY_STRING_A067))entry=67;
		if(events.count(INTERFACEKEY_STRING_A068))entry=68;
		if(events.count(INTERFACEKEY_STRING_A069))entry=69;
		if(events.count(INTERFACEKEY_STRING_A070))entry=70;
		if(events.count(INTERFACEKEY_STRING_A071))entry=71;
		if(events.count(INTERFACEKEY_STRING_A072))entry=72;
		if(events.count(INTERFACEKEY_STRING_A073))entry=73;
		if(events.count(INTERFACEKEY_STRING_A074))entry=74;
		if(events.count(INTERFACEKEY_STRING_A075))entry=75;
		if(events.count(INTERFACEKEY_STRING_A076))entry=76;
		if(events.count(INTERFACEKEY_STRING_A077))entry=77;
		if(events.count(INTERFACEKEY_STRING_A078))entry=78;
		if(events.count(INTERFACEKEY_STRING_A079))entry=79;
		if(events.count(INTERFACEKEY_STRING_A080))entry=80;
		if(events.count(INTERFACEKEY_STRING_A081))entry=81;
		if(events.count(INTERFACEKEY_STRING_A082))entry=82;
		if(events.count(INTERFACEKEY_STRING_A083))entry=83;
		if(events.count(INTERFACEKEY_STRING_A084))entry=84;
		if(events.count(INTERFACEKEY_STRING_A085))entry=85;
		if(events.count(INTERFACEKEY_STRING_A086))entry=86;
		if(events.count(INTERFACEKEY_STRING_A087))entry=87;
		if(events.count(INTERFACEKEY_STRING_A088))entry=88;
		if(events.count(INTERFACEKEY_STRING_A089))entry=89;
		if(events.count(INTERFACEKEY_STRING_A090))entry=90;
		if(events.count(INTERFACEKEY_STRING_A091))entry=91;
		if(events.count(INTERFACEKEY_STRING_A092))entry=92;
		if(events.count(INTERFACEKEY_STRING_A093))entry=93;
		if(events.count(INTERFACEKEY_STRING_A094))entry=94;
		if(events.count(INTERFACEKEY_STRING_A095))entry=95;
		if(events.count(INTERFACEKEY_STRING_A096))entry=96;
		if(events.count(INTERFACEKEY_STRING_A097))entry=97;
		if(events.count(INTERFACEKEY_STRING_A098))entry=98;
		if(events.count(INTERFACEKEY_STRING_A099))entry=99;
		if(events.count(INTERFACEKEY_STRING_A100))entry=100;
		if(events.count(INTERFACEKEY_STRING_A101))entry=101;
		if(events.count(INTERFACEKEY_STRING_A102))entry=102;
		if(events.count(INTERFACEKEY_STRING_A103))entry=103;
		if(events.count(INTERFACEKEY_STRING_A104))entry=104;
		if(events.count(INTERFACEKEY_STRING_A105))entry=105;
		if(events.count(INTERFACEKEY_STRING_A106))entry=106;
		if(events.count(INTERFACEKEY_STRING_A107))entry=107;
		if(events.count(INTERFACEKEY_STRING_A108))entry=108;
		if(events.count(INTERFACEKEY_STRING_A109))entry=109;
		if(events.count(INTERFACEKEY_STRING_A110))entry=110;
		if(events.count(INTERFACEKEY_STRING_A111))entry=111;
		if(events.count(INTERFACEKEY_STRING_A112))entry=112;
		if(events.count(INTERFACEKEY_STRING_A113))entry=113;
		if(events.count(INTERFACEKEY_STRING_A114))entry=114;
		if(events.count(INTERFACEKEY_STRING_A115))entry=115;
		if(events.count(INTERFACEKEY_STRING_A116))entry=116;
		if(events.count(INTERFACEKEY_STRING_A117))entry=117;
		if(events.count(INTERFACEKEY_STRING_A118))entry=118;
		if(events.count(INTERFACEKEY_STRING_A119))entry=119;
		if(events.count(INTERFACEKEY_STRING_A120))entry=120;
		if(events.count(INTERFACEKEY_STRING_A121))entry=121;
		if(events.count(INTERFACEKEY_STRING_A122))entry=122;
		if(events.count(INTERFACEKEY_STRING_A123))entry=123;
		if(events.count(INTERFACEKEY_STRING_A124))entry=124;
		if(events.count(INTERFACEKEY_STRING_A125))entry=125;
		if(events.count(INTERFACEKEY_STRING_A126))entry=126;
		if(events.count(INTERFACEKEY_STRING_A128))entry=128;
		if(events.count(INTERFACEKEY_STRING_A129))entry=129;
		if(events.count(INTERFACEKEY_STRING_A130))entry=130;
		if(events.count(INTERFACEKEY_STRING_A131))entry=131;
		if(events.count(INTERFACEKEY_STRING_A132))entry=132;
		if(events.count(INTERFACEKEY_STRING_A133))entry=133;
		if(events.count(INTERFACEKEY_STRING_A134))entry=134;
		if(events.count(INTERFACEKEY_STRING_A135))entry=135;
		if(events.count(INTERFACEKEY_STRING_A136))entry=136;
		if(events.count(INTERFACEKEY_STRING_A137))entry=137;
		if(events.count(INTERFACEKEY_STRING_A138))entry=138;
		if(events.count(INTERFACEKEY_STRING_A139))entry=139;
		if(events.count(INTERFACEKEY_STRING_A140))entry=140;
		if(events.count(INTERFACEKEY_STRING_A141))entry=141;
		if(events.count(INTERFACEKEY_STRING_A142))entry=142;
		if(events.count(INTERFACEKEY_STRING_A143))entry=143;
		if(events.count(INTERFACEKEY_STRING_A144))entry=144;
		if(events.count(INTERFACEKEY_STRING_A145))entry=145;
		if(events.count(INTERFACEKEY_STRING_A146))entry=146;
		if(events.count(INTERFACEKEY_STRING_A147))entry=147;
		if(events.count(INTERFACEKEY_STRING_A148))entry=148;
		if(events.count(INTERFACEKEY_STRING_A149))entry=149;
		if(events.count(INTERFACEKEY_STRING_A150))entry=150;
		if(events.count(INTERFACEKEY_STRING_A151))entry=151;
		if(events.count(INTERFACEKEY_STRING_A152))entry=152;
		if(events.count(INTERFACEKEY_STRING_A153))entry=153;
		if(events.count(INTERFACEKEY_STRING_A154))entry=154;
		if(events.count(INTERFACEKEY_STRING_A155))entry=155;
		if(events.count(INTERFACEKEY_STRING_A156))entry=156;
		if(events.count(INTERFACEKEY_STRING_A157))entry=157;
		if(events.count(INTERFACEKEY_STRING_A158))entry=158;
		if(events.count(INTERFACEKEY_STRING_A159))entry=159;
		if(events.count(INTERFACEKEY_STRING_A160))entry=160;
		if(events.count(INTERFACEKEY_STRING_A161))entry=161;
		if(events.count(INTERFACEKEY_STRING_A162))entry=162;
		if(events.count(INTERFACEKEY_STRING_A163))entry=163;
		if(events.count(INTERFACEKEY_STRING_A164))entry=164;
		if(events.count(INTERFACEKEY_STRING_A165))entry=165;
		if(events.count(INTERFACEKEY_STRING_A166))entry=166;
		if(events.count(INTERFACEKEY_STRING_A167))entry=167;
		if(events.count(INTERFACEKEY_STRING_A168))entry=168;
		if(events.count(INTERFACEKEY_STRING_A169))entry=169;
		if(events.count(INTERFACEKEY_STRING_A170))entry=170;
		if(events.count(INTERFACEKEY_STRING_A171))entry=171;
		if(events.count(INTERFACEKEY_STRING_A172))entry=172;
		if(events.count(INTERFACEKEY_STRING_A173))entry=173;
		if(events.count(INTERFACEKEY_STRING_A174))entry=174;
		if(events.count(INTERFACEKEY_STRING_A175))entry=175;
		if(events.count(INTERFACEKEY_STRING_A176))entry=176;
		if(events.count(INTERFACEKEY_STRING_A177))entry=177;
		if(events.count(INTERFACEKEY_STRING_A178))entry=178;
		if(events.count(INTERFACEKEY_STRING_A179))entry=179;
		if(events.count(INTERFACEKEY_STRING_A180))entry=180;
		if(events.count(INTERFACEKEY_STRING_A181))entry=181;
		if(events.count(INTERFACEKEY_STRING_A182))entry=182;
		if(events.count(INTERFACEKEY_STRING_A183))entry=183;
		if(events.count(INTERFACEKEY_STRING_A184))entry=184;
		if(events.count(INTERFACEKEY_STRING_A185))entry=185;
		if(events.count(INTERFACEKEY_STRING_A186))entry=186;
		if(events.count(INTERFACEKEY_STRING_A187))entry=187;
		if(events.count(INTERFACEKEY_STRING_A188))entry=188;
		if(events.count(INTERFACEKEY_STRING_A189))entry=189;
		if(events.count(INTERFACEKEY_STRING_A190))entry=190;
		if(events.count(INTERFACEKEY_STRING_A191))entry=191;
		if(events.count(INTERFACEKEY_STRING_A192))entry=192;
		if(events.count(INTERFACEKEY_STRING_A193))entry=193;
		if(events.count(INTERFACEKEY_STRING_A194))entry=194;
		if(events.count(INTERFACEKEY_STRING_A195))entry=195;
		if(events.count(INTERFACEKEY_STRING_A196))entry=196;
		if(events.count(INTERFACEKEY_STRING_A197))entry=197;
		if(events.count(INTERFACEKEY_STRING_A198))entry=198;
		if(events.count(INTERFACEKEY_STRING_A199))entry=199;
		if(events.count(INTERFACEKEY_STRING_A200))entry=200;
		if(events.count(INTERFACEKEY_STRING_A201))entry=201;
		if(events.count(INTERFACEKEY_STRING_A202))entry=202;
		if(events.count(INTERFACEKEY_STRING_A203))entry=203;
		if(events.count(INTERFACEKEY_STRING_A204))entry=204;
		if(events.count(INTERFACEKEY_STRING_A205))entry=205;
		if(events.count(INTERFACEKEY_STRING_A206))entry=206;
		if(events.count(INTERFACEKEY_STRING_A207))entry=207;
		if(events.count(INTERFACEKEY_STRING_A208))entry=208;
		if(events.count(INTERFACEKEY_STRING_A209))entry=209;
		if(events.count(INTERFACEKEY_STRING_A210))entry=210;
		if(events.count(INTERFACEKEY_STRING_A211))entry=211;
		if(events.count(INTERFACEKEY_STRING_A212))entry=212;
		if(events.count(INTERFACEKEY_STRING_A213))entry=213;
		if(events.count(INTERFACEKEY_STRING_A214))entry=214;
		if(events.count(INTERFACEKEY_STRING_A215))entry=215;
		if(events.count(INTERFACEKEY_STRING_A216))entry=216;
		if(events.count(INTERFACEKEY_STRING_A217))entry=217;
		if(events.count(INTERFACEKEY_STRING_A218))entry=218;
		if(events.count(INTERFACEKEY_STRING_A219))entry=219;
		if(events.count(INTERFACEKEY_STRING_A220))entry=220;
		if(events.count(INTERFACEKEY_STRING_A221))entry=221;
		if(events.count(INTERFACEKEY_STRING_A222))entry=222;
		if(events.count(INTERFACEKEY_STRING_A223))entry=223;
		if(events.count(INTERFACEKEY_STRING_A224))entry=224;
		if(events.count(INTERFACEKEY_STRING_A225))entry=225;
		if(events.count(INTERFACEKEY_STRING_A226))entry=226;
		if(events.count(INTERFACEKEY_STRING_A227))entry=227;
		if(events.count(INTERFACEKEY_STRING_A228))entry=228;
		if(events.count(INTERFACEKEY_STRING_A229))entry=229;
		if(events.count(INTERFACEKEY_STRING_A230))entry=230;
		if(events.count(INTERFACEKEY_STRING_A231))entry=231;
		if(events.count(INTERFACEKEY_STRING_A232))entry=232;
		if(events.count(INTERFACEKEY_STRING_A233))entry=233;
		if(events.count(INTERFACEKEY_STRING_A234))entry=234;
		if(events.count(INTERFACEKEY_STRING_A235))entry=235;
		if(events.count(INTERFACEKEY_STRING_A236))entry=236;
		if(events.count(INTERFACEKEY_STRING_A237))entry=237;
		if(events.count(INTERFACEKEY_STRING_A238))entry=238;
		if(events.count(INTERFACEKEY_STRING_A239))entry=239;
		if(events.count(INTERFACEKEY_STRING_A240))entry=240;
		if(events.count(INTERFACEKEY_STRING_A241))entry=241;
		if(events.count(INTERFACEKEY_STRING_A242))entry=242;
		if(events.count(INTERFACEKEY_STRING_A243))entry=243;
		if(events.count(INTERFACEKEY_STRING_A244))entry=244;
		if(events.count(INTERFACEKEY_STRING_A245))entry=245;
		if(events.count(INTERFACEKEY_STRING_A246))entry=246;
		if(events.count(INTERFACEKEY_STRING_A247))entry=247;
		if(events.count(INTERFACEKEY_STRING_A248))entry=248;
		if(events.count(INTERFACEKEY_STRING_A249))entry=249;
		if(events.count(INTERFACEKEY_STRING_A250))entry=250;
		if(events.count(INTERFACEKEY_STRING_A251))entry=251;
		if(events.count(INTERFACEKEY_STRING_A252))entry=252;
		if(events.count(INTERFACEKEY_STRING_A253))entry=253;
		if(events.count(INTERFACEKEY_STRING_A254))entry=254;
		if(events.count(INTERFACEKEY_STRING_A255))entry=255;
		}

	if(entry!=255)
		{
		if(entry=='\x0')
			{
			if(str.length()>0)str.resize(str.length()-1);
			}
		else
			{
			int cursor=str.length();
			if(cursor>=maxlen)cursor=maxlen-1;
			if(cursor<0)cursor=0;

			if(str.length()<cursor+1)str.resize(cursor+1);

			if(entry>='a'&&entry<='z'&&(flag & STRINGENTRY_CAPS))str[cursor]=entry+'A'-'a';
			else str[cursor]=entry;
			}

		events.clear();

		return 1;
		}

	return 0;
}

//To Do
//get the gview.c references inside the DEBUG_MOVIE defines
//make scrolling and stringentry use newer pressed functions for better speed
