#ifndef TEXTLINES_H
#define TEXTLINES_H

struct textlinesst
{
	stringvectst text;



	void load_raw_to_lines(char *filename);

	void clean_lines()
		{
		text.clean();
		}
	void copy_from(textlinesst &ot)
		{
		text.copy_from(ot.text);
		}
};

#endif