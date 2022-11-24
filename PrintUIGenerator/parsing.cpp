#include "parsing.hpp"
#include <assert.h>
#include <string>
#include <charconv>

// LAST ERROR 78

char const * advance_to_closing_bracket(char const * pos, char const * end) {
	int32_t depth_count = 0;
	bool in_quote = false;

	while(pos < end) {
		if(*pos == '{' && !in_quote)
			++depth_count;
		else if(*pos == '}' && !in_quote)
			--depth_count;
		else if(*pos == '\"')
			in_quote = !in_quote;
		else if(*pos == '\\' && in_quote) {
			++pos;
		}
		if(depth_count < 0)
			return pos;
		++pos;
	}
	return pos;
}

char const * advance_to_closing_parenthesis(char const * pos, char const * end) {
	int32_t depth_count = 0;
	bool in_quote = false;

	while(pos < end) {
		if(*pos == '(' && !in_quote)
			++depth_count;
		else if(*pos == ')' && !in_quote)
			--depth_count;
		else if(*pos == '\"')
			in_quote = !in_quote;
		else if(*pos == '\\' && in_quote) {
			++pos;
		}
		if(depth_count < 0)
			return pos;
		++pos;
	}
	return pos;
}

char const * advance_to_closing_square_bracket(char const * pos, char const * end) {
	int32_t depth_count = 0;
	bool in_quote = false;

	while(pos < end) {
		if(*pos == '[' && !in_quote)
			++depth_count;
		else if(*pos == ']' && !in_quote)
			--depth_count;
		else if(*pos == '\"')
			in_quote = !in_quote;
		else if(*pos == '\\' && in_quote) {
			++pos;
		}
		if(depth_count < 0)
			return pos;
		++pos;
	}
	return pos;
}

bool is_ascii_ws(char v) {
	return v == ' ' || v == '\t' || v == '\n' || v == '\r';
}

char const * advance_to_non_whitespace(char const * pos, char const * end) {
	while(pos < end) {
		if(*pos != ' ' && *pos != '\t' && *pos != '\n' && *pos != '\r')
			return pos;
		++pos;
	}
	return pos;
}

char const * advance_to_non_whitespace_non_comma(char const * pos, char const * end) {
	while(pos < end) {
		if(*pos != ' ' && *pos != '\t' && *pos != '\n' && *pos != '\r' && *pos != ',')
			return pos;
		++pos;
	}
	return pos;
}

char const * advance_to_identifier_end(char const * pos, char const * end) {
	while(pos < end) {
		if(*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r'
			|| *pos == ',' || *pos == '(' || *pos == ')' || *pos == '[' || *pos == ']' || *pos == '{'
			|| *pos == '}' || *pos == ';' || *pos == '.' || *pos == '?' || *pos == '/' || *pos == '<'
			|| *pos == '>' || *pos == '\'' || *pos == '\"' || *pos == '|' || *pos == '\\'
			|| *pos == '=' || *pos == '+' || *pos == '-' || *pos == '~' || *pos == '!' || *pos == '#'
			|| *pos == '$' || *pos == '%' || *pos == '^' || *pos == '&' || *pos == '*')
			return pos;
		++pos;
	}
	return pos;
}


char const * advance_to_at(char const * pos, char const * end) {
	while(pos < end) {
		if(*pos == '@')
			return pos;
		++pos;
	}
	return pos;
}

std::string remove_ats(char const * pos, char const * end) {
	std::string result;

	while(pos < end) {
		if(*pos != '@')
			result += *pos;
		++pos;
	}

	return result;
}

