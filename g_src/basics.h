#ifndef BASICS_H
#define BASICS_H

#ifndef WIN32
#include <stdint.h>
#endif
#include <string>
using std::string;

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

void gamelog_string(const string &str);
void errorlog_string(const string &str);
void errorlog_string(const char *ptr);

bool grab_token_string(string &dest,string &source,char compc=':');
bool grab_token_string(string &dest,string &source,int32_t &pos,char compc=':');
bool grab_token_string_pos(string &dest,string &source,int32_t pos,char compc=':');
void replace_token_string(string &token,string &str,int32_t pos,char compc,string &nw,char repc);
bool grab_token_list_as_string(string &dest,string &source,int32_t &pos,char compc=':');
bool grab_token_expression(string &dest,string &source,int32_t &pos,char compc=':');
char grab_variable_token(string &str,string &token,char sec_comp,int32_t &pos,int32_t i_pos);

int32_t convert_string_to_long(string &str);
uint32_t convert_string_to_ulong(string &str);
void add_long_to_string(int32_t n,string &str);
void convert_long_to_string(int32_t n,string &str);
void convert_ulong_to_string(uint32_t n,string &str);

void lower_case_string(string &str);
void upper_case_string(string &str);
void simplify_string(string &str);
void capitalize_string_words(string &str);
void capitalize_string_first_word(string &str);
void abbreviate_string(string &str,int32_t len);

void get_number(int32_t number,string &str);
void get_ordinal(int32_t number,string &str,bool shorten);

enum GameMode
{
	GAMEMODE_DWARF,
	GAMEMODE_ADVENTURE,
	GAMEMODENUM,
	GAMEMODE_NONE
};

enum GameType
{
	GAMETYPE_DWARF_MAIN,
	GAMETYPE_ADVENTURE_MAIN,
	GAMETYPE_VIEW_LEGENDS,
	GAMETYPE_DWARF_RECLAIM,
	GAMETYPE_DWARF_ARENA,
	GAMETYPE_ADVENTURE_ARENA,
	GAMETYPE_ADVENTURE_DUNGEON,
	GAMETYPE_DWARF_TUTORIAL,
	GAMETYPE_DWARF_UNRETIRE,
	GAMETYPE_ADVENTURE_WORLD_DEBUG,
	GAMETYPENUM,
	GAMETYPE_NONE
};

enum Song
{
	SONG_TITLE,
	SONG_GAME,
	SONGNUM
};


enum justification : unsigned char {
  justify_left, justify_center, justify_right,
    justify_cont,
    not_truetype
};

extern int charmap[256];

#endif
