class command_linest
{
	public:
		string original;
		stringvectst arg_vect;

		long gen_id;
		unsigned long world_seed;
		char use_seed;
		string world_param;
		char use_param;



		void init(const string &str);
		char grab_arg(string &source,long &pos);
		void handle_arg(string &arg);

		command_linest()
			{
			gen_id=-1;
			use_seed=0;
			use_param=0;
			}
};