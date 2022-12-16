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


namespace printui::parse {
	struct char_range {
		char const* start;
		char const* end;

		std::string_view to_string() const {
			return std::string_view(start, end - start);
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

	font_fallback a_font_fallback(char const* start, char const* end);

	void parse_main_font_description(printui::font_description& result, char const* start, char const* end) {
		char const* pos = start;
		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.to_string());
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
				std::string kstr(extracted.key.to_string());
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
							font_name_to_index.insert_or_assign(std::string(extracted.values[0].to_string()), index);
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
				std::string kstr(extracted.key.to_string());
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
				std::string kstr(extracted.key.to_string());
				if(kstr == "brush") {
					if(extracted.values.size() >= 1 && i < int32_t(brushes.size())) {
						parse_brush(brushes[i], extracted.values[0].start, extracted.values[0].end);
						++i;
					}
				}
			}
		}
	}

	std::wstring to_wstring(std::string_view str) {

		WCHAR* buffer = new WCHAR[str.length() * 2];

		auto chars_written = MultiByteToWideChar(
			CP_UTF8,
			MB_PRECOMPOSED,
			str.data(),
			int32_t(str.length()),
			buffer,
			int32_t(str.length() * 2)
		);

		std::wstring result(buffer, size_t(chars_written));

		delete[] buffer;
		return result;
	}

	std::string to_string(std::wstring_view str) {

		char* buffer = new char[str.length() * 4];

		auto chars_written = WideCharToMultiByte(
			CP_UTF8,
			0,
			str.data(),
			int32_t(str.length()),
			buffer,
			int32_t(str.length() * 4),
			nullptr,
			nullptr
		);

		std::string result(buffer, size_t(chars_written));

		delete[] buffer;
		return result;
	}

	void custom_fonts_only(launch_settings& ls, std::unordered_map<std::string, uint32_t, text::string_hash, std::equal_to<>>& font_name_to_index, char const* start, char const* end) {

		char const* pos = start;
		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.start, extracted.key.end);
				if(kstr == "custom_font") {
					if(extracted.values.size() >= 1) {
						parse_named_font_description(ls, font_name_to_index, extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "font_fallback") {
					if(extracted.values.size() >= 1) {
						ls.fallbacks.push_back(a_font_fallback(extracted.values[0].start, extracted.values[0].end));
					}
				}
			}
		}
	}

	void settings_file(launch_settings& ls, std::unordered_map<std::string, uint32_t, text::string_hash, std::equal_to<>>& font_name_to_index, char const* start, char const* end) {

		char const* pos = start;
		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.start, extracted.key.end);
				if(kstr == "layout_base_size") {
					if(extracted.values.size() >= 1) {
						ls.layout_base_size = float(std::stoi(std::string(extracted.values[0].to_string())));
					}
				} else if(kstr == "line_width") {
					if(extracted.values.size() >= 1) {
						ls.line_width = std::stoi(std::string(extracted.values[0].to_string()));
					}
				} else if(kstr == "small_line_width") {
					if(extracted.values.size() >= 1) {
						ls.small_width = std::stoi(std::string(extracted.values[0].to_string()));
					}
				} else if(kstr == "primary_font") {
					if(extracted.values.size() >= 1) {
						printui::font_description new_font;
						parse_main_font_description(new_font, extracted.values[0].start, extracted.values[0].end);
						ls.primary_font = new_font;
					}
				} else if(kstr == "font_fallback") {
					if(extracted.values.size() >= 1) {
						ls.fallbacks.push_back(a_font_fallback(extracted.values[0].start, extracted.values[0].end));
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
				} else if(kstr == "window_border") {
					if(extracted.values.size() >= 1) {
						ls.window_border = std::stoi(std::string(extracted.values[0].to_string()));
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
						else if(val == "controller_pointer")
							ls.imode = printui::input_mode::controller_with_pointer;
						else if(val == "mouse_and_keyboard")
							ls.imode = printui::input_mode::mouse_and_keyboard;
						else if(val == "default")
							ls.imode = printui::input_mode::system_default;
						else if(val == "auto")
							ls.imode = printui::input_mode::follow_input;
					}
				} else if(kstr == "uianimations") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
							ls.uianimations = true;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							ls.uianimations = false;
					}
				} else if(kstr == "caret_blink") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
							ls.caret_blink = true;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							ls.caret_blink = false;
					}
				} else if(kstr == "region") {
					if(extracted.values.size() >= 1) {
						ls.locale_region = to_wstring(extracted.values[0].to_string());
					}
				} else if(kstr == "lang") {
					if(extracted.values.size() >= 1) {
						ls.locale_lang = to_wstring(extracted.values[0].to_string());
					}
				} else if(kstr == "text_directory") {
					if(extracted.values.size() >= 1) {
						ls.text_directory = to_wstring(extracted.values[0].to_string());
					}
				} else if(kstr == "texture_directory") {
					if(extracted.values.size() >= 1) {
						ls.texture_directory = to_wstring(extracted.values[0].to_string());
					}
				} else if(kstr == "font_directory") {
					if(extracted.values.size() >= 1) {
						ls.font_directory = to_wstring(extracted.values[0].to_string());
					}
				} else if(kstr == "icon_directory") {
					if(extracted.values.size() >= 1) {
						ls.icon_directory = to_wstring(extracted.values[0].to_string());
					}
				} else if(kstr == "animation_speed_multiplier") {
					if(extracted.values.size() >= 1) {
						ls.animation_speed_multiplier = std::stof(std::string(extracted.values[0].to_string()));
					}
				} else if(kstr == "locale_is_default") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
							ls.locale_is_default = true;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							ls.locale_is_default = false;
					}
				}

			}
		}
	}

	std::string make_font_description(font_description const& fnt) {
		std::string result;

		result += "\tname{ " + to_string(fnt.name) + " }\n";
		result += "\tbold{ " + std::string(fnt.is_bold ? "yes" : "no") + " }\n";
		result += "\toblique{ " + std::string(fnt.is_oblique ? "yes" : "no") + " }\n";

		if(fnt.span == font_span::normal)
			result += "\tspan{ normal }\n";
		else if(fnt.span == font_span::condensed)
			result += "\tspan{ condensed }\n";
		else if(fnt.span == font_span::wide)
			result += "\tspan{ wide }\n";

		if(fnt.type == font_type::roman)
			result += "\ttype{ roman }\n";
		else if(fnt.type == font_type::sans)
			result += "\ttype{ sans }\n";
		else if(fnt.type == font_type::script)
			result += "\ttype{ script }\n";
		else if(fnt.type == font_type::italic)
			result += "\ttype{ italic }\n";

		result += "\ttop_leading{ " + std::to_string(fnt.top_leading) + " }\n";
		result += "\tbottom_leading{ " + std::to_string(fnt.bottom_leading) + " }\n";

		return result;
	}

	std::string make_brush_description(brush const& bt) {
		std::string result;

		result += "\t\tcolor{ " + std::to_string(bt.rgb.r) + " }{ " + std::to_string(bt.rgb.g) + " }{ " + std::to_string(bt.rgb.b) + " }\n";
		result += "\t\tis_light{ " + std::string(bt.is_light_color ? "yes" : "no") + " }\n";
		if(bt.texture.length() > 0) {
			result += "\t\ttexture{ " + to_string(bt.texture) + " }\n";
		}

		return result;
	}

	std::string make_palette_description(launch_settings const& ls) {
		std::string result;

		for(auto& b : ls.brushes) {
			result += "\tbrush{\n" + make_brush_description(b) + "\t}\n";
		}

		return result;
	}

	std::string create_settings_file(launch_settings const& ls) {
		std::string result;

		result += "layout_base_size{ " + std::to_string(int32_t(ls.layout_base_size))  + " }\n";
		result += "line_width{ " + std::to_string(int32_t(ls.line_width)) + " }\n";
		result += "small_line_width{ " + std::to_string(int32_t(ls.small_width)) + " }\n";
		result += "primary_font{\n" + make_font_description(ls.primary_font) + "}\n";
		result += "small_font{\n" + make_font_description(ls.small_font) + "}\n";
		result += "palette{\n" + make_palette_description(ls) + "}\n";
		if(ls.preferred_orientation == layout_orientation::horizontal_left_to_right)
			result += "orientation{ ltr }\n";
		else if(ls.preferred_orientation == layout_orientation::horizontal_right_to_left)
			result += "orientation{ rtl }\n";
		else if(ls.preferred_orientation == layout_orientation::vertical_left_to_right)
			result += "orientation{ vltr }\n";
		else if(ls.preferred_orientation == layout_orientation::vertical_right_to_left)
			result += "orientation{ vrtl }\n";
		
		result += "window_border{ " + std::to_string(int32_t(ls.window_border)) + " }\n";

		if(ls.imode == printui::input_mode::keyboard_only)
			result += "input_mode{ keyboard }\n";
		else if(ls.imode == printui::input_mode::mouse_only)
			result += "input_mode{ mouse }\n";
		else if(ls.imode == printui::input_mode::controller_only)
			result += "input_mode{ controller }\n";
		else if(ls.imode == printui::input_mode::mouse_and_keyboard)
			result += "input_mode{ mouse_and_keyboard }\n";
		else if(ls.imode == printui::input_mode::system_default)
			result += "input_mode{ default }\n";
		else if(ls.imode == printui::input_mode::follow_input)
			result += "input_mode{ auto }\n";
		else if(ls.imode == printui::input_mode::controller_with_pointer)
			result += "input_mode{ controller_pointer }\n";

		result += "uianimations{ " + std::string(ls.uianimations ? "yes" : "no") + " }\n";
		result += "caret_blink{ " + std::string(ls.caret_blink ? "yes" : "no") + " }\n";
		result += "region{ " + to_string(ls.locale_region) + " }\n";
		result += "lang{ " + to_string(ls.locale_lang) + " }\n";
		result += "text_directory{ " + to_string(ls.text_directory) + " }\n";
		result += "texture_directory{ " + to_string(ls.texture_directory) + " }\n";
		result += "font_directory{ " + to_string(ls.font_directory) + " }\n";
		result += "icon_directory{ " + to_string(ls.icon_directory) + " }\n";
		result += "animation_speed_multiplier{ " + std::to_string(ls.animation_speed_multiplier) + " }\n";
		result += "locale_is_default{ " + std::string(ls.locale_is_default ? "yes" : "no") + " }\n";
		
		return result;
	}

	printui::font_fallback a_font_fallback(char const* start, char const* end) {
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
						r.start = std::stoi(std::string(extracted.values[0].to_string()), nullptr, 0);
						r.end = std::stoi(std::string(extracted.values[1].to_string()), nullptr, 0);
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

	void font_fallbacks_file(std::vector<printui::font_fallback>& result, char const* start, char const* end) {
		char const* pos = start;
		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.start, extracted.key.end);
				if(kstr == "font_fallback") {
					if(extracted.values.size() >= 1) {
						result.push_back(a_font_fallback(extracted.values[0].start, extracted.values[0].end));
					}
				}
			}
		}
	}

}

