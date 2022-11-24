#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include <string>
#include "..\display_testbed\printui_utility.hpp"


TEST_CASE("match conditions", "[parsing_tests]") {
	printui::text::text_data_storage tm;
	auto res = tm.parse_match_conditions("{1.other 2.masc} stuff");

	REQUIRE(res[0] == 's');
	REQUIRE(tm.match_keys_storage.size() == 2);
	REQUIRE(tm.attribute_name_to_id.size() == 1);
	REQUIRE(tm.last_attribute_id_mapped == printui::text::last_predefined + 1);
	REQUIRE(tm.attribute_name_to_id["masc"] == printui::text::last_predefined + 1);
	REQUIRE(tm.match_keys_storage[0].attribute == printui::text::other);
	REQUIRE(tm.match_keys_storage[0].parameter_id == 0);
	REQUIRE(tm.match_keys_storage[1].attribute == printui::text::last_predefined + 1);
	REQUIRE(tm.match_keys_storage[1].parameter_id == 1);
}

TEST_CASE("attributes", "[parsing_tests]") {
	printui::text::text_data_storage tm;
	auto res = tm.parse_attributes("other masc");

	REQUIRE(res[0] == printui::text::other);
	REQUIRE(res[1] == printui::text::last_predefined + 1);
	REQUIRE(res[2] == -1ui16);
	REQUIRE(tm.attribute_name_to_id.size() == 1);
	REQUIRE(tm.last_attribute_id_mapped == printui::text::last_predefined + 1);
	REQUIRE(tm.attribute_name_to_id["masc"] == printui::text::last_predefined + 1);
}

TEST_CASE("simple entry", "[parsing_tests]") {
	std::unordered_map<std::string, uint32_t, printui::text::string_hash, std::equal_to<>> font_name_to_index;
	printui::text::text_data_storage tm;

	auto res = tm.consume_single_entry("first_entry { content } second_entry { 2222 }", font_name_to_index);

	REQUIRE(res[0] == 's');
	REQUIRE(tm.internal_text_name_map[std::string("first_entry")] == 0);
	REQUIRE(tm.stored_functions[0].pattern_count == 1);
	REQUIRE(tm.stored_functions[0].begin_patterns == 0);
	REQUIRE(tm.stored_functions[0].attributes[0] == -1i16);
	REQUIRE(tm.static_matcher_storage.size() == 1);
	REQUIRE(tm.static_matcher_storage[0].num_keys == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_end == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_count == 7);
	REQUIRE(tm.codepoint_storage.substr(0, 7) == L"content");
}

TEST_CASE("entry with attributes", "[parsing_tests]") {
	std::unordered_map<std::string, uint32_t, printui::text::string_hash, std::equal_to<>> font_name_to_index;
	printui::text::text_data_storage tm;

	auto res = tm.consume_single_entry("first_entry {other masc}{ content } second_entry { 2222 }", font_name_to_index);

	REQUIRE(res[0] == 's');
	REQUIRE(tm.internal_text_name_map[std::string("first_entry")] == 0);
	REQUIRE(tm.stored_functions[0].pattern_count == 1);
	REQUIRE(tm.stored_functions[0].begin_patterns == 0);
	REQUIRE(tm.static_matcher_storage.size() == 1);
	REQUIRE(tm.static_matcher_storage[0].num_keys == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_end == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_count == 7);
	REQUIRE(tm.codepoint_storage.substr(0, 7) == L"content");

	REQUIRE(tm.stored_functions[0].attributes[0] == printui::text::other);
	REQUIRE(tm.stored_functions[0].attributes[1] == printui::text::last_predefined + 1);
	REQUIRE(tm.stored_functions[0].attributes[2] == -1ui16);
	REQUIRE(tm.attribute_name_to_id.size() == 1);
	REQUIRE(tm.last_attribute_id_mapped == printui::text::last_predefined + 1);
	REQUIRE(tm.attribute_name_to_id["masc"] == printui::text::last_predefined + 1);
}

TEST_CASE("entry with spaces", "[parsing_tests]") {
	std::unordered_map<std::string, uint32_t, printui::text::string_hash, std::equal_to<>> font_name_to_index;
	printui::text::text_data_storage tm;

	auto res = tm.consume_single_entry("first_entry {other masc} {\n\tcon    te\r\nn_t } second_entry { 2222 }", font_name_to_index);

	REQUIRE(res[0] == 's');
	REQUIRE(tm.internal_text_name_map[std::string("first_entry")] == 0);
	REQUIRE(tm.stored_functions[0].pattern_count == 1);
	REQUIRE(tm.stored_functions[0].begin_patterns == 0);
	REQUIRE(tm.static_matcher_storage.size() == 1);
	REQUIRE(tm.static_matcher_storage[0].num_keys == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_end == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_count == 9);
	REQUIRE(tm.codepoint_storage.substr(0, 9) == L"con te\nnt");

	REQUIRE(tm.stored_functions[0].attributes[0] == printui::text::other);
	REQUIRE(tm.stored_functions[0].attributes[1] == printui::text::last_predefined + 1);
	REQUIRE(tm.stored_functions[0].attributes[2] == -1ui16);
	REQUIRE(tm.attribute_name_to_id.size() == 1);
	REQUIRE(tm.last_attribute_id_mapped == printui::text::last_predefined + 1);
	REQUIRE(tm.attribute_name_to_id["masc"] == printui::text::last_predefined + 1);
}

