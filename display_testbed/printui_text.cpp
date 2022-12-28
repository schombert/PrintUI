#include "printui_main_header.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <algorithm>
#include <charconv>
#include <array>
#include <dwrite_3.h>
#include <Shlobj.h>
#include <wchar.h>
#include <usp10.h>
#include <initguid.h>
#include <inputscope.h>
#include <Textstor.h>
#include <tsattrs.h>
#include <msctf.h>
#include <olectl.h>
#include <icu.h>

#include "printui_text_definitions.hpp"
#include "printui_windows_definitions.hpp"

#pragma comment(lib, "Usp10.lib")
#pragma comment(lib, "icu.lib")
#pragma comment(lib, "Dwrite.lib")

namespace printui::text {

	struct param_id_position_pair {
		int32_t param_id;
		int32_t position;

		bool operator<(param_id_position_pair const& o) {
			return o.position > position;
		}
	};

	text_with_formatting& text_with_formatting::operator+=(text_with_formatting const& o) {
		auto old_length = text.length();
		text += o.text;
		for(auto& f : o.formatting) {
			formatting.push_back(format_marker{ uint16_t(f.position + old_length), f.format });
		}
		return *this;
	}

	void text_with_formatting::append(text_data_storage const& tm, static_text_with_formatting const& o) {
		auto old_length = text.length();

		text.append(tm.codepoint_storage.data() + o.code_points_start, tm.codepoint_storage.data() + o.code_points_start + o.code_points_count);

		for(uint32_t i = o.formatting_start; i < o.formatting_end; ++i) {
			formatting.push_back(format_marker{ uint16_t(tm.static_format_storage[i].position + old_length), tm.static_format_storage[i].format });
		}
	}

	void text_with_formatting::make_substitutions(replaceable_instance const* start, replaceable_instance const* end) {
		auto num_params = end - start;
		auto const old_formatting_size = formatting.size();

		for(uint32_t i = 0; i < old_formatting_size; ++i) {
			if(std::holds_alternative<parameter_id>(formatting[i].format)) {
				auto id = std::get<parameter_id>(formatting[i].format).id;

				const auto old_start = formatting[i].position;
				auto old_end = old_start + 1;

				if(id < num_params) {
					formatting[i].format = substitution_mark{ id };

					const auto length_difference = start[id].text_content.text.length() - 1;

					text.replace(old_start, 1, start[id].text_content.text);

					formatting.push_back(format_marker{ uint16_t(start[id].text_content.text.length() + old_start), substitution_mark{ id } });

					for(uint32_t j = 0; j < formatting.size(); ++j) {
						if(formatting[j].position >= old_end) {
							formatting[j].position += uint16_t(length_difference);
						}
					}

					for(auto& f : start[id].text_content.formatting) {
						formatting.push_back(format_marker{ uint16_t(f.position + old_start), f.format });
					}
				}
			}
		}

		std::sort(formatting.begin(), formatting.end());
	}

	bool matched_pattern::matches_parameters(text_data_storage const& tm, replaceable_instance const* param_start, replaceable_instance const* param_end) const {

		for(uint32_t i = 0; i < num_keys; ++i) {
			auto key = tm.match_keys_storage[match_key_start + i];
			if(key.parameter_id > (param_end - param_start))
				return false;
			bool matched_one = false;
			for(uint32_t j = 0; !matched_one && j < replaceable_instance::max_attributes; ++j) {
				if(param_start[key.parameter_id].attributes[j] == key.attribute)
					matched_one = true;
			}
			if(!matched_one)
				return false;
		}

		return true;
	}


	replaceable_instance text_manager::parameter_to_text(text_parameter p) const {
		replaceable_instance result;
		if(std::holds_alternative<int_param>(p)) {
			auto value = std::get<int_param>(p);
			return format_int(value.value, value.decimal_places);
		} else if(std::holds_alternative<fp_param>(p)) {
			auto value = std::get<fp_param>(p);
			return format_double(value.value, value.decimal_places);
		} else if(std::holds_alternative<text_id>(p)) {
			result = text_data.stored_functions[std::get<text_id>(p).id].instantiate(text_data, nullptr, nullptr);
		}
		return result;
	}

	replaceable_instance text_function::instantiate(text_data_storage const& tm, replaceable_instance const* start, replaceable_instance const* end) const {

		replaceable_instance result;
		result.attributes = attributes;

		int32_t last_matched_group = -1;

		for(uint32_t j = 0; j < pattern_count; ++j) {
			auto& pattern = tm.static_matcher_storage[begin_patterns + j];
			if(int32_t(pattern.group) != last_matched_group && pattern.matches_parameters(tm, start, end)) {
				last_matched_group = int32_t(pattern.group);
				result.text_content.append(tm, pattern.base_text);
			}
		}

		return result;
	}

	

	/*
	n 	the absolute value of N.*
	i 	the integer digits of N.*
	v 	the number of visible fraction digits in N, with trailing zeros.*
	w 	the number of visible fraction digits in N, without trailing zeros.*
	f 	the visible fraction digits in N, with trailing zeros, expressed as an integer.*
	t 	the visible fraction digits in N, without trailing zeros, expressed as an integer.*
	c 	compact decimal exponent value: exponent of the power of 10 used in compact decimal formatting.
	e 	a deprecated synonym for ‘c’. Note: it may be redefined in the future.
	*/

	int64_t remove_zeros(int64_t f) {
		while(f % 10 == 0) {
			f = f / 10;
		}
		return f;
	}