char const * advance_to_whitespace(char const * pos, char const * end) {
	while(pos < end) {
		if(*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r')
			return pos;
		++pos;
	}
	return pos;
}

char const * advance_to_whitespace_or_brace(char const * pos, char const * end) {
	while(pos < end) {
		if(*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r' || *pos == '{')
			return pos;
		++pos;
	}
	return pos;
}

bool check_for_identifier(char const* identifier, char const * start, char const * end) {
	char const* pos = advance_to_non_whitespace_non_comma(start, end);
	char const* id_end = advance_to_identifier_end(pos, end);
	while(pos < id_end) {
		if(*identifier == 0)
			return pos == id_end;
		if(*identifier != *pos)
			return false;
		++identifier;
		++pos;
	}
	return *identifier == 0;
}

char const * reverse_to_non_whitespace(char const * start, char const * pos) {
	while(pos > start) {
		if(*(pos - 1) != ' ' && *(pos - 1) != '\t' && *(pos - 1) != '\n' && *(pos - 1) != '\r')
			return pos;
		--pos;
	}
	return pos;
}


row_col_pair calculate_line_from_position(char const * start, char const * pos) {
	row_col_pair result;
	result.row = 1;
	result.column = 1;

	char const* t = start;
	while(t < pos) {
		if(*t == '\n') {
			result.column = 0;
			++result.row;
		}
		++result.column;
		++t;
	}

	return result;
}

parsed_item extract_item(char const* input, char const* end, char const* global_start, error_record& err) {
	parsed_item result;

	result.key.start = advance_to_non_whitespace(input, end);
	result.key.end = advance_to_whitespace_or_brace(result.key.start, end);

	char const* position = advance_to_non_whitespace(result.key.end, end);
	result.terminal = position;

	while(position < end) {
		if(*position != '{')
			break;
		position = advance_to_non_whitespace(position + 1, end);
		char const* value_close = advance_to_closing_bracket(position, end);
		if(value_close == end) {
			err.add(calculate_line_from_position(global_start, position), 2, "Open bracket was not properly closed.");
		}
		result.values.push_back(char_range{ position, reverse_to_non_whitespace(position, value_close) });
		position = advance_to_non_whitespace(value_close + 1, end);
		result.terminal = std::min(end, position);
	}

	return result;
}

font_fallback parse_font_fallback(char const* start, char const* end, char const* global_start, error_record& err_out) {
	font_fallback result;

	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, global_start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "range") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 1,
						std::string("wrong number of parameters for \"range\""));
				} else {
					unicode_range r;
					r.start = std::stoi(extracted.values[0].to_string(), nullptr, 0);
					r.end = std::stoi(extracted.values[1].to_string(), nullptr, 0);
					result.ranges.push_back(r);
				}
			} else if(kstr == "name") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 2,
						std::string("wrong number of parameters for \"name\""));
				} else {
					result.name = extracted.values[0].to_string();
				}
			} else if(kstr == "scale") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 76,
						std::string("wrong number of parameters for \"scale\""));
				} else {
					std::from_chars(extracted.values[0].start, extracted.values[0].end, result.scale, std::chars_format::fixed);
				}
			} else if(kstr == "bold") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 3,
						std::string("wrong number of parameters for \"bold\""));
				} else {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.is_bold = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.is_bold = false;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 4,
							std::string("unknown parameter for \"bold\""));
				}
			} else if(kstr == "oblique") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 5,
						std::string("wrong number of parameters for \"oblique\""));
				} else {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.is_oblique = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.is_oblique = false;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 6,
							std::string("unknown parameter for \"oblique\""));
				}
			} else if(kstr == "span") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 7,
						std::string("wrong number of parameters for \"span\""));
				} else {
					const auto val = extracted.values[0].to_string();
					if(val == "normal" || val == "NORMAL")
						result.span = font_span::normal;
					else if(val == "condensed" || val == "CONDENSED")
						result.span = font_span::condensed;
					else if(val == "wide" || val == "WIDE")
						result.span = font_span::wide;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 8,
							std::string("unknown parameter for \"span\""));
				}
			} else {
				err_out.add(calculate_line_from_position(global_start, extracted.key.start), 9,
					std::string("unexpected token \"") + kstr + "\" while parsing font_fallback");
			}
		}
	}
	return result;
}

