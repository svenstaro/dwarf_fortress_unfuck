#include "platform.h"
#include <cerrno>
#include <string>
#include <cstring>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <map>
#include <set>
#include <stdio.h>
//#include <unistd.h>

extern "C" {
#include <zlib.h>
#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <signal.h>
#endif
}
#include "svector.h"

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

#include "random.h"

using std::string;

#include "basics.h"
#include "endian.h"
#include "files.h"
#include "enabler.h"
#include "find_files.h"

inline void CHECK_ERR(int err, const char* msg)
{
	if (err != Z_OK)
		{
		MessageBox(NULL, "One of the compressed files on disk has errors in it.  Restore from backup if you are able.", 0, 0);
		exit(1);
		}
}

using std::fstream;

char file_compressorst::def_ibuff[FILE_IN_BUFF];
char file_compressorst::def_obuff[FILE_OUT_BUFF];

char file_compressorst::load_posnull_pointer()
{
	int8_t dummy;
	read_file(dummy);

	if(!dummy)return 0;
	else return 1;
}

char file_compressorst::save_posnull_pointer(void *ptr)
{
	int8_t dummy;

	if(ptr==NULL)
		{
		dummy=0;
		write_file(dummy);
		return 0;
		}
	else
		{
		dummy=1;
		write_file(dummy);
		return 1;
		}
}

char file_compressorst::write_file(string &str)
{
	int16_t ln=(int16_t)str.length();
	if(ln>=10000||ln<0)ln=0;

	if(!write_file(ln))return 0;

	if(ln==0)return 1;

	if(!write_file((void *)str.c_str(),sizeof(char)*ln))return 0;

	return 1;
}

char file_compressorst::read_file(string &str)
{
	str.erase();

	int16_t ln;

	if(!read_file(ln))return 0;

	if(ln==0)return 1;

	char *strar=new char[ln+1];

	if(!read_file(strar,sizeof(char)*ln))
		{
		delete[] strar;
		return 0;
		}
	strar[ln]='\x0';

	str=strar;

	delete[] strar;
	return 1;
}

char file_compressorst::write_file(void *write_var,long write_size)
{
	if (!f.is_open())return 0;

	if(!compressed)
		{
		f.write((char *)write_var,write_size);
		return 1;
		}

	//WRITE OUT THE VARIABLE CHUNK BY CHUNK
	while(write_size>0)
		{
		//FLUSH THE BUFFER IF NECESSARY
		if(in_buffer_amount_loaded>=in_buffersize)
			{
			if(!flush_in_buffer())return 0;
			}

		//SET THE AMOUNT TO COPY
		long copy_size=in_buffersize-in_buffer_amount_loaded;
		if(write_size<copy_size)copy_size=write_size;

		//COPY THE NEXT CHUNK INTO THE BUFFER
		memmove(in_buffer+in_buffer_amount_loaded,write_var,copy_size);

		write_var=((char *)write_var) + copy_size;
		write_size-=copy_size;
		in_buffer_amount_loaded+=copy_size;
		}

	return 1;
}

char file_compressorst::flush_in_buffer()
{
	if (!f.is_open())return 0;
	if(!compressed)return 1;
	if(in_buffer_amount_loaded==0)return 1;//EXTRA CALLS TO FLUSH SHOULDN'T KILL YOU

	//TARN: adapted from zlib example files

	//SET UP THE COMPRESSOR
	z_stream c_stream;
	int err;

	c_stream.zalloc = (alloc_func)0;
	c_stream.zfree = (free_func)0;
	c_stream.opaque = (voidpf)0;

	err = deflateInit(&c_stream, 9);
	CHECK_ERR(err, "deflateInit");

	c_stream.next_out = (Bytef*)out_buffer;
	c_stream.avail_out = out_buffersize;

	//SEND IT TO THE COMPRESSOR
	c_stream.next_in = (Bytef*)in_buffer;
	c_stream.avail_in = in_buffer_amount_loaded;

	while (c_stream.total_in != in_buffer_amount_loaded && c_stream.total_out < out_buffersize)
		{
		err = deflate(&c_stream, Z_NO_FLUSH);
		CHECK_ERR(err, "deflate");
		}

	//FINISH UP THE STREAM
	while(1)
		{
		err = deflate(&c_stream, Z_FINISH);
		if (err == Z_STREAM_END) break;
		CHECK_ERR(err, "deflate");
		}

	err = deflateEnd(&c_stream);
	CHECK_ERR(err, "deflateEnd");

	//SAVE THE COMPRESSED BIT AS A GIANT BLOCK
	if(c_stream.total_out>0)
		{
		int32_t compsize=byteswap((int32_t)(c_stream.total_out));

		//WRITE IT
		f.write((char*)&compsize,sizeof(int32_t));
		f.write(out_buffer,c_stream.total_out);
		}

	in_buffer_position=0;
	in_buffer_amount_loaded=0;

	return 1;
}