TEST_CASE("entry with italics", "[parsing_tests]") {
	std::unordered_map<std::string, uint32_t, printui::text::string_hash, std::equal_to<>> font_name_to_index;
	printui::text::text_data_storage tm;

	auto res = tm.consume_single_entry("first_entry { con_\\it{ te }_nt } second_entry { 2222 }", font_name_to_index);

	REQUIRE(res[0] == 's');
	REQUIRE(tm.internal_text_name_map[std::string("first_entry")] == 0);
	REQUIRE(tm.stored_functions[0].pattern_count == 1);
	REQUIRE(tm.stored_functions[0].begin_patterns == 0);
	REQUIRE(tm.stored_functions[0].attributes[0] == -1i16);
	REQUIRE(tm.static_matcher_storage.size() == 1);
	REQUIRE(tm.static_matcher_storage[0].num_keys == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_end == 2);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_count == 7);
	REQUIRE(tm.codepoint_storage.substr(0, 7) == L"content");
	REQUIRE(tm.static_format_storage.size() == 2);
	REQUIRE(tm.static_format_storage[0].position == 3);
	REQUIRE(tm.static_format_storage[1].position == 5);
	REQUIRE(std::holds_alternative<printui::text::extra_formatting>(tm.static_format_storage[0].format));
	REQUIRE(std::holds_alternative<printui::text::extra_formatting>(tm.static_format_storage[1].format));
	REQUIRE(std::get<printui::text::extra_formatting>(tm.static_format_storage[0].format) == printui::text::extra_formatting::italic);
	REQUIRE(std::get<printui::text::extra_formatting>(tm.static_format_storage[1].format) == printui::text::extra_formatting::italic);
}

TEST_CASE("entry with replacement", "[parsing_tests]") {
	std::unordered_map<std::string, uint32_t, printui::text::string_hash, std::equal_to<>> font_name_to_index;
	printui::text::text_data_storage tm;

	auto res = tm.consume_single_entry("first_entry { cont \\1 ent } second_entry { 2222 }", font_name_to_index);

	REQUIRE(res[0] == 's');
	REQUIRE(tm.internal_text_name_map[std::string("first_entry")] == 0);
	REQUIRE(tm.stored_functions[0].pattern_count == 1);
	REQUIRE(tm.stored_functions[0].begin_patterns == 0);
	REQUIRE(tm.stored_functions[0].attributes[0] == -1i16);
	REQUIRE(tm.static_matcher_storage.size() == 1);
	REQUIRE(tm.static_matcher_storage[0].num_keys == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_end == 1);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_count == 10);
	REQUIRE(tm.codepoint_storage.substr(0, 10) == L"cont ? ent");
	REQUIRE(tm.static_format_storage.size() == 1);
	REQUIRE(tm.static_format_storage[0].position == 5);
	REQUIRE(std::holds_alternative<printui::text::parameter_id>(tm.static_format_storage[0].format));
	REQUIRE(std::get<printui::text::parameter_id>(tm.static_format_storage[0].format).id == 0);
}

TEST_CASE("entry two matches", "[parsing_tests]") {
	std::unordered_map<std::string, uint32_t, printui::text::string_hash, std::equal_to<>> font_name_to_index;
	printui::text::text_data_storage tm;

	auto res = tm.consume_single_entry("first_entry { cont \\match{1.other}{ AAA }{}{ BBBB } ent } second_entry { 2222 }", font_name_to_index);

	REQUIRE(res[0] == 's');
	REQUIRE(tm.internal_text_name_map[std::string("first_entry")] == 0);
	REQUIRE(tm.stored_functions[0].pattern_count == 4);
	REQUIRE(tm.stored_functions[0].begin_patterns == 0);
	REQUIRE(tm.stored_functions[0].attributes[0] == -1i16);
	REQUIRE(tm.static_matcher_storage.size() == 4);
	REQUIRE(tm.static_matcher_storage[0].num_keys == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.formatting_end == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_start == 0);
	REQUIRE(tm.static_matcher_storage[0].base_text.code_points_count == 5);

	REQUIRE(tm.static_matcher_storage[1].num_keys == 1);
	REQUIRE(tm.static_matcher_storage[1].match_key_start == 0);
	REQUIRE(tm.static_matcher_storage[1].base_text.formatting_start == 0);
	REQUIRE(tm.static_matcher_storage[1].base_text.formatting_end == 0);
	REQUIRE(tm.static_matcher_storage[1].base_text.code_points_start == 5);
	REQUIRE(tm.static_matcher_storage[1].base_text.code_points_count == 3);

	REQUIRE(tm.static_matcher_storage[2].num_keys == 0);
	REQUIRE(tm.static_matcher_storage[2].base_text.formatting_start == 0);
	REQUIRE(tm.static_matcher_storage[2].base_text.formatting_end == 0);
	REQUIRE(tm.static_matcher_storage[2].base_text.code_points_start == 8);
	REQUIRE(tm.static_matcher_storage[2].base_text.code_points_count == 4);

	REQUIRE(tm.static_matcher_storage[2].group == tm.static_matcher_storage[1].group);
	REQUIRE(tm.static_matcher_storage[3].group != tm.static_matcher_storage[1].group);
	REQUIRE(tm.static_matcher_storage[0].group != tm.static_matcher_storage[1].group);

	REQUIRE(tm.static_matcher_storage[3].num_keys == 0);
	REQUIRE(tm.static_matcher_storage[3].base_text.formatting_start == 0);
	REQUIRE(tm.static_matcher_storage[3].base_text.formatting_end == 0);
	REQUIRE(tm.static_matcher_storage[3].base_text.code_points_start == 12);
	REQUIRE(tm.static_matcher_storage[3].base_text.code_points_count == 4);


	REQUIRE(tm.codepoint_storage.substr(0, 16) == L"cont AAABBBB ent");
}
