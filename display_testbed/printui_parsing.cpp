#include "printui_main_header.hpp"

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
							result.weight = 700;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							result.weight = 400;
						else
							result.weight = std::stoi(std::string(val));
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
							result.span = 100.0f;
						else if(val == "condensed" || val == "CONDENSED")
							result.span = 75.0f;
						else if(val == "wide" || val == "WIDE")
							result.span = 125.0f;
						else
							result.span = std::stof(std::string(val));
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
							result.weight = 700;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							result.weight = 400;
						else
							result.weight = std::stoi(std::string(val));
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
							result.span = 100.0f;
						else if(val == "condensed" || val == "CONDENSED")
							result.span = 75.0f;
						else if(val == "wide" || val == "WIDE")
							result.span = 125.0f;
						else
							result.span = std::stof(std::string(val));
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

	keyboard_key_descriptor parse_key_description(char const* start, char const* end) {
		keyboard_key_descriptor result;

		char const* pos = start;
		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.to_string());
				if(kstr == "scancode") {
					if(extracted.values.size() >= 1) {
						result.scancode = uint32_t(std::stoi(std::string(extracted.values[0].to_string())));
					}
				} else if(kstr == "name") {
					for(uint32_t i = 0; i < extracted.values.size(); ++i) {
						result.display_name += wchar_t(uint16_t(std::stoi(std::string(extracted.values[i].to_string()))));
					}
					
				} 
			}
		}
		return result;
	}

	void parse_keysettings(launch_settings& ls, char const* start, char const* end) {
		char const* pos = start;
		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.to_string());
				if(kstr == "key0") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[0] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key1") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[1] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key2") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[2] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key3") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[3] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key4") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[4] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key5") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[5] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key6") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[6] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key7") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[7] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key8") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[8] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key9") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[9] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key10") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[10] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "key11") {
					if(extracted.values.size() >= 1) {
						ls.keys.main_keys[11] = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "keyescape") {
					if(extracted.values.size() >= 1) {
						ls.keys.primary_escape = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "keyinfo") {
					if(extracted.values.size() >= 1) {
						ls.keys.info_key = parse_key_description(extracted.values[0].start, extracted.values[0].end);
					}
				} else if(kstr == "sticky_info") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
							ls.keys.info_key_is_sticky = true;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							ls.keys.info_key_is_sticky = false;
					}
				} else if(kstr == "type") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "left_hand")
							ls.keys.type = keyboard_type::left_hand;
						else if(val == "right_hand")
							ls.keys.type = keyboard_type::right_hand;
						else if(val == "right_hand_tilted")
							ls.keys.type = keyboard_type::right_hand_tilted;
						else if(val == "custom")
							ls.keys.type = keyboard_type::custom;
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

	void parse_controllersettings(controller_mappings& controller, char const* start, char const* end);

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
				} else if(kstr == "keysettings") {
					if(extracted.values.size() >= 1) {
						parse_keysettings(ls, extracted.values[0].start, extracted.values[0].end);
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
				} else if(kstr == "header_font") {
					if(extracted.values.size() >= 1) {
						printui::font_description new_font;
						parse_main_font_description(new_font, extracted.values[0].start, extracted.values[0].end);
						ls.header_font = new_font;
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
				} else if(kstr == "global_size_multiplier") {
					if(extracted.values.size() >= 1) {
						ls.global_size_multiplier = std::stof(std::string(extracted.values[0].to_string()));
					}
				} else if(kstr == "small_size_multiplier") {
					if(extracted.values.size() >= 1) {
						ls.small_size_multiplier = std::stof(std::string(extracted.values[0].to_string()));
					}
				} else if(kstr == "heading_size_multiplier") {
					if(extracted.values.size() >= 1) {
						ls.heading_size_multiplier = std::stof(std::string(extracted.values[0].to_string()));
					}
				} else if(kstr == "controllersettings") {
					if(extracted.values.size() >= 1) {
						parse_controllersettings(ls.controller, extracted.values[0].start, extracted.values[0].end);
					}
				}

			}
		}
	}
	
	std::string make_key_description(keyboard_key_descriptor const& k) {
		std::string result;
		result += "\t\tscancode{ " + std::to_string(k.scancode) + " }\n";
		result += "\t\tname";
		for(uint32_t i = 0; i < k.display_name.length(); ++i) {
			result += "{ " + std::to_string(uint16_t(k.display_name[i])) + " }";
		}
		result += "\n";
		return result;
	}

	std::string make_keysettings_description(key_mappings const& k) {
		std::string result;
		result += "keysettings{\n";

			result += "\tkey0{\n";
			result += make_key_description(k.main_keys[0]);
			result += "\t}\n";

			result += "\tkey1{\n";
			result += make_key_description(k.main_keys[1]);
			result += "\t}\n";

			result += "\tkey2{\n";
			result += make_key_description(k.main_keys[2]);
			result += "\t}\n";

			result += "\tkey3{\n";
			result += make_key_description(k.main_keys[3]);
			result += "\t}\n";

			result += "\tkey4{\n";
			result += make_key_description(k.main_keys[4]);
			result += "\t}\n";

			result += "\tkey5{\n";
			result += make_key_description(k.main_keys[5]);
			result += "\t}\n";

			result += "\tkey6{\n";
			result += make_key_description(k.main_keys[6]);
			result += "\t}\n";

			result += "\tkey7{\n";
			result += make_key_description(k.main_keys[7]);
			result += "\t}\n";

			result += "\tkey8{\n";
			result += make_key_description(k.main_keys[8]);
			result += "\t}\n";

			result += "\tkey9{\n";
			result += make_key_description(k.main_keys[9]);
			result += "\t}\n";

			result += "\tkey10{\n";
			result += make_key_description(k.main_keys[10]);
			result += "\t}\n";

			result += "\tkey11{\n";
			result += make_key_description(k.main_keys[11]);
			result += "\t}\n";

			result += "\tkeyescape{\n";
			result += make_key_description(k.primary_escape);
			result += "\t}\n";

			result += "\tkeyinfo{\n";
			result += make_key_description(k.info_key);
			result += "\t}\n";
		
		result += "\tsticky_info{ " + std::string(k.info_key_is_sticky ? "yes" : "no") + " }\n";

		result += "\ttype{ ";
		if(k.type == keyboard_type::left_hand) {
			result += "left_hand";
		} else if(k.type == keyboard_type::right_hand) {
			result += "right_hand";
		} else if(k.type == keyboard_type::right_hand_tilted) {
			result += "right_hand_tilted";
		} else if(k.type == keyboard_type::custom) {
			result += "custom";
		}
		result += " }\n";

		result += "}\n";
		return result;
	}

	std::string make_font_description(font_description const& fnt) {
		std::string result;

		result += "\tname{ " + to_string(fnt.name) + " }\n";
		result += "\tbold{ " + std::to_string(fnt.weight) + " }\n";
		result += "\toblique{ " + std::string(fnt.is_oblique ? "yes" : "no") + " }\n";
		result += "\tspan{ " + std::to_string(fnt.span) + " }\n";


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

	controller_button button_from_string(std::string_view v) {
		if(v == "y")
			return controller_button::y;
		else if(v == "x")
			return controller_button::x;
		else if(v == "b")
			return controller_button::b;
		else if(v == "a")
			return controller_button::a;
		else if(v == "lb")
			return controller_button::lb;
		else if(v == "rb")
			return controller_button::rb;
		else if(v == "start")
			return controller_button::start;
		else if(v == "back")
			return controller_button::back;
		else if(v == "tleft")
			return controller_button::tleft;
		else if(v == "tright")
			return controller_button::tright;
		else if(v == "dup")
			return controller_button::dup;
		else if(v == "ddown")
			return controller_button::ddown;
		else if(v == "dleft")
			return controller_button::dleft;
		else if(v == "dright")
			return controller_button::dright;
		return controller_button::no_button;
	}
	std::string to_string(controller_button b) {
		switch(b) {
			case controller_button::y:
				return "y";
			case controller_button::x:
				return "x";
			case controller_button::b:
				return "b";
			case controller_button::a:
				return "a";
			case controller_button::lb:
				return "lb";
			case controller_button::rb:
				return "rb";
			case controller_button::start:
				return "start";
			case controller_button::back:
				return "back";
			case controller_button::tleft:
				return "tleft";
			case controller_button::tright:
				return "tright";
			case controller_button::dup:
				return "dup";
			case controller_button::ddown:
				return "ddown";
			case controller_button::dleft:
				return "dleft";
			case controller_button::dright:
				return "dright";
			case controller_button::no_button:
				return "none";
			default:
				return "none";
		}
	}
	std::string create_controllersettings(controller_mappings const& controller) {
		std::string result;
		result += "controllersettings {\n";
		result += "\tbutton1{ " + to_string(controller.button1) + " }\n";
		result += "\tbutton2{ " + to_string(controller.button2) + " }\n";
		result += "\tbutton3{ " + to_string(controller.button3) + " }\n";
		result += "\tbutton4{ " + to_string(controller.button4) + " }\n";

		result += "\tsensitivity{ " + std::to_string(controller.sensitivity) + " }\n";
		result += "\tdeadzone{ " + std::to_string(controller.deadzone) + " }\n";
		result += "\tfirst_group{ " + to_string(controller.first_group) + " }\n";
		result += "\tsecond_group{ " + to_string(controller.second_group) + " }\n";

		result += "\tinfo1{ " + to_string(controller.info.first_button) + " }\n";
		result += "\tinfo2{ " + to_string(controller.info.second_button) + " }\n";
		result += "\tesc1{ " + to_string(controller.escape.first_button) + " }\n";
		result += "\tesc2{ " + to_string(controller.escape.second_button) + " }\n";

		result += "\tfirst_sticky{ " + std::string(controller.first_group_sticky ? "yes" : "no") + " }\n";
		result += "\tsecond_sticky{ " + std::string(controller.second_group_sticky ? "yes" : "no") + " }\n";
		result += "\tinfo_sticky{ " + std::string(controller.info.sticky ? "yes" : "no") + " }\n";
		result += "\tesc_sticky{ " + std::string(controller.escape.sticky ? "yes" : "no") + " }\n";

		result += "\tleft_stick{ " + std::string(controller.left_thumbstick ? "yes" : "no") + " }\n";

		result += "}\n";
		return result;
	}
	void parse_controllersettings(controller_mappings& controller, char const* start, char const* end) {
		char const* pos = start;

		while(pos < end) {
			auto extracted = extract_item(pos, end);
			pos = extracted.terminal;

			if(extracted.key.start != extracted.key.end) {
				std::string kstr(extracted.key.to_string());
				if(kstr == "button1") {
					if(extracted.values.size() >= 1) {
						controller.button1 = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "button2") {
					if(extracted.values.size() >= 1) {
						controller.button2 = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "button3") {
					if(extracted.values.size() >= 1) {
						controller.button3 = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "button4") {
					if(extracted.values.size() >= 1) {
						controller.button4 = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "sensitivity") {
					if(extracted.values.size() >= 1) {
						controller.sensitivity = std::stof(std::string(extracted.values[0].to_string()));
					}
				} else if(kstr == "deadzone") {
					if(extracted.values.size() >= 1) {
						controller.deadzone = std::stod(std::string(extracted.values[0].to_string()));
					}
				} else if(kstr == "first_group") {
					if(extracted.values.size() >= 1) {
						controller.first_group = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "second_group") {
					if(extracted.values.size() >= 1) {
						controller.second_group = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "info1") {
					if(extracted.values.size() >= 1) {
						controller.info.first_button = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "info2") {
					if(extracted.values.size() >= 1) {
						controller.info.second_button = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "esc1") {
					if(extracted.values.size() >= 1) {
						controller.escape.first_button = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "esc2") {
					if(extracted.values.size() >= 1) {
						controller.escape.second_button = button_from_string(extracted.values[0].to_string());
					}
				} else if(kstr == "first_sticky") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
							controller.first_group_sticky = true;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							controller.first_group_sticky = false;
					}
				} else if(kstr == "second_sticky") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
							controller.second_group_sticky = true;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							controller.second_group_sticky = false;
					}
				} else if(kstr == "info_sticky") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
							controller.info.sticky = true;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							controller.info.sticky = false;
					}
				} else if(kstr == "esc_sticky") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
							controller.escape.sticky = true;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							controller.escape.sticky = false;
					}
				} else if(kstr == "left_stick") {
					if(extracted.values.size() >= 1) {
						const auto val = extracted.values[0].to_string();
						if(val == "yes" || val == "y" || val == "YES" || val == "Y" || val == "true")
							controller.left_thumbstick = true;
						else if(val == "no" || val == "n" || val == "NO" || val == "N" || val == "false")
							controller.left_thumbstick = false;
					}
				}
			}
		}
	}

	std::string create_settings_file(launch_settings const& ls) {
		std::string result;

		result += "layout_base_size{ " + std::to_string(int32_t(ls.layout_base_size))  + " }\n";
		result += "line_width{ " + std::to_string(int32_t(ls.line_width)) + " }\n";
		result += "small_line_width{ " + std::to_string(int32_t(ls.small_width)) + " }\n";
		result += "primary_font{\n" + make_font_description(ls.primary_font) + "}\n";
		result += "small_font{\n" + make_font_description(ls.small_font) + "}\n";
		result += "header_font{\n" + make_font_description(ls.header_font) + "}\n";
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
		result += "global_size_multiplier{ " + std::to_string(ls.global_size_multiplier) + " }\n";
		result += "small_size_multiplier{ " + std::to_string(ls.small_size_multiplier) + " }\n";
		result += "heading_size_multiplier{ " + std::to_string(ls.heading_size_multiplier) + " }\n";

		result += make_keysettings_description(ls.keys);
		result += create_controllersettings(ls.controller);
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

