#include <string>
#include <vector>
#include <fstream>
#include <filesystem> 
#include <iostream> 
#include <algorithm>
#include <cstring>

#include "parsing.hpp"
#include "source_builder.hpp"
#include "code_fragments.hpp"

void clear_file(std::string const& file_name) {
	std::fstream fileout;
	fileout.open(file_name, std::ios::out);
	if(fileout.is_open()) {
		fileout << "";
		fileout.close();
	}
}

int main(int argc, char* argv[]) {
	if(argc > 1) {
		std::fstream input_file;
		std::string input_file_name(argv[1]);
		input_file.open(argv[1], std::ios::in);

		const std::string output_file_name = [otemp = std::string(argv[1])]() mutable {
			if(otemp.length() >= 4 && otemp[otemp.length() - 4] == '.') {
				otemp[otemp.length() - 3] = 'c';
				otemp[otemp.length() - 2] = 'p';
				otemp[otemp.length() - 1] = 'p';
				return otemp;
			}
			return otemp + ".cpp";
		}();

		error_record err(input_file_name);

		if(!input_file.is_open()) {
			err.add(row_col_pair{ 0,0 }, 1000, "Could not open input file");
			clear_file(output_file_name);
			std::cout << err.accumulated;
			return -1;
		}

		// PARSE FILE

		std::string file_data((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
		file_contents parsed_file = parse_file(file_data.c_str(), file_data.c_str() + file_data.length(), err);
		input_file.close();

		if(err.accumulated.length() > 0) {
			clear_file(output_file_name);
			std::cout << err.accumulated;
			return -1;
		}

		// PREPROCESS STAGE

		// EMIT RESULTS

		std::string output;
		basic_builder o;

		output += generate_includes(o).to_string(0);

		output += "namespace printui {\n";

		output += generate_class_definition(o, parsed_file).to_string(1);
		output += generate_function_declarations(o, parsed_file).to_string(1);
		output += generate_destructor(o, parsed_file).to_string(1);
		output += generate_core_window_functions(o, parsed_file).to_string(1);
		
		output += generate_create_persistent_resources(o, parsed_file).to_string(1);
		output += generate_create_device_resources(o, parsed_file).to_string(1);
		output += generate_create_sized_resources(o, parsed_file).to_string(1);
		output += generate_palette_functions(o, parsed_file).to_string(1);
		output += generate_win_proc(o, parsed_file).to_string(1);
		output += generate_dpi_functions(o, parsed_file).to_string(1);

		output += "}\n";
		//newline at end of file
		output += "\n";

		std::fstream fileout;
		fileout.open(output_file_name, std::ios::out);
		if(fileout.is_open()) {
			fileout << output;
			fileout.close();
		} else {
			err.add(row_col_pair{ 0,0 }, 1001, "Could not open output file");
			std::cout << err.accumulated;
			return -1;
		}
	}

	return 0;
}