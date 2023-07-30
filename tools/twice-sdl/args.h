#ifndef TWICE_ARGS_H
#define TWICE_ARGS_H

#include <string>
#include <unordered_map>
#include <vector>

namespace twice {

struct option {
	std::string long_opt;
	char short_opt;
	bool has_arg;
};

struct parsed_option {
	int count;
	std::string arg;
};

class arg_parser {
	static const std::vector<option> options;
	std::unordered_map<std::string, parsed_option> parsed_options;
	std::vector<std::string> parsed_args;

	void add_parsed_option(
			const std::string& name, const std::string& arg);
	int parse_long_opt(int i, int argc, char **argv);
	int parse_short_opts(int i, int num, int argc, char **argv);

      public:
	int parse_args(int argc, char **argv);
	std::string get_arg(std::size_t i);
	std::size_t num_args();
	const parsed_option *get_option(const std::string& name);
};

} // namespace twice

#endif
