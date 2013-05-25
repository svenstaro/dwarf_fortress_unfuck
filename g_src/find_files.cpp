#include "../game_g.h"
#include "../game_extv.h"

void find_files_by_pattern(const char* pattern, svector<char *>& filenames)
{
	HANDLE h;
	WIN32_FIND_DATA finddata;
	char *c;

	h=FindFirstFile(pattern,&finddata);

	if(h!=INVALID_HANDLE_VALUE)
		{
		if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,".."))
			{
			if(!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
				c=new char[strlen(finddata.cFileName)+1];
					strcpy(c,finddata.cFileName);
				filenames.push_back(c);
				}
			}

		while(FindNextFile(h,&finddata))
			{
			if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,".."))
				{
				if(!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
					c=new char[strlen(finddata.cFileName)+1];
						strcpy(c,finddata.cFileName);
					filenames.push_back(c);
					}
				}
			}
		
		FindClose(h);
		}
}

void find_files_by_pattern_with_exception(const char* pattern, svector<char *>& filenames,const char *exception)
{
	HANDLE h;
	WIN32_FIND_DATA finddata;
	char *c;

	h=FindFirstFile(pattern,&finddata);

	if(h!=INVALID_HANDLE_VALUE)
		{
		if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,"..")&&strcmp(finddata.cFileName,exception))
			{
			if(!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
				c=new char[strlen(finddata.cFileName)+1];
					strcpy(c,finddata.cFileName);
				filenames.push_back(c);
				}
			}

		while(FindNextFile(h,&finddata))
			{
			if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,"..")&&strcmp(finddata.cFileName,exception))
				{
				if(!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
					c=new char[strlen(finddata.cFileName)+1];
						strcpy(c,finddata.cFileName);
					filenames.push_back(c);
					}
				}
			}
		
		FindClose(h);
		}
}

void find_files_by_pattern(const char* pattern, stringvectst &filenames)
{
	HANDLE h;
	WIN32_FIND_DATA finddata;

	h=FindFirstFile(pattern,&finddata);

	if(h!=INVALID_HANDLE_VALUE)
		{
		if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,".."))
			{
			if(!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))filenames.add_string(finddata.cFileName);
			}

		while(FindNextFile(h,&finddata))
			{
			if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,".."))
				{
				if(!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))filenames.add_string(finddata.cFileName);
				}
			}
		
		FindClose(h);
		}
}

void find_files_by_pattern_with_exception(const char* pattern, stringvectst &filenames,const char *exception)
{
	HANDLE h;
	WIN32_FIND_DATA finddata;

	h=FindFirstFile(pattern,&finddata);

	if(h!=INVALID_HANDLE_VALUE)
		{
		if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,"..")&&strcmp(finddata.cFileName,exception))
			{
			if(!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))filenames.add_string(finddata.cFileName);
			}

		while(FindNextFile(h,&finddata))
			{
			if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,"..")&&strcmp(finddata.cFileName,exception))
				{
				if(!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))filenames.add_string(finddata.cFileName);
				}
			}
		
		FindClose(h);
		}
}

void find_directories_by_pattern(const char* pattern, stringvectst &filenames)
{
	HANDLE h;
	WIN32_FIND_DATA finddata;

	h=FindFirstFile(pattern,&finddata);

	if(h!=INVALID_HANDLE_VALUE)
		{
		if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,".."))
			{
			if(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)filenames.add_string(finddata.cFileName);
			}

		while(FindNextFile(h,&finddata))
			{
			if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,".."))
				{
				if(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)filenames.add_string(finddata.cFileName);
				}
			}
		
		FindClose(h);
		}
}

void find_directories_by_pattern_with_exception(const char* pattern, stringvectst &filenames,const char *exception)
{
	HANDLE h;
	WIN32_FIND_DATA finddata;

	h=FindFirstFile(pattern,&finddata);

	if(h!=INVALID_HANDLE_VALUE)
		{
		if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,"..")&&strcmp(finddata.cFileName,exception))
			{
			if(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)filenames.add_string(finddata.cFileName);
			}

		while(FindNextFile(h,&finddata))
			{
			if(strcmp(finddata.cFileName,".")&&strcmp(finddata.cFileName,"..")&&strcmp(finddata.cFileName,exception))
				{
				if(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)filenames.add_string(finddata.cFileName);
				}
			}
		
		FindClose(h);
		}
}