std::pair<std::string, font_description> parse_font_description(char const* start, char const* end, char const* global_start, error_record& err_out) {
	std::pair<std::string, font_description> result;

	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, global_start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "tag") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 10,
						std::string("wrong number of parameters for \"tag\""));
				} else {
					result.first = extracted.values[0].to_string();
				}
			} else if(kstr == "name") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 11,
						std::string("wrong number of parameters for \"name\""));
				} else {
					result.second.name = extracted.values[0].to_string();
				}
			} else if(kstr == "bold") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 12,
						std::string("wrong number of parameters for \"bold\""));
				} else {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.second.is_bold = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.second.is_bold = false;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 13,
							std::string("unknown parameter for \"bold\""));
				}
			} else if(kstr == "oblique") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 14,
						std::string("wrong number of parameters for \"oblique\""));
				} else {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.second.is_oblique = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.second.is_oblique = false;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 15,
							std::string("unknown parameter for \"oblique\""));
				}
			} else if(kstr == "span") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 16,
						std::string("wrong number of parameters for \"span\""));
				} else {
					const auto val = extracted.values[0].to_string();
					if(val == "normal" || val == "NORMAL")
						result.second.span = font_span::normal;
					else if(val == "condensed" || val == "CONDENSED")
						result.second.span = font_span::condensed;
					else if(val == "wide" || val == "WIDE")
						result.second.span = font_span::wide;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 17,
							std::string("unknown parameter for \"span\""));
				}
			} else if(kstr == "type") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 18,
						std::string("wrong number of parameters for \"type\""));
				} else {
					const auto val = extracted.values[0].to_string();
					if(val == "roman" || val == "ROMAN")
						result.second.type = font_type::roman;
					else if(val == "sans" || val == "SANS")
						result.second.type = font_type::sans;
					else if(val == "script" || val == "SCRIPT")
						result.second.type = font_type::script;
					else if(val == "italic" || val == "ITALIC")
						result.second.type = font_type::italic;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 19,
							std::string("unknown parameter for \"type\""));
				}
			} else if(kstr == "font_fallback") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 33,
						std::string("wrong number of parameters for \"font_fallback\""));
				} else {
					result.second.fallbacks.push_back(
						parse_font_fallback(extracted.values[0].start, extracted.values[0].end, global_start, err_out));
				}
			} else {
				err_out.add(calculate_line_from_position(global_start, extracted.key.start), 20,
					std::string("unexpected token \"") + kstr + "\" while parsing font_description");
			}
		}
	}
	return result;
}

ui_selector parse_ui_selector(char const * start, char const * end, char const * global_start, error_record & err_out) {
	ui_selector result;
	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, global_start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "icon") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 21,
						std::string("wrong number of parameters for \"icon\""));
				} else {
					result.name = extracted.values[0].to_string();
				}
			} else if(kstr == "capitals") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 22,
						std::string("wrong number of parameters for \"capitals\""));
				} else {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.capital_letters = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.capital_letters = false;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 23,
							std::string("unknown parameter for \"capitals\""));
				}
			} else {
				err_out.add(calculate_line_from_position(global_start, extracted.key.start), 24,
					std::string("unexpected token \"") + kstr + "\" while parsing ui_selector");
			}
		}
	}
	return result;
}

brush parse_brush(char const* start, char const* end, char const* global_start, error_record& err_out) {
	brush result;
	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, global_start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "color") {
				if(extracted.values.size() != 3) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 40,
						std::string("wrong number of parameters for \"color\""));
				} else {
					std::from_chars(extracted.values[0].start, extracted.values[0].end, result.r, std::chars_format::fixed);
					if(result.r > 1.0f) result.r /= 255.0f;

					std::from_chars(extracted.values[1].start, extracted.values[1].end, result.g, std::chars_format::fixed);
					if(result.g > 1.0f) result.g /= 255.0f;

					std::from_chars(extracted.values[2].start, extracted.values[2].end, result.b, std::chars_format::fixed);
					if(result.b > 1.0f) result.b /= 255.0f;
				}
			} else if(kstr == "is_light") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 41,
						std::string("wrong number of parameters for \"is_light\""));
				} else {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.is_light_color = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.is_light_color = false;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 42,
							std::string("unknown parameter for \"is_light\""));
				}
			} else if(kstr == "texture") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 43,
						std::string("wrong number of parameters for \"texture\""));
				} else {
					result.texture = extracted.values[0].to_string();
				}
			} else {
				err_out.add(calculate_line_from_position(global_start, extracted.key.start), 39,
					std::string("unexpected token \"") + kstr + "\" while parsing brush");
			}
		}
	}
	return result;
}

