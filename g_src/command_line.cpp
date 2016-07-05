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

#include "basics.h"

#include "command_line.h"

void command_linest::init(const string &str)
{
	original=str;

	//BUILD THE TOKEN LIST
	long pos=0;
	while(grab_arg(original,pos));

	//HANDLE EACH TOKEN
	long l;
	for(l=0;l<arg_vect.str.size();l++)
		{
		handle_arg(arg_vect.str[l]->dat);
		}
}

char command_linest::grab_arg(string &source,long &pos)
{
	string dest;

	while(pos<source.length())
		{
		//HIT A NEW ARGUMENT?  RETURN, OTHERWISE SKIP AND START UP
		if(source[pos]=='-')
			{
			if(dest.empty()){pos++;continue;}
			else
				{
				pos++;
				arg_vect.add_string(dest);
				return 1;
				}
			}

		dest+=source[pos];

		pos++;
		}

	if(!dest.empty())arg_vect.add_string(dest);
	return 0;
}

void command_linest::handle_arg(string &arg)
{
	long pos=0;
	string dest;

	grab_token_string_pos(dest,arg,pos,' ');
	pos+=(int32_t)dest.length();

	short arg_pos=0;
	if(dest=="gen")
		{
		//KEEP GOING FOR A NUMBER
		while(pos+1<arg.length())
			{
			dest.erase();
			pos++;
			auto s=arg.begin(),e=arg.end();
			s+=pos;
			bool quote=false;
			for(;s<e;++s)
				{
				if((*s)=='"')
					{
					if(quote)break;
					else quote=true;
					++pos;
					continue;
					}
				else if((*s)==' '&&!quote)break;
				dest+=(*s);
				}
			pos+=(int32_t)dest.length();


			if(!dest.empty())
				{
				if(arg_pos==0)gen_id=convert_string_to_long(dest);
				if(arg_pos==1)
					{
					if(dest!="RANDOM")
						{
						world_seed=convert_string_to_ulong(dest);
						use_seed=1;
						}
					}
				if(arg_pos==2)
					{
					if(dest!="NONE")
						{
						world_param=dest;
						use_param=1;
						}
					}

				arg_pos++;
				}
			}
		}
}