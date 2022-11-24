#include "printui_utility.hpp"

#include <algorithm>
#include <charconv>
#include <array>

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

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

char const* advance_to_closing_bracket(char const* pos, char const* end) {
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

char const* advance_to_non_whitespace(char const* pos, char const* end) {
	while(pos < end) {
		if(*pos != ' ' && *pos != '\t' && *pos != '\n' && *pos != '\r')
			return pos;
		++pos;
	}
	return pos;
}

char const* advance_to_whitespace_or_brace(char const* pos, char const* end) {
	while(pos < end) {
		if(*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r' || *pos == '{')
			return pos;
		++pos;
	}
	return pos;
}

char const* reverse_to_non_whitespace(char const* start, char const* pos) {
	while(pos > start) {
		if(*(pos - 1) != ' ' && *(pos - 1) != '\t' && *(pos - 1) != '\n' && *(pos - 1) != '\r')
			return pos;
		--pos;
	}
	return pos;
}

parsed_item extract_item(char const* input, char const* end) {
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

		result.values.push_back(char_range{ position, reverse_to_non_whitespace(position, value_close) });
		position = advance_to_non_whitespace(value_close + 1, end);
		result.terminal = std::min(end, position);
	}

	return result;
}

void parse_main_font_description(printui::font_description& result, char const* start, char const* end) {
	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "name") {
				if(extracted.values.size() >= 1) {
					int32_t wchars_num = MultiByteToWideChar(CP_UTF8, 0, extracted.values[0].start, int32_t(extracted.values[0].end - extracted.values[0].start), nullptr, 0);
					result.name.resize(wchars_num, L' ');
					MultiByteToWideChar(CP_UTF8, 0,
						extracted.values[0].start, int32_t(extracted.values[0].end - extracted.values[0].start),
						result.name.data(), wchars_num);
				}
			} else if(kstr == "bold") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.is_bold = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.is_bold = false;
				}
			} else if(kstr == "oblique") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.is_oblique = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.is_oblique = false;
				}
			} else if(kstr == "span") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "normal" || val == "NORMAL")
						result.span = printui::font_span::normal;
					else if(val == "condensed" || val == "CONDENSED")
						result.span = printui::font_span::condensed;
					else if(val == "wide" || val == "WIDE")
						result.span = printui::font_span::wide;
				}
			} else if(kstr == "type") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "roman" || val == "ROMAN")
						result.type = printui::font_type::roman;
					else if(val == "sans" || val == "SANS" || val == "grotesque" || val == "GROTESQUE")
						result.type = printui::font_type::sans;
					else if(val == "script" || val == "SCRIPT")
						result.type = printui::font_type::script;
					else if(val == "italic" || val == "ITALIC")
						result.type = printui::font_type::italic;
				}
			} else if(kstr == "top_leading") {
				if(extracted.values.size() >= 1) {
					result.top_leading = std::stoi(std::string(extracted.values[0].start, extracted.values[0].end));
				}
			} else if(kstr == "bottom_leading") {
				if(extracted.values.size() >= 1) {
					result.bottom_leading = std::stoi(std::string(extracted.values[0].start, extracted.values[0].end));
				}
			}
		}
	}
}

void parse_named_font_description(printui::launch_settings& ls, std::unordered_map<std::string, uint32_t, printui::text::string_hash, std::equal_to<>>& font_name_to_index, char const* start, char const* end) {
	printui::font_description result;
	int32_t index = -1;

	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "name") {
				if(extracted.values.size() >= 1) {
					int32_t wchars_num = MultiByteToWideChar(CP_UTF8, 0, extracted.values[0].start, int32_t(extracted.values[0].end - extracted.values[0].start), nullptr, 0);
					result.name.resize(wchars_num, L' ');
					MultiByteToWideChar(CP_UTF8, 0,
						extracted.values[0].start, int32_t(extracted.values[0].end - extracted.values[0].start),
						result.name.data(), wchars_num);
				}
			} else if(kstr == "bold") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.is_bold = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.is_bold = false;
				}
			} else if(kstr == "oblique") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						result.is_oblique = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						result.is_oblique = false;
				}
			} else if(kstr == "span") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "normal" || val == "NORMAL")
						result.span = printui::font_span::normal;
					else if(val == "condensed" || val == "CONDENSED")
						result.span = printui::font_span::condensed;
					else if(val == "wide" || val == "WIDE")
						result.span = printui::font_span::wide;
				}
			} else if(kstr == "tag") {
				if(extracted.values.size() >= 1) {
					auto mapit = font_name_to_index.find(extracted.values[0].to_string());
					if(mapit != font_name_to_index.end()) {
						index = int32_t(mapit->second);
					} else {
						index = int32_t(ls.named_fonts.size());
						font_name_to_index.insert_or_assign(extracted.values[0].to_string(), index);
						ls.named_fonts.emplace_back();
					}
				}
			} else if(kstr == "type") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "roman" || val == "ROMAN")
						result.type = printui::font_type::roman;
					else if(val == "sans" || val == "SANS" || val == "grotesque" || val == "GROTESQUE")
						result.type = printui::font_type::sans;
					else if(val == "script" || val == "SCRIPT")
						result.type = printui::font_type::script;
					else if(val == "italic" || val == "ITALIC")
						result.type = printui::font_type::italic;
				}
			} else if(kstr == "top_leading") {
				if(extracted.values.size() >= 1) {
					result.top_leading = std::stoi(std::string(extracted.values[0].start, extracted.values[0].end));
				}
			} else if(kstr == "bottom_leading") {
				if(extracted.values.size() >= 1) {
					result.bottom_leading = std::stoi(std::string(extracted.values[0].start, extracted.values[0].end));
				}
			}
		}
	}

	if(index >= 0) {
		ls.named_fonts[index] = result;
	}
}