std::vector<brush> parse_palette(char const* start, char const* end, char const* global_start, error_record& err_out) {
	std::vector<brush> result;
	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, global_start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "brush") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 45,
						std::string("wrong number of parameters for \"brush\""));
				} else {
					result.push_back(
						parse_brush(extracted.values[0].start, extracted.values[0].end, global_start, err_out));
				}
			} else {
				err_out.add(calculate_line_from_position(global_start, extracted.key.start), 44,
					std::string("unexpected token \"") + kstr + "\" while parsing palette");
			}
		}
	}
	return result;
}

std::vector<uint32_t> parse_sub_palette(char const* start, char const* end, char const* global_start, error_record& err_out) {
	std::vector<uint32_t> result;
	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, global_start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "brush") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 67,
						std::string("wrong number of parameters for \"brush\""));
				} else {
					result.push_back(uint32_t(std::stoi(extracted.values[0].to_string())));
				}
			} else {
				err_out.add(calculate_line_from_position(global_start, extracted.key.start), 68,
					std::string("unexpected token \"") + kstr + "\" while parsing palette (in object)");
			}
		}
	}
	return result;
}

std::variant<uint32_t, size_specifier> parse_size(std::string const& str, char const* start, char const* global_start, error_record& err_out) {
	if(str == "max")
		return size_specifier::max;
	else if(str == "flex")
		return size_specifier::flex;
	else if(str == "auto")
		return size_specifier::automatic;
	else
		return uint32_t(std::stoi(str));
}

text_description parse_text_description(char const* start, char const* end, char const* global_start, error_record& err_out) {
	text_description result;
	return result;
}

column parse_column(char const* start, char const* end, char const* global_start, error_record& err_out) {
	column result;
	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, global_start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "size") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 68,
						std::string("wrong number of parameters for \"size\""));
				} else {
					result.size.min_size = parse_size(extracted.values[0].to_string(), extracted.values[0].start, global_start, err_out);
					result.size.max_size = parse_size(extracted.values[1].to_string(), extracted.values[1].start, global_start, err_out);
				}
			} else if(kstr == "count") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 69,
						std::string("wrong number of parameters for \"count\""));
				} else {
					result.count.min = uint32_t(std::stoi(extracted.values[0].to_string()));
					result.count.max = uint32_t(std::stoi(extracted.values[1].to_string()));
				}
			} else if(kstr == "page_justification") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 70,
						std::string("wrong number of parameters for \"page_justification\""));
				} else {
					auto val = extracted.values[0].to_string();
					if(val == "leading")
						result.page_justification = justification::leading;
					else if(val == "trailing")
						result.page_justification = justification::trailing;
					else if(val == "centered")
						result.page_justification = justification::centered;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 71,
							std::string("unknown justification type"));
				}
			} else if(kstr == "line_justification") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 72,
						std::string("wrong number of parameters for \"line_justification\""));
				} else {
					auto val = extracted.values[0].to_string();
					if(val == "leading")
						result.line_justification = justification::leading;
					else if(val == "trailing")
						result.line_justification = justification::trailing;
					else if(val == "centered")
						result.line_justification = justification::centered;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 71,
							std::string("unknown justification type"));
				}
			} else if(kstr == "header") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 73,
						std::string("wrong number of parameters for \"header\""));
				} else {
					result.header = parse_text_description(extracted.values[0].start, extracted.values[0].end, global_start, err_out);
				}
			} else if(kstr == "decoration") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 74,
						std::string("wrong number of parameters for \"decoration\""));
				} else {
					
				}
			} else if(kstr == "contents") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 75,
						std::string("wrong number of parameters for \"contents\""));
				} else {
					
				}
			} else {
				err_out.add(calculate_line_from_position(global_start, extracted.key.start), 67,
					std::string("unexpected token \"") + kstr + "\" while parsing column");
			}
		}
	}
	return result;
}

