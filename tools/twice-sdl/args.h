#ifndef TWICE_ARGS_H
#define TWICE_ARGS_H

#include <set>
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
	using opt_list = const std::vector<option>;
	using valid_opt_arg_list = const std::unordered_map<std::string,
			std::set<std::string>>;

	static opt_list options;
	static valid_opt_arg_list valid_option_args;

	std::unordered_map<std::string, parsed_option> parsed_options;
	std::vector<std::string> parsed_args;

	void add_parsed_option(
			const std::string& name, const std::string& arg);
	int parse_long_opt(int i, int argc, char **argv);
	int parse_short_opts(int i, int num, int argc, char **argv);
	bool invalid_option_arg(
			const std::string& name, const std::string& arg);

      public:
	int parse_args(int argc, char **argv);
	std::string get_arg(std::size_t i);
	std::size_t num_args();
	const parsed_option *get_option(const std::string& name);
};

extern arg_parser parser;

} // namespace twice

#endif
