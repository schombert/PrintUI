#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <limits>
#include <optional>
#include <variant>

struct char_range {
	char const* start;
	char const* end;

	std::string to_string() const {
		return std::string(start, end);
	}
};

struct parsed_item {
	char_range key;
	std::vector<char_range> values;
	char const* terminal;
};

struct row_col_pair {
	int32_t row;
	int32_t column;
};

struct error_record {
	std::string accumulated;
	std::string file_name;

	error_record(std::string const& fn) : file_name(fn) {}

	void add(row_col_pair const& rc, int32_t code, std::string const& s) {
		//<origin>[(position)]: [category] <kind> <code>: <description>

		accumulated += file_name;
		if(rc.row > 0) {
			if(rc.column > 0) {
				accumulated += "(" + std::to_string(rc.row) + "," + std::to_string(rc.column) + ")";
			} else {
				accumulated += "(" + std::to_string(rc.row) + ")";
			}
		} else {

		}
		accumulated += ": error ";
		accumulated += std::to_string(code);
		accumulated += ": ";
		accumulated += s;
		accumulated += "\n";
	}
};

char const* advance_to_closing_bracket(char const* pos, char const* end);
char const* advance_to_non_whitespace(char const* pos, char const* end);
char const* advance_to_whitespace(char const* pos, char const* end);
char const* advance_to_whitespace_or_brace(char const* pos, char const* end);
char const* reverse_to_non_whitespace(char const* start, char const* pos);

row_col_pair calculate_line_from_position(char const* start, char const* pos);
parsed_item extract_item(char const* input, char const* end, char const* global_start, error_record& err);



enum class font_type { roman, sans, script, italic };
inline char const* to_string(font_type i) {
	switch(i) {
		case font_type::roman: return "font_type::roman";
		case font_type::sans: return "font_type::sans";
		case font_type::script: return "font_type::script";
		case font_type::italic: return "font_type::italic";
		default: return "";
	}
}

enum class font_span { normal, condensed, wide };
inline char const* to_string(font_span i) {
	switch(i) {
		case font_span::normal: return "font_span::normal";
		case font_span::condensed: return "font_span::condensed";
		case font_span::wide: return "font_span::wide";
		default: return "";
	}
}

inline char const* to_font_weight(bool is_bold) {
	if(is_bold)
		return "DWRITE_FONT_WEIGHT_BOLD";
	else
		return "DWRITE_FONT_WEIGHT_NORMAL";
}
inline char const* to_font_style(bool is_italic) {
	if(is_italic)
		return "1.0f";
	else
		return "0.0f";
}
inline char const* to_font_span(font_span i) {
	switch(i) {
		case font_span::normal: return "100.0f";
		case font_span::condensed: return "75.0f";
		case font_span::wide: return "125.0f";
		default: return "";
	}
}

struct ui_selector {
	std::string name;
	bool capital_letters = true;
};

struct unicode_range {
	uint32_t start = 0;
	uint32_t end = 0;
};

struct font_fallback {
	std::string name;
	bool is_bold = false;
	bool is_oblique = false;
	font_span span = font_span::normal;
	float scale = 1.0f;
	std::vector<unicode_range> ranges;
};

struct font_description {
	std::string name;
	font_type type = font_type::roman;
	bool is_bold = false;
	bool is_oblique = false;
	font_span span = font_span::normal;

	std::vector<font_fallback> fallbacks;
};

struct brush {
	std::string texture;
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	bool is_light_color = true;
};

enum class size_specifier {max, flex, automatic};

struct size_description {
	std::variant<uint32_t, size_specifier> min_size = size_specifier::automatic;
	std::variant<uint32_t, size_specifier> max_size = size_specifier::automatic;;
};

struct data_description {
	std::string name;
	std::string type;
};

struct text_description {
	//todo
};

struct column_count_range {
	uint32_t min = 1;
	uint32_t max = 1;
};

struct content_window {
	size_description x_size;
	size_description y_size;
};

enum class justification{ leading, trailing, centered };

struct column {
	column_count_range count;
	size_description size;
	justification line_justification = justification::leading;
	justification page_justification = justification::centered;
	std::optional< text_description> header;

	// std::vector<???> decorations
	// std::vector<???> contents
};

enum class page_usage {permanent, transient, persistent};
enum class orientation {page_orientation, line_orientation};

struct page {
	std::string name;
	size_description size;
	page_usage usage = page_usage::permanent;
	orientation directionality = orientation::line_orientation;

	std::optional<data_description> identifying_data;
	std::optional<text_description> display_name;
	std::vector<data_description> data_members;
	std::vector<uint32_t> palette;
	std::vector<column> columns;
};

struct file_contents {
	ui_selector selector_description;
	font_description primary_font;
	std::optional<font_description> small_font;
	std::vector<std::pair<std::string, font_description>> custom_fonts;

	std::vector<brush> palette;

	std::vector<page> pages;

	std::string font_directory;
	std::string texture_directory;
	std::string icon_directory;

	std::string heading;
	std::string footer;
	std::string line_lead;
	std::string line_end;
	std::string name  = "printui application";
	content_window content;

	int32_t layout_base_size = 20;
	int32_t line_width = 16;
	int32_t small_width = 8;
	int32_t window_border = 2;
};

file_contents parse_file(char const* start, char const* end, error_record& err_out);
ui_selector parse_ui_selector(char const* start, char const* end, char const* global_start, error_record& err_out);
font_fallback parse_font_fallback(char const* start, char const* end, char const* global_start, error_record& err_out);
std::pair<std::string, font_description> parse_font_description(char const* start, char const* end, char const* global_start, error_record& err_out);
brush parse_brush(char const* start, char const* end, char const* global_start, error_record& err_out);
std::vector<brush> parse_palette(char const* start, char const* end, char const* global_start, error_record& err_out);
content_window parse_content_window(char const* start, char const* end, char const* global_start, error_record& err_out);
page parse_page(char const* start, char const* end, char const* global_start, error_record& err_out);
std::variant<uint32_t, size_specifier> parse_size(std::string const& str, char const* start, char const* global_start, error_record& err_out);
std::vector<uint32_t> parse_sub_palette(char const* start, char const* end, char const* global_start, error_record& err_out);
column parse_column(char const* start, char const* end, char const* global_start, error_record& err_out);
text_description parse_text_description(char const* start, char const* end, char const* global_start, error_record& err_out);