page parse_page(char const* start, char const* end, char const* global_start, error_record& err_out) {
	page result;
	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, global_start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "size") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 56,
						std::string("wrong number of parameters for \"size\""));
				} else {
					result.size.min_size = parse_size(extracted.values[0].to_string(), extracted.values[0].start, global_start, err_out);
					result.size.max_size = parse_size(extracted.values[1].to_string(), extracted.values[1].start, global_start, err_out);
				}
			} else if(kstr == "name") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 57,
						std::string("wrong number of parameters for \"name\""));
				} else {
					result.name = extracted.values[0].to_string();
				}
			} else if(kstr == "usage") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 58,
						std::string("wrong number of parameters for \"usage\""));
				} else {
					auto val = extracted.values[0].to_string();
					if(val == "permanent")
						result.usage = page_usage::permanent;
					else if(val == "persistent")
						result.usage = page_usage::persistent;
					else if(val == "transient")
						result.usage = page_usage::transient;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 59,
							std::string("unknown usage type"));
				}
			} else if(kstr == "orientation") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 60,
						std::string("wrong number of parameters for \"orientation\""));
				} else {
					auto val = extracted.values[0].to_string();
					if(val == "line")
						result.directionality = orientation::line_orientation;
					else if(val == "page")
						result.directionality = orientation::page_orientation;
					else
						err_out.add(calculate_line_from_position(global_start, extracted.key.start), 61,
							std::string("unknown orientation type"));
				}
			} else if(kstr == "identifier") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 62,
						std::string("wrong number of parameters for \"identifier\""));
				} else {
					result.identifying_data = data_description{ extracted.values[1].to_string(), extracted.values[0].to_string() };
				}
			} else if(kstr == "data") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 63,
						std::string("wrong number of parameters for \"data\""));
				} else {
					result.data_members.push_back(data_description{ extracted.values[1].to_string(), extracted.values[0].to_string() });
				}
			} else if(kstr == "display_name") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 64,
						std::string("wrong number of parameters for \"display_name\""));
				} else {
					result.display_name = parse_text_description(extracted.values[0].start, extracted.values[0].end, global_start, err_out);
				}
			} else if(kstr == "palette") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 65,
						std::string("wrong number of parameters for \"palette\""));
				} else {
					result.palette = parse_sub_palette(extracted.values[0].start, extracted.values[0].end, global_start, err_out);
				}
			} else if(kstr == "col" || kstr == "row") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 66,
						std::string("wrong number of parameters for \"col\"/\"row\""));
				} else {
					result.columns.push_back(parse_column(extracted.values[0].start, extracted.values[0].end, global_start, err_out));
				}
			} else {
				err_out.add(calculate_line_from_position(global_start, extracted.key.start), 55,
					std::string("unexpected token \"") + kstr + "\" while parsing page");
			}
		}
	}
	return result;
}

content_window parse_content_window(char const* start, char const* end, char const* global_start, error_record& err_out) {
	content_window result;
	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, global_start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "x") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 53,
						std::string("wrong number of parameters for \"x\""));
				} else {
					result.x_size.min_size = parse_size(extracted.values[0].to_string(), extracted.values[0].start, global_start, err_out);
					result.x_size.max_size = parse_size(extracted.values[1].to_string(), extracted.values[1].start, global_start, err_out);
				}
			} else if(kstr == "y") {
				if(extracted.values.size() != 2) {
					err_out.add(calculate_line_from_position(global_start, extracted.key.start), 53,
						std::string("wrong number of parameters for \"y\""));
				} else {
					result.y_size.min_size = parse_size(extracted.values[0].to_string(), extracted.values[0].start, global_start, err_out);
					result.y_size.max_size = parse_size(extracted.values[1].to_string(), extracted.values[1].start, global_start, err_out);
				}
			} else {
				err_out.add(calculate_line_from_position(global_start, extracted.key.start), 54,
					std::string("unexpected token \"") + kstr + "\" while parsing content_window");
			}
		}
	}
	return result;
}

