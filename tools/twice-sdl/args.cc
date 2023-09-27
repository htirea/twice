#include "args.h"

#include <algorithm>
#include <iostream>

namespace twice {

void
arg_parser::add_parsed_option(const std::string& name, const std::string& arg)
{
	auto it = parsed_options.find(name);
	if (it == parsed_options.end()) {
		parsed_options.insert({ name, { 1, arg } });
	} else {
		it->second.count++;
		it->second.arg = arg;
	}
}

int
arg_parser::parse_long_opt(int i, int argc, char **argv)
{
	std::string name = argv[i] + 2;
	auto it = std::find_if(options.begin(), options.end(),
			[&](const auto& opt) { return opt.long_opt == name; });
	if (it == options.end()) {
		std::cerr << "unknown option --" << name << '\n';
		return -1;
	}

	std::string arg;
	if (it->has_arg) {
		if (i + 1 >= argc) {
			std::cerr << "option --" << name
				  << " requires an argument\n";
			return -1;
		}
		arg = argv[i + 1];
		if (invalid_option_arg(name, arg)) {
			return -1;
		}
		i++;
	}
	add_parsed_option(name, arg);

	return i + 1;
}

int
arg_parser::parse_short_opts(int i, int num, int argc, char **argv)
{
	char name = argv[i][num + 1];
	auto it = std::find_if(
			options.begin(), options.end(), [&](const auto& opt) {
				return opt.short_opt == name;
			});
	if (it == options.end()) {
		std::cerr << "unknown option -" << name << '\n';
		return -1;
	}

	std::string arg;
	if (it->has_arg) {
		arg = argv[i] + num + 2;
		if (arg.empty()) {
			if (i + 1 >= argc) {
				std::cerr << "option -" << name
					  << " requires an argument\n";
				return -1;
			}
			arg = argv[i + 1];
			if (invalid_option_arg(it->long_opt, arg)) {
				return -1;
			}
			i++;
		} else if (invalid_option_arg(it->long_opt, arg)) {
			return -1;
		}
		add_parsed_option(it->long_opt, arg);
		return i + 1;
	}
	add_parsed_option(it->long_opt, arg);

	return argv[i][num + 2] == '\0'
	                       ? i + 1
	                       : parse_short_opts(i, num + 1, argc, argv);
}

bool
arg_parser::invalid_option_arg(const std::string& name, const std::string& arg)
{
	auto it = valid_option_args.find(name);
	if (it == valid_option_args.end()) {
		return false;
	}

	if (it->second.find(arg) == it->second.end()) {
		std::cerr << "argument to option --" << name
			  << " must be one of: \n";
		for (const auto& x : it->second) {
			std::cerr << '\t' << x << '\n';
		}
		return true;
	}

	return false;
}

int
arg_parser::parse_args(int argc, char **argv)
{
	bool force_arg = false;
	for (int i = 1; i < argc;) {
		std::string arg = argv[i];

		if (arg.empty()) {
			i++;
		} else if (force_arg) {
			parsed_args.push_back(arg);
			i++;
		} else if (arg == "--") {
			force_arg = true;
			i++;
		} else if (arg == "-") {
			parsed_args.push_back(arg);
			i++;
		} else if (arg.length() >= 3 && arg[0] == '-' &&
				arg[1] == '-') {
			i = parse_long_opt(i, argc, argv);
			if (i < 0) {
				return 1;
			}
		} else if (arg.length() >= 2 && arg[0] == '-') {
			i = parse_short_opts(i, 0, argc, argv);
			if (i < 0) {
				return 1;
			}
		} else {
			parsed_args.push_back(arg);
			i++;
		}
	}

	return 0;
}

std::string
arg_parser::get_arg(std::size_t i)
{
	return parsed_args[i];
}

std::size_t
arg_parser::num_args()
{
	return parsed_args.size();
}

const parsed_option *
arg_parser::get_option(const std::string& name)
{
	auto it = parsed_options.find(name);
	if (it == parsed_options.end()) {
		return nullptr;
	} else {
		return &it->second;
	}
}

} // namespace twice