char file_compressorst::read_file(void *read_var,long read_size)
{
	if (!f.is_open())return 0;

	if(!compressed)
		{
		f.read((char *)read_var,read_size);
		return 1;
		}

	//NOW LOAD INTO read_var UNTIL DONE
	while(read_size>0)
		{
		//GET A BUFFER IF NECESSARY
		if(in_buffer_amount_loaded==0||
			in_buffer_position>=in_buffer_amount_loaded)
			{
			if(!load_new_in_buffer())return 0;
			}

		//BAIL IF STILL NO BUFFER LEFT
		if(in_buffer_amount_loaded==0)return 0;

		//SET THE AMOUNT TO COPY
		long copy_size=in_buffer_amount_loaded-in_buffer_position;
		if(read_size<copy_size)copy_size=read_size;

		//COPY
		memmove(read_var,in_buffer+in_buffer_position,copy_size);

		read_var=((char *)read_var) + copy_size;
		read_size-=copy_size;
		in_buffer_position+=copy_size;
		}
		
	return 1;
}

char file_compressorst::load_new_in_buffer()
{
	if (!f.is_open())return 0;
	if(!compressed)return 1;

	in_buffer_position=0;
	in_buffer_amount_loaded=0;

	//LOAD THE BLOCK OF COMPRESSED DATA
	f.read((char*)&out_buffer_amount_written,sizeof(int32_t));
	out_buffer_amount_written=byteswap(out_buffer_amount_written);
	f.read(out_buffer,out_buffer_amount_written);

	int err;

	if(out_buffer_amount_written>0)
		{
		//TARN: adapted from zlib example files

		//UNCOMPRESS
		z_stream d_stream; // decompression stream

		d_stream.zalloc = (alloc_func)0;
		d_stream.zfree = (free_func)0;
		d_stream.opaque = (voidpf)0;

		d_stream.next_in  = (Bytef*)out_buffer;
		d_stream.avail_in = out_buffer_amount_written;

		err = inflateInit(&d_stream);
		CHECK_ERR(err, "inflateInit");

		d_stream.next_out = (Bytef*)in_buffer;
		d_stream.avail_out = in_buffersize;

		while (d_stream.total_out < in_buffersize && d_stream.total_in < out_buffer_amount_written)
			{
			//d_stream.avail_in = d_stream.avail_out = 1; // force small buffers
			err = inflate(&d_stream, Z_NO_FLUSH);

			if (err == Z_STREAM_END) break;
			CHECK_ERR(err, "inflate");
			}

		err = inflateEnd(&d_stream);
		CHECK_ERR(err, "inflateEnd");

		in_buffer_amount_loaded=d_stream.total_out;

		return 1;
		}
	else return 0;
}

void file_compressorst::close_file()
{
	if (f.is_open())
		{
		f.close();
		}
}

char file_compressorst::open_file(const string &filename,char existing_only)
{
	if(filename.empty())return 0;

	//RESET DATA
	in_buffer_amount_loaded=0;
	in_buffer_position=0;
	out_buffer_amount_written=0;

/*
	//CHECK IF FILE ALREADY EXISTS
	f.open(filename.c_str(), fstream::in);
	char file_exists = f.is_open();
	if (file_exists)f.close();

	//OPEN FILE
	if(!existing_only || file_exists)
		f.open(filename.c_str(), fstream::in | fstream::out | fstream::binary);
	else
		f.open(filename.c_str(), fstream::in | fstream::out | fstream::binary);
*/	
	//if(existing_only)h=CreateFile(filename.c_str(),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	//else h=CreateFile(filename.c_str(),GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	if(f.is_open())f.close();
	f.clear();

	if(existing_only)
		f.open(filename.c_str(), fstream::in | fstream::out | fstream::binary);
	else
		f.open(filename.c_str(), fstream::in | fstream::out | fstream::binary | fstream::trunc);

	if (f.is_open())return 1;
	else return 0;
}

file_compressorst::file_compressorst(char *new_in_buffer,long new_in_buffersize,
									 char *new_out_buffer,long new_out_buffersize)
{
	compressed=true;

	in_buffer=new_in_buffer;
	in_buffersize=new_in_buffersize;
	in_buffer_amount_loaded=0;
	in_buffer_position=0;

	out_buffer=new_out_buffer;
	out_buffersize=new_out_buffersize;
	out_buffer_amount_written=0;
	
	f.clear();
}

void file_compressorst::set_buffer_info(char *new_in_buffer,long new_in_buffersize,
										 char *new_out_buffer,long new_out_buffersize)
{
	in_buffer=new_in_buffer;
	in_buffersize=new_in_buffersize;
	in_buffer_amount_loaded=0;
	in_buffer_position=0;

	out_buffer=new_out_buffer;
	out_buffersize=new_out_buffersize;
	out_buffer_amount_written=0;
	
	f.clear();
}


file_compressorst::file_compressorst()
{
	compressed=true;

	in_buffer=def_ibuff;
	in_buffersize=FILE_IN_BUFF;
	in_buffer_amount_loaded=0;
	in_buffer_position=0;

	out_buffer=def_obuff;
	out_buffersize=FILE_OUT_BUFF;
	out_buffer_amount_written=0;
	
	f.clear();
}

void copy_file(const string &src,const string &dst)
{
	std::ifstream in_stream(src.c_str(),std::ios_base::binary);
	std::ofstream out_stream(dst.c_str(),std::ios_base::binary);
	if(in_stream.is_open()&&out_stream.is_open())
		{
		out_stream<<in_stream.rdbuf();
		}
	in_stream.close();
	out_stream.close();
}

void replace_file(const string &src, const string &dst) {
#ifdef WIN32
  remove(dst.c_str());
#endif
  rename(src.c_str(), dst.c_str());
}