file_contents parse_file(char const * start, char const * end, error_record & err_out) {
	file_contents parsed_file;

	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end, start, err_out);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr(extracted.key.start, extracted.key.end);
			if(kstr == "font_directory") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 25,
						std::string("wrong number of parameters for \"font_directory\""));
				} else {
					parsed_file.font_directory = extracted.values[0].to_string();
				}
			} else if(kstr == "texture_directory") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 26,
						std::string("wrong number of parameters for \"texture_directory\""));
				} else {
					parsed_file.texture_directory = extracted.values[0].to_string();
				}
			} else if(kstr == "icon_directory") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 27,
						std::string("wrong number of parameters for \"icon_directory\""));
				} else {
					parsed_file.icon_directory = extracted.values[0].to_string();
				}
			} else if(kstr == "app_name") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 77,
						std::string("wrong number of parameters for \"app_name\""));
				} else {
					parsed_file.name = extracted.values[0].to_string();
				}
			} else if(kstr == "layout_base_size") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 29,
						std::string("wrong number of parameters for \"layout_base_size\""));
				} else {
					parsed_file.layout_base_size = std::stoi(extracted.values[0].to_string());
				}
			} else if(kstr == "line_width") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 30,
						std::string("wrong number of parameters for \"line_width\""));
				} else {
					parsed_file.line_width = std::stoi(extracted.values[0].to_string());
				}
			} else if(kstr == "samll_line_width") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 21,
						std::string("wrong number of parameters for \"samll_line_width\""));
				} else {
					parsed_file.small_width = std::stoi(extracted.values[0].to_string());
				}
			} else if(kstr == "window_border") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 78,
						std::string("wrong number of parameters for \"window_border\""));
				} else {
					parsed_file.window_border = std::stoi(extracted.values[0].to_string());
				}
			} else if(kstr == "ui_selector") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 32,
						std::string("wrong number of parameters for \"ui_selector\""));
				} else {
					parsed_file.selector_description =
						parse_ui_selector(extracted.values[0].start, extracted.values[0].end, start, err_out);
				}
			} else if(kstr == "primary_font") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 34,
						std::string("wrong number of parameters for \"primary_font\""));
				} else {
					parsed_file.primary_font =
						parse_font_description(extracted.values[0].start, extracted.values[0].end, start, err_out).second;
				}
			} else if(kstr == "small_font") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 35,
						std::string("wrong number of parameters for \"small_font\""));
				} else {
					parsed_file.small_font =
						parse_font_description(extracted.values[0].start, extracted.values[0].end, start, err_out).second;
				}
			} else if(kstr == "custom_font") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 37,
						std::string("wrong number of parameters for \"custom_font\""));
				} else {
					parsed_file.custom_fonts.push_back(
						parse_font_description(extracted.values[0].start, extracted.values[0].end, start, err_out));
					if(parsed_file.custom_fonts.back().first == "") {
						err_out.add(calculate_line_from_position(start, extracted.key.start), 38,
							std::string("A custom font must have an identifying tag"));
					}
				}
			} else if(kstr == "palette") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 46,
						std::string("wrong number of parameters for \"palette\""));
				} else {
					parsed_file.palette =
						parse_palette(extracted.values[0].start, extracted.values[0].end, start, err_out);
				}
			} else if(kstr == "heading") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 47,
						std::string("wrong number of parameters for \"heading\""));
				} else {
					parsed_file.heading = extracted.values[0].to_string();
				}
			} else if(kstr == "footer") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 48,
						std::string("wrong number of parameters for \"footer\""));
				} else {
					parsed_file.footer = extracted.values[0].to_string();
				}
			} else if(kstr == "line_lead") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 49,
						std::string("wrong number of parameters for \"line_lead\""));
				} else {
					parsed_file.line_lead = extracted.values[0].to_string();
				}
			} else if(kstr == "line_end") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 50,
						std::string("wrong number of parameters for \"line_end\""));
				} else {
					parsed_file.line_end = extracted.values[0].to_string();
				}
			} else if(kstr == "content_window") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 51,
						std::string("wrong number of parameters for \"content_window\""));
				} else {
					parsed_file.content =
						parse_content_window(extracted.values[0].start, extracted.values[0].end, start, err_out);
				}
			} else if(kstr == "page") {
				if(extracted.values.size() != 1) {
					err_out.add(calculate_line_from_position(start, extracted.key.start), 52,
						std::string("wrong number of parameters for \"page\""));
				} else {
					parsed_file.pages.push_back(
						parse_page(extracted.values[0].start, extracted.values[0].end, start, err_out));
				}
			} else {
				err_out.add(calculate_line_from_position(start, extracted.key.start), 36,
					std::string("unexpetected top level key: ") + kstr);
			}
		}
	}

	return parsed_file;
}

