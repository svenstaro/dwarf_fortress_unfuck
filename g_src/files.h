//Copyright (c) 2006, Tarn Adams
//All rights reserved.  See game.cpp or license.txt for more information.
#ifndef FILES_H
#define FILES_H

#define FILE_IN_BUFF 1000000
#define FILE_OUT_BUFF 2000000

#include <string>
using std::string;

#include <fstream>
#include "endian.h"
#include "svector.h"

class file_compressorst
{
	public:
		bool compressed;
		std::fstream f;


		char open_file(const string &filename,char existing_only);
		void close_file();
		char write_file(string &str);
		char read_file(string &str);

		char load_posnull_pointer();
		char save_posnull_pointer(void *ptr);

		void write_file_fixedlength(char *var)
			{
			int16_t len=(int16_t)strlen(var);
			write_file(len);
			if(len>0)write_file(var,len*sizeof(char));
			}
		void read_file_fixedlength(char *var)
			{
			int16_t len;
			read_file(len);//DO NOT NEED TO ALLOCATE SPACE
			if(len>0)read_file(var,len*sizeof(char));
			var[len]='\x0';
			}
		char write_file(int32_t var)
			{
			var=byteswap(var);
			return write_file(&var,sizeof(int32_t));
			}
		char read_file(int32_t &var)
			{
			char ret = read_file(&var,sizeof(int32_t));
			var = byteswap(var);
			return ret;
			}
		char write_file(int16_t var)
			{
			var=byteswap(var);
			return write_file(&var,sizeof(int16_t));
			}
		char read_file(int16_t &var)
			{
			char ret = read_file(&var,sizeof(int16_t));
			var = byteswap(var);
			return ret;
			}
		char write_file(int8_t var)
			{
			return write_file(&var,sizeof(int8_t));
			}
		char read_file(int8_t &var)
			{
			return read_file(&var,sizeof(int8_t));
			}
		char write_file(bool var)
			{
			int8_t temp;
			if(var)temp=1;
			else temp=0;
			return write_file(&temp,sizeof(int8_t));
			}
		char read_file(bool &var)
			{
			int8_t temp;
			if(!read_file(&temp,sizeof(int8_t)))return 0;
			var=(temp!=0);
			return 1;
			}
		char write_file(uint32_t var)
			{
			var=byteswap(var);
			return write_file(&var,sizeof(uint32_t));
			}
		char read_file(uint32_t &var)
			{
			char ret = read_file(&var,sizeof(uint32_t));
			var = byteswap(var);
			return ret;
			}
		char write_file(uint16_t var)
			{
			var=byteswap(var);
			return write_file(&var,sizeof(uint16_t));
			}
		char read_file(uint16_t &var)
			{
			char ret = read_file(&var,sizeof(uint16_t));
			var = byteswap(var);
			return ret;
			}
		char write_file(uint8_t var)
			{
			return write_file(&var,sizeof(uint8_t));
			}
		char read_file(uint8_t &var)
			{
			return read_file(&var,sizeof(uint8_t));
			}
		void write_file(svector<bool> &vect)
			{
			int32_t s=(int32_t)vect.size();
			write_file(s);
			bool bl;//JUST FOR PARITY WITH read BELOW
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				bl=(*i_b);
				write_file(bl);
				}
			}
		void read_file(svector<bool> &vect)
			{
			int32_t s;
			read_file(s);
			vect.resize(s);
			bool bl;//NO IDEA WHY IT CAN'T JUST TAKE vect[i]
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				read_file(bl);
				(*i_b)=bl;
				}
			}
		void write_file(svector<int16_t> &vect)
			{
			int32_t s=(int32_t)vect.size();
			write_file(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				write_file((*i_b));
				}
			}
		void read_file(svector<int16_t> &vect)
			{
			int32_t s;
			read_file(s);
			vect.resize(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				read_file((*i_b));
				}
			}
		void write_file(svector<uint16_t> &vect)
			{
			int32_t s=(int32_t)vect.size();
			write_file(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				write_file((*i_b));
				}
			}
		void read_file(svector<uint16_t> &vect)
			{
			int32_t s;
			read_file(s);
			vect.resize(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				read_file((*i_b));
				}
			}
		void write_file(svector<uint8_t> &vect)
			{
			int32_t s=(int32_t)vect.size();
			write_file(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				write_file((*i_b));
				}
			}
		void read_file(svector<uint8_t> &vect)
			{
			int32_t s;
			read_file(s);
			vect.resize(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				read_file((*i_b));
				}
			}
		void write_file(svector<int8_t> &vect)
			{
			int32_t s=(int32_t)vect.size();
			write_file(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				write_file((*i_b));
				}
			}
		void read_file(svector<int8_t> &vect)
			{
			int32_t s;
			read_file(s);
			vect.resize(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				read_file((*i_b));
				}
			}
		void write_file(svector<int32_t> &vect)
			{
			int32_t s=(int32_t)vect.size();
			write_file(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				write_file((*i_b));
				}
			}
		void read_file(svector<int32_t> &vect)
			{
			int32_t s;
			read_file(s);
			vect.resize(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				read_file((*i_b));
				}
			}
		void write_file(svector<uint32_t> &vect)
			{
			int32_t s=(int32_t)vect.size();
			write_file(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				write_file((*i_b));
				}
			}
		void read_file(svector<uint32_t> &vect)
			{
			int32_t s;
			read_file(s);
			vect.resize(s);
			auto i_b=vect.begin(),i_e=vect.end();
			for(;i_b<i_e;++i_b)
				{
				read_file((*i_b));
				}
			}


		char load_new_in_buffer();
		char flush_in_buffer();

		file_compressorst();
		file_compressorst(char *new_in_buffer,long new_in_buffersize,
						char *new_out_buffer,long new_out_buffersize);
		~file_compressorst()
			{
			close_file();
			}
		void set_buffer_info(char *new_in_buffer,long new_in_buffersize,
						 char *new_out_buffer,long new_out_buffersize);

	private:
		char write_file(void *write_var,long write_size);
		char read_file(void *read_var,long read_size);

		char *in_buffer;
		long in_buffersize;
		long in_buffer_amount_loaded;
		long in_buffer_position;

		char *out_buffer;
		long out_buffersize;
		int32_t out_buffer_amount_written;

		static char def_ibuff[FILE_IN_BUFF];
		static char def_obuff[FILE_OUT_BUFF];
};

void copy_file(const string &src,const string &dst);
// Replaces dst with src, removing src in the process. Atomic if possible.
void replace_file(const string &src, const string &dst);
#endif