	attribute_type card_2am(int64_t i, int64_t f, int32_t ) {
		if(i == 0 || (std::abs(i) == 1 && f == 0))
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2ff(int64_t i, int64_t , int32_t ) {
		if(i == 0 || i == 1)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2ast(int64_t i, int64_t , int32_t v) {
		if(i == 1 && v == 0)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2si(int64_t i, int64_t f, int32_t ) {
		if((std::abs(i) == 1 || i == 0) && f == 0)
			return text::one;
		else if(i == 0 && f == 1)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2ak(int64_t i, int64_t f, int32_t ) {
		if((std::abs(i) == 1 || i == 0) && f == 0)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2tzm(int64_t i, int64_t f, int32_t ) {
		if((std::abs(i) == 1 || i == 0 || (std::abs(i) >= 11 && std::abs(i) <= 99)) && f == 0)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2af(int64_t i, int64_t f, int32_t ) {
		if((std::abs(i) == 1) && f == 0)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2da(int64_t i, int64_t f, int32_t ) {
		if((std::abs(i) == 1) && f == 0)
			return text::one;
		else if((i == 0 || i == 1) && f != 0)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2is(int64_t i, int64_t f, int32_t ) {
		if((i % 10 == 1) && (i % 100 != 11) && f == 0)
			return text::one;
		else if(remove_zeros(f) % 10 == 1 && remove_zeros(f) % 100 != 11)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2mk(int64_t i, int64_t f, int32_t v) {
		if(v == 0 && i % 10 == 1 && i % 100 != 11)
			return text::one;
		else if(f % 10 == 1 && f % 100 != 11)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_2ceb(int64_t i, int64_t f, int32_t v) {
		if(v == 0 && (i == 1 || i == 2 || i == 3))
			return text::one;
		else if(v == 0 && i % 10 != 4 && i % 10 != 6 && i % 10 != 9)
			return text::one;
		else if(v != 0 && f % 10 != 4 && f % 10 != 6 && f % 10 != 9)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_3lv(int64_t i, int64_t f, int32_t v) {
		if(f == 0 && (i % 10 == 0 || (i % 100 >= 11 && i % 100 <= 19)))
			return text::zero;
		else if(v == 2 && f % 100 >= 11 && f % 100 <= 19)
			return text::zero;
		else if(f == 0 && std::abs(i) % 10 == 1 && std::abs(i) % 100 != 11)
			return text::one;
		else if(v == 2 && f % 10 == 1 && f % 100 != 11)
			return text::one;
		else if(v != 2 && f % 10 == 1)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_3lag(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && i == 0)
			return text::zero;
		else if(i == 0 || i == 1)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_3ksh(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && i == 0)
			return text::zero;
		else if(f == 0 && std::abs(i) == 1)
			return text::one;
		else
			return text::other;
	}

	attribute_type card_3he(int64_t i, int64_t , int32_t v) {
		if((i == 1 && v == 0) || (i == 0 && v != 0))
			return text::one;
		else if(i == 2 && v == 0)
			return text::two;
		else
			return text::other;
	}

	attribute_type card_3iu(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && std::abs(i) == 1)
			return text::one;
		else if(f == 0 && std::abs(i) == 2)
			return text::two;
		else
			return text::other;
	}

	attribute_type card_3shi(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && std::abs(i) == 1)
			return text::one;
		else if(i == 0)
			return text::one;
		else if(f == 0 && std::abs(i) >= 2 && std::abs(i) <= 10)
			return text::few;
		else
			return text::other;
	}

	attribute_type card_3mo(int64_t i, int64_t f, int32_t v) {
		if(v == 0 && i == 1)
			return text::one;
		else if(v != 0 || (f == 0 && i == 0) || (f == 0 && abs(i) != 1 && abs(i) % 100 >= 1 && abs(i) % 100 <= 19))
			return text::few;
		else
			return text::other;
	}

	attribute_type card_3bs(int64_t i, int64_t f, int32_t v) {
		if(v == 0 and i % 10 == 1 && i % 100 != 11)
			return text::one;
		else if(f % 10 == 1 && f % 100 != 11)
			return text::one;
		else if(v == 0 && i % 10 >= 2 && i % 10 <= 4 && i % 100 != 12 && i % 100 != 13 && i % 100 != 14)
			return text::few;
		else if(f % 10 >= 2 && f % 10 <= 4 && f % 100 != 12 && f % 100 != 13 && f % 100 != 14)
			return text::few;
		else
			return text::other;
	}

	attribute_type card_3fr(int64_t i, int64_t , int32_t v) {
		if(i == 0 || i == 1)
			return text::one;
		else if(i != 0 && i % 1000000 == 0 && v == 0)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_3pt(int64_t i, int64_t , int32_t v) {
		if(i == 0 || i == 1)
			return text::one;
		else if(i != 0 && i % 1000000 == 0 && v == 0)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_3ca(int64_t i, int64_t , int32_t v) {
		if(i == 1 && v == 0)
			return text::one;
		else if(i != 0 && i % 1000000 == 0 and v == 0)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_3es(int64_t i, int64_t f, int32_t v) {
		if(std::abs(i) == 1 && f == 0)
			return text::one;
		else if(i != 0 && i % 1000000 == 0 && v == 0)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_4gd(int64_t i, int64_t f, int32_t ) {
		if((std::abs(i) == 1 || std::abs(i) == 11) && f == 0)
			return text::one;
		else if((std::abs(i) == 2 || std::abs(i) == 12) && f == 0)
			return text::two;
		else if(std::abs(i) >= 3 && std::abs(i) <= 19 && f == 0)
			return text::few;
		else
			return text::many;
	}

	attribute_type card_4sl(int64_t i, int64_t , int32_t v) {
		if(v == 0 && i % 100 == 1)
			return text::one;
		else if(v == 0 && i % 100 == 2)
			return text::two;
		else if(v == 0 && (i % 100 == 3 || i % 100 == 4))
			return text::few;
		else if(v != 0)
			return text::few;
		else
			return text::other;
	}

	attribute_type card_4dsb(int64_t i, int64_t f, int32_t v) {
		if(v == 0 && i % 100 == 1)
			return text::one;
		else if(f % 100 == 1)
			return text::one;
		else if(v == 0 && i % 100 == 2)
			return text::two;
		else if(f % 100 == 2)
			return text::two;
		else if(v == 0 && (i % 100 == 3 || i % 100 == 4))
			return text::few;
		else if((f % 100 == 3 || f % 100 == 4))
			return text::few;
		else
			return text::other;
	}

	attribute_type card_4cs(int64_t i, int64_t , int32_t v) {
		if(i == 1 && v == 0)
			return text::one;
		else if(v == 0 && (i == 2 || i == 3 || i == 4))
			return text::few;
		else if(v != 0)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_4pl(int64_t i, int64_t , int32_t v) {
		if(i == 1 && v == 0)
			return text::one;
		else if(v == 0 && (i % 10 == 2 || i % 10 == 3 || i % 10 == 4) && i % 100 != 12 && i % 100 != 13 && i % 100 != 14)
			return text::few;
		else if(v == 0 && i != 1 && i % 10 != 2 && i % 10 != 3 && i % 10 != 4)
			return text::many;
		else if(v == 0 && (i % 100 == 12 || i % 100 == 13 || i % 100 == 14))
			return text::many;
		else
			return text::other;
	}

	attribute_type card_4be(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && std::abs(i) % 10 == 1 && std::abs(i) % 100 != 11)
			return text::one;
		else if(f == 0 && (std::abs(i) % 10 == 2 || std::abs(i) % 10 == 3 || std::abs(i) % 10 == 4) && std::abs(i) % 100 != 12 && std::abs(i) % 100 != 13 && std::abs(i) % 100 != 14)
			return text::few;
		else if(f == 0 && std::abs(i) % 10 == 0)
			return text::many;
		else if(f == 0 && std::abs(i) % 10 >= 5 && std::abs(i) % 10 <= 9)
			return text::many;
		else if(f == 0 && std::abs(i) % 100 >= 11 && std::abs(i) % 100 <= 14)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_4lt(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && std::abs(i) % 10 == 1 && (std::abs(i) % 100 < 11 || std::abs(i) % 100 > 19))
			return text::one;
		else if(f == 0 && std::abs(i) % 10 >= 2 && std::abs(i) % 10 <= 9 && (std::abs(i) % 100 < 11 || std::abs(i) % 100 > 19))
			return text::few;
		else if(f != 0)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_4ru(int64_t i, int64_t , int32_t v) {
		if(v == 0 && i % 10 == 1 && i % 100 != 11)
			return text::one;
		else if(v == 0 && (std::abs(i) % 10 == 2 || std::abs(i) % 10 == 3 || std::abs(i) % 10 == 4) && std::abs(i) % 100 != 12 && std::abs(i) % 100 != 13 && std::abs(i) % 100 != 14)
			return text::few;
		else if(v == 0 && i % 10 == 0)
			return text::many;
		else if(v == 0 && i % 10 >= 5 && i % 10 <= 9)
			return text::many;
		else if(v == 0 && (i % 100 == 11 || i % 100 == 12 || i % 100 == 13 || i % 100 == 14))
			return text::many;
		else
			return text::other;
	}

	attribute_type card_5br(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && std::abs(i) % 10 == 1 && std::abs(i) % 100 != 11 && std::abs(i) % 100 != 71 && std::abs(i) % 100 != 91)
			return text::one;
		else if(f == 0 && std::abs(i) % 10 == 2 && std::abs(i) % 100 != 12 && std::abs(i) % 100 != 72 && std::abs(i) % 100 != 92)
			return text::two;
		else if(f == 0 && (std::abs(i) % 10 == 3 || std::abs(i) % 10 == 4 || std::abs(i) % 10 == 9) && (std::abs(i) % 100 < 10 || (std::abs(i) % 100 > 19 && std::abs(i) % 100 < 70) || (std::abs(i) % 100 > 79 && std::abs(i) % 100 < 90)))
			return text::few;
		else if(f == 0 && i != 0 && i % 1000000 == 0)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_5mt(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && std::abs(i) == 1)
			return text::one;
		else if(f == 0 && std::abs(i) == 2)
			return text::two;
		else if(f == 0 && i == 0)
			return text::few;
		else if(f == 0 && std::abs(i) % 100 >= 3 && std::abs(i) % 100 <= 10)
			return text::few;
		else if(f == 0 && std::abs(i) % 100 >= 11 && std::abs(i) % 100 <= 19)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_5ga(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && std::abs(i) == 1)
			return text::one;
		else if(f == 0 && std::abs(i) == 2)
			return text::two;
		else if(f == 0 && std::abs(i) >= 3 && std::abs(i) <= 6)
			return text::few;
		else if(f == 0 && std::abs(i) >= 7 && std::abs(i) <= 10)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_5gv(int64_t i, int64_t , int32_t v) {
		if(v == 0 && i % 10 == 1)
			return text::one;
		else if(v == 0 && i % 10 == 2)
			return text::two;
		else if(v == 0 && (i % 100 == 0 || i % 100 == 20 || i % 100 == 40 || i % 100 == 60 || i % 100 == 80))
			return text::few;
		else if(v != 0)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_6kw(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && i == 0)
			return text::zero;
		else if(f == 0 && std::abs(i) == 1)
			return text::one;
		else if(f == 0 && (std::abs(i) % 100 == 2 || std::abs(i) % 100 == 22 || std::abs(i) % 100 == 42 || std::abs(i) % 100 == 62 || std::abs(i) % 100 == 82))
			return text::two;
		else if(f == 0 && std::abs(i) % 1000 == 0
			&& ((std::abs(i) % 100000 >= 1000 && std::abs(i) % 100000 <= 20000) || std::abs(i) % 100000 == 40000 || std::abs(i) % 100000 == 60000 || std::abs(i) % 100000 == 80000))
			return text::two;
		else if(f == 0 && std::abs(i) % 1000000 == 100000)
			return text::two;
		else if(f == 0 && (std::abs(i) % 100 == 3 || std::abs(i) % 100 == 23 || std::abs(i) % 100 == 43 || std::abs(i) % 100 == 63 || std::abs(i) % 100 == 83))
			return text::few;
		else if(f == 0 && std::abs(i) != 1 && (std::abs(i) % 100 == 1 || std::abs(i) % 100 == 21 || std::abs(i) % 100 == 41 || std::abs(i) % 100 == 61 || std::abs(i) % 100 == 81))
			return text::many;
		else
			return text::other;
	}

	attribute_type card_6ar(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && i == 0)
			return text::zero;
		else if(f == 0 && std::abs(i) == 1)
			return text::one;
		else if(f == 0 && std::abs(i) == 2)
			return text::two;
		else if(f == 0 && std::abs(i) % 100 >= 3 && std::abs(i) % 100 <= 10)
			return text::few;
		else if(f == 0 && std::abs(i) % 100 >= 11 && std::abs(i) % 100 <= 99)
			return text::many;
		else
			return text::other;
	}

	attribute_type card_6cy(int64_t i, int64_t f, int32_t ) {
		if(f == 0 && i == 0)
			return text::zero;
		else if(f == 0 && std::abs(i) == 1)
			return text::one;
		else if(f == 0 && std::abs(i) == 2)
			return text::two;
		else if(f == 0 && std::abs(i) == 3)
			return text::few;
		else if(f == 0 && std::abs(i) == 6)
			return text::many;
		else
			return text::other;
	}

	attribute_type ord_2sv(int64_t i) {
		if((std::abs(i) % 10 == 1 || std::abs(i) % 10 == 2) && std::abs(i) % 100 != 11 && std::abs(i) % 100 != 12)
			return text::ord_one;
		else
			return text::ord_other;
	}

	attribute_type ord_2bal(int64_t i) {
		if(std::abs(i) == 1)
			return text::ord_one;
		else
			return text::ord_other;
	}

	attribute_type ord_2hu(int64_t i) {
		if(std::abs(i) == 1 || std::abs(i) == 5)
			return text::ord_one;
		else
			return text::ord_other;
	}

	attribute_type ord_2ne(int64_t i) {
		if(std::abs(i) >= 1 && std::abs(i) <= 4)
			return text::ord_one;
		else
			return text::ord_other;
	}

	attribute_type ord_2be(int64_t i) {
		if((std::abs(i) % 10 == 2 || std::abs(i) % 10 == 3) && std::abs(i) % 100 != 12 && std::abs(i) % 100 != 13)
			return text::ord_few;
		else
			return text::ord_other;
	}

	attribute_type ord_2uk(int64_t i) {
		if(std::abs(i) % 10 == 3 && std::abs(i) % 100 != 13)
			return text::ord_few;
		else
			return text::ord_other;
	}

	attribute_type ord_2tk(int64_t i) {
		if(std::abs(i) % 10 == 9 || std::abs(i) % 10 == 6 || std::abs(i) == 10)
			return text::ord_few;
		else
			return text::ord_other;
	}

	attribute_type ord_2kk(int64_t i) {
		if(std::abs(i) % 10 == 9 || std::abs(i) % 10 == 6 || (std::abs(i) % 10 == 0 && i != 0))
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_2it(int64_t i) {
		if(std::abs(i) == 11 || std::abs(i) == 8 || std::abs(i) == 80 || std::abs(i) == 800)
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_2lij(int64_t i) {
		if(std::abs(i) == 11 || std::abs(i) == 8 || (std::abs(i) >= 80 && std::abs(i) <= 89) || (std::abs(i) >= 800 && std::abs(i) <= 899))
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_3ka(int64_t i) {
		if(i == 1)
			return text::ord_one;
		else if(i == 0 || i % 100 == 40 || i % 100 == 60 || i % 100 == 80 || (i % 100 >= 2 && i % 100 <= 20))
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_3sq(int64_t i) {
		if(std::abs(i) == 1)
			return text::ord_one;
		else if(std::abs(i) % 10 == 4 && std::abs(i) % 100 != 14)
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_3kw(int64_t i) {
		if(std::abs(i) >= 1 && std::abs(i) <= 4)
			return text::ord_one;
		else if(std::abs(i) % 100 >= 1 && std::abs(i) % 100 <= 4)
			return text::ord_one;
		else if(std::abs(i) % 100 >= 21 && std::abs(i) % 100 <= 24)
			return text::ord_one;
		else if(std::abs(i) % 100 >= 41 && std::abs(i) % 100 <= 44)
			return text::ord_one;
		else if(std::abs(i) % 100 >= 61 && std::abs(i) % 100 <= 64)
			return text::ord_one;
		else if(std::abs(i) % 100 >= 81 && std::abs(i) % 100 <= 84)
			return text::ord_one;
		else if(std::abs(i) == 5 || std::abs(i) % 100 == 5)
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_4en(int64_t i) {
		if(std::abs(i) % 10 == 1 && std::abs(i) % 100 != 11)
			return text::ord_one;
		else if(std::abs(i) % 10 == 2 && std::abs(i) % 100 != 12)
			return text::ord_two;
		else if(std::abs(i) % 10 == 3 && std::abs(i) % 100 != 13)
			return text::ord_few;
		else
			return text::ord_other;
	}

	attribute_type ord_4mr(int64_t i) {
		if(std::abs(i) == 1)
			return text::ord_one;
		else if(std::abs(i) == 2 || std::abs(i) == 3)
			return text::ord_two;
		else if(std::abs(i) == 4)
			return text::ord_few;
		else
			return text::ord_other;
	}

	attribute_type ord_4gd(int64_t i) {
		if(std::abs(i) == 1 || std::abs(i) == 11)
			return text::ord_one;
		else if(std::abs(i) == 2 || std::abs(i) == 12)
			return text::ord_two;
		else if(std::abs(i) == 3 || std::abs(i) == 13)
			return text::ord_few;
		else
			return text::ord_other;
	}

	attribute_type ord_4ca(int64_t i) {
		if(std::abs(i) == 1 || std::abs(i) == 3)
			return text::ord_one;
		else if(std::abs(i) == 2)
			return text::ord_two;
		else if(std::abs(i) == 4)
			return text::ord_few;
		else
			return text::ord_other;
	}

	attribute_type ord_4mk(int64_t i) {
		if(i % 10 == 1 && i % 100 != 11)
			return text::ord_one;
		else if(i % 10 == 2 && i % 100 != 12)
			return text::ord_two;
		else if((i % 10 == 7 || i % 10 == 8) && i % 100 != 17 && i % 100 != 18)
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_4az(int64_t i) {
		if(i % 10 == 1 || i % 10 == 2 || i % 10 == 5 || i % 10 == 7 || i % 10 == 8 || i % 100 == 20 || i % 100 == 50 || i % 100 == 70 || i % 100 == 80)
			return text::ord_one;
		else if(i % 10 == 3 || i % 10 == 4 || i % 1000 == 100 || i % 1000 == 200 || i % 1000 == 300 || i % 1000 == 400 || i % 1000 == 500 || i % 1000 == 600 || i % 1000 == 700 || i % 1000 == 800 || i % 1000 == 900)
			return text::ord_few;
		else if(i == 0 || i % 10 == 6 || i % 100 == 40 || i % 100 == 60 || i % 100 == 90)
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_5gu(int64_t i) {
		if(std::abs(i) == 1)
			return text::ord_one;
		else if(std::abs(i) == 2 || std::abs(i) == 3)
			return text::ord_two;
		else if(std::abs(i) == 4)
			return text::ord_few;
		else if(std::abs(i) == 6)
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_5as(int64_t i) {
		if(std::abs(i) == 1 || std::abs(i) == 5 || std::abs(i) == 7 || std::abs(i) == 8 || std::abs(i) == 9 || std::abs(i) == 10)
			return text::ord_one;
		else if(std::abs(i) == 2 || std::abs(i) == 3)
			return text::ord_two;
		else if(std::abs(i) == 4)
			return text::ord_few;
		else if(std::abs(i) == 6)
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_5or(int64_t i) {
		if(std::abs(i) == 1 || std::abs(i) == 5 || std::abs(i) == 7 || std::abs(i) == 8 || std::abs(i) == 9)
			return text::ord_one;
		else if(std::abs(i) == 2 || std::abs(i) == 3)
			return text::ord_two;
		else if(std::abs(i) == 4)
			return text::ord_few;
		else if(std::abs(i) == 6)
			return text::ord_many;
		else
			return text::ord_other;
	}

	attribute_type ord_6cy(int64_t i) {
		if(i == 0 || std::abs(i) == 7 || std::abs(i) == 8 || std::abs(i) == 9)
			return text::ord_zero;
		else if(std::abs(i) == 1)
			return text::ord_one;
		else if(std::abs(i) == 2)
			return text::ord_two;
		else if(std::abs(i) == 4 || std::abs(i) == 3)
			return text::ord_few;
		else if(std::abs(i) == 6 || std::abs(i) == 5)
			return text::ord_many;
		else
			return text::ord_other;
	}

	void parse_into_cardinality_fns(std::unordered_map<std::wstring_view, cardinal_plural_fn> & map, std::wstring_view list, cardinal_plural_fn fn) {

		for(size_t pos = 0; pos < list.length(); pos = list.find(' ', pos)) {
			if(pos != 0)
				++pos;
			auto next = std::min(list.find(L' ', pos), list.length());
			map.insert_or_assign(list.substr(pos, next - pos), fn);
		}
	}

	void parse_into_ordinal_fns(std::unordered_map<std::wstring_view, ordinal_plural_fn>& map, std::wstring_view list, ordinal_plural_fn fn) {

		for(size_t pos = 0; pos < list.length(); pos = list.find(' ', pos)) {
			if(pos != 0)
				++pos;
			auto next = std::min(list.find(L' ', pos), list.length());
			map.insert_or_assign(list.substr(pos, next - pos), fn);
		}
	}

	text_manager::text_manager() {
		parse_into_cardinality_fns(cardinal_functions, L"am as bn doi fa gu hi kn pcm zu", card_2am);
		parse_into_cardinality_fns(cardinal_functions, L"ff hy kab", card_2ff);
		parse_into_cardinality_fns(cardinal_functions, L"ast de en et fi fy gl ia io ji lij nl sc scn sv sw ur yi", card_2ast);
		cardinal_functions.insert_or_assign(L"si", card_2si);
		parse_into_cardinality_fns(cardinal_functions, L"ak bho guw ln mg nso pa ti wa", card_2ak);
		cardinal_functions.insert_or_assign(L"tzm", card_2tzm);
		parse_into_cardinality_fns(cardinal_functions, L"af an asa az bal bem bez bg brx ce cgg chr ckb dv ee el eo eu fo fur gsw ha haw hu jgo jmc ka kaj kcg kk kkj kl ks ksb ku ky lb lg mas mgo ml mn mr nah nb nd ne nn nnh no nr ny nyn om or os pap ps rm rof rwk saq sd sdh seh sn so sq ss ssy st syr ta te teo tig tk tn tr ts ug uz ve vo vun wae xh xog", card_2af);
		cardinal_functions.insert_or_assign(L"da", card_2da);
		cardinal_functions.insert_or_assign(L"is", card_2is);
		cardinal_functions.insert_or_assign(L"mk", card_2mk);
		parse_into_cardinality_fns(cardinal_functions, L"ceb fil tl", card_2ceb);

		parse_into_cardinality_fns(cardinal_functions, L"lv prg", card_3lv);
		cardinal_functions.insert_or_assign(L"lag", card_3lag);
		cardinal_functions.insert_or_assign(L"ksh", card_3ksh);
		parse_into_cardinality_fns(cardinal_functions, L"he iw", card_3he);
		parse_into_cardinality_fns(cardinal_functions, L"iu naq sat se sma smi smj smn sms", card_3iu);

		cardinal_functions.insert_or_assign(L"shi", card_3shi);
		parse_into_cardinality_fns(cardinal_functions, L"mo ro", card_3mo);
		parse_into_cardinality_fns(cardinal_functions, L"bs hr sh sr", card_3bs);

		cardinal_functions.insert_or_assign(L"fr", card_3fr);
		cardinal_functions.insert_or_assign(L"pt", card_3pt);
		parse_into_cardinality_fns(cardinal_functions, L"ca it pt-PT vec", card_3ca);
		cardinal_functions.insert_or_assign(L"es", card_3es);

		cardinal_functions.insert_or_assign(L"gd", card_4gd);
		cardinal_functions.insert_or_assign(L"sl", card_4sl);
		parse_into_cardinality_fns(cardinal_functions, L"dsb hsb", card_4dsb);

		parse_into_cardinality_fns(cardinal_functions, L"cs sk", card_4cs);
		cardinal_functions.insert_or_assign(L"pl", card_4pl);
		cardinal_functions.insert_or_assign(L"be", card_4be);
		cardinal_functions.insert_or_assign(L"lt", card_4lt);
		parse_into_cardinality_fns(cardinal_functions, L"ru uk", card_4ru);

		cardinal_functions.insert_or_assign(L"br", card_5br);
		cardinal_functions.insert_or_assign(L"mt", card_5mt);
		cardinal_functions.insert_or_assign(L"ga", card_5ga);
		cardinal_functions.insert_or_assign(L"gv", card_5gv);

		cardinal_functions.insert_or_assign(L"kw", card_6kw);
		parse_into_cardinality_fns(cardinal_functions, L"ar ars", card_6ar);
		cardinal_functions.insert_or_assign(L"cy", card_6cy);


		ordinal_functions.insert_or_assign(L"sv", ord_2sv);
		parse_into_ordinal_fns(ordinal_functions, L"bal fil fr ga hy lo mo ms ro tl vi", ord_2bal);
		ordinal_functions.insert_or_assign(L"hu", ord_2hu);
		ordinal_functions.insert_or_assign(L"ne", ord_2ne);

		ordinal_functions.insert_or_assign(L"be", ord_2be);
		ordinal_functions.insert_or_assign(L"uk", ord_2uk);
		ordinal_functions.insert_or_assign(L"tk", ord_2tk);

		ordinal_functions.insert_or_assign(L"kk", ord_2kk);
		parse_into_ordinal_fns(ordinal_functions, L"it sc scn vec", ord_2it);
		ordinal_functions.insert_or_assign(L"lij", ord_2lij);

		ordinal_functions.insert_or_assign(L"ka", ord_3ka);
		ordinal_functions.insert_or_assign(L"sq", ord_3sq);
		ordinal_functions.insert_or_assign(L"kw", ord_3kw);

		ordinal_functions.insert_or_assign(L"en", ord_4en);
		ordinal_functions.insert_or_assign(L"mr", ord_4mr);
		ordinal_functions.insert_or_assign(L"gd", ord_4gd);
		ordinal_functions.insert_or_assign(L"ca", ord_4ca);

		ordinal_functions.insert_or_assign(L"mk", ord_4mk);

		ordinal_functions.insert_or_assign(L"az", ord_4az);

		parse_into_ordinal_fns(ordinal_functions, L"gu hi", ord_5gu);
		parse_into_ordinal_fns(ordinal_functions, L"as bn", ord_5as);
		ordinal_functions.insert_or_assign(L"or", ord_5or);

		ordinal_functions.insert_or_assign(L"cy", ord_6cy);


		register_name("ui_settings_name", ::text_id::ui_settings_name);
		register_name("settings_header", ::text_id::settings_header);
		register_name("orientation_label", ::text_id::orientation_label);
		register_name("orientation_ltr", ::text_id::orientation_ltr);
		register_name("orientation_rtl", ::text_id::orientation_rtl);
		register_name("orientation_vltr", ::text_id::orientation_vltr);
		register_name("orientation_vrtl", ::text_id::orientation_vrtl);
		register_name("input_mode_label", ::text_id::input_mode_label);
		register_name("input_mode_keyboard_only", ::text_id::input_mode_keyboard_only);
		register_name("input_mode_mouse_only", ::text_id::input_mode_mouse_only);
		register_name("input_mode_controller_only", ::text_id::input_mode_controller_only);
		register_name("input_mode_controller_with_pointer", ::text_id::input_mode_controller_with_pointer);
		register_name("input_mode_mouse_and_keyboard", ::text_id::input_mode_mouse_and_keyboard);
		register_name("input_mode_follow_input", ::text_id::input_mode_follow_input);
		register_name("language_label", ::text_id::language_label);
		register_name("page_fraction", ::text_id::page_fraction);
		register_name("minimize_info", ::text_id::minimize_info);
		register_name("maximize_info", ::text_id::maximize_info);
		register_name("restore_info", ::text_id::restore_info);
		register_name("settings_info", ::text_id::settings_info);
		register_name("info_info", ::text_id::info_info);
		register_name("close_info", ::text_id::close_info);
		register_name("orientation_info", ::text_id::orientation_info);
		register_name("input_mode_info", ::text_id::input_mode_info);
		register_name("input_mode_mouse_info", ::text_id::input_mode_mouse_info);
		register_name("input_mode_automatic_info", ::text_id::input_mode_automatic_info);
		register_name("input_mode_controller_info", ::text_id::input_mode_controller_info);
		register_name("input_mode_controller_hybrid_info", ::text_id::input_mode_controller_hybrid_info);
		register_name("input_mode_keyboard_info", ::text_id::input_mode_keyboard_info);
		register_name("input_mode_mk_hybrid_info", ::text_id::input_mode_mk_hybrid_info);
		register_name("language_info", ::text_id::language_info);
		register_name("ui_settings_info", ::text_id::ui_settings_info);
		register_name("minimize_name", ::text_id::minimize_name);
		register_name("maximize_name", ::text_id::maximize_name);
		register_name("restore_name", ::text_id::restore_name);
		register_name("close_name", ::text_id::close_name);
		register_name("settings_name", ::text_id::settings_name);
		register_name("info_name", ::text_id::info_name);
		register_name("info_name_on", ::text_id::info_name_on);
		register_name("window_bar_name", ::text_id::window_bar_name);
		register_name("expandable_container_localized_name", ::text_id::expandable_container_localized_name);
		register_name("settings_tabs_name", ::text_id::settings_tabs_name);
		register_name("selection_list_localized_name", ::text_id::selection_list_localized_name);
		register_name("close_settings_name", ::text_id::close_settings_name);
		register_name("close_menu_name", ::text_id::close_menu_name);
		register_name("page_prev_name", ::text_id::page_prev_name);
		register_name("page_next_name", ::text_id::page_next_name);
		register_name("page_prev_prev_name", ::text_id::page_prev_prev_name);
		register_name("page_next_next_name", ::text_id::page_next_next_name);
		register_name("page_footer_name", ::text_id::page_footer_name);
		register_name("page_footer_info", ::text_id::page_footer_info);
		register_name("generic_toggle_on", ::text_id::generic_toggle_on);
		register_name("generic_toggle_off", ::text_id::generic_toggle_off);
		register_name("ui_animations_label", ::text_id::ui_animations_label);
		register_name("ui_animations_info", ::text_id::ui_animations_info);
		register_name("ui_scale", ::text_id::ui_scale);
		register_name("ui_scale_edit_name", ::text_id::ui_scale_edit_name);
		register_name("ui_scale_info", ::text_id::ui_scale_info);
		register_name("primary_font_label", ::text_id::primary_font_label);
		register_name("primary_font_info", ::text_id::primary_font_info);
		register_name("small_font_label", ::text_id::small_font_label);
		register_name("small_font_info", ::text_id::small_font_info);
		register_name("fonts_header", ::text_id::fonts_header);
		register_name("generic_toggle_yes", ::text_id::generic_toggle_yes);
		register_name("generic_toggle_no", ::text_id::generic_toggle_no);
		register_name("font_weight", ::text_id::font_weight);
		register_name("font_stretch", ::text_id::font_stretch);
		register_name("font_italic", ::text_id::font_italic);
		register_name("font_weight_edit_name", ::text_id::font_weight_edit_name);
		register_name("font_stretch_edit_name", ::text_id::font_stretch_edit_name);
		register_name("font_italic_info", ::text_id::font_italic_info);
		register_name("font_weight_info", ::text_id::font_weight_info);
		register_name("font_stretch_info", ::text_id::font_stretch_info);
		register_name("header_font_label", ::text_id::header_font_label);
		register_name("header_font_info", ::text_id::header_font_info);
		register_name("relative_size_label", ::text_id::relative_size_label);
		register_name("relative_size_edit_name", ::text_id::relative_size_edit_name);
		register_name("relative_size_small_info", ::text_id::relative_size_small_info);
		register_name("relative_size_header_info", ::text_id::relative_size_header_info);
		register_name("top_lead_label", ::text_id::top_lead_label);
		register_name("top_lead_edit_name", ::text_id::top_lead_edit_name);
		register_name("top_lead_edit_info", ::text_id::top_lead_edit_info);
		register_name("bottom_lead_label", ::text_id::bottom_lead_label);
		register_name("bottom_lead_edit_name", ::text_id::bottom_lead_edit_name);
		register_name("bottom_lead_edit_info", ::text_id::bottom_lead_edit_info);
		register_name("keyboard_header", ::text_id::keyboard_header);
		register_name("key_ord_name", ::text_id::key_ord_name);
		register_name("scan_code", ::text_id::scan_code);
		register_name("key_display_name", ::text_id::key_display_name);
		register_name("keyboard_arrangement", ::text_id::keyboard_arrangement);
		register_name("keyboard_left", ::text_id::keyboard_left);
		register_name("keyboard_right", ::text_id::keyboard_right);
		register_name("keyboard_tilted", ::text_id::keyboard_tilted);
		register_name("keyboard_custom", ::text_id::keyboard_custom);
		register_name("keyboard_arragnement_info", ::text_id::keyboard_arragnement_info);
		register_name("key_escape_name", ::text_id::key_escape_name);
		register_name("key_escape_info", ::text_id::key_escape_info);
		register_name("key_info_name", ::text_id::key_info_name);
		register_name("key_info_info", ::text_id::key_info_info);
		register_name("info_key_is_sticky", ::text_id::info_key_is_sticky);
		register_name("keyboard_code_info", ::text_id::keyboard_code_info);
		register_name("keyboard_display_name_info", ::text_id::keyboard_display_name_info);
		register_name("keyboard_display_edit_name", ::text_id::keyboard_display_edit_name);
		register_name("info_key_sticky_info", ::text_id::info_key_sticky_info);
	}

	UINT GetGrouping(WCHAR const* locale) {
		WCHAR wszGrouping[32];
		if(!GetLocaleInfoEx(locale, LOCALE_SGROUPING, wszGrouping, ARRAYSIZE(wszGrouping)))
			return 3;

		UINT nGrouping = 0;
		PWSTR pwsz = wszGrouping;
		for(;;) {
			if(*pwsz == '0') {
				break;
			} else if(iswdigit(*pwsz)) {
				nGrouping = nGrouping * 10 + (UINT)(*pwsz - '0');
			} else if(*pwsz == 0) {
				nGrouping = nGrouping * 10;
				break;
			}
			++pwsz;
		}
		return nGrouping;
	}
	
	std::wstring get_locale_name(window_data const& win, std::wstring const& directory) {
		auto result = win.file_system.find_matching_file_name(directory + L"\\*.dat");
		if(result.length() > 4) {
			result.pop_back();
			result.pop_back();
			result.pop_back();
			result.pop_back();
		} else {
			result.clear();
		}
		return result;
	}

	std::wstring text_manager::locale_display_name(window_data const& win) const {
		std::wstring locale_path = win.file_system.get_root_directory() + win.dynamic_settings.text_directory + L"\\" + locale_name();
		auto result = win.file_system.find_matching_file_name(locale_path + L"\\*.dat");
		if(result.length() > 4) {
			result.pop_back();
			result.pop_back();
			result.pop_back();
			result.pop_back();
			return result;
		} else {
			return locale_name();
		}
	}

	void text_manager::update_with_new_locale(window_data& win, bool update_settings) {
		std::wstring full_compound = app_region.size() != 0 ? app_lang + L"-" + app_region : app_lang;

		if(auto it = cardinal_functions.find(full_compound); it != cardinal_functions.end()) {
			cardinal_classification = it->second;
		} else if(auto itb = cardinal_functions.find(app_lang); itb != cardinal_functions.end()) {
			cardinal_classification = itb->second;
		} else {
			cardinal_classification = [](int64_t, int64_t, int32_t) { return text::other; };
		}

		if(auto it = ordinal_functions.find(full_compound); it != ordinal_functions.end()) {
			ordinal_classification = it->second;
		} else if(auto itb = ordinal_functions.find(app_lang); itb != ordinal_functions.end()) {
			ordinal_classification = itb->second;
		} else {
			ordinal_classification = [](int64_t) { return text::ord_other; };
		}


		{
			DWORD temp = 0;
			GetLocaleInfoEx(os_locale_is_default ? LOCALE_NAME_USER_DEFAULT : full_compound.c_str(), LOCALE_ILZERO | LOCALE_RETURN_NUMBER, (LPWSTR)&temp, sizeof(temp) / sizeof(WCHAR));
			LeadingZero = temp;
		}
		{
			DWORD temp = 0;
			GetLocaleInfoEx(os_locale_is_default ? LOCALE_NAME_USER_DEFAULT : full_compound.c_str(), LOCALE_INEGNUMBER | LOCALE_RETURN_NUMBER, (LPWSTR)&temp, sizeof(temp) / sizeof(WCHAR));
			NegativeOrder = temp;
		}

		Grouping = GetGrouping(os_locale_is_default ? LOCALE_NAME_USER_DEFAULT : full_compound.c_str());

		GetLocaleInfoEx(os_locale_is_default ? LOCALE_NAME_USER_DEFAULT : full_compound.c_str(), LOCALE_SDECIMAL, lpDecimalSep, 5);
		GetLocaleInfoEx(os_locale_is_default ? LOCALE_NAME_USER_DEFAULT : full_compound.c_str(), LOCALE_STHOUSAND, lpThousandSep, 5);
		
		lcid = os_locale_is_default ? LocaleNameToLCID(LOCALE_NAME_USER_DEFAULT, 0) : LocaleNameToLCID(full_compound.c_str(), LOCALE_ALLOW_NEUTRAL_NAMES);


		{
			std::wstring locale_path = win.file_system.get_root_directory() + win.dynamic_settings.text_directory + L"\\" + full_compound;
			win.dynamic_settings.fallbacks.clear();
			if(update_settings) {
				win.load_locale_settings(locale_path);
			} else {
				win.load_locale_fonts(locale_path);
			}

			auto locale_name = get_locale_name(win,locale_path);
			win.window_bar.print_ui_settings.lang_menu.open_button.set_text(win, locale_name.length() > 0 ? locale_name : full_compound);
		}

		win.dynamic_settings.fallbacks.clear();
		win.text_interface.initialize_font_fallbacks(win);
		win.text_interface.initialize_fonts(win);

		// load text from files
		populate_text_content(win);

		auto window_title_id = text_id_from_name("window_title").id;
		if(window_title_id != uint32_t(-1)) {
			auto title_text = instantiate_text(uint16_t(window_title_id)).text_content.text;
			win.set_window_title(title_text);
		} else {
			win.set_window_title(L"");
		}

		win.change_orientation(win.dynamic_settings.preferred_orientation);
	}

	void text_manager::update_fonts(window_data& win) {
		win.text_interface.initialize_fonts(win);
		win.change_orientation(win.dynamic_settings.preferred_orientation);
	}


	void text_manager::change_locale(std::wstring const& lang, std::wstring const& region, window_data& win, bool update_settings) {
		app_lang = lang;
		app_region = region;

		std::wstring full_compound = app_region.size() != 0 ? app_lang + L"-" + app_region : app_lang;

		if(IsValidLocaleName(full_compound.c_str())) {
			os_locale = full_compound;
			os_locale_is_default = false;
		} else {
			os_locale_is_default = true;
		}

		update_with_new_locale(win, update_settings);
		win.dynamic_settings.locale_is_default = false;
	}

	void text_manager::default_locale(window_data& win, bool update_settings) {

		WCHAR buffer[LOCALE_NAME_MAX_LENGTH] = { 0 };
		GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH);

		size_t first_dash = 0;
		for(; buffer[first_dash] != 0; ++first_dash) {
			if(buffer[first_dash] == L'-')
				break;
		}
		size_t underscore_before_dash = 0;
		for(; underscore_before_dash < first_dash; ++underscore_before_dash) {
			if(buffer[underscore_before_dash] == L'_') {
				break;
			}
		}

		app_lang = std::wstring(buffer, buffer + underscore_before_dash);

		size_t underscore_after_dash = buffer[first_dash] != 0 ? first_dash + 1 : first_dash;
		for(; buffer[underscore_after_dash] != 0; ++underscore_after_dash) {
			if(buffer[underscore_after_dash] == L'_') {
				break;
			}
		}

		app_region = buffer[first_dash] != 0
			? std::wstring(buffer + first_dash + 1, buffer + underscore_after_dash)
			: std::wstring();


		os_locale = buffer;
		os_locale_is_default = true;


		update_with_new_locale(win, update_settings);
	}


	struct content_and_remainder {
		std::string_view content;
		std::string_view remainder;
		bool insert_space;
		bool insert_linebreak;
	};

	content_and_remainder consume_token(std::string_view in) {
		content_and_remainder result;

		uint32_t i = 0;

		if(in.length() > 0 && (in[0] == '}' || in[0] == '{')) {
			result.content = in.substr(0, 1);
			i = 1;

		} else {
			for(; i < in.length(); ++i) {
				if(in[i] != ' ' && in[i] != '\t' && in[i] != '\r' && in[i] != '\n' && in[i] != '_') {
					break;
				}
			}

			auto const start = i;

			for(; i < in.length(); ++i) {
				if(in[i] == ' ' || in[i] == '\t' || in[i] == '\r' || in[i] == '\n' || in[i] == '_' || in[i] == '{' || in[i] == '}') {
					break;
				}
			}

			result.content = in.substr(start, i - start);
		}

		if(i == in.length() || in[i] == '_' || in[i] == '{' || in[i] == '}') {
			result.insert_space = false;
		} else {
			result.insert_space = true;
		}

		result.insert_linebreak = false;
		for(; i < in.length(); ++i) {
			if(in[i] == '\n')
				result.insert_linebreak = true;
			if(in[i] != ' ' && in[i] != '\t' && in[i] != '\r' && in[i] != '\n' && in[i] != '_') {
				break;
			}
		}

		result.remainder = in.substr(i);
		return result;
	}

	std::string_view text_data_storage::parse_match_conditions(std::string_view body) {
		body = consume_token(body).remainder; // eat first token

		while(body.length() > 0) {
			auto consumed_token = consume_token(body);
			body = consumed_token.remainder;

			if(consumed_token.content.length() == 1 && consumed_token.content[0] == '}')
				return body;

			if(consumed_token.content.length() >= 3 && std::isdigit(consumed_token.content[0]) && consumed_token.content[0] != '0') {
				auto parameter_id = consumed_token.content[0] - '1';
				auto rem_string = consumed_token.content.substr(2); // skip dot

				if(rem_string == "zero") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::zero, uint8_t(parameter_id) });
				} else if(rem_string == "one") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::one, uint8_t(parameter_id) });
				} else if(rem_string == "two") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::two, uint8_t(parameter_id) });
				} else if(rem_string == "few") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::few, uint8_t(parameter_id) });
				} else if(rem_string == "many") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::many, uint8_t(parameter_id) });
				} else if(rem_string == "other") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::other, uint8_t(parameter_id) });
				} else if(rem_string == "ord-zero") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::ord_zero, uint8_t(parameter_id) });
				} else if(rem_string == "ord-one") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::ord_one, uint8_t(parameter_id) });
				} else if(rem_string == "ord-two") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::ord_two, uint8_t(parameter_id) });
				} else if(rem_string == "ord-few") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::ord_few, uint8_t(parameter_id) });
				} else if(rem_string == "ord-many") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::ord_many, uint8_t(parameter_id) });
				} else if(rem_string == "ord-other") {
					this->match_keys_storage.push_back(param_attribute_pair{ text::ord_other, uint8_t(parameter_id) });
				} else {
					if(auto it = attribute_name_to_id.find(rem_string); it != attribute_name_to_id.end()) {
						match_keys_storage.push_back(param_attribute_pair{ it->second, uint8_t(parameter_id) });
					} else {
						++last_attribute_id_mapped;
						attribute_name_to_id.insert_or_assign(std::string(rem_string), last_attribute_id_mapped);
						match_keys_storage.push_back(param_attribute_pair{ last_attribute_id_mapped, uint8_t(parameter_id) });
					}
				}
			}
		}
		return body;
	}

	std::string_view text_data_storage::assemble_entry_content(std::string_view body, std::unordered_map<std::string, uint32_t, string_hash, std::equal_to<>> const& font_name_to_index, bool allow_match) {
		
		size_t last_text_start = 0;
		size_t formatting_start = 0;
		uint8_t group = 0;

		bool previous_insert_space = false;
		bool previous_insert_line = false;
		size_t current_matcher_index = size_t(-2);

		std::vector<formatting_content> formatting_stack;

		auto restart_match_body = [&]() {
			if(current_matcher_index + 1 != static_matcher_storage.size()) {
				last_text_start = codepoint_storage.length();
				formatting_start = static_format_storage.size();

				if(!static_matcher_storage.empty()) {
					static_matcher_storage.back().base_text.code_points_count = uint16_t(last_text_start - static_matcher_storage.back().base_text.code_points_start);
					static_matcher_storage.back().base_text.formatting_end = uint16_t(formatting_start);
					group = static_matcher_storage.back().group + 1;
				}

				current_matcher_index = static_matcher_storage.size();
				static_matcher_storage.emplace_back();

				static_matcher_storage[current_matcher_index].base_text.code_points_start = uint32_t(last_text_start);
				static_matcher_storage[current_matcher_index].base_text.formatting_start = uint16_t(formatting_start);
				static_matcher_storage[current_matcher_index].group = group;
			}
		};
		auto append_spaces = [&](content_and_remainder const& st) { 
			if(previous_insert_line && previous_insert_space) {
				restart_match_body();
				codepoint_storage += L'\n';
			} else if(previous_insert_space) {
				restart_match_body();
				codepoint_storage += L' ';
			}
			previous_insert_line = st.insert_linebreak;
			previous_insert_space = st.insert_space;
		};
		auto update_spaces = [&](content_and_remainder const& st) {
			previous_insert_line = st.insert_linebreak;
			previous_insert_space = st.insert_space;
		};

		while(body.length() > 0) {
			auto t = consume_token(body);

			if(t.content.length() == 1 && t.content[0] == '}' && formatting_stack.empty()) {
				if(current_matcher_index + 1 == static_matcher_storage.size()) {
					static_matcher_storage.back().base_text.code_points_count = uint16_t(codepoint_storage.length() - static_matcher_storage.back().base_text.code_points_start);
					static_matcher_storage.back().base_text.formatting_end = uint16_t(static_format_storage.size());
				}
				return body;
			}

			body = t.remainder;
			
			if(t.content.length() == 0) {
				// do nothing
			} else if(t.content.length() == 1 && t.content[0] == '}') {
				restart_match_body();
				static_format_storage.emplace_back(format_marker{ uint16_t(codepoint_storage.length() - last_text_start), formatting_stack.back() });
				formatting_stack.pop_back();
				update_spaces(t);
			} else {
				append_spaces(t);
				if(t.content != "\\match") {
					restart_match_body();
				}

				if(t.content[0] != '\\') {
					WCHAR* temp_buffer = new WCHAR[t.content.length() * 2];

					auto chars_written = MultiByteToWideChar(
						CP_UTF8,
						MB_PRECOMPOSED,
						t.content.data(),
						int32_t(t.content.length()),
						temp_buffer,
						int32_t(t.content.length()) * 2
					);
					codepoint_storage.append(temp_buffer, chars_written);

					delete[] temp_buffer;
				} else if(t.content == "\\it") {
					auto next_token = consume_token(body);
					if(next_token.content.length() == 1 && next_token.content[0] == '{') {
						body = next_token.remainder;
						static_format_storage.emplace_back(format_marker{ uint16_t(codepoint_storage.length() - last_text_start), extra_formatting::italic });
						formatting_stack.push_back(extra_formatting::italic);
						previous_insert_space = false;
					}
				} else if(t.content == "\\b") {
					auto next_token = consume_token(body);
					if(next_token.content.length() == 1 && next_token.content[0] == '{') {
						body = next_token.remainder;
						static_format_storage.emplace_back(format_marker{ uint16_t(codepoint_storage.length() - last_text_start), extra_formatting::bold });
						formatting_stack.push_back(extra_formatting::bold);
						previous_insert_space = false;
					}
				} else if(t.content == "\\sc") {
					auto next_token = consume_token(body);
					if(next_token.content.length() == 1 && next_token.content[0] == '{') {
						body = next_token.remainder;
						static_format_storage.emplace_back(format_marker{ uint16_t(codepoint_storage.length() - last_text_start), extra_formatting::small_caps });
						formatting_stack.push_back(extra_formatting::small_caps);
						previous_insert_space = false;
					}
				} else if(t.content == "\\os") {
					auto next_token = consume_token(body);
					if(next_token.content.length() == 1 && next_token.content[0] == '{') {
						body = next_token.remainder;
						static_format_storage.emplace_back(format_marker{ uint16_t(codepoint_storage.length() - last_text_start), extra_formatting::old_numbers });
						formatting_stack.push_back(extra_formatting::old_numbers);
						previous_insert_space = false;
					}
				} else if(t.content == "\\tb") {
					auto next_token = consume_token(body);
					if(next_token.content.length() == 1 && next_token.content[0] == '{') {
						body = next_token.remainder;
						static_format_storage.emplace_back(format_marker{ uint16_t(codepoint_storage.length() - last_text_start), extra_formatting::tabular_numbers });
						formatting_stack.push_back(extra_formatting::tabular_numbers);
						previous_insert_space = false;
					}
				} else if(t.content == "\\c") {
					auto next_token = consume_token(body);
					if(next_token.content.length() == 1 && next_token.content[0] == '{') {

						body = next_token.remainder;

						while(true) {
							auto content_token = consume_token(body);
							body = content_token.remainder;

							if(content_token.content.length() == 1 && content_token.content[0] == '}') {
								update_spaces(content_token);
								break;
							}
							if(content_token.content.length() == 0)
								break;

							if(content_token.content == "left-brace") {
								codepoint_storage += L'{';
							} else if(content_token.content == "right-brace") {
								codepoint_storage += L'}';
							} else if(std::isdigit(content_token.content[0])) {
								std::string temp(content_token.content);
								uint32_t val = std::strtoul(temp.c_str(), nullptr, 0);

								if(val < 0x10000) {
									codepoint_storage += wchar_t(val);
								} else {
									auto p = make_surrogate_pair(val);
									codepoint_storage += wchar_t(p.high);
									codepoint_storage += wchar_t(p.low);
								}
							} else if(content_token.content == "em-space") {
								codepoint_storage += wchar_t(0x2003);
							} else if(content_token.content == "en-space") {
								codepoint_storage += wchar_t(0x2002);
							} else if(content_token.content == "3rd-em") {
								codepoint_storage += wchar_t(0x2004);
							} else if(content_token.content == "4th-em") {
								codepoint_storage += wchar_t(0x2005);
							} else if(content_token.content == "6th-em") {
								codepoint_storage += wchar_t(0x2006);
							} else if(content_token.content == "thin-space") {
								codepoint_storage += wchar_t(0x2009);
							} else if(content_token.content == "hair-space") {
								codepoint_storage += wchar_t(0x200A);
							} else if(content_token.content == "figure-space") {
								codepoint_storage += wchar_t(0x2007);
							} else if(content_token.content == "ideo-space") {
								codepoint_storage += wchar_t(0x3000);
							} else if(content_token.content == "hyphen") {
								codepoint_storage += wchar_t(0x2010);
							} else if(content_token.content == "figure-dash") {
								codepoint_storage += wchar_t(0x2012);
							} else if(content_token.content == "en-dash") {
								codepoint_storage += wchar_t(0x2013);
							} else if(content_token.content == "em-dash") {
								codepoint_storage += wchar_t(0x2014);
							} else if(content_token.content == "minus") {
								codepoint_storage += wchar_t(0x2212);
							} else {
								WCHAR* temp_buffer = new WCHAR[content_token.content.length() * 2];

								auto chars_written = MultiByteToWideChar(
									CP_UTF8,
									MB_PRECOMPOSED,
									content_token.content.data(),
									int32_t(content_token.content.length()),
									temp_buffer,
									int32_t(content_token.content.length()) * 2
								);
								codepoint_storage.append(temp_buffer, chars_written);

								delete[] temp_buffer;
							}
						}

					}
				} else if(t.content.length() == 2 && std::isdigit(t.content[1]) && t.content[1] != '0') {
					static_format_storage.emplace_back(format_marker{ uint16_t(codepoint_storage.length() - last_text_start), parameter_id{ uint8_t(t.content[1] - '1') } });
					codepoint_storage += L'?';
				} else if(t.content == "\\match" && allow_match) {
					++group;
					for(; body.length() > 0 && body[0] == '{';) {

						auto condition_id = match_keys_storage.size();
						body = parse_match_conditions(body);
						auto conditions_count = match_keys_storage.size() - condition_id;

						if(body.length() > 0 && body[0] == '{') {
							auto id_before = static_matcher_storage.size();

							body = consume_token(body).remainder;
							body = assemble_entry_content(body, font_name_to_index, false);

							if(static_matcher_storage.size() != id_before) {
								static_matcher_storage.back().group = group;
								static_matcher_storage.back().match_key_start = uint16_t(condition_id);
								static_matcher_storage.back().num_keys = uint8_t(conditions_count);
							}

							auto end_token = consume_token(body);
							update_spaces(end_token);
							body = end_token.remainder;
						}
					}
					++group;

				} else { // other = font name
					uint8_t fid = 0;

					if(auto it = font_name_to_index.find(t.content.substr(1)); it != font_name_to_index.end()) {
						fid = uint8_t(it->second);
					}

					auto next_token = consume_token(body);
					if(next_token.content.length() == 1 && next_token.content[0] == '{') {
						body = next_token.remainder;
						static_format_storage.emplace_back(format_marker{ uint16_t(codepoint_storage.length() - last_text_start), font_id{fid} });
						formatting_stack.push_back(font_id{ fid });
						previous_insert_space = false;
					}
				}
			}
		} // while body not empty loop

		if(current_matcher_index + 1 == static_matcher_storage.size()) {
			static_matcher_storage.back().base_text.code_points_count = uint16_t(codepoint_storage.length() - static_matcher_storage.back().base_text.code_points_start);
			static_matcher_storage.back().base_text.formatting_end = uint16_t(static_format_storage.size());
		}
		return body;
	}

	struct top_level_cr {
		std::string_view content;
		std::string_view remainder;
	};

	top_level_cr consume_text_name(std::string_view in) {
		top_level_cr result;

		uint32_t i = 0;

		for(; i < in.length(); ++i) {
			if(in[i] != ' ' && in[i] != '\t' && in[i] != '\r' && in[i] != '\n') {
				break;
			}
		}

		auto const start = i;

		for(; i < in.length(); ++i) {
			if(in[i] == ' ' || in[i] == '\t' || in[i] == '\r' || in[i] == '\n' || in[i] == '{' || in[i] == '}') {
				break;
			}
		}

		result.content = in.substr(start, i - start);

		for(; i < in.length(); ++i) {
			if(in[i] != ' ' && in[i] != '\t' && in[i] != '\r' && in[i] != '\n') {
				break;
			}
		}

		result.remainder = in.substr(i);
		return result;
	}
	
	top_level_cr consume_braced_content(std::string_view in) {
		top_level_cr result;
		
		uint32_t i = 0;
		int32_t depth = 0;

		for(; i < in.length(); ++i) {
			if(in[i] == '{') {
				++depth;
			} else if(in[i] == '}') {
				--depth;
			}
			if(depth <= 0) {
				break;
			}
		}
		++i;

		result.content = in.substr(1, i > 1 ? (i - 2) : 0);

		for(; i < in.length(); ++i) {
			if(in[i] != ' ' && in[i] != '\t' && in[i] != '\r') {
				break;
			}
		}

		result.remainder = in.substr(i);
		return result;
	}

	std::array<attribute_type, replaceable_instance::max_attributes> text_data_storage::parse_attributes(std::string_view body) {
		std::array<attribute_type, replaceable_instance::max_attributes> result = {-1i16, -1i16, -1i16, -1i16, -1i16, -1i16, -1i16, -1i16};
		uint32_t used = 0;

		while(body.length() > 0 && used < replaceable_instance::max_attributes) {
			auto consumed_token = consume_token(body);
			body = consumed_token.remainder;


			if(consumed_token.content.length() != 0) {

				if(consumed_token.content == "zero") {
					result[used] = text::zero;
					++used;
				} else if(consumed_token.content == "one") {
					result[used] = text::one;
					++used;
				} else if(consumed_token.content == "two") {
					result[used] = text::two;
					++used;
				} else if(consumed_token.content == "few") {
					result[used] = text::few;
					++used;
				} else if(consumed_token.content == "many") {
					result[used] = text::many;
					++used;
				} else if(consumed_token.content == "other") {
					result[used] = text::other;
					++used;
				} else if(consumed_token.content == "ord-zero") {
					result[used] = text::ord_zero;
					++used;
				} else if(consumed_token.content == "ord-one") {
					result[used] = text::ord_one;
					++used;
				} else if(consumed_token.content == "ord-two") {
					result[used] = text::ord_two;
					++used;
				} else if(consumed_token.content == "ord-few") {
					result[used] = text::ord_few;
					++used;
				} else if(consumed_token.content == "ord-many") {
					result[used] = text::ord_many;
					++used;
				} else if(consumed_token.content == "ord-other") {
					result[used] = text::ord_other;
					++used;
				} else {
					if(auto it = attribute_name_to_id.find(consumed_token.content); it != attribute_name_to_id.end()) {
						result[used] = it->second;
						++used;
					} else {
						++last_attribute_id_mapped;
						attribute_name_to_id.insert_or_assign(std::string(consumed_token.content), last_attribute_id_mapped);
						result[used] = last_attribute_id_mapped;
						++used;
					}
				}
			}
		}
		return result;
	}

	std::string_view text_data_storage::consume_single_entry(std::string_view body, std::unordered_map<std::string, uint32_t, string_hash, std::equal_to<>> const& font_name_to_index) {
		top_level_cr entry_name;

		while(body.length() > 0) {
			entry_name = consume_text_name(body);
			body = entry_name.remainder;

			if(body.length() != 0 && body[0] == '{')
				break; //name begins some content
		}
		bool already_exists = false;
		uint16_t id = 0;
		if(auto it = internal_text_name_map.find(std::string(entry_name.content)); it == internal_text_name_map.end()) {
			stored_functions.emplace_back();
			id = uint16_t(stored_functions.size() - 1);
			internal_text_name_map.insert_or_assign(std::string(entry_name.content), id);
		} else {
			id = it->second;
			if(stored_functions[it->second].pattern_count != 0) {
				already_exists = true;
			}
		}

		auto first_content = consume_braced_content(body);
		if(first_content.remainder.length() > 0 && first_content.remainder[0] == '{') {
			auto second_content = consume_braced_content(first_content.remainder);

			if(!already_exists) {

				auto pre_entry_size = static_matcher_storage.size();
				assemble_entry_content(second_content.content, font_name_to_index, true);
				auto created_entries = static_matcher_storage.size() - pre_entry_size;
				if(created_entries != 0) {

					stored_functions[id].attributes = parse_attributes(first_content.content);
					stored_functions[id].begin_patterns = uint16_t(pre_entry_size);
					stored_functions[id].pattern_count = uint16_t(created_entries);
				}
			}

			return second_content.remainder;
		} else {
			if(first_content.content.length() > 0 && !already_exists) {
				auto pre_entry_size = static_matcher_storage.size();
				assemble_entry_content(first_content.content, font_name_to_index, true);
				auto created_entries = static_matcher_storage.size() - pre_entry_size;
				if(created_entries != 0) {
					stored_functions[id].begin_patterns = uint16_t(pre_entry_size);
					stored_functions[id].pattern_count = uint16_t(created_entries);
				}
			}
			return first_content.remainder;
		}

	}

	void text_data_storage::consume_text_file(std::string_view body, std::unordered_map<std::string, uint32_t, string_hash, std::equal_to<>> const& font_name_to_index) {
		while(body.length() > 0) {
			body = consume_single_entry(body, font_name_to_index);
		}
	}

	replaceable_instance text_manager::format_int(int64_t value, uint32_t decimal_places) const {
		replaceable_instance result;

		NUMBERFMTW fmt;
		fmt.Grouping = Grouping;
		fmt.LeadingZero = LeadingZero;
		fmt.lpDecimalSep = const_cast<wchar_t*>(lpDecimalSep);
		fmt.lpThousandSep = const_cast<wchar_t*>(lpThousandSep);
		fmt.NegativeOrder = NegativeOrder;
		fmt.NumDigits = decimal_places;

		auto value_string = std::to_wstring(value);
		WCHAR local_buffer[MAX_PATH] = {0};

		if(os_locale_is_default) {	
			GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, value_string.c_str(), &fmt, local_buffer, MAX_PATH);
			result.text_content.text = std::wstring(local_buffer);
		} else {
			GetNumberFormatEx(os_locale.c_str(), 0, value_string.c_str(), &fmt, local_buffer, MAX_PATH);
			result.text_content.text = std::wstring(local_buffer);
		}
		result.attributes[0] = cardinal_classification(value, 0, decimal_places);
		result.attributes[1] = ordinal_classification(value);
		return result;
	}
	replaceable_instance text_manager::format_double(double value, uint32_t decimal_places) const {
		replaceable_instance result;

		NUMBERFMTW fmt;
		fmt.Grouping = Grouping;
		fmt.LeadingZero = LeadingZero;
		fmt.lpDecimalSep = const_cast<wchar_t*>(lpDecimalSep);
		fmt.lpThousandSep = const_cast<wchar_t*>(lpThousandSep);
		fmt.NegativeOrder = NegativeOrder;
		fmt.NumDigits = decimal_places;
		
		if(std::isfinite(value)) {
			auto value_string = std::to_wstring(value);
			WCHAR local_buffer[MAX_PATH] = { 0 };

			if(os_locale_is_default) {
				GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, value_string.c_str(), &fmt, local_buffer, MAX_PATH);
				result.text_content.text = std::wstring(local_buffer);
			} else {
				GetNumberFormatEx(os_locale.c_str(), 0, value_string.c_str(), &fmt, local_buffer, MAX_PATH);
				result.text_content.text = std::wstring(local_buffer);
			}

			auto decimal_part = (value >= 0) ? (value - std::floor(value)) : (value - std::ceil(value));
			for(uint32_t i = 0; i < decimal_places; ++i) {
				decimal_part *= 10.0;
			}

			result.attributes[0] = cardinal_classification(int64_t(value), int64_t(std::nearbyint(decimal_places)), decimal_places);
			result.attributes[1] = ordinal_classification(int64_t(value));
		} else if(std::isnan(value)) {
			result.text_content.text = L"#NAN";
		} else { // infinite
			if(value > 0) {
				result.text_content.text = L"∞";
			} else {
				result.text_content.text = L"-∞";
			}
			result.attributes[0] = text::other;
			result.attributes[1] = text::ord_other;
		}

		return result;
	}

	wchar_t const* text_manager::locale_string() const {
		if(!os_locale_is_default && os_locale.length() != 0)
			return os_locale.c_str();
		else
			return nullptr;
	}

	std::wstring text_manager::locale_name() const {
		if(!os_locale_is_default && os_locale.length() != 0)
			return os_locale;
		else
			return app_region.size() != 0 ? app_lang + L"-" + app_region : app_lang;
	}

	bool text_manager::is_locale_default() const {
		return os_locale_is_default;
	}

	bool text_manager::is_current_locale(std::wstring const& lang, std::wstring const& region) const {
		return app_lang == lang && app_region == region;
	}

	double text_manager::text_to_double(wchar_t const* start, uint32_t count) const {
		auto olestr = SysAllocStringLen(start, count);
		double result = 0;
		if(olestr) {
			VarR8FromStr(olestr, lcid, 0, &result);
			SysFreeString(olestr);
		}
		return result;
	}
	int64_t text_manager::text_to_int(wchar_t const* start, uint32_t count) const {
		auto olestr = SysAllocStringLen(start, count);
		int64_t result = 0;
		if(olestr) {
			VarI8FromStr(olestr, lcid, 0, &result);
			SysFreeString(olestr);
		}
		return result;
		/*
		VARIANT var;

		VariantInit(&var);
		var.bstrVal = SysAllocStringLen(start, count);
		var.vt = VT_BSTR;

		HRESULT hr = VariantChangeTypeEx(&var, &var, lcid, 0, VT_R8);

		
			return = var.dblVal;

		VariantClear(&var);
		*/
	}

	void text_manager::load_text_from_directory(window_data const& win, std::wstring const& directory) {
		std::wstring txt = directory + L"\\*.txt";

		win.file_system.for_each_filtered_file(txt, [&](std::wstring const& name) {
			std::wstring fname = directory + L"\\" + name;
			win.file_system.with_file_content(fname, [&](std::string_view content) {
				text_data.consume_text_file(content, font_name_to_index);
			});
		});
	}

	void text_manager::populate_text_content(window_data const& win) {
		// reset all data 

		text_data.codepoint_storage.clear();
		text_data.static_format_storage.clear();
		text_data.static_matcher_storage.clear();
		text_data.match_keys_storage.clear();
		text_data.attribute_name_to_id.clear();
		text_data.last_attribute_id_mapped = last_predefined;
		++text_generation;

		for(auto& f : text_data.stored_functions) {
			f.attributes = std::array<attribute_type, replaceable_instance::max_attributes>{ -1i8, -1i8, -1i8, -1i8, -1i8, -1i8, -1i8, -1i8 };
			f.begin_patterns = 0;
			f.pattern_count = 0;
		}

		std::wstring full_name = app_lang + L"-" + app_region;


		WCHAR module_name[MAX_PATH] = {};
		int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);
		while(path_used >= 0 && module_name[path_used] != L'\\') {
			module_name[path_used] = 0;
			--path_used;
		}

		std::wstring desired_full_path = module_name + win.dynamic_settings.text_directory + L"\\" + full_name;
		std::wstring desired_abbr_path = module_name + win.dynamic_settings.text_directory + L"\\" + app_lang;

		bool full_valid = win.file_system.directory_exists(desired_full_path);
		bool abbr_valid = win.file_system.directory_exists(desired_abbr_path);

		if(full_valid)
			load_text_from_directory(win, desired_full_path);
		if(abbr_valid)
			load_text_from_directory(win, desired_abbr_path);

		if(!full_valid && !abbr_valid) {
			std::wstring en_path = module_name + win.dynamic_settings.text_directory + L"\\en";
			if(win.file_system.directory_exists(en_path)) {
				load_text_from_directory(win, en_path);
			} else {
				// now we just take the first option w/o a dash
				std::wstring base_path = module_name + win.dynamic_settings.text_directory;
				bool directory_found = false;
				win.file_system.for_each_directory(module_name + win.dynamic_settings.text_directory, [&](std::wstring const& name) { 
					if(!directory_found && name.find(L'-') == std::wstring::npos) {
						load_text_from_directory(win, base_path + L"\\" + name);
						directory_found = true;
					}
				});
			}
		}
	}

	text_id text_manager::text_id_from_name(std::string_view name) const {
		if(auto it = text_data.internal_text_name_map.find(std::string(name)); it != text_data.internal_text_name_map.end()) {
			return text_id{ it->second };
		} else {
			return text_id{ uint32_t(-1) };
		}
	}

	replaceable_instance text_manager::instantiate_text(uint16_t id, text_parameter const* s, text_parameter const* e) const {
		replaceable_instance result;
		if(id == uint16_t(-1)) {
			return result;
		} else {
			std::vector< replaceable_instance> parameters;
			parameters.reserve(e - s);

			for(uint32_t i = 0; i < e - s; ++i) {
				parameters.emplace_back(parameter_to_text(s[i]));
			}

			result = text_data.stored_functions[id].instantiate(text_data, parameters.data(), parameters.data() + (e - s));
			result.text_content.make_substitutions(parameters.data(), parameters.data() + (e - s));
			return result;
		}
	}
	replaceable_instance text_manager::instantiate_text(std::string_view key, text_parameter const* s, text_parameter const* e) const {
		if(auto it = text_data.internal_text_name_map.find(std::string(key)); it != text_data.internal_text_name_map.end()) {
			return instantiate_text(it->second, s, e);
		} else {
			return replaceable_instance{};
		}
	}

	void text_manager::register_name(std::string_view n, uint16_t id) {
		text_data.internal_text_name_map.insert_or_assign(std::string(n), id);
		if(text_data.stored_functions.size() <= id) {
			text_data.stored_functions.resize(id + 1);
		}
	}

	void apply_default_vertical_options(IDWriteTypography* t) {
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_GLYPH_COMPOSITION_DECOMPOSITION, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_LOCALIZED_FORMS, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_MARK_POSITIONING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_MARK_TO_MARK_POSITIONING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_REQUIRED_LIGATURES, 1 });

		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_VERTICAL_ALTERNATES_AND_ROTATION, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_VERTICAL_WRITING, 1 });
	}
	void apply_default_ltr_options(IDWriteTypography* t) {
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_GLYPH_COMPOSITION_DECOMPOSITION, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_LOCALIZED_FORMS, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_MARK_POSITIONING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_MARK_TO_MARK_POSITIONING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_REQUIRED_LIGATURES, 1 });

		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG(DWRITE_MAKE_OPENTYPE_TAG('l','t','r','a')), 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG(DWRITE_MAKE_OPENTYPE_TAG('l','t','r','m')), 1 });

		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_LIGATURES, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_CURSIVE_POSITIONING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_KERNING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG(DWRITE_MAKE_OPENTYPE_TAG('r','c','l','t')), 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG(DWRITE_MAKE_OPENTYPE_TAG('d','i','s','t')), 1 });
	}
	void apply_default_rtl_options(IDWriteTypography* t) {
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_GLYPH_COMPOSITION_DECOMPOSITION, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_LOCALIZED_FORMS, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_MARK_POSITIONING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_MARK_TO_MARK_POSITIONING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_REQUIRED_LIGATURES, 1 });

		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG(DWRITE_MAKE_OPENTYPE_TAG('r','t','l','a')), 1 });

		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_LIGATURES, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_CURSIVE_POSITIONING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_KERNING, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG(DWRITE_MAKE_OPENTYPE_TAG('r','c','l','t')), 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG(DWRITE_MAKE_OPENTYPE_TAG('d','i','s','t')), 1 });
	}

	void apply_old_style_figures_options(IDWriteTypography* t) {
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_OLD_STYLE_FIGURES, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_PROPORTIONAL_FIGURES, 1 });
	}
	void apply_lining_figures_options(IDWriteTypography* t) {
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_CASE_SENSITIVE_FORMS, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_LINING_FIGURES, 1 });
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_TABULAR_FIGURES, 1 });
	}

	void apply_small_caps_options(IDWriteTypography* t) {
		t->AddFontFeature(DWRITE_FONT_FEATURE{ DWRITE_FONT_FEATURE_TAG_SMALL_CAPITALS, 1 });
	}

	

	std::vector<language_description> ennumerate_languages(window_data const& win) {
		std::vector<language_description> result;

		WCHAR module_name[MAX_PATH] = {};
		int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);
		while(path_used >= 0 && module_name[path_used] != L'\\') {
			module_name[path_used] = 0;
			--path_used;
		}

		std::wstring locale_path = module_name + win.dynamic_settings.text_directory;

		win.file_system.for_each_directory(locale_path, [&](std::wstring const& name) {
			size_t first_dash = 0;
			size_t region_end = name.length();

			for(; first_dash < region_end; ++first_dash) {
				if(name[first_dash] == L'-')
					break;
			}

			auto locale_name = get_locale_name(win, locale_path + L"\\" + name);
			if(locale_name.length() > 0) {
				result.push_back(language_description{
					name.substr(0, first_dash), // language
					first_dash != region_end // region
						? name.substr(first_dash + 1) : std::wstring(),
					locale_name });
			}
		});

		return result;
	}


	bool codepoint_is_nonspacing(uint32_t c) noexcept {
		return u_charType(c) == U_NON_SPACING_MARK;
	}

	uint16_t single_numeric_16[] = {
	0x002B,//		PLUS
	0x002C,//		COMMA
	0x002D,//		MINUS
	0x002E,//		DECIMAL
	0x00B1,//		PLUS MINUS
	0x2052,//		COMMERCIAL MINUS
	0xFF0B,//		FULL PLUS
	0xFF0C,//		FULL COMMA
	0xFF0D,//		FULL MINUS
	0xFF0E,//		FULL DECIMAL
	0xFE50,//		SMALL COMMA
	0xFE62,//		SMALL PLUS
	0xFE52,//		SMALL DECIMAL
	0xFE51,//		SMALL COMMA
	0xFF64,//		HALF COMMA
	0xFF61,//		HALF DECIMAL
	0x2396,//		DECIMAL SEPARATOR
	0x066B,//		ARABIC DECIMAL SEPARATOR
	0x066C//		ARABIC THOUSANDS
	};

	bool codepoint16_is_numeric(uint16_t c16) noexcept {
		for(uint32_t i = 0; i < sizeof(single_numeric_16) / sizeof(uint16_t); ++i) {
			if(single_numeric_16[i] == c16) {
				return true;
			}
		}
		return u_getIntPropertyValue(c16, UCHAR_NUMERIC_TYPE) != U_NT_NONE;
	}

	bool codepoint32_is_numeric(uint32_t c) noexcept {
		return u_getIntPropertyValue(c, UCHAR_NUMERIC_TYPE) != U_NT_NONE;
	}

	uint32_t assemble_codepoint(uint16_t high, uint16_t low) noexcept {
		uint32_t high_bits = (high & 0x03FF) << 10;
		uint32_t low_bits = low & 0x03FF;
		uint32_t temp = high_bits | low_bits;
		return temp + 0x10000;
	}
	
	surrogate_pair make_surrogate_pair(uint32_t val) noexcept {
		uint32_t v = val - 0x10000;
		uint32_t h = ((v >> 10) & 0x03FF) | 0xD800;
		uint32_t l = (v & 0x03FF) | 0xDC00;
		return surrogate_pair{uint16_t(h), uint16_t(l) };
	}

	bool is_low_surrogate(uint16_t char_code) noexcept {
		return char_code >= 0xDC00 && char_code <= 0xDFFF;
	}
	bool is_high_surrogate(uint16_t char_code) noexcept {
		return char_code >= 0xD800 && char_code <= 0xDBFF;
	}

	bool cursor_ignorable16(uint16_t at_position, uint16_t trailing) {
		if(at_position >= 0xDC00 && at_position <= 0xDFFF) {
			return false; // low surrogate
		} else if(at_position == 0x200B || at_position == 0x200C || at_position == 0x200D || at_position == 0x2060 || at_position == 0xFEFF) {
			//200B = zero width space
			//200C = zero width non joiner
			//200D = zero width joiner
			//2060 = word joiner
			//FEFF = zero width, no break space
			return false;
		} else if(at_position < 0x20) {
			// legacy ascii control codes
			if(at_position == 0x0A) // '\n'
				return true;
			else if(at_position == 0x09) // '\t'
				return true;
			return false;
		} else if(at_position >= 0x7F && at_position <= 0x9F) {
			return false;
		} else if(at_position == 0x061C || at_position == 0x200E || at_position == 0x200F || (at_position >= 0x202A && at_position <= 0x202E) || (at_position >= 0x2066 && at_position <= 0x2069)) {
			return false;
		} else {
			if(at_position >= 0xD800 && at_position <= 0xDBFF) { // high surrogate
				return codepoint_is_nonspacing(assemble_codepoint(at_position, trailing));
			} else { // non surrogate
				return codepoint_is_nonspacing(at_position);
			}
		}
	}

	// TODO: replace with proper unicode decomposition
	// https://www.unicode.org/reports/tr29/

	int32_t num_logical_chars_in_range(std::wstring_view str) {
		int32_t total = 0;
		for(uint32_t i = 0; i < str.length(); ++i) {
			auto char_code = str[i];
			if(char_code >= 0xDC00 && char_code <= 0xDFFF) {
				// don't count a low surrogate
			} else if(char_code == 0x200B || char_code == 0x200C || char_code == 0x200D || char_code == 0x2060 || char_code == 0xFEFF) {
				//200B = zero width space
				//200C = zero width non joiner
				//200D = zero width joiner
				//2060 = word joiner
				//FEFF = zero width, no break space
			} else if(char_code < 0x20) {
				// legacy ascii control codes
				if(char_code == 0x0A) // '\n'
					++total;
				else if(char_code == 0x09) // '\t'
					++total;
			} else if(char_code >= 0x7F && char_code <= 0x9F) {
				// other control codes
			} else if(char_code == 0x061C || char_code == 0x200E || char_code == 0x200F || (char_code >= 0x202A && char_code <= 0x202E) || (char_code >= 0x2066 && char_code <= 0x2069)) {
				// bidi control codes
			} else {
				if(char_code >= 0xD800 && char_code <= 0xDBFF) { // high surrogate
					if(i + 1 != str.length()) {
						if(!codepoint_is_nonspacing(assemble_codepoint(str[i], str[i + 1])))
							++total;
					}
				} else { // non surrogate
					if(!codepoint_is_nonspacing(str[i]))
						++total;
				}
			}
		}
		return total;
	}

	/*
	0009..000D    ; White_Space # Cc   [5] <control-0009>..<control-000D>
	0020          ; White_Space # Zs       SPACE
	0085          ; White_Space # Cc       <control-0085>
	00A0          ; White_Space # Zs       NO-BREAK SPACE
	1680          ; White_Space # Zs       OGHAM SPACE MARK
	2000..200A    ; White_Space # Zs  [11] EN QUAD..HAIR SPACE
	2028          ; White_Space # Zl       LINE SEPARATOR
	2029          ; White_Space # Zp       PARAGRAPH SEPARATOR
	202F          ; White_Space # Zs       NARROW NO-BREAK SPACE
	205F          ; White_Space # Zs       MEDIUM MATHEMATICAL SPACE
	3000          ; White_Space # Zs       IDEOGRAPHIC SPACE
	*/

	bool is_space(uint32_t c) noexcept {
		return (c == 0x3000 || c == 0x205F || c == 0x202F || c == 0x2029 || c == 0x2028 || c == 0x1680 || c == 0x00A0
			|| c == 0x0085 || c == 0x0020 || (0x0009 <= c && c <= 0x000D) || (0x2000 <= c && c <= 0x200A));
	}

	struct text_analysis_object {
		std::vector<SCRIPT_LOGATTR> char_attributes;
		std::vector<uint16_t> line_breaks;
	};

	void release_text_analysis_object(text_analysis_object* ptr) {
		delete ptr;
	}
	text_analysis_object* make_analysis_object() {
		return new text_analysis_object();
	}
	void impl_update_analyzed_text(text_analysis_object* ptr, arranged_text* txt, std::wstring const& str, bool ltr, text_manager const& tm) {
		IDWriteTextLayout* formatted_text = (IDWriteTextLayout*)txt;

		if(formatted_text) {
			std::vector< DWRITE_LINE_METRICS> lines;
			uint32_t number_of_lines = 0;
			formatted_text->GetLineMetrics(nullptr, 0, &number_of_lines);
			lines.resize(number_of_lines);
			memset(lines.data(), 0, sizeof(DWRITE_LINE_METRICS) * number_of_lines);
			formatted_text->GetLineMetrics(lines.data(), number_of_lines, &number_of_lines);

			ptr->line_breaks.resize(lines.size());
			uint32_t running_total = 0;
			for(uint32_t i = 0; i < lines.size(); ++i) {
				running_total += lines[i].length;
				ptr->line_breaks[i] = uint16_t(running_total);
			}
		} else {
			ptr->line_breaks.clear();
		}

		int32_t items_got = 0;
		std::vector<SCRIPT_ITEM> processed_items(8);
		int32_t current_size = 8;

		SCRIPT_CONTROL control;
		memset(&control, 0, sizeof(SCRIPT_CONTROL));
		SCRIPT_STATE state;
		memset(&state, 0, sizeof(SCRIPT_STATE));

		control.uDefaultLanguage = LANGIDFROMLCID(tm.lcid);
		state.uBidiLevel = ltr ? 0 : 1;
		state.fArabicNumContext = tm.app_lang == L"ar" ? 1 : 0;

		while(ScriptItemize(
			str.data(),
			int32_t(str.length()),
			current_size - 1,
			&control,
			&state,
			processed_items.data(),
			&items_got) == E_OUTOFMEMORY) {
			current_size *= 2;
			processed_items.resize(current_size);
		}

		ptr->char_attributes.resize(str.length());
		memset(ptr->char_attributes.data(), 0, sizeof(SCRIPT_LOGATTR) * str.length());
		for(int32_t i = 0; i < items_got; ++i) {
			auto char_count = processed_items[i + 1].iCharPos - processed_items[i].iCharPos;
			ScriptBreak(str.data() + processed_items[i].iCharPos,
				char_count,
				&(processed_items[i].a),
				ptr->char_attributes.data() + processed_items[i].iCharPos);
			for(int32_t j = processed_items[i].iCharPos; j < processed_items[i + 1].iCharPos; ++j) {
				ptr->char_attributes[j].fReserved = (processed_items[i].a.s.uBidiLevel & 0x01);
			}
		}
		
		bool in_numeric_run = false;
		for(uint32_t i = 0; i < str.length(); ++i) {
			if(is_high_surrogate(str[i])) {
				if(i + 1 < str.length()) {
					auto code_point = assemble_codepoint(str[i], str[i + 1]);
					if(codepoint32_is_numeric(code_point)) {
						if(!in_numeric_run) {
							ptr->char_attributes[i].fWordStop = 1;
							in_numeric_run = true;
						}
					} else {
						if(in_numeric_run) {
							ptr->char_attributes[i].fWordStop = 1;
							in_numeric_run = false;
						}
					}
				}
			} else if(is_low_surrogate(str[i])) {
				// ignore
			} else {
				if(is_space(str[i])) {
					//ignore 
				} else if(codepoint16_is_numeric(str[i])) {
					if(!in_numeric_run) {
						ptr->char_attributes[i].fWordStop = 1;
						in_numeric_run = true;
					}
				} else {
					if(in_numeric_run) {
						ptr->char_attributes[i].fWordStop = 1;
						in_numeric_run = false;
					}
				}
			}
		}
		
	}
	void update_analyzed_text(text_analysis_object* ptr, arranged_text* txt, std::wstring const& str, bool ltr, text_manager const& tm) {
		impl_update_analyzed_text(ptr, txt, str, ltr, tm);
	}

	int32_t number_of_lines(text_analysis_object* ptr) {
		return int32_t(ptr->line_breaks.size());
	}
	int32_t line_of_position(text_analysis_object* ptr, int32_t position) {
		for(uint32_t j = 0; j < ptr->line_breaks.size(); ++j) {
			if(uint16_t(position) < ptr->line_breaks[j]) {
				return int32_t(j);
			}
		}
		return std::max(int32_t(ptr->line_breaks.size()) - 1, 0);
	}
	int32_t start_of_line(text_analysis_object* ptr, int32_t line) {
		if(line <= 0)
			return 0;
		else if(uint32_t(line) >= ptr->line_breaks.size())
			return int32_t(ptr->line_breaks.back());
		else
			return int32_t(ptr->line_breaks[line - 1]);
	}
	int32_t end_of_line(text_analysis_object* ptr, int32_t line) {
		if(line < 0)
			return 0;
		else if(uint32_t(line) < ptr->line_breaks.size())
			return int32_t(ptr->line_breaks[line]);
		else
			return int32_t(ptr->line_breaks.back());
	}

	int32_t left_visual_cursor_position(text_analysis_object* ptr, int32_t position, std::wstring const& str, bool ltr, text_manager const& tm) {
		auto is_ltr = position_is_ltr(ptr, position);
		auto in_line = line_of_position(ptr, position);
		auto default_pos = is_ltr ? get_previous_cursor_position(ptr, position) : get_next_cursor_position(ptr, position);

		auto line_begin = start_of_line(ptr, in_line);
		auto line_end = end_of_line(ptr, in_line);
		if(is_ltr && position == line_begin) {
			return text::get_previous_cursor_position(ptr, end_of_line(ptr, in_line - 1));
		}
		if(!is_ltr && default_pos == line_end) {
			return start_of_line(ptr, in_line + 1);
		}
		
		auto default_in_line = line_of_position(ptr, default_pos);
		auto default_ltr = position_is_ltr(ptr, default_pos);

		if(default_ltr == is_ltr && in_line == default_in_line)
			return default_pos;

		int32_t items_got = 0;
		std::vector<SCRIPT_ITEM> processed_items(8);
		int32_t current_size = 8;

		SCRIPT_CONTROL control;
		memset(&control, 0, sizeof(SCRIPT_CONTROL));
		SCRIPT_STATE state;
		memset(&state, 0, sizeof(SCRIPT_STATE));

		control.uDefaultLanguage = LANGIDFROMLCID(tm.lcid);
		state.uBidiLevel = ltr ? 0 : 1;
		state.fArabicNumContext = tm.app_lang == L"ar" ? 1 : 0;

		while(ScriptItemize(
			str.data() + line_begin,
			line_end - line_begin,
			current_size - 1,
			&control,
			&state,
			processed_items.data(),
			&items_got) == E_OUTOFMEMORY) {
			current_size *= 2;
			processed_items.resize(current_size);
		}
		int32_t run_position = 0;
		std::vector<BYTE> run_embedding_levels(items_got);
		std::vector<int32_t> visual_to_logical(items_got);
		std::vector<int32_t> logical_to_visual(items_got);

		for(int32_t i = 0; i < items_got; ++i) {
			if(processed_items[i].iCharPos <= (position - line_begin) && (position - line_begin) < processed_items[i + 1].iCharPos) {
				run_position = i;
			}
			run_embedding_levels[i] = processed_items[i].a.s.uBidiLevel;
		}

		ScriptLayout(items_got, run_embedding_levels.data(), visual_to_logical.data(), logical_to_visual.data());
		auto visual_position_of_run = logical_to_visual[run_position];
		if(visual_position_of_run == 0) {
			if(is_ltr) {
				return text::get_previous_cursor_position(ptr, end_of_line(ptr, in_line - 1));
			} else {
				return start_of_line(ptr, in_line + 1);
			}
		}
		auto logical_position_of_left_run = visual_to_logical[visual_position_of_run - 1];
		auto next_run_is_ltr = (processed_items[logical_position_of_left_run].a.s.uBidiLevel & 0x01) == 0;
		if(next_run_is_ltr) {
			// find rightmost char position by moving back from the run after it
			return get_previous_cursor_position(ptr, line_begin + processed_items[logical_position_of_left_run + 1].iCharPos);
		} else {
			// rightmost char position is first char
			return line_begin + processed_items[logical_position_of_left_run].iCharPos;
		}
	}
	int32_t right_visual_cursor_position(text_analysis_object* ptr, int32_t position, std::wstring const& str, bool ltr, text_manager const& tm) {
		auto is_ltr = position_is_ltr(ptr, position);
		auto in_line = line_of_position(ptr, position);
		auto default_pos = is_ltr ? get_next_cursor_position(ptr, position) : get_previous_cursor_position(ptr, position);
		
		auto line_begin = start_of_line(ptr, in_line);
		auto line_end = end_of_line(ptr, in_line);
		if(is_ltr && default_pos == line_end) {
			return start_of_line(ptr, in_line + 1);
		}
		if(!is_ltr && position == line_begin) {
			return text::get_previous_cursor_position(ptr, end_of_line(ptr, in_line - 1));
		}

		auto default_in_line = line_of_position(ptr, default_pos);
		auto default_ltr = position_is_ltr(ptr, default_pos);

		if(default_ltr == is_ltr && default_in_line == in_line)
			return default_pos;

		int32_t items_got = 0;
		std::vector<SCRIPT_ITEM> processed_items(8);
		int32_t current_size = 8;

		SCRIPT_CONTROL control;
		memset(&control, 0, sizeof(SCRIPT_CONTROL));
		SCRIPT_STATE state;
		memset(&state, 0, sizeof(SCRIPT_STATE));

		control.uDefaultLanguage = LANGIDFROMLCID(tm.lcid);
		state.uBidiLevel = ltr ? 0 : 1;
		state.fArabicNumContext = tm.app_lang == L"ar" ? 1 : 0;

		while(ScriptItemize(
			str.data() + line_begin,
			line_end - line_begin,
			current_size - 1,
			&control,
			&state,
			processed_items.data(),
			&items_got) == E_OUTOFMEMORY) {
			current_size *= 2;
			processed_items.resize(current_size);
		}
		int32_t run_position = 0;
		std::vector<BYTE> run_embedding_levels(items_got);
		std::vector<int32_t> visual_to_logical(items_got);
		std::vector<int32_t> logical_to_visual(items_got);

		for(int32_t i = 0; i < items_got; ++i) {
			if(processed_items[i].iCharPos <= (position - line_begin) && (position - line_begin) < processed_items[i + 1].iCharPos) {
				run_position = i;
			}
			run_embedding_levels[i] = processed_items[i].a.s.uBidiLevel;
		}

		ScriptLayout(items_got, run_embedding_levels.data(), visual_to_logical.data(), logical_to_visual.data());
		auto visual_position_of_run = logical_to_visual[run_position];
		if(visual_position_of_run == items_got - 1) { // is already rightmost
			if(is_ltr) {
				return start_of_line(ptr, in_line + 1);
			} else {
				return text::get_previous_cursor_position(ptr, end_of_line(ptr, in_line - 1));
			}
		}
		auto logical_position_of_left_run = visual_to_logical[visual_position_of_run + 1];
		auto next_run_is_ltr = (processed_items[logical_position_of_left_run].a.s.uBidiLevel & 0x01) == 0;
		if(next_run_is_ltr) {
			// leftmost char position is first char
			return processed_items[logical_position_of_left_run].iCharPos + line_begin;
		} else {
			// find leftmost char position by moving back from the run after it
			return get_previous_cursor_position(ptr, processed_items[logical_position_of_left_run + 1].iCharPos + line_begin);
		}
	}

	int32_t number_of_cursor_positions_in_range(text_analysis_object* ptr, int32_t start, int32_t count) {
		int32_t total = 0;
		auto const array_size = ptr->char_attributes.size();
		for(int32_t i = 0; i < count && start + i < array_size; ++i) {
			if(ptr->char_attributes[start + i].fCharStop)
				++total;
		}
		return total;
	}
	int32_t get_previous_cursor_position(text_analysis_object* ptr, int32_t position) {
		--position;
		if(position < ptr->char_attributes.size()) {
			for(; position > 0; --position) {
				if(ptr->char_attributes[position].fCharStop) {
					return position;
				}
			}
		}
		return 0;
	}
	int32_t get_next_cursor_position(text_analysis_object* ptr, int32_t position) {
		++position;
		auto const array_size = ptr->char_attributes.size();
		for(; position < array_size; ++position) {
			if(ptr->char_attributes[position].fCharStop) {
				return position;
			}
		}
		return int32_t(array_size);
	}
	int32_t left_visual_word_position(text_analysis_object* ptr, int32_t position) {
		if(position_is_ltr(ptr, position)) {
			return get_previous_word_position(ptr, position);
		} else {
			return get_next_word_position(ptr, position);
		}
	}
	int32_t right_visual_word_position(text_analysis_object* ptr, int32_t position) {
		if(position_is_ltr(ptr, position)) {
			return get_next_word_position(ptr, position);
		} else {
			return get_previous_word_position(ptr, position);
		}
	}
	int32_t get_previous_word_position(text_analysis_object* ptr, int32_t position) {
		--position;
		if(position < ptr->char_attributes.size()) {
			for(; position > 0; --position) {
				if(ptr->char_attributes[position].fWordStop || ptr->char_attributes[position].fSoftBreak) {
					return position;
				}
			}
		}
		return 0;
	}
	int32_t get_next_word_position(text_analysis_object* ptr, int32_t position) {
		++position;
		auto const array_size = ptr->char_attributes.size();
		for(; position < array_size; ++position) {
			if(ptr->char_attributes[position].fWordStop || ptr->char_attributes[position].fSoftBreak) {
				return position;
			}
		}
		return int32_t(array_size);
	}
	bool position_is_ltr(text_analysis_object* ptr, int32_t position) {
		if(position < ptr->char_attributes.size()) {
			return  ptr->char_attributes[position].fReserved == 0;
		} else if(ptr->char_attributes.size() > 0){
			return ptr->char_attributes[ptr->char_attributes.size() - 1].fReserved == 0;
		} else {
			return false;
		}
	}

	bool is_cursor_position(text_analysis_object* ptr, int32_t position) {
		bool result = false;
		if(position < ptr->char_attributes.size()) {
			result = ptr->char_attributes[position].fCharStop != 0;
		}
		return result;
	}
	bool is_word_position(text_analysis_object* ptr, int32_t position) {
		bool result = false;
		if(position < ptr->char_attributes.size()) {
			result = ptr->char_attributes[position].fWordStop != 0;
		}
		return result;
	}

	enum class lock_state : uint8_t {
		unlocked, locked_read, locked_readwrite
	};

	struct mouse_sink {
		ITfMouseSink* sink;
		LONG range_start;
		LONG range_length;
	};

	struct text_services_object : public ITextStoreACP2, public ITfInputScope, public ITfContextOwnerCompositionSink, public ITfMouseTrackerACP {
		
		std::vector<TS_ATTRVAL> gathered_attributes;
		std::vector<mouse_sink> installed_mouse_sinks;
		window_data& win;
		edit_interface* ei = nullptr;
		ITfDocumentMgr* document = nullptr;
		ITfContext* primary_context = nullptr;
		TfEditCookie content_identifier = 0;
		ITextStoreACPSink* advise_sink = nullptr;
		lock_state document_lock_state = lock_state::unlocked;
		bool notify_on_text_change = false;
		bool notify_on_selection_change = false;
		bool relock_pending = false;
		bool in_composition = false;

		ULONG m_refCount;

		void free_gathered_attributes() {
			for(auto& i : gathered_attributes) {
				VariantClear(&(i.varValue));
			}
			gathered_attributes.clear();
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) {
			if(riid == __uuidof(IUnknown))
				*ppvObject = static_cast<ITextStoreACP2*>(this);
			else if(riid == __uuidof(ITextStoreACP2))
				*ppvObject = static_cast<ITextStoreACP2*>(this);
			else if(riid == __uuidof(ITfInputScope))
				*ppvObject = static_cast<ITfInputScope*>(this);
			else if(riid == __uuidof(ITfContextOwnerCompositionSink))
				*ppvObject = static_cast<ITfContextOwnerCompositionSink*>(this);
			else if(riid == __uuidof(ITfMouseTrackerACP))
				*ppvObject = static_cast<ITfMouseTrackerACP*>(this);
			else {
				*ppvObject = NULL;
				return E_NOINTERFACE;
			}
			(static_cast<IUnknown*>(*ppvObject))->AddRef();
			return S_OK;
		}
		virtual ULONG STDMETHODCALLTYPE AddRef() {
			return InterlockedIncrement(&m_refCount);
		}
		virtual ULONG STDMETHODCALLTYPE Release() {
			long val = InterlockedDecrement(&m_refCount);
			if(val == 0) {
				delete this;
			}
			return val;
		}

		//ITfMouseTrackerACP
		virtual HRESULT STDMETHODCALLTYPE AdviseMouseSink(__RPC__in_opt ITfRangeACP* range, __RPC__in_opt ITfMouseSink* pSink, __RPC__out DWORD* pdwCookie) {
			if(!range || !pSink || !pdwCookie)
				return E_INVALIDARG;
			for(uint32_t i = 0; i < installed_mouse_sinks.size(); ++i) {
				if(installed_mouse_sinks[i].sink == nullptr) {
					installed_mouse_sinks[i].sink = pSink;
					pSink->AddRef();
					installed_mouse_sinks[i].range_start = 0;
					installed_mouse_sinks[i].range_length = 0;
					range->GetExtent(&(installed_mouse_sinks[i].range_start), &(installed_mouse_sinks[i].range_length));
					*pdwCookie = DWORD(i);
					return S_OK;
				}
			}
			installed_mouse_sinks.emplace_back();
			installed_mouse_sinks.back().sink = pSink;
			pSink->AddRef();
			installed_mouse_sinks.back().range_start = 0;
			installed_mouse_sinks.back().range_length = 0;
			range->GetExtent(&(installed_mouse_sinks.back().range_start), &(installed_mouse_sinks.back().range_length));
			*pdwCookie = DWORD(installed_mouse_sinks.size() - 1);
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE UnadviseMouseSink(DWORD dwCookie) {
			if(dwCookie < installed_mouse_sinks.size()) {
				safe_release(installed_mouse_sinks[dwCookie].sink);
			}
			return S_OK;
		}

		bool send_mouse_event(int32_t x, int32_t y, uint32_t buttons) {
			if(installed_mouse_sinks.empty())
				return false;
			if(!ei)
				return false;

			auto detailed_pos = ei->get_detailed_position(win, screen_space_point{ x,y });
			for(auto& ms : installed_mouse_sinks) {
				if(ms.sink && int32_t(detailed_pos.position) >= ms.range_start && int32_t(detailed_pos.position) <= ms.range_start + ms.range_length) {
					BOOL eaten = false;
					ms.sink->OnMouseEvent(detailed_pos.position, detailed_pos.quadrent, buttons, &eaten);
					if(eaten)
						return true;
				}
			}
			return false;
		}

		//ITfContextOwnerCompositionSink
		virtual HRESULT STDMETHODCALLTYPE OnStartComposition(__RPC__in_opt ITfCompositionView* /*pComposition*/, __RPC__out BOOL* pfOk) {
			*pfOk = TRUE;
			in_composition = true;
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE OnUpdateComposition(__RPC__in_opt ITfCompositionView* pComposition, __RPC__in_opt ITfRange* /*pRangeNew*/) {
			ITfRange* view_range = nullptr;
			pComposition->GetRange(&view_range);
			if(view_range) {
				ITfRangeACP* acp_range = nullptr;
				view_range->QueryInterface(IID_PPV_ARGS(&acp_range));
				if(ei && acp_range) {
					LONG start = 0;
					LONG count = 0;
					acp_range->GetExtent(&start, &count);
					if(count > 0) {
						ei->set_temporary_selection(win, uint32_t(start), uint32_t(start + count));
					}
				}
				safe_release(acp_range);
			}
			safe_release(view_range);

			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE OnEndComposition(__RPC__in_opt ITfCompositionView* /*pComposition*/) {
			if(ei) {
				ei->register_composition_result(win);
			}
			in_composition = false;
			return S_OK;
		}

		// ITextStoreACP2
		virtual HRESULT STDMETHODCALLTYPE AdviseSink(__RPC__in REFIID riid, __RPC__in_opt IUnknown* punk, DWORD dwMask) {
			if(!IsEqualGUID(riid, IID_ITextStoreACPSink)) {
				return E_INVALIDARG;
			}
			ITextStoreACPSink* temp = nullptr;
			if(FAILED(punk->QueryInterface(IID_ITextStoreACPSink, (void**)&temp))) {
				return E_NOINTERFACE;
			}
			if(advise_sink) {
				if(advise_sink == temp) {
					safe_release(temp);
				} else {
					return CONNECT_E_ADVISELIMIT;
				}
			} else {
				advise_sink = temp;
			}
			notify_on_text_change = (TS_AS_TEXT_CHANGE & dwMask) != 0;
			notify_on_selection_change = (TS_AS_SEL_CHANGE & dwMask) != 0;
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE UnadviseSink(__RPC__in_opt IUnknown* /*punk*/) {
			safe_release(advise_sink);
			notify_on_text_change = false;
			notify_on_selection_change = false;
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE RequestLock(DWORD dwLockFlags, __RPC__out HRESULT* phrSession) {
			if(!advise_sink) {
				*phrSession = E_FAIL;
				return E_UNEXPECTED;
			}
			
			relock_pending = false;

			if(document_lock_state != lock_state::unlocked) {
				if(dwLockFlags & TS_LF_SYNC) {
					*phrSession = TS_E_SYNCHRONOUS;
					return S_OK;
				} else {
					if(document_lock_state == lock_state::locked_read && (dwLockFlags & TS_LF_READWRITE) == TS_LF_READWRITE) {
						relock_pending = true;
						*phrSession = TS_S_ASYNC;
						return S_OK;
					}
				}
				return E_FAIL;
			}

			if((TS_LF_READ & dwLockFlags) != 0) {
				document_lock_state = lock_state::locked_read;
			}
			if((TS_LF_READWRITE & dwLockFlags) != 0) {
				document_lock_state = lock_state::locked_readwrite;
			}

			*phrSession = advise_sink->OnLockGranted(dwLockFlags);
			document_lock_state = lock_state::unlocked;

			if(relock_pending) {
				relock_pending = false;
				HRESULT hr = S_OK;
				RequestLock(TS_LF_READWRITE, &hr);
			}

			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetStatus(__RPC__out TS_STATUS* pdcs) {
			if(!pdcs)
				return E_INVALIDARG;
			pdcs->dwStaticFlags = TS_SS_NOHIDDENTEXT;
			pdcs->dwDynamicFlags = (ei && ei->is_read_only() ? TS_SD_READONLY : 0);
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG /*cch*/, __RPC__out LONG* pacpResultStart, __RPC__out LONG* pacpResultEnd) {
			if(!pacpResultStart || !pacpResultEnd || acpTestStart > acpTestEnd)
				return E_INVALIDARG;
			if(!ei)
				return TF_E_DISCONNECTED;
			*pacpResultStart = std::min(acpTestStart, LONG(ei->get_text_length()));
			*pacpResultEnd = std::min(acpTestEnd, LONG(ei->get_text_length()));
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetSelection(ULONG ulIndex, ULONG ulCount, __RPC__out_ecount_part(ulCount, *pcFetched) TS_SELECTION_ACP* pSelection, __RPC__out ULONG* pcFetched) {
			if(!pcFetched)
				return E_INVALIDARG;
			if(document_lock_state == lock_state::unlocked)
				return TS_E_NOLOCK;

			if(ei && ulCount > 0 && (ulIndex == 0 || ulIndex == TF_DEFAULT_SELECTION)) {
				if(!pSelection)
					return E_INVALIDARG;
				auto start = ei->get_cursor();
				auto end = ei->get_selection_anchor();
				pSelection[0].acpStart = int32_t(std::min(start, end));
				pSelection[0].acpEnd = int32_t(std::max(start, end));
				pSelection[0].style.ase = start < end ? TS_AE_START : TS_AE_END;
				pSelection[0].style.fInterimChar = FALSE;
				*pcFetched = 1;
			} else {
				*pcFetched = 0;
			}
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE SetSelection(ULONG ulCount, __RPC__in_ecount_full(ulCount) const TS_SELECTION_ACP* pSelection) {
			if(document_lock_state != lock_state::locked_readwrite)
				return TF_E_NOLOCK;
			if(!ei)
				return TF_E_DISCONNECTED;
			if(ulCount > 0) {
				if(!pSelection)
					return E_INVALIDARG;

				auto start = pSelection->style.ase == TS_AE_START ? pSelection->acpEnd : pSelection->acpStart;
				auto end = pSelection->style.ase == TS_AE_START ? pSelection->acpStart : pSelection->acpEnd;
				ei->set_selection(win, start, end);
			}
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetText(LONG acpStart, LONG acpEnd, __RPC__out_ecount_part(cchPlainReq, *pcchPlainRet) WCHAR* pchPlain, ULONG cchPlainReq, __RPC__out ULONG* pcchPlainRet, __RPC__out_ecount_part(cRunInfoReq, *pcRunInfoRet) TS_RUNINFO* prgRunInfo, ULONG cRunInfoReq, __RPC__out ULONG* pcRunInfoRet, __RPC__out LONG* pacpNext) {

			if(!pcchPlainRet || !pcRunInfoRet)
				return E_INVALIDARG;
			if(!pchPlain && cchPlainReq != 0)
				return E_INVALIDARG;
			if(!prgRunInfo && cRunInfoReq != 0)
				return E_INVALIDARG;
			if(!pacpNext)
				return E_INVALIDARG;
			if(document_lock_state == lock_state::unlocked)
				return TF_E_NOLOCK;

			*pcchPlainRet = 0;

			if(!ei)
				return TF_E_DISCONNECTED;

			if((cchPlainReq == 0) && (cRunInfoReq == 0)) {
				return S_OK;
			}
			auto len = LONG(ei->get_text_length());
			acpEnd = std::min(acpEnd, len);
			if(acpEnd == -1)
				acpEnd = len;

			acpEnd = std::min(acpEnd, acpStart + LONG(cchPlainReq));
			if(acpStart != acpEnd) {
				auto text = ei->get_text();
				std::copy(text.c_str(), text.c_str() + (acpEnd - acpStart), pchPlain);
				*pcchPlainRet = (acpEnd - acpStart);
			}
			if(*pcchPlainRet < cchPlainReq) {
				*(pchPlain + *pcchPlainRet) = 0;
			}
			if(cRunInfoReq != 0) {
				prgRunInfo[0].uCount = acpEnd - acpStart;
				prgRunInfo[0].type = TS_RT_PLAIN;
				*pcRunInfoRet = 1;
			} else {
				*pcRunInfoRet = 0;
			}

			*pacpNext = acpEnd;

			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE SetText(DWORD /*dwFlags*/, LONG acpStart, LONG acpEnd, __RPC__in_ecount_full(cch) const WCHAR* pchText, ULONG cch, __RPC__out TS_TEXTCHANGE* pChange) {
			if(document_lock_state != lock_state::locked_readwrite)
				return TF_E_NOLOCK;
			if(!ei)
				return TF_E_DISCONNECTED;
			if(!pChange)
				return E_INVALIDARG;
			if(!pchText && cch > 0)
				return E_INVALIDARG;

			auto len = LONG(ei->get_text_length());

			if(acpStart > len)
				return E_INVALIDARG;

			LONG acpRemovingEnd = acpEnd >= acpStart ? std::min(acpEnd, len) : acpStart;


			win.text_services_interface.send_notifications = false;
			ei->insert_text(win, uint32_t(acpStart), uint32_t(acpRemovingEnd), std::wstring_view(pchText, cch));
			win.text_services_interface.send_notifications = true;
			

			pChange->acpStart = acpStart;
			pChange->acpOldEnd = acpRemovingEnd;
			pChange->acpNewEnd = acpStart + cch;

			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetFormattedText(LONG /*acpStart*/, LONG /*acpEnd*/, __RPC__deref_out_opt IDataObject** /*ppDataObject*/) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE GetEmbedded(LONG /*acpPos*/, __RPC__in REFGUID /*rguidService*/, __RPC__in REFIID /*riid*/, __RPC__deref_out_opt IUnknown** ppunk) {
			if(!ppunk)
				return E_INVALIDARG;
			*ppunk = nullptr;
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInsertEmbedded(__RPC__in const GUID* /*pguidService*/, __RPC__in const FORMATETC* pFormatEtc, __RPC__out BOOL* pfInsertable) {
			if(!pFormatEtc || !pfInsertable)
				return E_INVALIDARG;
			if(pfInsertable)
				*pfInsertable = FALSE;
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE InsertEmbedded(DWORD /*dwFlags*/, LONG /*acpStart*/, LONG /*acpEnd*/, __RPC__in_opt IDataObject* /*pDataObject*/, __RPC__out TS_TEXTCHANGE* /*pChange*/) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE InsertTextAtSelection(DWORD dwFlags, __RPC__in_ecount_full(cch) const WCHAR* pchText, ULONG cch, __RPC__out LONG* pacpStart, __RPC__out LONG* pacpEnd, __RPC__out TS_TEXTCHANGE* pChange) {

			
			if(!ei)
				return TF_E_DISCONNECTED;

			LONG acpStart = std::min(ei->get_cursor(), ei->get_selection_anchor());
			LONG acpEnd = std::max(ei->get_cursor(), ei->get_selection_anchor());

			if((dwFlags & TS_IAS_QUERYONLY) != 0) {
				if(document_lock_state == lock_state::unlocked)
					return TS_E_NOLOCK;
				if(pacpStart)
					*pacpStart = acpStart;
				if(pacpEnd)
					*pacpEnd = acpStart + cch;
				return S_OK;
			}

			if(document_lock_state != lock_state::locked_readwrite)
				return TF_E_NOLOCK;
			if(!pchText)
				return E_INVALIDARG;

			
			win.text_services_interface.send_notifications = false;
			ei->insert_text(win, uint32_t(acpStart), uint32_t(acpEnd), std::wstring_view(pchText, cch));
			win.text_services_interface.send_notifications = true;
			
			if(pacpStart)
				*pacpStart = acpStart;
			if(pacpEnd)
				*pacpEnd = acpStart + cch;

			if(pChange) {
				pChange->acpStart = acpStart;
				pChange->acpOldEnd = acpEnd;
				pChange->acpNewEnd = acpStart + cch;
			}

			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection(DWORD /*dwFlags*/, __RPC__in_opt IDataObject* /*pDataObject*/, __RPC__out LONG* /*pacpStart*/, __RPC__out LONG* /*pacpEnd*/, __RPC__out TS_TEXTCHANGE* /*pChange*/) {

			return E_NOTIMPL;
		}

		void fill_gathered_attributes(ULONG cFilterAttrs, __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs, bool fill_variants, int32_t position) {
			free_gathered_attributes();

			for(uint32_t i = 0; i < cFilterAttrs; ++i) {
				if(IsEqualGUID(paFilterAttrs[i], GUID_PROP_INPUTSCOPE)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_UNKNOWN;
						ITfInputScope* is = nullptr;
						this->QueryInterface(IID_PPV_ARGS(&is));
						gathered_attributes.back().varValue.punkVal = is;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Font_FaceName)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BSTR;
						gathered_attributes.back().varValue.bstrVal = SysAllocString(win.dynamic_settings.primary_font.name.c_str());
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Font_SizePts)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_I4;
						gathered_attributes.back().varValue.lVal = int32_t(win.dynamic_settings.primary_font.font_size * 96.0f / win.dpi);
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Font_Style_Bold)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						gathered_attributes.back().varValue.boolVal = win.dynamic_settings.primary_font.weight > 400 ? VARIANT_TRUE : VARIANT_FALSE;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Font_Style_Height)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_I4;
						gathered_attributes.back().varValue.lVal = int32_t(win.dynamic_settings.primary_font.font_size);
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Font_Style_Hidden)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						gathered_attributes.back().varValue.boolVal = VARIANT_FALSE;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Font_Style_Italic)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						gathered_attributes.back().varValue.boolVal = win.dynamic_settings.primary_font.is_oblique ? VARIANT_TRUE : VARIANT_FALSE;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Font_Style_Weight)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_I4;
						gathered_attributes.back().varValue.lVal = win.dynamic_settings.primary_font.weight;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_Alignment_Center)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						gathered_attributes.back().varValue.boolVal = ei && ei->get_alignment() == content_alignment::centered ? VARIANT_TRUE : VARIANT_FALSE;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_Alignment_Justify)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						gathered_attributes.back().varValue.boolVal = ei && ei->get_alignment() == content_alignment::justified ? VARIANT_TRUE : VARIANT_FALSE;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_Alignment_Left)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						gathered_attributes.back().varValue.boolVal = ei && ei->get_alignment() == content_alignment::leading ? VARIANT_TRUE : VARIANT_FALSE;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_Alignment_Right)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						gathered_attributes.back().varValue.boolVal = ei && ei->get_alignment() == content_alignment::trailing ? VARIANT_TRUE : VARIANT_FALSE;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_Orientation)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_I4;
						gathered_attributes.back().varValue.lVal = 0;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_ReadOnly)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						gathered_attributes.back().varValue.boolVal = ei && ei->is_read_only() ? VARIANT_TRUE : VARIANT_FALSE;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_RightToLeft)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						if(position >= 0) {
							gathered_attributes.back().varValue.boolVal = ei && ei->position_is_ltr(position) ? VARIANT_FALSE : VARIANT_TRUE;
						} else {
							gathered_attributes.back().varValue.boolVal = win.orientation == layout_orientation::horizontal_right_to_left ? VARIANT_TRUE : VARIANT_FALSE;
						}
					}
				} else if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_VerticalWriting)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						gathered_attributes.back().varValue.boolVal = horizontal(win.orientation) ? VARIANT_FALSE : VARIANT_TRUE;
					}
				} else if(IsEqualGUID(paFilterAttrs[i], GUID_PROP_COMPOSING)) {
					gathered_attributes.emplace_back();
					gathered_attributes.back().idAttr = paFilterAttrs[i];
					gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
					if(fill_variants) {
						gathered_attributes.back().varValue.vt = VT_BOOL;
						if(position >= 0) {
							gathered_attributes.back().varValue.boolVal = ei && (uint32_t(position) >= ei->get_temporary_position()) && (uint32_t(position) < ei->get_temporary_length() + ei->get_temporary_position()) ? VARIANT_TRUE : VARIANT_FALSE;
						} else {
							gathered_attributes.back().varValue.boolVal = VARIANT_FALSE;
						}
					}
				} 
			}
		}

		virtual HRESULT STDMETHODCALLTYPE RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs) {
			if(!paFilterAttrs && cFilterAttrs > 0)
				return E_INVALIDARG;
			if(!ei)
				return TF_E_DISCONNECTED;

			bool fill_variants = (TS_ATTR_FIND_WANT_VALUE & dwFlags) != 0;
			fill_gathered_attributes(cFilterAttrs, paFilterAttrs, fill_variants, -1);
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs, DWORD dwFlags) {
			if(!paFilterAttrs && cFilterAttrs > 0)
				return E_INVALIDARG;
			if(!ei)
				return TF_E_DISCONNECTED;

			bool fill_variants = (TS_ATTR_FIND_WANT_VALUE & dwFlags) != 0;
			fill_gathered_attributes(cFilterAttrs, paFilterAttrs, fill_variants, acpPos);
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs, DWORD dwFlags) {
			if(!paFilterAttrs && cFilterAttrs > 0)
				return E_INVALIDARG;
			if(!ei)
				return TF_E_DISCONNECTED;

			free_gathered_attributes();
			bool fill_variants = (TS_ATTR_FIND_WANT_VALUE & dwFlags) != 0;
			bool attributes_that_end = (TS_ATTR_FIND_WANT_END & dwFlags) != 0;
			for(uint32_t i = 0; i < cFilterAttrs; ++i) {
				if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_RightToLeft)) {
					if(acpPos > 0 && ei && ei->position_is_ltr(uint32_t(acpPos - 1)) != ei->position_is_ltr(uint32_t(acpPos))) {

						gathered_attributes.emplace_back();
						gathered_attributes.back().idAttr = paFilterAttrs[i];
						gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
						if(fill_variants) {
							gathered_attributes.back().varValue.vt = VT_BOOL;

							if(attributes_that_end)
								gathered_attributes.back().varValue.boolVal = ei && ei->position_is_ltr(uint32_t(acpPos - 1)) ? VARIANT_FALSE : VARIANT_TRUE;
							else
								gathered_attributes.back().varValue.boolVal = ei && ei->position_is_ltr(uint32_t(acpPos)) ? VARIANT_FALSE : VARIANT_TRUE;
						} 
					}
				} else if(IsEqualGUID(paFilterAttrs[i], GUID_PROP_COMPOSING)) {
					if(acpPos > 0 && ei &&
						(	((uint32_t(acpPos - 1) >= ei->get_temporary_position()) && (uint32_t(acpPos - 1) < ei->get_temporary_length() + ei->get_temporary_position())) 
							!=
							((uint32_t(acpPos) >= ei->get_temporary_position()) && (uint32_t(acpPos) < ei->get_temporary_length() + ei->get_temporary_position())))) {

						gathered_attributes.emplace_back();
						gathered_attributes.back().idAttr = paFilterAttrs[i];
						gathered_attributes.back().dwOverlapId = int32_t(gathered_attributes.size());
						if(fill_variants) {
							gathered_attributes.back().varValue.vt = VT_BOOL;
							if(attributes_that_end)
								gathered_attributes.back().varValue.boolVal = ei && (uint32_t(acpPos - 1) >= ei->get_temporary_position()) && (uint32_t(acpPos - 1) < ei->get_temporary_length() + ei->get_temporary_position()) ? VARIANT_TRUE : VARIANT_FALSE;
							else 
								gathered_attributes.back().varValue.boolVal = ei && (uint32_t(acpPos) >= ei->get_temporary_position()) && (uint32_t(acpPos) < ei->get_temporary_length() + ei->get_temporary_position()) ? VARIANT_TRUE : VARIANT_FALSE;
							
						}
					}
				}
			}
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs, DWORD dwFlags, __RPC__out LONG* pacpNext, __RPC__out BOOL* pfFound, __RPC__out LONG* plFoundOffset) {
			if(!pacpNext || !pfFound || !plFoundOffset)
				return E_INVALIDARG;

			*pfFound = FALSE;
			if(plFoundOffset)
				*plFoundOffset = 0;
			*pacpNext = acpStart;

			if(!ei)
				return TF_E_DISCONNECTED;

			LONG initial_position = std::max(acpStart, LONG(1));
			LONG end_position = std::min(acpHalt, LONG(ei->get_text_length() - 1));
			end_position = std::max(initial_position, end_position);

			if((TS_ATTR_FIND_BACKWARDS & dwFlags) != 0) {
				std::swap(initial_position, end_position);
			}
			while(initial_position != end_position) {
				for(uint32_t i = 0; i < cFilterAttrs; ++i) {
					if(IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_RightToLeft)) {
						if(ei->position_is_ltr(uint32_t(initial_position - 1)) != ei->position_is_ltr(uint32_t(initial_position))) {
							*pfFound = TRUE;
							if(plFoundOffset)
								*plFoundOffset = initial_position;
							*pacpNext = initial_position;
							return S_OK;
						}
					} else if(IsEqualGUID(paFilterAttrs[i], GUID_PROP_COMPOSING)) {
						if(((uint32_t(initial_position - 1) >= ei->get_temporary_position()) && (uint32_t(initial_position - 1) < ei->get_temporary_length() + ei->get_temporary_position()))
							!=
							((uint32_t(initial_position) >= ei->get_temporary_position()) && (uint32_t(initial_position) < ei->get_temporary_length() + ei->get_temporary_position()))) {
							*pfFound = TRUE;
							if(plFoundOffset)
								*plFoundOffset = initial_position;
							*pacpNext = initial_position;
							return S_OK;
						}
					}
				}
				if((TS_ATTR_FIND_BACKWARDS & dwFlags) != 0) {
					--initial_position;
				} else {
					++initial_position;
				}
			}
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs(ULONG ulCount, __RPC__out_ecount_part(ulCount, *pcFetched) TS_ATTRVAL* paAttrVals, __RPC__out ULONG* pcFetched) {

			*pcFetched = 0;
			uint32_t i = 0;
			for(; i < ulCount && i < gathered_attributes.size(); ++i) {
				paAttrVals[i] = gathered_attributes[i];
				*pcFetched++;
			}
			for(; i < gathered_attributes.size(); ++i) {
				VariantClear(&(gathered_attributes[i].varValue));
			}
			gathered_attributes.clear();
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetEndACP(__RPC__out LONG* pacp) {
			if(!pacp)
				return E_INVALIDARG;
			if(document_lock_state == lock_state::unlocked)
				return TS_E_NOLOCK;
			*pacp = 0;
			if(!ei)
				return TF_E_DISCONNECTED;
			*pacp = LONG(ei->get_text_length());
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetActiveView(__RPC__out TsViewCookie* pvcView) {
			if(!pvcView)
				return E_INVALIDARG;
			*pvcView = 0;
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetACPFromPoint(TsViewCookie /*vcView*/, __RPC__in const POINT* ptScreen, DWORD dwFlags, __RPC__out LONG* pacp) {

			*pacp = 0;
			if(!ei)
				return TF_E_DISCONNECTED;

			if((GXFPF_NEAREST & dwFlags) == 0) {
				auto control = ei->get_layout_interface();
				if(!control)
					return TF_E_DISCONNECTED;

				auto window_rect = win.window_interface.get_window_location();
				auto client_rect = control->l_id != layout_reference_none ?
					win.get_current_location(control->l_id) :
					screen_space_rect{ 0,0,0,0 };
				if(ptScreen->x < window_rect.x + client_rect.x || ptScreen->y < window_rect.y + client_rect.y
					|| ptScreen->x > window_rect.x + client_rect.x + client_rect.width || ptScreen->y > window_rect.y + client_rect.y + client_rect.height) {
					return TS_E_INVALIDPOINT;
				}
			}

			auto from_point = ei->get_position_from_screen_point(win, screen_space_point{ int32_t(ptScreen->x), int32_t(ptScreen->y) });
			*pacp = from_point;
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetTextExt(TsViewCookie /*vcView*/, LONG acpStart, LONG acpEnd, __RPC__out RECT* prc, __RPC__out BOOL* pfClipped) {
			if(!pfClipped || !prc)
				return E_INVALIDARG;
			if(document_lock_state == lock_state::unlocked)
				return TS_E_NOLOCK;

			*pfClipped = FALSE;
			*prc = RECT{ 0,0,0,0 };

			if(win.window_interface.is_minimized()) {
				*pfClipped = TRUE;
				return S_OK;
			}
			if(!ei)
				return TF_E_DISCONNECTED;

			std::vector<screen_space_rect> selection_rects;
			ei->get_range_bounds(win, uint32_t(acpStart), uint32_t(acpEnd), selection_rects);
			if(selection_rects.size() > 0) {
				prc->left = selection_rects[0].x;
				prc->top = selection_rects[0].y;
				prc->right = selection_rects[0].x + selection_rects[0].width;
				prc->bottom = selection_rects[0].y + selection_rects[0].height;
			}
			for(uint32_t i = 1; i < selection_rects.size(); ++i) {
				prc->left = std::min(LONG(selection_rects[i].x), prc->left);
				prc->top = std::min(LONG(selection_rects[i].y), prc->top);
				prc->right = std::max(LONG(selection_rects[i].x + selection_rects[i].width), prc->right);
				prc->bottom = std::max(LONG(selection_rects[i].y + selection_rects[i].height), prc->bottom);
			}
			
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetScreenExt(TsViewCookie /*vcView*/, __RPC__out RECT* prc) {
			if(!prc)
				return E_INVALIDARG;

			*prc = RECT{ 0,0,0,0 };

			if(!ei)
				return TF_E_DISCONNECTED;
			auto control = ei->get_layout_interface();
			if(!control)
				return TF_E_DISCONNECTED;
			if(win.window_interface.is_minimized())
				return S_OK;

			auto& node = win.get_node(control->l_id);
			auto location = win.get_current_location(control->l_id);
			ui_rectangle temp;
			temp.x_position = uint16_t(location.x);
			temp.y_position = uint16_t(location.y);
			temp.width = uint16_t(location.width);
			temp.height = uint16_t(location.height);

			auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, node.width - (node.left_margin() + node.right_margin()), 1, temp);

			auto window_rect = win.window_interface.get_window_location();

			prc->left = window_rect.x + new_content_rect.x;
			prc->top = window_rect.y + new_content_rect.y;
			prc->right = window_rect.x + new_content_rect.x + new_content_rect.width;
			prc->bottom = window_rect.y + new_content_rect.y + new_content_rect.height;

			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetInputScopes(__RPC__deref_out_ecount_full_opt(*pcCount) InputScope** pprgInputScopes, __RPC__out UINT* pcCount) {
			*pprgInputScopes = (InputScope*)CoTaskMemAlloc(sizeof(InputScope));
			if(ei) {
				switch(ei->get_type()) {
					case edit_contents::generic_text:
						*(*pprgInputScopes) = IS_TEXT;
						break;
					case edit_contents::number:
						*(*pprgInputScopes) = IS_NUMBER;
						break;
					case edit_contents::single_char:
						*(*pprgInputScopes) = IS_ONECHAR;
						break;
					case edit_contents::email:
						*(*pprgInputScopes) = IS_EMAIL_SMTPEMAILADDRESS;
						break;
					case edit_contents::date:
						*(*pprgInputScopes) = IS_DATE_FULLDATE;
						break;
					case edit_contents::time:
						*(*pprgInputScopes) = IS_TIME_FULLTIME;
						break;
					case edit_contents::url:
						*(*pprgInputScopes) = IS_URL;
						break;
				}
			} else {
				*(*pprgInputScopes) = IS_DEFAULT;
			}
			*pcCount = 1;
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE GetPhrase(__RPC__deref_out_ecount_full_opt(*pcCount) BSTR** ppbstrPhrases, __RPC__out UINT* pcCount) {
			*ppbstrPhrases = nullptr;
			*pcCount = 0;
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE GetRegularExpression(__RPC__deref_out_opt BSTR* pbstrRegExp) {
			*pbstrRegExp = nullptr;
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE GetSRGS(__RPC__deref_out_opt BSTR* pbstrSRGS) {
			*pbstrSRGS = nullptr;
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE GetXML(__RPC__deref_out_opt BSTR* pbstrXML) {
			*pbstrXML = nullptr;
			return E_NOTIMPL;
		}

		text_services_object(ITfThreadMgr* manager_ptr, TfClientId owner, window_data& win, edit_interface* ei) : win(win), ei(ei), m_refCount(1) {
			manager_ptr->CreateDocumentMgr(&document);
			auto hr = document->CreateContext(owner, 0, static_cast<ITextStoreACP2*>(this), &primary_context, &content_identifier);
			hr = document->Push(primary_context);
		}
		virtual ~text_services_object() {
			free_gathered_attributes();
		}
	};


	void release_text_services_object(text_services_object* ptr) {
		ptr->ei = nullptr;
		safe_release(ptr->primary_context);
		safe_release(ptr->document);
		ptr->Release();
	}


	win32_text_services::win32_text_services() {
		CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER,
			IID_ITfThreadMgr, (void**)&manager_ptr);
	}
	win32_text_services::~win32_text_services() {
		safe_release(manager_ptr);
	}

	void win32_text_services::start_text_services() {
		manager_ptr->Activate(&client_id);
		//manager_ptr->SuspendKeystrokeHandling();
	}
	void win32_text_services::end_text_services() {
		manager_ptr->Deactivate();
	}
	void win32_text_services::on_text_change(text_services_object* ts, uint32_t old_start, uint32_t old_end, uint32_t new_end) {
		if(send_notifications && ts->advise_sink && ts->notify_on_text_change) {
			TS_TEXTCHANGE chng{LONG(old_start), LONG(old_end), LONG(new_end) };
			ts->advise_sink->OnTextChange(0, &chng);
		}
	}
	void win32_text_services::on_selection_change(text_services_object* ts) {
		if(send_notifications && ts->advise_sink && ts->notify_on_selection_change) {
			ts->advise_sink->OnSelectionChange();
		}
	}
	bool win32_text_services::send_mouse_event_to_tso(text_services_object* ts, int32_t x, int32_t y, uint32_t buttons) {
		return ts->send_mouse_event(x, y, buttons);
	}

	void win32_text_services::set_focus(window_data& win, text_services_object* o) {
		//manager_ptr->SetFocus(o->document);

		auto hwnd = win.window_interface.get_hwnd();
		if(hwnd) {
			ITfDocumentMgr* old_doc = nullptr;
			manager_ptr->AssociateFocus((HWND)hwnd, o ? o->document : nullptr, &old_doc);
			safe_release(old_doc);
		} 
	}
	void win32_text_services::suspend_keystroke_handling() {
		//manager_ptr->SuspendKeystrokeHandling();
	}
	void win32_text_services::resume_keystroke_handling() {
		//manager_ptr->ResumeKeystrokeHandling();
	}

	text_services_object* win32_text_services::create_text_service_object(window_data& win, edit_interface& ei) {
		return new text_services_object(manager_ptr, client_id, win, &ei);
	}

	void direct_write_text::load_fonts_from_directory(window_data const& win, std::wstring const& directory, IDWriteFontSetBuilder2* bldr) {
		win.file_system.for_each_filtered_file(directory + L"\\*.otf", [bldr, &directory](std::wstring const& name) {
			std::wstring fname = directory + L"\\" + name;
		bldr->AddFontFile(fname.c_str());
			});
		win.file_system.for_each_filtered_file(directory + L"\\*.ttf", [bldr, &directory](std::wstring const& name) {
			std::wstring fname = directory + L"\\" + name;
		bldr->AddFontFile(fname.c_str());
			});
	}

	void direct_write_text::load_fallbacks_by_type(std::vector<font_fallback> const& fb, font_type type, IDWriteFontFallbackBuilder* bldr, IDWriteFontCollection1* collection) {
		for(auto& f : fb) {
			if(f.type == type) {
				WCHAR const* fn[] = { f.name.c_str() };
				std::vector<DWRITE_UNICODE_RANGE> ranges;
				for(auto& r : f.ranges) {
					ranges.push_back(DWRITE_UNICODE_RANGE{ r.start, r.end });
				}

				bldr->AddMapping(ranges.data(), uint32_t(ranges.size()), fn, 1, collection, nullptr, nullptr, f.scale);
			}
		}

		IDWriteFontFallback* system_fallback;
		dwrite_factory->GetSystemFontFallback(&system_fallback);
		bldr->AddMappings(system_fallback);
		safe_release(system_fallback);
	}

	void direct_write_text::initialize_font_fallbacks(window_data& win) {
		win.file_system.load_global_font_fallbacks(win.dynamic_settings);

		{
			safe_release(font_fallbacks);
			IDWriteFontFallbackBuilder* bldr = nullptr;
			dwrite_factory->CreateFontFallbackBuilder(&bldr);

			load_fallbacks_by_type(win.dynamic_settings.fallbacks, win.dynamic_settings.primary_font.type, bldr, font_collection);

			bldr->CreateFontFallback(&font_fallbacks);
			safe_release(bldr);
		}
		{
			safe_release(small_font_fallbacks);
			IDWriteFontFallbackBuilder* bldr = nullptr;
			dwrite_factory->CreateFontFallbackBuilder(&bldr);

			load_fallbacks_by_type(win.dynamic_settings.fallbacks, win.dynamic_settings.small_font.type, bldr, font_collection);

			bldr->CreateFontFallback(&small_font_fallbacks);
			safe_release(bldr);
		}
		{
			safe_release(header_font_fallbacks);
			IDWriteFontFallbackBuilder* bldr = nullptr;
			dwrite_factory->CreateFontFallbackBuilder(&bldr);

			load_fallbacks_by_type(win.dynamic_settings.fallbacks, win.dynamic_settings.header_font.type, bldr, font_collection);

			bldr->CreateFontFallback(&header_font_fallbacks);
			safe_release(bldr);
		}
	}

	direct_write_text::direct_write_text() {
		DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(dwrite_factory), reinterpret_cast<IUnknown**>(&dwrite_factory));
	}

	void direct_write_text::create_font_collection(window_data& win) {
		safe_release(font_collection);

		IDWriteFontSetBuilder2* bldr = nullptr;
		dwrite_factory->CreateFontSetBuilder(&bldr);

		{
			auto root = win.file_system.get_root_directory();
			auto font_dir = root + win.dynamic_settings.font_directory;
			load_fonts_from_directory(win, font_dir, bldr);
		}
		{
			auto root = win.file_system.get_common_printui_directory();
			auto font_dir = root + win.dynamic_settings.font_directory;
			load_fonts_from_directory(win, font_dir, bldr);
		}

		IDWriteFontSet* sysfs = nullptr;
		dwrite_factory->GetSystemFontSet(&sysfs);
		if(sysfs) bldr->AddFontSet(sysfs);

		IDWriteFontSet* fs = nullptr;
		bldr->CreateFontSet(&fs);
		dwrite_factory->CreateFontCollectionFromFontSet(fs, DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC, &font_collection);

		safe_release(fs);
		safe_release(sysfs);
		safe_release(bldr);
	}

	DWRITE_FONT_STRETCH from_float_stretch_to_enum(float val) {
		if(val < 56.25f) {
			return DWRITE_FONT_STRETCH_ULTRA_CONDENSED;
		} else if(val < 68.75f) {
			return DWRITE_FONT_STRETCH_EXTRA_CONDENSED;
		} else if(val < 81.25f) {
			return DWRITE_FONT_STRETCH_CONDENSED;
		} else if(val < 93.75f) {
			return DWRITE_FONT_STRETCH_SEMI_CONDENSED;
		} else if(val < 106.25f) {
			return DWRITE_FONT_STRETCH_NORMAL;
		} else if(val < 118.75f) {
			return DWRITE_FONT_STRETCH_SEMI_EXPANDED;
		} else if(val < 137.5f) {
			return DWRITE_FONT_STRETCH_EXPANDED;
		} else if(val < 175.0f) {
			return DWRITE_FONT_STRETCH_EXTRA_EXPANDED;
		} else {
			return DWRITE_FONT_STRETCH_ULTRA_EXPANDED;
		}
	}

	void direct_write_text::initialize_fonts(window_data& win) {
		// metrics
		{
			IDWriteFont3* f = nullptr;
			IDWriteFontList2* fl;
			DWRITE_FONT_AXIS_VALUE fax[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					float(win.dynamic_settings.primary_font.weight)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					win.dynamic_settings.primary_font.span }
			};

			font_collection->GetMatchingFonts(win.dynamic_settings.primary_font.name.c_str(), fax, 2, &fl);
			fl->GetFont(0, &f);
			safe_release(fl);

			update_font_metrics(win.dynamic_settings.primary_font, win.text_data.locale_string(), float(win.layout_size), win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f, f);
			safe_release(f);
		}
		{

			IDWriteFontList2* fl2;
			DWRITE_FONT_AXIS_VALUE fax2[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					float(win.dynamic_settings.small_font.weight)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					win.dynamic_settings.small_font.span }
			};

			IDWriteFont3* f2 = nullptr;
			font_collection->GetMatchingFonts(win.dynamic_settings.small_font.name.c_str(), fax2, 2, &fl2);
			fl2->GetFont(0, &f2);
			safe_release(fl2);

			update_font_metrics(win.dynamic_settings.small_font, win.text_data.locale_string(), std::round(float(win.layout_size) * win.dynamic_settings.small_size_multiplier), win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f, f2);
			safe_release(f2);
			
		}
		{
			IDWriteFont3* f = nullptr;
			IDWriteFontList2* fl;
			DWRITE_FONT_AXIS_VALUE fax[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					float(win.dynamic_settings.header_font.weight)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					win.dynamic_settings.header_font.span } };

			font_collection->GetMatchingFonts(win.dynamic_settings.header_font.name.c_str(), fax, 2, &fl);
			fl->GetFont(0, &f);
			safe_release(fl);

			update_font_metrics(win.dynamic_settings.header_font, win.text_data.locale_string(), std::round(float(win.layout_size) * win.dynamic_settings.heading_size_multiplier), win.dynamic_settings.global_size_multiplier * win.dpi / 96.0f, f);
			safe_release(f);
		}


		auto locale_str = win.text_data.locale_string() != nullptr ? win.text_data.locale_string() : L"";
		// text formats
		{
			safe_release(common_text_format);

			DWRITE_FONT_AXIS_VALUE fax[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					float(win.dynamic_settings.primary_font.weight)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					win.dynamic_settings.primary_font.span } ,
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_OPTICAL_SIZE,
					win.dynamic_settings.primary_font.font_size * 96.0f / win.dpi } };

			dwrite_factory->CreateTextFormat(win.dynamic_settings.primary_font.name.c_str(), font_collection, fax, 3, win.dynamic_settings.primary_font.font_size, locale_str, &common_text_format);
			common_text_format->SetFontFallback(font_fallbacks);
			common_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			common_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, win.dynamic_settings.primary_font.line_spacing, win.dynamic_settings.primary_font.baseline);
			common_text_format->SetAutomaticFontAxes(DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE);

		}
		{
			safe_release(small_text_format);

			DWRITE_FONT_AXIS_VALUE fax[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					float(win.dynamic_settings.small_font.weight)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					win.dynamic_settings.small_font.span } ,
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_OPTICAL_SIZE,
					win.dynamic_settings.small_font.font_size * 96.0f / win.dpi} };


			dwrite_factory->CreateTextFormat(win.dynamic_settings.small_font.name.c_str(), font_collection, fax, 3, win.dynamic_settings.small_font.font_size, locale_str, &small_text_format);
			small_text_format->SetFontFallback(small_font_fallbacks);

			small_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			small_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, win.dynamic_settings.small_font.line_spacing, win.dynamic_settings.small_font.baseline);
			small_text_format->SetAutomaticFontAxes(DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE);
		}
		{
			safe_release(header_text_format);

			DWRITE_FONT_AXIS_VALUE fax[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					float(win.dynamic_settings.header_font.weight)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					win.dynamic_settings.header_font.span },
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_OPTICAL_SIZE,
					win.dynamic_settings.header_font.font_size * 96.0f / win.dpi} };

			dwrite_factory->CreateTextFormat(win.dynamic_settings.header_font.name.c_str(), font_collection, fax, 3, win.dynamic_settings.header_font.font_size, locale_str, &header_text_format);
			header_text_format->SetFontFallback(header_font_fallbacks);
			header_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			header_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, win.dynamic_settings.header_font.line_spacing, win.dynamic_settings.header_font.baseline + std::round((win.layout_size * 2.0f - win.dynamic_settings.header_font.line_spacing / 2.0f)));
			//header_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, win.dynamic_settings.header_font.line_spacing, win.dynamic_settings.header_font.baseline);
			header_text_format->SetAutomaticFontAxes(DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE);
		}

		update_text_rendering_parameters(win);

		win.window_bar.print_ui_settings.primary_font_menu.open_button.set_text(win, win.dynamic_settings.primary_font.name);
		win.window_bar.print_ui_settings.small_font_menu.open_button.set_text(win, win.dynamic_settings.small_font.name);
		win.window_bar.print_ui_settings.header_font_menu.open_button.set_text(win, win.dynamic_settings.header_font.name);

		if(win.window_bar.print_ui_settings.primary_font_italic_toggle.toggle_is_on != win.dynamic_settings.primary_font.is_oblique) {
			win.window_bar.print_ui_settings.primary_font_italic_toggle.change_toggle_state(win, win.dynamic_settings.primary_font.is_oblique);
		}
		if(win.window_bar.print_ui_settings.small_font_italic_toggle.toggle_is_on != win.dynamic_settings.small_font.is_oblique) {
			win.window_bar.print_ui_settings.small_font_italic_toggle.change_toggle_state(win, win.dynamic_settings.small_font.is_oblique);
		}
		if(win.window_bar.print_ui_settings.header_font_italic_toggle.toggle_is_on != win.dynamic_settings.header_font.is_oblique) {
			win.window_bar.print_ui_settings.header_font_italic_toggle.change_toggle_state(win, win.dynamic_settings.header_font.is_oblique);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.primary_font.weight, 0);
			win.window_bar.print_ui_settings.primary_font_weight_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.primary_font.span, 0);
			win.window_bar.print_ui_settings.primary_font_stretch_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.small_font.weight, 0);
			win.window_bar.print_ui_settings.small_font_weight_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.small_font.span, 0);
			win.window_bar.print_ui_settings.small_font_stretch_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.header_font.weight, 0);
			win.window_bar.print_ui_settings.header_font_weight_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.header_font.span, 0);
			win.window_bar.print_ui_settings.header_font_stretch_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.primary_font.top_leading, 0);
			win.window_bar.print_ui_settings.primary_top_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.primary_font.bottom_leading, 0);
			win.window_bar.print_ui_settings.primary_bottom_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.small_font.top_leading, 0);
			win.window_bar.print_ui_settings.small_top_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.small_font.bottom_leading, 0);
			win.window_bar.print_ui_settings.small_bottom_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.header_font.top_leading, 0);
			win.window_bar.print_ui_settings.header_top_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.header_font.bottom_leading, 0);
			win.window_bar.print_ui_settings.header_bottom_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.small_size_multiplier, 3);
			win.window_bar.print_ui_settings.small_relative_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.heading_size_multiplier, 3);
			win.window_bar.print_ui_settings.header_relative_e.quiet_set_text(win, result_to_str.text_content.text);
		}
	}

	void direct_write_text::update_font_metrics(font_description& desc, wchar_t const* locale, float target_pixels, float dpi_scale, IDWriteFont* font) {

		DWRITE_FONT_METRICS metrics;
		font->GetMetrics(&metrics);


		desc.line_spacing = target_pixels;

		auto top_lead = std::floor(desc.top_leading * dpi_scale);
		auto bottom_lead = std::floor(desc.bottom_leading * dpi_scale);

		// try to get cap height an integral number of pixels
		auto temp_font_size = float(metrics.designUnitsPerEm) * (target_pixels - (top_lead + bottom_lead)) / float(metrics.ascent + metrics.descent + metrics.lineGap);
		auto temp_cap_height = float(metrics.capHeight) * temp_font_size / (float(metrics.designUnitsPerEm));
		auto desired_cap_height = std::round(temp_cap_height);
		auto adjusted_size = (metrics.capHeight > 0 && metrics.capHeight <= metrics.ascent) ? desired_cap_height * float(metrics.designUnitsPerEm) / float(metrics.capHeight) : temp_font_size;
		{
		//	auto temp_x_height = float(metrics.xHeight) * temp_font_size / (float(metrics.designUnitsPerEm));
		//	auto desired_x_height = std::round(temp_x_height);
		//	adjusted_size = (metrics.xHeight > 0 && metrics.xHeight < metrics.ascent) ? desired_x_height * float(metrics.designUnitsPerEm) / float(metrics.xHeight) : adjusted_size;
		}

		desc.font_size = adjusted_size;
		desc.baseline = std::round(top_lead + float(metrics.ascent + metrics.lineGap / 2) * temp_font_size / (float(metrics.designUnitsPerEm)));

		desc.descender = float(metrics.descent) * desc.font_size / (float(metrics.designUnitsPerEm));

		IDWriteTextAnalyzer* analyzer_base = nullptr;
		IDWriteTextAnalyzer1* analyzer = nullptr;
		auto hr = dwrite_factory->CreateTextAnalyzer(&analyzer_base);

		desc.vertical_baseline = std::round(target_pixels / 2.0f);

		if(SUCCEEDED(hr)) {
			hr = analyzer_base->QueryInterface(IID_PPV_ARGS(&analyzer));
		}
		if(SUCCEEDED(hr)) {
			IDWriteFontFace* face = nullptr;
			font->CreateFontFace(&face);

			BOOL vert_exists_out = FALSE;
			int32_t vert_base_out = 0;
			analyzer->GetBaseline(face, DWRITE_BASELINE_CENTRAL, TRUE, TRUE, DWRITE_SCRIPT_ANALYSIS{}, locale, &vert_base_out, &vert_exists_out);

			desc.vertical_baseline = std::round(top_lead + float(vert_base_out) * desc.font_size / (float(metrics.designUnitsPerEm)));

			safe_release(face);
		}
		safe_release(analyzer);
		safe_release(analyzer_base);
	}

	void release_arranged_text(arranged_text* ptr) {
		auto p = (IDWriteTextLayout*)(ptr);
		p->Release();
	}

	void direct_write_text::apply_formatting(IDWriteTextLayout* target, std::vector<format_marker> const& formatting, std::vector<font_description> const& named_fonts) const {
		{
			//apply fonts
			std::vector<uint32_t> font_stack;
			uint32_t font_start = 0;

			for(auto f : formatting) {
				if(std::holds_alternative<font_id>(f.format)) {
					auto fid = std::get<font_id>(f.format);
					if(fid.id < named_fonts.size()) {
						if(font_stack.empty()) {
							font_stack.push_back(fid.id);
							font_start = f.position;

						} else if(font_stack.back() != fid.id) {
							target->SetFontFamilyName(named_fonts[font_stack.back()].name.c_str(), DWRITE_TEXT_RANGE{ font_start, f.position });
							target->SetFontWeight(DWRITE_FONT_WEIGHT(named_fonts[font_stack.back()].weight), DWRITE_TEXT_RANGE{ font_start, f.position - font_start });
							target->SetFontStretch(from_float_stretch_to_enum(named_fonts[font_stack.back()].span), DWRITE_TEXT_RANGE{ font_start, f.position - font_start });
							if(named_fonts[font_stack.back()].is_oblique) {
								target->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, DWRITE_TEXT_RANGE{ font_start, f.position - font_start });
							}

							font_stack.push_back(fid.id);
							font_start = f.position;

						} else {
							target->SetFontFamilyName(named_fonts[fid.id].name.c_str(), DWRITE_TEXT_RANGE{ font_start, f.position - font_start });
							target->SetFontWeight(DWRITE_FONT_WEIGHT(named_fonts[fid.id].weight), DWRITE_TEXT_RANGE{ font_start, f.position - font_start });
							target->SetFontStretch(from_float_stretch_to_enum(named_fonts[fid.id].span), DWRITE_TEXT_RANGE{ font_start, f.position - font_start });
							if(named_fonts[fid.id].is_oblique) {
								target->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, DWRITE_TEXT_RANGE{ font_start, f.position - font_start });
							}

							font_stack.pop_back();
							font_start = f.position;
						}
					}
				}
			}
		}

		{
			//apply bold
			bool bold_is_applied = false;
			uint32_t bold_start = 0;
			for(auto f : formatting) {
				if(std::holds_alternative<extra_formatting>(f.format)) {
					if(std::get<extra_formatting>(f.format) == extra_formatting::bold) {
						if(bold_is_applied) {
							target->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_RANGE{ bold_start, f.position - bold_start });
							bold_is_applied = false;
						} else {
							bold_start = f.position;
							bold_is_applied = true;
						}
					}
				}
			}
		}

		{
			//apply italics
			bool italic_is_applied = false;
			uint32_t italic_start = 0;
			for(auto f : formatting) {
				if(std::holds_alternative<extra_formatting>(f.format)) {
					if(std::get<extra_formatting>(f.format) == extra_formatting::italic) {
						if(italic_is_applied) {
							target->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, DWRITE_TEXT_RANGE{ italic_start, f.position - italic_start });
							italic_is_applied = false;
						} else {
							italic_start = f.position;
							italic_is_applied = true;
						}
					}
				}
			}
		}

		{
			//apply small caps
			IDWriteTypography* t = nullptr;

			bool typo_applied = false;
			uint32_t typo_start = 0;
			for(auto f : formatting) {
				if(std::holds_alternative<extra_formatting>(f.format)) {
					if(std::get<extra_formatting>(f.format) == extra_formatting::small_caps) {
						if(typo_applied) {
							if(!t) {
								dwrite_factory->CreateTypography(&t);
								if(auto dir = target->GetReadingDirection(); dir == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT) {
									apply_default_ltr_options(t);
								} else if(dir == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT) {
									apply_default_rtl_options(t);
								} else {
									apply_default_vertical_options(t);
								}
								apply_small_caps_options(t);
							}
							target->SetTypography(t, DWRITE_TEXT_RANGE{ typo_start, f.position - typo_start });
							typo_applied = false;
						} else {
							typo_start = f.position;
							typo_applied = true;
						}
					}
				}
			}

			safe_release(t);
		}

		{
			//apply tabular numbers
			IDWriteTypography* t = nullptr;

			bool typo_applied = false;
			uint32_t typo_start = 0;
			for(auto f : formatting) {
				if(std::holds_alternative<extra_formatting>(f.format)) {
					if(std::get<extra_formatting>(f.format) == extra_formatting::tabular_numbers) {
						if(typo_applied) {
							if(!t) {
								dwrite_factory->CreateTypography(&t);
								if(auto dir = target->GetReadingDirection(); dir == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT) {
									apply_default_ltr_options(t);
								} else if(dir == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT) {
									apply_default_rtl_options(t);
								} else {
									apply_default_vertical_options(t);
								}
								apply_lining_figures_options(t);
							}
							target->SetTypography(t, DWRITE_TEXT_RANGE{ typo_start, f.position - typo_start });
							typo_applied = false;
						} else {
							typo_start = f.position;
							typo_applied = true;
						}
					}
				}
			}

			safe_release(t);
		}

		{
			//apply old style numbers
			IDWriteTypography* t = nullptr;

			bool typo_applied = false;
			uint32_t typo_start = 0;
			for(auto f : formatting) {
				if(std::holds_alternative<extra_formatting>(f.format)) {
					if(std::get<extra_formatting>(f.format) == extra_formatting::old_numbers) {
						if(typo_applied) {
							if(!t) {
								dwrite_factory->CreateTypography(&t);
								if(auto dir = target->GetReadingDirection(); dir == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT) {
									apply_default_ltr_options(t);
								} else if(dir == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT) {
									apply_default_rtl_options(t);
								} else {
									apply_default_vertical_options(t);
								}
								apply_old_style_figures_options(t);
							}
							target->SetTypography(t, DWRITE_TEXT_RANGE{ typo_start, f.position - typo_start });
							typo_applied = false;
						} else {
							typo_start = f.position;
							typo_applied = true;
						}
					}
				}
			}

			safe_release(t);
		}
	}


	arrangement_result direct_write_text::create_text_arragement(window_data const& win, std::wstring_view text, content_alignment text_alignment, text_size text_sz, bool single_line, int32_t max_width, std::vector<format_marker> const* formatting) const {

		IDWriteTextLayout* formatted_text = nullptr;
		arrangement_result result;

		auto text_format = text_sz == text_size::standard ? common_text_format : (text_sz == text_size::note ? small_text_format : header_text_format);
		auto& font = text_sz == text_size::standard ? win.dynamic_settings.primary_font : (text_sz == text_size::note ? win.dynamic_settings.small_font : win.dynamic_settings.header_font);

		text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		text_format->SetReadingDirection(DWRITE_READING_DIRECTION(reading_direction_from_orientation(win.orientation)));
		text_format->SetFlowDirection(DWRITE_FLOW_DIRECTION(flow_direction_from_orientation(win.orientation)));
		text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		text_format->SetOpticalAlignment(DWRITE_OPTICAL_ALIGNMENT_NO_SIDE_BEARINGS);

		if(!horizontal(win.orientation)) {
			if(text_sz == text_size::header) {
				if(win.orientation == layout_orientation::vertical_right_to_left)
					text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, font.line_spacing, font.vertical_baseline + std::round((win.layout_size * 2.0f - win.dynamic_settings.header_font.line_spacing) / 2.0f));
				else
					text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, font.line_spacing, font.vertical_baseline - std::round((win.layout_size * 2.0f - win.dynamic_settings.header_font.line_spacing) / 2.0f));
			} else {
				text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, font.line_spacing, font.vertical_baseline);
			}
		} else {
			if(text_sz == text_size::header)
				header_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, win.dynamic_settings.header_font.line_spacing, win.dynamic_settings.header_font.baseline + std::round((win.layout_size * 2.0f - win.dynamic_settings.header_font.line_spacing) / 2.0f));
			else
				text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, font.line_spacing, font.baseline);
		}
		if(horizontal(win.orientation)) {

			dwrite_factory->CreateTextLayout(text.data(), uint32_t(text.length()), text_format, float(font.line_spacing), float(font.line_spacing), &formatted_text);

			if(font.is_oblique)
				formatted_text->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, DWRITE_TEXT_RANGE{ 0, uint32_t(text.length()) });
			if(formatting)
				apply_formatting(formatted_text, *formatting, win.dynamic_settings.named_fonts);

			DWRITE_TEXT_METRICS text_metrics;
			formatted_text->GetMetrics(&text_metrics);

			result.width_used = int32_t(std::ceil((text_metrics.width) / float(win.layout_size)));

			if(single_line || result.width_used <= max_width) {
				formatted_text->SetMaxWidth(float(result.width_used * win.layout_size));
				formatted_text->SetMaxHeight(float(font.line_spacing));
				formatted_text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT(content_alignment_to_text_alignment(text_alignment)));
				formatted_text->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);
				result.lines_used = 1;
			} else {
				formatted_text->SetMaxWidth(float(max_width * win.layout_size));
				formatted_text->SetMaxHeight(float(100 * font.line_spacing));
				formatted_text->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);

				formatted_text->GetMetrics(&text_metrics);
				result.lines_used = int32_t(std::ceil(text_metrics.lineCount * font.line_spacing / float(win.layout_size)));
				result.width_used = max_width;
				formatted_text->SetMaxHeight(float(text_metrics.lineCount * font.line_spacing));
				formatted_text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT(content_alignment_to_text_alignment(text_alignment)));
			}

		} else {
			dwrite_factory->CreateTextLayout(text.data(), uint32_t(text.length()), text_format, float(font.line_spacing), float(font.line_spacing), &formatted_text);

			if(font.is_oblique)
				formatted_text->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, DWRITE_TEXT_RANGE{ 0, uint32_t(text.length()) });
			if(formatting)
				apply_formatting(formatted_text, *formatting, win.dynamic_settings.named_fonts);

			DWRITE_TEXT_METRICS text_metrics;
			formatted_text->GetMetrics(&text_metrics);

			result.width_used = int32_t(std::ceil(text_metrics.height / float(win.layout_size)));
			if(single_line || result.width_used <= max_width) {
				formatted_text->SetMaxHeight(float(result.width_used * font.line_spacing));
				formatted_text->SetMaxWidth(float(font.line_spacing));
				formatted_text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT(content_alignment_to_text_alignment(text_alignment)));
				formatted_text->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);
				result.lines_used = 1;
			} else {
				formatted_text->SetMaxHeight(float(max_width * win.layout_size));
				formatted_text->SetMaxWidth(float(100 * font.line_spacing));
				formatted_text->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);

				formatted_text->GetMetrics(&text_metrics);
				result.lines_used = int32_t(std::ceil(text_metrics.lineCount * font.line_spacing / float(win.layout_size)));
				result.width_used = max_width;
				formatted_text->SetMaxWidth(float(text_metrics.lineCount * font.line_spacing));
				formatted_text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT(content_alignment_to_text_alignment(text_alignment)));
			}
		}
		result.ptr = (arranged_text*)(formatted_text);
		return result;
	}

	int32_t get_height(window_data const& win, arranged_text* txt, text_size sz) {
		DWRITE_TEXT_METRICS text_metrics;
		IDWriteTextLayout* formatted_text = (IDWriteTextLayout*)txt;
		formatted_text->GetMetrics(&text_metrics);
		return int32_t(text_metrics.lineCount * (sz == text_size::standard ? win.dynamic_settings.primary_font.line_spacing : (sz == text_size::note ? win.dynamic_settings.small_font.line_spacing : win.dynamic_settings.header_font.line_spacing)));
	}
	int32_t get_width(window_data const&, arranged_text* txt) {
		DWRITE_TEXT_METRICS text_metrics;
		IDWriteTextLayout* formatted_text = (IDWriteTextLayout*)txt;
		formatted_text->GetMetrics(&text_metrics);

		return int32_t(std::ceil(text_metrics.width));
	}
	bool appropriate_directionality(window_data const& win, arranged_text* txt) {
		IDWriteTextLayout* formatted_text = (IDWriteTextLayout*)txt;

		if(formatted_text->GetReadingDirection() !=
				DWRITE_READING_DIRECTION(reading_direction_from_orientation(win.orientation))
			|| formatted_text->GetFlowDirection() !=
				DWRITE_FLOW_DIRECTION(flow_direction_from_orientation(win.orientation))) {
			return false;
		} else {
			return true;
		}
	}

	bool adjust_layout_region(arranged_text* txt, int32_t width, int32_t height) {
		IDWriteTextLayout* formatted_text = (IDWriteTextLayout*)txt;

		bool adjusted = false;
		if(formatted_text->GetMaxWidth() != float(width)) {
			formatted_text->SetMaxWidth(float(width));
			adjusted = true;
		}
		if(formatted_text->GetMaxHeight() != float(height)) {
			formatted_text->SetMaxHeight(float(height));
			adjusted = true;
		}
		return adjusted;
	}

	std::vector<text_metrics> get_metrics_for_range(arranged_text* txt, uint32_t position, uint32_t length) {
		IDWriteTextLayout* formatted_text = (IDWriteTextLayout*)txt;

		uint32_t metrics_size = 0;
		formatted_text->HitTestTextRange(position, length, 0, 0, nullptr, 0, &metrics_size);

		std::vector<DWRITE_HIT_TEST_METRICS> mstorage(metrics_size);
		formatted_text->HitTestTextRange(position, length, 0, 0, mstorage.data(), metrics_size, &metrics_size);

		std::vector<text_metrics> result;
		result.reserve(metrics_size);

		for(auto& r : mstorage) {
			result.push_back(text_metrics{r.textPosition, r.length, r.left, r.top, r.width, r.height, r.bidiLevel});
		}

		return result;
	}

	text_metrics get_metrics_at_position(arranged_text* txt, uint32_t position) {
		IDWriteTextLayout* formatted_text = (IDWriteTextLayout*)txt;

		DWRITE_HIT_TEST_METRICS r;
		memset(&r, 0, sizeof(DWRITE_HIT_TEST_METRICS));
		float xout = 0;
		float yout = 0;
		formatted_text->HitTestTextPosition(position, FALSE, &xout, &yout, &r);
		return text_metrics{ r.textPosition, r.length, r.left, r.top, r.width, r.height, r.bidiLevel };
	}

	hit_test_metrics hit_test_text(arranged_text* txt, int32_t x, int32_t y) {
		IDWriteTextLayout* formatted_text = (IDWriteTextLayout*)txt;
		BOOL is_trailing = FALSE;
		BOOL is_inside = FALSE;
		DWRITE_HIT_TEST_METRICS r;
		memset(&r, 0, sizeof(DWRITE_HIT_TEST_METRICS));
		formatted_text->HitTestPoint(float(x), float(y), &is_trailing, &is_inside, &r);
		return hit_test_metrics{ text_metrics{ r.textPosition, r.length, r.left, r.top, r.width, r.height, r.bidiLevel } ,
			is_inside == TRUE, is_trailing == TRUE };
	}

	text_format direct_write_text::create_text_format(wchar_t const* name, int32_t capheight) const {
		IDWriteTextFormat3* label_format;

		DWRITE_FONT_AXIS_VALUE fax[] = {
			DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT, DWRITE_FONT_WEIGHT_BOLD},
			DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH, 100.0f } ,
			DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC, 0.0f } };

		IDWriteFont3* label_font = nullptr;

		IDWriteFontList2* fl;
		font_collection->GetMatchingFonts(name, fax, 3, &fl);
		fl->GetFont(0, &label_font);
		safe_release(fl);

		DWRITE_FONT_METRICS metrics;
		label_font->GetMetrics(&metrics);
		safe_release(label_font);

		auto adjusted_size = (metrics.capHeight > 0 && metrics.capHeight <= metrics.ascent) ? (capheight * float(metrics.designUnitsPerEm) / float(metrics.capHeight)) : (capheight * float(metrics.designUnitsPerEm) / float(metrics.ascent));

		auto baseline = float(metrics.ascent) * (adjusted_size) / (float(metrics.designUnitsPerEm));

		dwrite_factory->CreateTextFormat(L"Arial", font_collection, fax, 3, adjusted_size, L"", &label_format);
		label_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		label_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		label_format->SetAutomaticFontAxes(DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE);

		return text_format{(text_format_ptr*)(label_format), baseline };
	}
	void direct_write_text::release_text_format(text_format fmt) const {
		IDWriteTextFormat3* format = (IDWriteTextFormat3*)(fmt.ptr);
		format->Release();
	}
	void* direct_write_text::to_dwrite_format(text_format fmt) const {
		IDWriteTextFormat3* format = (IDWriteTextFormat3*)(fmt.ptr);
		return (void*)format;
	}
	void* direct_write_text::to_dwrite_layout(arranged_text* ptr) const {
		IDWriteTextLayout* formatted_text = (IDWriteTextLayout*)ptr;
		return (void*)formatted_text;
	}

	void* direct_write_text::get_dwrite_factory() const {
		return (void*)dwrite_factory;
	}

	std::vector<std::wstring> direct_write_text::ennumerate_fonts(std::wstring const& locale) const {
		std::vector<std::wstring> result;

		auto total_number = font_collection->GetFontFamilyCount();
		result.reserve(total_number);

		for(uint32_t i = 0; i < total_number; ++i) {
			IDWriteFontFamily* ffam = nullptr;
			font_collection->GetFontFamily(i, &ffam);

			IDWriteLocalizedStrings* famnames = nullptr;
			ffam->GetFamilyNames(&famnames);

			if(famnames->GetCount() > 0) {
				BOOL exists = FALSE;
				uint32_t index_out = 0;
				famnames->FindLocaleName(locale.c_str(), &index_out, &exists);

				if(exists == FALSE) {
					famnames->FindLocaleName(L"en", &index_out, &exists);
				}
				if(exists == FALSE) {
					index_out = 0;
				}

				uint32_t length_out = 0;
				famnames->GetStringLength(index_out, &length_out);

				wchar_t* name = new wchar_t[length_out + 1];
				famnames->GetString(index_out, name, length_out + 1);

				result.emplace_back(name);
				
				delete[] name;
			}
			safe_release(famnames);
			safe_release(ffam);
		}

		return result;
	}

	void direct_write_text::update_text_rendering_parameters(window_data& win) {
		
		safe_release(common_text_params);
		safe_release(small_text_params);
		safe_release(header_text_params);

		auto hwnd = (HWND)(win.window_interface.get_hwnd());
		if(hwnd) {
			IDWriteRenderingParams* rparams = nullptr;
			auto monitor_handle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
			dwrite_factory->CreateMonitorRenderingParams(monitor_handle, &rparams);
			if(rparams) {

				float default_gamma = rparams->GetGamma();
				float default_contrast = rparams->GetEnhancedContrast();

				IDWriteRenderingParams3* rparams3 = nullptr;
				rparams->QueryInterface(IID_PPV_ARGS(&rparams3));

				float default_gs_contrast = rparams3 ? rparams3->GetGrayscaleEnhancedContrast() : 1.0f;
				auto grid_fit = rparams3 ? rparams3->GetGridFitMode() : DWRITE_GRID_FIT_MODE_DEFAULT;

				dwrite_factory->CreateCustomRenderingParams(
					default_gamma,
					0.0f,
					0.0f,
					0.5f,
					DWRITE_PIXEL_GEOMETRY_FLAT,
					DWRITE_RENDERING_MODE1_OUTLINE,
					DWRITE_GRID_FIT_MODE_DISABLED,
					&header_text_params);

				dwrite_factory->CreateCustomRenderingParams(
					default_gamma,
					default_contrast,
					default_gs_contrast,
					0.5f,
					DWRITE_PIXEL_GEOMETRY_FLAT,
					DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC,
					grid_fit,
					&common_text_params);

				dwrite_factory->CreateCustomRenderingParams(
					default_gamma,
					default_contrast,
					default_gs_contrast,
					0.5f,
					DWRITE_PIXEL_GEOMETRY_FLAT,
					DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC_DOWNSAMPLED,
					grid_fit,
					&small_text_params);


				safe_release(rparams3);
				safe_release(rparams);
			}
		}
	}

	void* direct_write_text::get_rendering_paramters(text_size sz) const {
		switch(sz) {
			case text_size::note:
				return small_text_params;
			case text_size::standard:
				return common_text_params;
			case text_size::header:
				return header_text_params;
		}
		return common_text_params;
	}
}
