#ifndef TEXTURE_HANDLER_H
#define TEXTURE_HANDLER_H

#include "textlines.h"

struct tile_pagest
{
	string token;

	string filename;
	short tile_dim_x;
	short tile_dim_y;
	short page_dim_x;
	short page_dim_y;

	svector<int32_t> texpos;
	svector<int32_t> datapos;
	svector<int32_t> texpos_gs;
	svector<int32_t> datapos_gs;

	char loaded;



	tile_pagest()
		{
		loaded=0;
		}

	void load_graphics(string &graphics_dir);
};

class texture_handlerst
{
	public:
		svector<tile_pagest *> page;

		svector<int32_t> texpos;
		svector<int32_t> datapos;

		void clean();
		void adopt_new_lines(textlinesst &lines,string &graphics_dir);

		~texture_handlerst()
			{
			clean();
			}

		tile_pagest *get_tile_page_by_token(string &tk)
			{
			int32_t t;
			for(t=0;t<page.size();t++)
				{
				if(page[t]->token==tk)return page[t];
				}
			return NULL;
			}
};

#endif