void parse_brush(printui::brush& b, char const* start, char const* end) {
	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "color") {
				if(extracted.values.size() >= 3) {
					std::from_chars(extracted.values[0].start, extracted.values[0].end, b.rgb.r, std::chars_format::fixed);
					if(b.rgb.r > 1.0f) b.rgb.r /= 255.0f;

					std::from_chars(extracted.values[1].start, extracted.values[1].end, b.rgb.g, std::chars_format::fixed);
					if(b.rgb.g > 1.0f) b.rgb.g /= 255.0f;

					std::from_chars(extracted.values[2].start, extracted.values[2].end, b.rgb.b, std::chars_format::fixed);
					if(b.rgb.b > 1.0f) b.rgb.b /= 255.0f;
				}
			} else if(kstr == "is_light") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
						b.is_light_color = true;
					else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
						b.is_light_color = false;
				}
			} else if(kstr == "texture") {
				if(extracted.values.size() >= 1) {
					int32_t wchars_num = MultiByteToWideChar(CP_UTF8, 0, extracted.values[0].start, int32_t(extracted.values[0].end - extracted.values[0].start), nullptr, 0);
					b.texture.resize(wchars_num, L' ');
					MultiByteToWideChar(CP_UTF8, 0,
						extracted.values[0].start, int32_t(extracted.values[0].end - extracted.values[0].start),
						b.texture.data(), wchars_num);
				}
			}
		}
	}
}

void parse_palette(std::vector<printui::brush>& brushes, char const* start, char const* end) {
	char const* pos = start;
	int32_t i = 0;


	while(pos < end) {
		auto extracted = extract_item(pos, end);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr = extracted.key.to_string();
			if(kstr == "brush") {
				if(extracted.values.size() >= 1 && i < int32_t(brushes.size())) {
					parse_brush(brushes[i], extracted.values[0].start, extracted.values[0].end);
					++i;
				}
			}
		}
	}
}
namespace printui::parse {
	void settings_file(launch_settings& ls, std::unordered_map<std::string, uint32_t, text::string_hash, std::equal_to<>>& font_name_to_index, char const* start, char const* end) {

		char const* pos = start;
		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.start, extracted.key.end);
				if(kstr == "layout_base_size") {
					if(extracted.values.size() >= 1) {
						ls.layout_base_size = float(std::stoi(extracted.values[0].to_string()));
					}
				} else if(kstr == "line_width") {
					if(extracted.values.size() >= 1) {
						ls.line_width = std::stoi(extracted.values[0].to_string());
					}
				} else if(kstr == "locale") {
					if(extracted.values.size() >= 1) {
						auto base_locale = extracted.values[0].to_string();


						WCHAR* buffer = new WCHAR[base_locale.length() * 2];

						auto chars_written = MultiByteToWideChar(
							CP_UTF8,
							MB_PRECOMPOSED,
							base_locale.data(),
							int32_t(base_locale.length()),
							buffer,
							int32_t(base_locale.length() *  2)
						);

						size_t first_dash = 0;
						for(; first_dash < chars_written; ++first_dash) {
							if(buffer[first_dash] == L'-')
								break;
						}

						ls.locale_lang = std::wstring(buffer, buffer + first_dash);
						ls.locale_region = first_dash < chars_written
							? std::wstring(buffer + first_dash + 1, buffer + chars_written)
							: std::wstring();

						delete[] buffer;
					}
				} else if(kstr == "samll_line_width") {
					if(extracted.values.size() >= 1) {
						ls.small_width = std::stoi(extracted.values[0].to_string());
					}
				} else if(kstr == "primary_font") {
					if(extracted.values.size() >= 1) {
						parse_main_font_description(ls.primary_font, extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "small_font") {
					if(extracted.values.size() >= 1) {
						parse_main_font_description(ls.small_font, extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "custom_font") {
					if(extracted.values.size() >= 1) {
						parse_named_font_description(ls, font_name_to_index, extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "palette") {
					if(extracted.values.size() >= 1) {
						parse_palette(ls.brushes, extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "window_border") {
					if(extracted.values.size() >= 1) {
						ls.window_border = std::stoi(extracted.values[0].to_string());
					}
				} else if(kstr == "input_mode") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "keyboard")
							ls.imode = printui::input_mode::keyboard_only;
						else if(val == "mouse")
							ls.imode = printui::input_mode::mouse_only;
						else if(val == "controller")
							ls.imode = printui::input_mode::controller_only;
						else if(val == "mouse_and_keyboard")
							ls.imode = printui::input_mode::mouse_and_keyboard;
						else if(val == "default")
							ls.imode = printui::input_mode::system_default;
						else if(val == "auto")
							ls.imode = printui::input_mode::follow_input;
					}
				}
			}
		}
	}

	void locale_settings_file(launch_settings& ls, std::unordered_map<std::string, uint32_t, text::string_hash, std::equal_to<>>& font_name_to_index, char const* start, char const* end) {

		char const* pos = start;
		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.start, extracted.key.end);
				if(kstr == "layout_base_size") {
					if(extracted.values.size() >= 1) {
						ls.layout_base_size = float(std::stoi(extracted.values[0].to_string()));
					}
				} else if(kstr == "line_width") {
					if(extracted.values.size() >= 1) {
						ls.line_width = std::stoi(extracted.values[0].to_string());
					}
				} else if(kstr == "samll_line_width") {
					if(extracted.values.size() >= 1) {
						ls.small_width = std::stoi(extracted.values[0].to_string());
					}
				} else if(kstr == "primary_font") {
					if(extracted.values.size() >= 1) {
						printui::font_description new_font;
						parse_main_font_description(new_font, extracted.values[0].start, extracted.values[0].end);
						ls.primary_font = new_font;
					}
				} else if(kstr == "small_font") {
					if(extracted.values.size() >= 1) {
						printui::font_description new_font;
						parse_main_font_description(new_font, extracted.values[0].start, extracted.values[0].end);
						ls.small_font = new_font;
					}
				} else if(kstr == "custom_font") {
					if(extracted.values.size() >= 1) {
						parse_named_font_description(ls, font_name_to_index, extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "palette") {
					if(extracted.values.size() >= 1) {
						parse_palette(ls.brushes, extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "orientation") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "ltr" || val == "LTR")
							ls.preferred_orientation = printui::layout_orientation::horizontal_left_to_right;
						else if(val == "rtl" || val == "RTL")
							ls.preferred_orientation = printui::layout_orientation::horizontal_right_to_left;
						else if(val == "vrtl" || val == "VRTL")
							ls.preferred_orientation = printui::layout_orientation::vertical_right_to_left;
						else if(val == "vltr" || val == "VLTR")
							ls.preferred_orientation = printui::layout_orientation::vertical_left_to_right;
						
					}
				}
			}
		}
	}
}

printui::font_fallback parse_font_fallback(char const* start, char const* end) {
	printui::font_fallback result;

	char const* pos = start;
	while(pos < end) {
		auto extracted = extract_item(pos, end);
		pos = extracted.terminal;

		if(extracted.key.start != extracted.key.end) {
			std::string kstr(extracted.key.start, extracted.key.end);
			if(kstr == "range") {
				if(extracted.values.size() >= 2) {
					printui::unicode_range r;
					r.start = std::stoi(extracted.values[0].to_string(), nullptr, 0);
					r.end = std::stoi(extracted.values[1].to_string(), nullptr, 0);
					result.ranges.push_back(r);
				}
			} else if(kstr == "name") {
				if(extracted.values.size() >= 1) {
					int32_t wchars_num = MultiByteToWideChar(CP_UTF8, 0, extracted.values[0].start, int32_t(extracted.values[0].end - extracted.values[0].start), nullptr, 0);
					result.name.resize(wchars_num, L' ');
					MultiByteToWideChar(CP_UTF8, 0,
						extracted.values[0].start, int32_t(extracted.values[0].end - extracted.values[0].start),
						result.name.data(), wchars_num);
				}
			} else if(kstr == "scale") {
				if(extracted.values.size() >= 1) {
					std::from_chars(extracted.values[0].start, extracted.values[0].end, result.scale, std::chars_format::fixed);
				}
			} else if(kstr == "type") {
				if(extracted.values.size() >= 1) {
					const auto val = extracted.values[0].to_string();
					if(val == "roman" || val == "ROMAN")
						result.type = printui::font_type::roman;
					else if(val == "sans" || val == "SANS")
						result.type = printui::font_type::sans;
					else if(val == "script" || val == "SCRIPT")
						result.type = printui::font_type::script;
					else if(val == "italic" || val == "ITALIC")
						result.type = printui::font_type::italic;
				}
			}
		}
	}

	return result;
}

namespace printui {

	void parse_font_fallbacks_file(std::vector<printui::font_fallback>& result, char const* start, char const* end) {
		char const* pos = start;
		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.start, extracted.key.end);
				if(kstr == "font_fallback") {
					if(extracted.values.size() >= 1) {
						result.push_back(parse_font_fallback(extracted.values[0].start, extracted.values[0].end));
					}
				}
			}
		}
	}

}

