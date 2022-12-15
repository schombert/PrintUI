#include "printui_utility.hpp"

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

#include "printui_text_definitions.hpp"
#include "printui_windows_definitions.hpp"

#pragma comment(lib, "Usp10.lib")

namespace printui::text {
	void load_fallbacks_by_type(std::vector<font_fallback> const& fb, font_type type, IDWriteFontFallbackBuilder* bldr, IDWriteFontCollection1* collection, IDWriteFactory6* dwrite_factory) {
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

	void load_fonts_from_directory(std::wstring const& directory, IDWriteFontSetBuilder2* bldr) {
		{
			std::wstring otf = directory + L"\\*.otf";

			WIN32_FIND_DATAW fdata;
			HANDLE search_handle = FindFirstFile(otf.c_str(), &fdata);
			if(search_handle != INVALID_HANDLE_VALUE) {
				do {
					std::wstring fname = directory + L"\\" + fdata.cFileName;
					bldr->AddFontFile(fname.c_str());
				} while(FindNextFile(search_handle, &fdata));
				FindClose(search_handle);
			}
		}
		{
			std::wstring otf = directory + L"\\*.ttf";

			WIN32_FIND_DATAW fdata;
			HANDLE search_handle = FindFirstFile(otf.c_str(), &fdata);
			if(search_handle != INVALID_HANDLE_VALUE) {
				do {
					std::wstring fname = directory + L"\\" + fdata.cFileName;
					bldr->AddFontFile(fname.c_str());
				} while(FindNextFile(search_handle, &fdata));
				FindClose(search_handle);
			}
		}
	}

	void update_font_metrics(font_description& desc, IDWriteFactory6* dwrite_factory, wchar_t const* locale, float target_pixels, float dpi_scale, IDWriteFont* font) {

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


	void create_font_collection(window_data& win, std::wstring font_directory) {
		IDWriteFontSetBuilder2* bldr = nullptr;
		win.dwrite_factory->CreateFontSetBuilder(&bldr);

		if(font_directory.length() > 0) {
			WCHAR module_name[MAX_PATH] = {};
			int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);
			while(path_used >= 0 && module_name[path_used] != L'\\') {
				module_name[path_used] = 0;
				--path_used;
			}
			wcscat_s(module_name, MAX_PATH, font_directory.c_str());
			load_fonts_from_directory(module_name, bldr);
		}

		wchar_t* local_path_out = nullptr;
		if(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_path_out) == S_OK) {
			load_fonts_from_directory(std::wstring(local_path_out) + L"\\printui\\fonts", bldr);
		}
		CoTaskMemFree(local_path_out);

		IDWriteFontSet* sysfs = nullptr;
		win.dwrite_factory->GetSystemFontSet(&sysfs);
		if(sysfs) bldr->AddFontSet(sysfs);

		IDWriteFontSet* fs = nullptr;
		bldr->CreateFontSet(&fs);
		win.dwrite_factory->CreateFontCollectionFromFontSet(fs, DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC, &win.font_collection);

		safe_release(fs);
		safe_release(sysfs);
		safe_release(bldr);
	}

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
	
	std::wstring get_locale_name(std::wstring const& directory) {
		std::wstring dat = directory + L"\\*.dat";

		WIN32_FIND_DATAW fdata;
		HANDLE search_handle = FindFirstFile(dat.c_str(), &fdata);
		if(search_handle != INVALID_HANDLE_VALUE) {
			std::wstring result(fdata.cFileName);
			result.pop_back();
			result.pop_back();
			result.pop_back();
			result.pop_back();

			FindClose(search_handle);

			return result;
		} else {
			return std::wstring();
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
			WCHAR module_name[MAX_PATH] = {};
			int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);
			while(path_used >= 0 && module_name[path_used] != L'\\') {
				module_name[path_used] = 0;
				--path_used;
			}

			std::wstring locale_path = module_name + win.dynamic_settings.text_directory + L"\\" + full_compound;
			win.dynamic_settings.fallbacks.clear();
			if(update_settings) {
				load_locale_settings(locale_path, win.dynamic_settings, win.text_data.font_name_to_index);
			} else {
				load_locale_fonts(locale_path, win.dynamic_settings, win.text_data.font_name_to_index);
			}

			auto locale_name = get_locale_name(locale_path);
			win.window_bar.print_ui_settings.lang_menu.open_button.set_text(win, locale_name.length() > 0 ? locale_name : full_compound);
		}

		win.dynamic_settings.fallbacks.clear();
		win.initialize_font_fallbacks();
		win.intitialize_fonts();

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

	double text_manager::text_to_double(wchar_t* start, uint32_t count) const {
		auto olestr = SysAllocStringLen(start, count);
		double result = 0;
		if(olestr) {
			VarR8FromStr(olestr, lcid, 0, &result);
			SysFreeString(olestr);
		}
		return result;
	}
	int64_t text_manager::text_to_int(wchar_t* start, uint32_t count) const {
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

	void text_manager::load_text_from_directory(std::wstring const& directory) {
		std::wstring txt = directory + L"\\*.txt";

		WIN32_FIND_DATAW fdata;
		HANDLE search_handle = FindFirstFile(txt.c_str(), &fdata);
		if(search_handle != INVALID_HANDLE_VALUE) {
			do {
				std::wstring fname = directory + L"\\" + fdata.cFileName;

				HANDLE file_handle = nullptr;
				HANDLE mapping_handle = nullptr;
				char const* mapped_bytes = nullptr;


				file_handle = CreateFileW(fname.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
				if(file_handle == INVALID_HANDLE_VALUE) {
					file_handle = nullptr;
				} else {
					mapping_handle = CreateFileMapping(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
					if(mapping_handle) {
						mapped_bytes = (char const*)MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);

					}
				}

				if(mapped_bytes) {
					_LARGE_INTEGER pvalue;
					GetFileSizeEx(file_handle, &pvalue);
					text_data.consume_text_file(std::string_view(mapped_bytes, mapped_bytes + pvalue.QuadPart), font_name_to_index);
				}


				if(mapped_bytes)
					UnmapViewOfFile(mapped_bytes);
				if(mapping_handle)
					CloseHandle(mapping_handle);
				if(file_handle)
					CloseHandle(file_handle);

			} while(FindNextFile(search_handle, &fdata));
			FindClose(search_handle);
		}
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

		bool full_valid = [&](){
			auto a = GetFileAttributesW(desired_full_path.c_str());
			return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}();
		bool abbr_valid = [&]() {
			auto a = GetFileAttributesW(desired_abbr_path.c_str());
			return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}();

		if(full_valid)
			load_text_from_directory(desired_full_path);
		if(abbr_valid)
			load_text_from_directory(desired_abbr_path);

		if(!full_valid && !full_valid) {
			std::wstring en_path = module_name + win.dynamic_settings.text_directory + L"\\en";
			bool en_valid = [&]() {
				auto a = GetFileAttributesW(desired_abbr_path.c_str());
				return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
			}();

			if(en_valid) {
				load_text_from_directory(en_path);
			} else {
				// now we just take the first option w/o a dash

				std::wstring base_path = module_name + win.dynamic_settings.text_directory + L"\\*";

				WIN32_FIND_DATAW fdata;
				HANDLE search_handle = FindFirstFile(base_path.c_str(), &fdata);
				if(search_handle != INVALID_HANDLE_VALUE) {
					do {
						if((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && std::wstring(fdata.cFileName).find(L'-') == std::wstring::npos && fdata.cFileName != std::wstring(L".") && fdata.cFileName != std::wstring(L"..")) {
							load_text_from_directory(module_name + win.dynamic_settings.text_directory + L"\\" + fdata.cFileName);
							break;
						}
					} while(FindNextFile(search_handle, &fdata));
					FindClose(search_handle);
				}
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



	void apply_formatting(IDWriteTextLayout* target, std::vector<format_marker> const& formatting, std::vector<font_description> const& named_fonts, IDWriteFactory6* dwrite_factory) {

		{
			//apply fonts
			std::vector<uint32_t> font_stack;
			uint32_t font_start = 0;

			for(auto f : formatting) {
				format_marker j;
				if(std::holds_alternative<font_id>(f.format)) {
					auto fid = std::get<font_id>(f.format);
					if(fid.id < named_fonts.size()) {
						if(font_stack.empty()) {
							font_stack.push_back(fid.id);
							font_start = f.position;

						} else if(font_stack.back() != fid.id) {
							target->SetFontFamilyName(named_fonts[font_stack.back()].name.c_str(), DWRITE_TEXT_RANGE{ font_start, f.position });
							if(named_fonts[font_stack.back()].is_bold) {
								target->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_RANGE{ font_start, f.position });
							}
							if(named_fonts[font_stack.back()].is_oblique) {
								target->SetFontStyle(DWRITE_FONT_STYLE_OBLIQUE, DWRITE_TEXT_RANGE{ font_start, f.position });
							}

							font_stack.push_back(fid.id);
							font_start = f.position;

						} else {
							target->SetFontFamilyName(named_fonts[fid.id].name.c_str(), DWRITE_TEXT_RANGE{ font_start, f.position });
							if(named_fonts[fid.id].is_bold) {
								target->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_RANGE{ font_start, f.position });
							}
							if(named_fonts[fid.id].is_oblique) {
								target->SetFontStyle(DWRITE_FONT_STYLE_OBLIQUE, DWRITE_TEXT_RANGE{ font_start, f.position });
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
				format_marker j;
				if(std::holds_alternative<extra_formatting>(f.format)) {
					if(std::get<extra_formatting>(f.format) == extra_formatting::bold) {
						if(bold_is_applied) {
							target->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_RANGE{ bold_start, f.position });
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
				format_marker j;
				if(std::holds_alternative<extra_formatting>(f.format)) {
					if(std::get<extra_formatting>(f.format) == extra_formatting::italic) {
						if(italic_is_applied) {
							target->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, DWRITE_TEXT_RANGE{ italic_start, f.position });
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
				format_marker j;
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
							target->SetTypography(t, DWRITE_TEXT_RANGE{ typo_start, f.position });
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
				format_marker j;
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
							target->SetTypography(t, DWRITE_TEXT_RANGE{ typo_start, f.position });
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
				format_marker j;
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
							target->SetTypography(t, DWRITE_TEXT_RANGE{ typo_start, f.position });
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

	

	std::vector<language_description> ennumerate_languages(window_data const& win) {
		std::vector<language_description> result;

		WCHAR module_name[MAX_PATH] = {};
		int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);
		while(path_used >= 0 && module_name[path_used] != L'\\') {
			module_name[path_used] = 0;
			--path_used;
		}

		std::wstring locale_path = module_name + win.dynamic_settings.text_directory;
		std::wstring locale_search = module_name + win.dynamic_settings.text_directory + L"\\*";
		WIN32_FIND_DATAW fdata;
		HANDLE search_handle = FindFirstFile(locale_search.c_str(), &fdata);
		if(search_handle != INVALID_HANDLE_VALUE) {
			do {
				if((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && fdata.cFileName != std::wstring(L".") && fdata.cFileName != std::wstring(L"..")) {
					
					size_t first_dash = 0;
					for(; fdata.cFileName[first_dash] != 0; ++first_dash) {
						if(fdata.cFileName[first_dash] == L'-')
							break;
					}
					
					size_t region_end = fdata.cFileName[first_dash] != 0 ? first_dash + 1 : first_dash;
					for(; fdata.cFileName[region_end] != 0; ++region_end) {
						;
					}

					auto locale_name = get_locale_name(locale_path + L"\\" + fdata.cFileName);
					if(locale_name.length() > 0) {
						result.push_back(language_description{
							std::wstring(fdata.cFileName, fdata.cFileName + first_dash), // language
							fdata.cFileName[first_dash] != 0 // region
								? std::wstring(fdata.cFileName + first_dash + 1, fdata.cFileName + region_end)
								: std::wstring(),
							locale_name });
					}
				}
			} while(FindNextFile(search_handle, &fdata));
			FindClose(search_handle);
		}

		return result;
	}

	//Non spacing marks
	//SINGLE 16 BIT
	uint16_t isolated_nonspacing[] = {
	0x05BF,          //       HEBREW POINT RAFE
	0x05C7,          //       HEBREW POINT QAMATS QATAN
	0x0670,          //       ARABIC LETTER SUPERSCRIPT ALEF
	0x0711,          //       SYRIAC LETTER SUPERSCRIPT ALAPH
	0x07FD,          //       NKO DANTAYALAN
	0x093A,          //       DEVANAGARI VOWEL SIGN OE
	0x093C,          //       DEVANAGARI SIGN NUKTA
	0x094D,          //       DEVANAGARI SIGN VIRAMA
	0x0981,          //       BENGALI SIGN CANDRABINDU
	0x09BC,          //       BENGALI SIGN NUKTA
	0x09CD,          //       BENGALI SIGN VIRAMA
	0x09FE,          //       BENGALI SANDHI MARK
	0x0A3C,          //       GURMUKHI SIGN NUKTA
	0x0A51,          //       GURMUKHI SIGN UDAAT
	0x0A75,          //       GURMUKHI SIGN YAKASH
	0x0ABC,          //       GUJARATI SIGN NUKTA
	0x0ACD,          //       GUJARATI SIGN VIRAMA
	0x0B01,          //       ORIYA SIGN CANDRABINDU
	0x0B3C,          //       ORIYA SIGN NUKTA
	0x0B3F,          //       ORIYA VOWEL SIGN I
	0x0B4D,          //       ORIYA SIGN VIRAMA
	0x0B82,          //       TAMIL SIGN ANUSVARA
	0x0BC0,          //       TAMIL VOWEL SIGN II
	0x0BCD,          //       TAMIL SIGN VIRAMA
	0x0C00,          //       TELUGU SIGN COMBINING CANDRABINDU ABOVE
	0x0C04,          //       TELUGU SIGN COMBINING ANUSVARA ABOVE
	0x0C3C,          //       TELUGU SIGN NUKTA
	0x0C81,          //       KANNADA SIGN CANDRABINDU
	0x0CBC,          //       KANNADA SIGN NUKTA
	0x0CBF,          //       KANNADA VOWEL SIGN I
	0x0CC6,          //       KANNADA VOWEL SIGN E
	0x0D4D,          //       MALAYALAM SIGN VIRAMA
	0x0D81,          //       SINHALA SIGN CANDRABINDU
	0x0DCA,          //       SINHALA SIGN AL-LAKUNA
	0x0DD6,          //       SINHALA VOWEL SIGN DIGA PAA-PILLA
	0x0E31,          //       THAI CHARACTER MAI HAN-AKAT
	0x0EB1,          //       LAO VOWEL SIGN MAI KAN
	0x0F35,          //       TIBETAN MARK NGAS BZUNG NYI ZLA
	0x0F37,          //       TIBETAN MARK NGAS BZUNG SGOR RTAGS
	0x0F39,          //       TIBETAN MARK TSA -PHRU
	0x0FC6,          //       TIBETAN SYMBOL PADMA GDAN
	0x1082,          //       MYANMAR CONSONANT SIGN SHAN MEDIAL WA
	0x108D,          //       MYANMAR SIGN SHAN COUNCIL EMPHATIC TONE
	0x109D,          //       MYANMAR VOWEL SIGN AITON AI
	0x17C6,          //       KHMER SIGN NIKAHIT
	0x17DD,          //       KHMER SIGN ATTHACAN
	0x180F,          //       MONGOLIAN FREE VARIATION SELECTOR FOUR
	0x18A9,          //       MONGOLIAN LETTER ALI GALI DAGALGA
	0x1932,          //       LIMBU SMALL LETTER ANUSVARA
	0x1A1B,          //       BUGINESE VOWEL SIGN AE
	0x1A56,          //       TAI THAM CONSONANT SIGN MEDIAL LA
	0x1A60,          //       TAI THAM SIGN SAKOT
	0x1A62,          //       TAI THAM VOWEL SIGN MAI SAT
	0x1A7F,          //       TAI THAM COMBINING CRYPTOGRAMMIC DOT
	0x1B34,          //       BALINESE SIGN REREKAN
	0x1B3C,          //       BALINESE VOWEL SIGN LA LENGA
	0x1B42,          //       BALINESE VOWEL SIGN PEPET
	0x1BE6,          //       BATAK SIGN TOMPI
	0x1BED,          //       BATAK VOWEL SIGN KARO O
	0x1CED,          //       VEDIC SIGN TIRYAK
	0x1CF4,          //       VEDIC TONE CANDRA ABOVE
	0x20E1,          //       COMBINING LEFT RIGHT ARROW ABOVE
	0x2D7F,          //       TIFINAGH CONSONANT JOINER
	0xA66F,          //       COMBINING CYRILLIC VZMET
	0xA802,          //       SYLOTI NAGRI SIGN DVISVARA
	0xA806,          //       SYLOTI NAGRI SIGN HASANTA
	0xA80B,          //       SYLOTI NAGRI SIGN ANUSVARA
	0xA82C,          //       SYLOTI NAGRI SIGN ALTERNATE HASANTA
	0xA8FF,          //       DEVANAGARI VOWEL SIGN AY
	0xA9B3,          //       JAVANESE SIGN CECAK TELU
	0xA9E5,          //       MYANMAR SIGN SHAN SAW
	0xAA43,          //       CHAM CONSONANT SIGN FINAL NG
	0xAA4C,          //       CHAM CONSONANT SIGN FINAL M
	0xAA7C,          //       MYANMAR SIGN TAI LAING TONE-2
	0xAAB0,          //       TAI VIET MAI KANG
	0xAAC1,          //       TAI VIET TONE MAI THO
	0xAAF6,          //       MEETEI MAYEK VIRAMA
	0xABE5,          //       MEETEI MAYEK VOWEL SIGN ANAP
	0xABE8,          //       MEETEI MAYEK VOWEL SIGN UNAP
	0xABED,          //       MEETEI MAYEK APUN IYEK
	0xFB1E           //       HEBREW POINT JUDEO-SPANISH VARIKA
	};

	//SINGLE 32 BIT
	uint32_t isolated_nonspacing_32[] = {
	0x101FD,         //       PHAISTOS DISC SIGN COMBINING OBLIQUE STROKE
	0x102E0,         //       COPTIC EPACT THOUSANDS MARK
	0x10A3F,         //       KHAROSHTHI VIRAMA
	0x11001,         //       BRAHMI SIGN ANUSVARA
	0x11070,         //       BRAHMI SIGN OLD TAMIL VIRAMA
	0x110C2,         //       KAITHI VOWEL SIGN VOCALIC R
	0x11173,         //       MAHAJANI SIGN NUKTA
	0x111CF,         //       SHARADA SIGN INVERTED CANDRABINDU
	0x11234,         //       KHOJKI SIGN ANUSVARA
	0x1123E,         //       KHOJKI SIGN SUKUN
	0x11241,         //       KHOJKI VOWEL SIGN VOCALIC R
	0x112DF,         //       KHUDAWADI SIGN ANUSVARA
	0x11340,         //       GRANTHA VOWEL SIGN II
	0x11446,         //       NEWA SIGN NUKTA
	0x1145E,         //       NEWA SANDHI MARK
	0x114BA,         //       TIRHUTA VOWEL SIGN SHORT E
	0x1163D,         //       MODI SIGN ANUSVARA
	0x116AB,         //       TAKRI SIGN ANUSVARA
	0x116AD,         //       TAKRI VOWEL SIGN AA
	0x116B7,         //       TAKRI SIGN NUKTA
	0x1193E,         //       DIVES AKURU VIRAMA
	0x11943,         //       DIVES AKURU SIGN NUKTA
	0x119E0,         //       NANDINAGARI SIGN VIRAMA
	0x11A47,         //       ZANABAZAR SQUARE SUBJOINER
	0x11C3F,         //       BHAIKSUKI SIGN VIRAMA
	0x11D3A,         //       MASARAM GONDI VOWEL SIGN E
	0x11D47,         //       MASARAM GONDI RA-KARA
	0x11D95,         //       GUNJALA GONDI SIGN ANUSVARA
	0x11D97,         //       GUNJALA GONDI VIRAMA
	0x11F40,         //       KAWI VOWEL SIGN EU
	0x11F42,         //       KAWI CONJOINER
	0x13440,         //       EGYPTIAN HIEROGLYPH MIRROR HORIZONTALLY
	0x16F4F,         //       MIAO SIGN CONSONANT MODIFIER BAR
	0x16FE4,         //       KHITAN SMALL SCRIPT FILLER
	0x1DA75,         //       SIGNWRITING UPPER BODY TILTING FROM HIP JOINTS
	0x1DA84,         //       SIGNWRITING LOCATION HEAD NECK
	0x1E08F,         //       COMBINING CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
	0x1E2AE          //       TOTO SIGN RISING TONE
	};

	//16 BIT RANGE
	uint16_t nonspacing_ranges[] = {
	0x0300, 0x036F,    // [112] COMBINING GRAVE ACCENT, 0xCOMBINING LATIN SMALL LETTER X
	0x0483, 0x0487,    //   [5] COMBINING CYRILLIC TITLO, 0xCOMBINING CYRILLIC POKRYTIE
	0x0591, 0x05BD,    //  [45] HEBREW ACCENT ETNAHTA, 0xHEBREW POINT METEG
	0x05C1, 0x05C2,    //   [2] HEBREW POINT SHIN DOT, 0xHEBREW POINT SIN DOT
	0x05C4, 0x05C5,    //   [2] HEBREW MARK UPPER DOT, 0xHEBREW MARK LOWER DOT
	0x0610, 0x061A,    //  [11] ARABIC SIGN SALLALLAHOU ALAYHE WASSALLAM, 0xARABIC SMALL KASRA
	0x064B, 0x065F,    //  [21] ARABIC FATHATAN, 0xARABIC WAVY HAMZA BELOW
	0x06D6, 0x06DC,    //   [7] ARABIC SMALL HIGH LIGATURE SAD WITH LAM WITH ALEF MAKSURA, 0xARABIC SMALL HIGH SEEN
	0x06DF, 0x06E4,    //   [6] ARABIC SMALL HIGH ROUNDED ZERO, 0xARABIC SMALL HIGH MADDA
	0x06E7, 0x06E8,    //   [2] ARABIC SMALL HIGH YEH, 0xARABIC SMALL HIGH NOON
	0x06EA, 0x06ED,    //   [4] ARABIC EMPTY CENTRE LOW STOP, 0xARABIC SMALL LOW MEEM
	0x0730, 0x074A,    //  [27] SYRIAC PTHAHA ABOVE, 0xSYRIAC BARREKH
	0x07A6, 0x07B0,    //  [11] THAANA ABAFILI, 0xTHAANA SUKUN
	0x07EB, 0x07F3,    //   [9] NKO COMBINING SHORT HIGH TONE, 0xNKO COMBINING DOUBLE DOT ABOVE
	0x0816, 0x0819,    //   [4] SAMARITAN MARK IN, 0xSAMARITAN MARK DAGESH
	0x081B, 0x0823,    //   [9] SAMARITAN MARK EPENTHETIC YUT, 0xSAMARITAN VOWEL SIGN A
	0x0825, 0x0827,    //   [3] SAMARITAN VOWEL SIGN SHORT A, 0xSAMARITAN VOWEL SIGN U
	0x0829, 0x082D,    //   [5] SAMARITAN VOWEL SIGN LONG I, 0xSAMARITAN MARK NEQUDAA
	0x0859, 0x085B,    //   [3] MANDAIC AFFRICATION MARK, 0xMANDAIC GEMINATION MARK
	0x0898, 0x089F,    //   [8] ARABIC SMALL HIGH WORD AL-JUZ, 0xARABIC HALF MADDA OVER MADDA
	0x08CA, 0x08E1,    //  [24] ARABIC SMALL HIGH FARSI YEH, 0xARABIC SMALL HIGH SIGN SAFHA
	0x08E3, 0x0902,    //  [32] ARABIC TURNED DAMMA BELOW, 0xDEVANAGARI SIGN ANUSVARA
	0x0941, 0x0948,    //   [8] DEVANAGARI VOWEL SIGN U, 0xDEVANAGARI VOWEL SIGN AI
	0x0951, 0x0957,    //   [7] DEVANAGARI STRESS SIGN UDATTA, 0xDEVANAGARI VOWEL SIGN UUE
	0x0962, 0x0963,    //   [2] DEVANAGARI VOWEL SIGN VOCALIC L, 0xDEVANAGARI VOWEL SIGN VOCALIC LL
	0x09C1, 0x09C4,    //   [4] BENGALI VOWEL SIGN U, 0xBENGALI VOWEL SIGN VOCALIC RR
	0x09E2, 0x09E3,    //   [2] BENGALI VOWEL SIGN VOCALIC L, 0xBENGALI VOWEL SIGN VOCALIC LL
	0x0A01, 0x0A02,    //   [2] GURMUKHI SIGN ADAK BINDI, 0xGURMUKHI SIGN BINDI
	0x0A41, 0x0A42,    //   [2] GURMUKHI VOWEL SIGN U, 0xGURMUKHI VOWEL SIGN UU
	0x0A47, 0x0A48,    //   [2] GURMUKHI VOWEL SIGN EE, 0xGURMUKHI VOWEL SIGN AI
	0x0A4B, 0x0A4D,    //   [3] GURMUKHI VOWEL SIGN OO, 0xGURMUKHI SIGN VIRAMA
	0x0A70, 0x0A71,    //   [2] GURMUKHI TIPPI, 0xGURMUKHI ADDAK
	0x0A81, 0x0A82,    //   [2] GUJARATI SIGN CANDRABINDU, 0xGUJARATI SIGN ANUSVARA
	0x0AC1, 0x0AC5,    //   [5] GUJARATI VOWEL SIGN U, 0xGUJARATI VOWEL SIGN CANDRA E
	0x0AC7, 0x0AC8,    //   [2] GUJARATI VOWEL SIGN E, 0xGUJARATI VOWEL SIGN AI
	0x0AE2, 0x0AE3,    //   [2] GUJARATI VOWEL SIGN VOCALIC L, 0xGUJARATI VOWEL SIGN VOCALIC LL
	0x0AFA, 0x0AFF,    //   [6] GUJARATI SIGN SUKUN, 0xGUJARATI SIGN TWO-CIRCLE NUKTA ABOVE
	0x0B41, 0x0B44,    //   [4] ORIYA VOWEL SIGN U, 0xORIYA VOWEL SIGN VOCALIC RR
	0x0B55, 0x0B56,    //   [2] ORIYA SIGN OVERLINE, 0xORIYA AI LENGTH MARK
	0x0B62, 0x0B63,    //   [2] ORIYA VOWEL SIGN VOCALIC L, 0xORIYA VOWEL SIGN VOCALIC LL
	0x0C3E, 0x0C40,    //   [3] TELUGU VOWEL SIGN AA, 0xTELUGU VOWEL SIGN II
	0x0C46, 0x0C48,    //   [3] TELUGU VOWEL SIGN E, 0xTELUGU VOWEL SIGN AI
	0x0C4A, 0x0C4D,    //   [4] TELUGU VOWEL SIGN O, 0xTELUGU SIGN VIRAMA
	0x0C55, 0x0C56,    //   [2] TELUGU LENGTH MARK, 0xTELUGU AI LENGTH MARK
	0x0C62, 0x0C63,    //   [2] TELUGU VOWEL SIGN VOCALIC L, 0xTELUGU VOWEL SIGN VOCALIC LL
	0x0CCC, 0x0CCD,    //   [2] KANNADA VOWEL SIGN AU, 0xKANNADA SIGN VIRAMA
	0x0CE2, 0x0CE3,    //   [2] KANNADA VOWEL SIGN VOCALIC L, 0xKANNADA VOWEL SIGN VOCALIC LL
	0x0D00, 0x0D01,    //   [2] MALAYALAM SIGN COMBINING ANUSVARA ABOVE, 0xMALAYALAM SIGN CANDRABINDU
	0x0D3B, 0x0D3C,    //   [2] MALAYALAM SIGN VERTICAL BAR VIRAMA, 0xMALAYALAM SIGN CIRCULAR VIRAMA
	0x0D41, 0x0D44,    //   [4] MALAYALAM VOWEL SIGN U, 0xMALAYALAM VOWEL SIGN VOCALIC RR
	0x0D62, 0x0D63,    //   [2] MALAYALAM VOWEL SIGN VOCALIC L, 0xMALAYALAM VOWEL SIGN VOCALIC LL
	0x0DD2, 0x0DD4,    //   [3] SINHALA VOWEL SIGN KETTI IS-PILLA, 0xSINHALA VOWEL SIGN KETTI PAA-PILLA
	0x0E34, 0x0E3A,    //   [7] THAI CHARACTER SARA I, 0xTHAI CHARACTER PHINTHU
	0x0E47, 0x0E4E,    //   [8] THAI CHARACTER MAITAIKHU, 0xTHAI CHARACTER YAMAKKAN
	0x0EB4, 0x0EBC,    //   [9] LAO VOWEL SIGN I, 0xLAO SEMIVOWEL SIGN LO
	0x0EC8, 0x0ECE,    //   [7] LAO TONE MAI EK, 0xLAO YAMAKKAN
	0x0F18, 0x0F19,    //   [2] TIBETAN ASTROLOGICAL SIGN -KHYUD PA, 0xTIBETAN ASTROLOGICAL SIGN SDONG TSHUGS
	0x0F71, 0x0F7E,    //  [14] TIBETAN VOWEL SIGN AA, 0xTIBETAN SIGN RJES SU NGA RO
	0x0F80, 0x0F84,    //   [5] TIBETAN VOWEL SIGN REVERSED I, 0xTIBETAN MARK HALANTA
	0x0F86, 0x0F87,    //   [2] TIBETAN SIGN LCI RTAGS, 0xTIBETAN SIGN YANG RTAGS
	0x0F8D, 0x0F97,    //  [11] TIBETAN SUBJOINED SIGN LCE TSA CAN, 0xTIBETAN SUBJOINED LETTER JA
	0x0F99, 0x0FBC,    //  [36] TIBETAN SUBJOINED LETTER NYA, 0xTIBETAN SUBJOINED LETTER FIXED-FORM RA
	0x102D, 0x1030,    //   [4] MYANMAR VOWEL SIGN I, 0xMYANMAR VOWEL SIGN UU
	0x1032, 0x1037,    //   [6] MYANMAR VOWEL SIGN AI, 0xMYANMAR SIGN DOT BELOW
	0x1039, 0x103A,    //   [2] MYANMAR SIGN VIRAMA, 0xMYANMAR SIGN ASAT
	0x103D, 0x103E,    //   [2] MYANMAR CONSONANT SIGN MEDIAL WA, 0xMYANMAR CONSONANT SIGN MEDIAL HA
	0x1058, 0x1059,    //   [2] MYANMAR VOWEL SIGN VOCALIC L, 0xMYANMAR VOWEL SIGN VOCALIC LL
	0x105E, 0x1060,    //   [3] MYANMAR CONSONANT SIGN MON MEDIAL NA, 0xMYANMAR CONSONANT SIGN MON MEDIAL LA
	0x1071, 0x1074,    //   [4] MYANMAR VOWEL SIGN GEBA KAREN I, 0xMYANMAR VOWEL SIGN KAYAH EE
	0x1085, 0x1086,    //   [2] MYANMAR VOWEL SIGN SHAN E ABOVE, 0xMYANMAR VOWEL SIGN SHAN FINAL Y
	0x135D, 0x135F,    //   [3] ETHIOPIC COMBINING GEMINATION AND VOWEL LENGTH MARK, 0xETHIOPIC COMBINING GEMINATION MARK
	0x1712, 0x1714,    //   [3] TAGALOG VOWEL SIGN I, 0xTAGALOG SIGN VIRAMA
	0x1732, 0x1733,    //   [2] HANUNOO VOWEL SIGN I, 0xHANUNOO VOWEL SIGN U
	0x1752, 0x1753,    //   [2] BUHID VOWEL SIGN I, 0xBUHID VOWEL SIGN U
	0x1772, 0x1773,    //   [2] TAGBANWA VOWEL SIGN I, 0xTAGBANWA VOWEL SIGN U
	0x17B4, 0x17B5,    //   [2] KHMER VOWEL INHERENT AQ, 0xKHMER VOWEL INHERENT AA
	0x17B7, 0x17BD,    //   [7] KHMER VOWEL SIGN I, 0xKHMER VOWEL SIGN UA
	0x17C9, 0x17D3,    //  [11] KHMER SIGN MUUSIKATOAN, 0xKHMER SIGN BATHAMASAT
	0x180B, 0x180D,    //   [3] MONGOLIAN FREE VARIATION SELECTOR ONE, 0xMONGOLIAN FREE VARIATION SELECTOR THREE
	0x1885, 0x1886,    //   [2] MONGOLIAN LETTER ALI GALI BALUDA, 0xMONGOLIAN LETTER ALI GALI THREE BALUDA
	0x1920, 0x1922,    //   [3] LIMBU VOWEL SIGN A, 0xLIMBU VOWEL SIGN U
	0x1927, 0x1928,    //   [2] LIMBU VOWEL SIGN E, 0xLIMBU VOWEL SIGN O
	0x1939, 0x193B,    //   [3] LIMBU SIGN MUKPHRENG, 0xLIMBU SIGN SA-I
	0x1A17, 0x1A18,    //   [2] BUGINESE VOWEL SIGN I, 0xBUGINESE VOWEL SIGN U
	0x1A58, 0x1A5E,    //   [7] TAI THAM SIGN MAI KANG LAI, 0xTAI THAM CONSONANT SIGN SA
	0x1A65, 0x1A6C,    //   [8] TAI THAM VOWEL SIGN I, 0xTAI THAM VOWEL SIGN OA BELOW
	0x1A73, 0x1A7C,    //  [10] TAI THAM VOWEL SIGN OA ABOVE, 0xTAI THAM SIGN KHUEN-LUE KARAN
	0x1AB0, 0x1ABD,    //  [14] COMBINING DOUBLED CIRCUMFLEX ACCENT, 0xCOMBINING PARENTHESES BELOW
	0x1ABF, 0x1ACE,    //  [16] COMBINING LATIN SMALL LETTER W BELOW, 0xCOMBINING LATIN SMALL LETTER INSULAR T
	0x1B00, 0x1B03,    //   [4] BALINESE SIGN ULU RICEM, 0xBALINESE SIGN SURANG
	0x1B36, 0x1B3A,    //   [5] BALINESE VOWEL SIGN ULU, 0xBALINESE VOWEL SIGN RA REPA
	0x1B6B, 0x1B73,    //   [9] BALINESE MUSICAL SYMBOL COMBINING TEGEH, 0xBALINESE MUSICAL SYMBOL COMBINING GONG
	0x1B80, 0x1B81,    //   [2] SUNDANESE SIGN PANYECEK, 0xSUNDANESE SIGN PANGLAYAR
	0x1BA2, 0x1BA5,    //   [4] SUNDANESE CONSONANT SIGN PANYAKRA, 0xSUNDANESE VOWEL SIGN PANYUKU
	0x1BA8, 0x1BA9,    //   [2] SUNDANESE VOWEL SIGN PAMEPET, 0xSUNDANESE VOWEL SIGN PANEULEUNG
	0x1BAB, 0x1BAD,    //   [3] SUNDANESE SIGN VIRAMA, 0xSUNDANESE CONSONANT SIGN PASANGAN WA
	0x1BE8, 0x1BE9,    //   [2] BATAK VOWEL SIGN PAKPAK E, 0xBATAK VOWEL SIGN EE
	0x1BEF, 0x1BF1,    //   [3] BATAK VOWEL SIGN U FOR SIMALUNGUN SA, 0xBATAK CONSONANT SIGN H
	0x1C2C, 0x1C33,    //   [8] LEPCHA VOWEL SIGN E, 0xLEPCHA CONSONANT SIGN T
	0x1C36, 0x1C37,    //   [2] LEPCHA SIGN RAN, 0xLEPCHA SIGN NUKTA
	0x1CD0, 0x1CD2,    //   [3] VEDIC TONE KARSHANA, 0xVEDIC TONE PRENKHA
	0x1CD4, 0x1CE0,    //  [13] VEDIC SIGN YAJURVEDIC MIDLINE SVARITA, 0xVEDIC TONE RIGVEDIC KASHMIRI INDEPENDENT SVARITA
	0x1CE2, 0x1CE8,    //   [7] VEDIC SIGN VISARGA SVARITA, 0xVEDIC SIGN VISARGA ANUDATTA WITH TAIL
	0x1CF8, 0x1CF9,    //   [2] VEDIC TONE RING ABOVE, 0xVEDIC TONE DOUBLE RING ABOVE
	0x1DC0, 0x1DFF,    //  [64] COMBINING DOTTED GRAVE ACCENT, 0xCOMBINING RIGHT ARROWHEAD AND DOWN ARROWHEAD BELOW
	0x20D0, 0x20DC,    //  [13] COMBINING LEFT HARPOON ABOVE, 0xCOMBINING FOUR DOTS ABOVE
	0x20E5, 0x20F0,    //  [12] COMBINING REVERSE SOLIDUS OVERLAY, 0xCOMBINING ASTERISK ABOVE
	0x2CEF, 0x2CF1,    //   [3] COPTIC COMBINING NI ABOVE, 0xCOPTIC COMBINING SPIRITUS LENIS
	0x2DE0, 0x2DFF,    //  [32] COMBINING CYRILLIC LETTER BE, 0xCOMBINING CYRILLIC LETTER IOTIFIED BIG YUS
	0x302A, 0x302D,    //   [4] IDEOGRAPHIC LEVEL TONE MARK, 0xIDEOGRAPHIC ENTERING TONE MARK
	0x3099, 0x309A,    //   [2] COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK, 0xCOMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
	0xA674, 0xA67D,    //  [10] COMBINING CYRILLIC LETTER UKRAINIAN IE, 0xCOMBINING CYRILLIC PAYEROK
	0xA69E, 0xA69F,    //   [2] COMBINING CYRILLIC LETTER EF, 0xCOMBINING CYRILLIC LETTER IOTIFIED E
	0xA6F0, 0xA6F1,    //   [2] BAMUM COMBINING MARK KOQNDON, 0xBAMUM COMBINING MARK TUKWENTIS
	0xA825, 0xA826,    //   [2] SYLOTI NAGRI VOWEL SIGN U, 0xSYLOTI NAGRI VOWEL SIGN E
	0xA8C4, 0xA8C5,    //   [2] SAURASHTRA SIGN VIRAMA, 0xSAURASHTRA SIGN CANDRABINDU
	0xA8E0, 0xA8F1,    //  [18] COMBINING DEVANAGARI DIGIT ZERO, 0xCOMBINING DEVANAGARI SIGN AVAGRAHA
	0xA926, 0xA92D,    //   [8] KAYAH LI VOWEL UE, 0xKAYAH LI TONE CALYA PLOPHU
	0xA947, 0xA951,    //  [11] REJANG VOWEL SIGN I, 0xREJANG CONSONANT SIGN R
	0xA980, 0xA982,    //   [3] JAVANESE SIGN PANYANGGA, 0xJAVANESE SIGN LAYAR
	0xA9B6, 0xA9B9,    //   [4] JAVANESE VOWEL SIGN WULU, 0xJAVANESE VOWEL SIGN SUKU MENDUT
	0xA9BC, 0xA9BD,    //   [2] JAVANESE VOWEL SIGN PEPET, 0xJAVANESE CONSONANT SIGN KERET
	0xAA29, 0xAA2E,    //   [6] CHAM VOWEL SIGN AA, 0xCHAM VOWEL SIGN OE
	0xAA31, 0xAA32,    //   [2] CHAM VOWEL SIGN AU, 0xCHAM VOWEL SIGN UE
	0xAA35, 0xAA36,    //   [2] CHAM CONSONANT SIGN LA, 0xCHAM CONSONANT SIGN WA
	0xAAB2, 0xAAB4,    //   [3] TAI VIET VOWEL I, 0xTAI VIET VOWEL U
	0xAAB7, 0xAAB8,    //   [2] TAI VIET MAI KHIT, 0xTAI VIET VOWEL IA
	0xAABE, 0xAABF,    //   [2] TAI VIET VOWEL AM, 0xTAI VIET TONE MAI EK
	0xAAEC, 0xAAED,    //   [2] MEETEI MAYEK VOWEL SIGN UU, 0xMEETEI MAYEK VOWEL SIGN AAI
	0xFE00, 0xFE0F,    //  [16] VARIATION SELECTOR-1, 0xVARIATION SELECTOR-16
	0xFE20, 0xFE2F     //  [16] COMBINING LIGATURE LEFT HALF, 0xCOMBINING CYRILLIC TITLO RIGHT HALF
	};

	//32 BIT RANGE
	uint32_t nonspacing_ranges_32[] = {
	0x10376, 0x1037A,  //   [5] COMBINING OLD PERMIC LETTER AN, 0xCOMBINING OLD PERMIC LETTER SII
	0x10A01, 0x10A03,  //   [3] KHAROSHTHI VOWEL SIGN I, 0xKHAROSHTHI VOWEL SIGN VOCALIC R
	0x10A05, 0x10A06,  //   [2] KHAROSHTHI VOWEL SIGN E, 0xKHAROSHTHI VOWEL SIGN O
	0x10A0C, 0x10A0F,  //   [4] KHAROSHTHI VOWEL LENGTH MARK, 0xKHAROSHTHI SIGN VISARGA
	0x10A38, 0x10A3A,  //   [3] KHAROSHTHI SIGN BAR ABOVE, 0xKHAROSHTHI SIGN DOT BELOW
	0x10AE5, 0x10AE6,  //   [2] MANICHAEAN ABBREVIATION MARK ABOVE, 0xMANICHAEAN ABBREVIATION MARK BELOW
	0x10D24, 0x10D27,  //   [4] HANIFI ROHINGYA SIGN HARBAHAY, 0xHANIFI ROHINGYA SIGN TASSI
	0x10EAB, 0x10EAC,  //   [2] YEZIDI COMBINING HAMZA MARK, 0xYEZIDI COMBINING MADDA MARK
	0x10EFD, 0x10EFF,  //   [3] ARABIC SMALL LOW WORD SAKTA, 0xARABIC SMALL LOW WORD MADDA
	0x10F46, 0x10F50,  //  [11] SOGDIAN COMBINING DOT BELOW, 0xSOGDIAN COMBINING STROKE BELOW
	0x10F82, 0x10F85,  //   [4] OLD UYGHUR COMBINING DOT ABOVE, 0xOLD UYGHUR COMBINING TWO DOTS BELOW
	0x11038, 0x11046,  //  [15] BRAHMI VOWEL SIGN AA, 0xBRAHMI VIRAMA
	0x11073, 0x11074,  //   [2] BRAHMI VOWEL SIGN OLD TAMIL SHORT E, 0xBRAHMI VOWEL SIGN OLD TAMIL SHORT O
	0x1107F, 0x11081,  //   [3] BRAHMI NUMBER JOINER, 0xKAITHI SIGN ANUSVARA
	0x110B3, 0x110B6,  //   [4] KAITHI VOWEL SIGN U, 0xKAITHI VOWEL SIGN AI
	0x110B9, 0x110BA,  //   [2] KAITHI SIGN VIRAMA, 0xKAITHI SIGN NUKTA
	0x11100, 0x11102,  //   [3] CHAKMA SIGN CANDRABINDU, 0xCHAKMA SIGN VISARGA
	0x11127, 0x1112B,  //   [5] CHAKMA VOWEL SIGN A, 0xCHAKMA VOWEL SIGN UU
	0x1112D, 0x11134,  //   [8] CHAKMA VOWEL SIGN AI, 0xCHAKMA MAAYYAA
	0x11180, 0x11181,  //   [2] SHARADA SIGN CANDRABINDU, 0xSHARADA SIGN ANUSVARA
	0x111B6, 0x111BE,  //   [9] SHARADA VOWEL SIGN U, 0xSHARADA VOWEL SIGN O
	0x111C9, 0x111CC,  //   [4] SHARADA SANDHI MARK, 0xSHARADA EXTRA SHORT VOWEL MARK
	0x1122F, 0x11231,  //   [3] KHOJKI VOWEL SIGN U, 0xKHOJKI VOWEL SIGN AI
	0x11236, 0x11237,  //   [2] KHOJKI SIGN NUKTA, 0xKHOJKI SIGN SHADDA
	0x112E3, 0x112EA,  //   [8] KHUDAWADI VOWEL SIGN U, 0xKHUDAWADI SIGN VIRAMA
	0x11300, 0x11301,  //   [2] GRANTHA SIGN COMBINING ANUSVARA ABOVE, 0xGRANTHA SIGN CANDRABINDU
	0x1133B, 0x1133C,  //   [2] COMBINING BINDU BELOW, 0xGRANTHA SIGN NUKTA
	0x11366, 0x1136C,  //   [7] COMBINING GRANTHA DIGIT ZERO, 0xCOMBINING GRANTHA DIGIT SIX
	0x11370, 0x11374,  //   [5] COMBINING GRANTHA LETTER A, 0xCOMBINING GRANTHA LETTER PA
	0x11438, 0x1143F,  //   [8] NEWA VOWEL SIGN U, 0xNEWA VOWEL SIGN AI
	0x11442, 0x11444,  //   [3] NEWA SIGN VIRAMA, 0xNEWA SIGN ANUSVARA
	0x114B3, 0x114B8,  //   [6] TIRHUTA VOWEL SIGN U, 0xTIRHUTA VOWEL SIGN VOCALIC LL
	0x114BF, 0x114C0,  //   [2] TIRHUTA SIGN CANDRABINDU, 0xTIRHUTA SIGN ANUSVARA
	0x114C2, 0x114C3,  //   [2] TIRHUTA SIGN VIRAMA, 0xTIRHUTA SIGN NUKTA
	0x115B2, 0x115B5,  //   [4] SIDDHAM VOWEL SIGN U, 0xSIDDHAM VOWEL SIGN VOCALIC RR
	0x115BC, 0x115BD,  //   [2] SIDDHAM SIGN CANDRABINDU, 0xSIDDHAM SIGN ANUSVARA
	0x115BF, 0x115C0,  //   [2] SIDDHAM SIGN VIRAMA, 0xSIDDHAM SIGN NUKTA
	0x115DC, 0x115DD,  //   [2] SIDDHAM VOWEL SIGN ALTERNATE U, 0xSIDDHAM VOWEL SIGN ALTERNATE UU
	0x11633, 0x1163A,  //   [8] MODI VOWEL SIGN U, 0xMODI VOWEL SIGN AI
	0x1163F, 0x11640,  //   [2] MODI SIGN VIRAMA, 0xMODI SIGN ARDHACANDRA
	0x116B0, 0x116B5,  //   [6] TAKRI VOWEL SIGN U, 0xTAKRI VOWEL SIGN AU
	0x1171D, 0x1171F,  //   [3] AHOM CONSONANT SIGN MEDIAL LA, 0xAHOM CONSONANT SIGN MEDIAL LIGATING RA
	0x11722, 0x11725,  //   [4] AHOM VOWEL SIGN I, 0xAHOM VOWEL SIGN UU
	0x11727, 0x1172B,  //   [5] AHOM VOWEL SIGN AW, 0xAHOM SIGN KILLER
	0x1182F, 0x11837,  //   [9] DOGRA VOWEL SIGN U, 0xDOGRA SIGN ANUSVARA
	0x11839, 0x1183A,  //   [2] DOGRA SIGN VIRAMA, 0xDOGRA SIGN NUKTA
	0x1193B, 0x1193C,  //   [2] DIVES AKURU SIGN ANUSVARA, 0xDIVES AKURU SIGN CANDRABINDU
	0x119D4, 0x119D7,  //   [4] NANDINAGARI VOWEL SIGN U, 0xNANDINAGARI VOWEL SIGN VOCALIC RR
	0x119DA, 0x119DB,  //   [2] NANDINAGARI VOWEL SIGN E, 0xNANDINAGARI VOWEL SIGN AI
	0x11A01, 0x11A0A,  //  [10] ZANABAZAR SQUARE VOWEL SIGN I, 0xZANABAZAR SQUARE VOWEL LENGTH MARK
	0x11A33, 0x11A38,  //   [6] ZANABAZAR SQUARE FINAL CONSONANT MARK, 0xZANABAZAR SQUARE SIGN ANUSVARA
	0x11A3B, 0x11A3E,  //   [4] ZANABAZAR SQUARE CLUSTER-FINAL LETTER YA, 0xZANABAZAR SQUARE CLUSTER-FINAL LETTER VA
	0x11A51, 0x11A56,  //   [6] SOYOMBO VOWEL SIGN I, 0xSOYOMBO VOWEL SIGN OE
	0x11A59, 0x11A5B,  //   [3] SOYOMBO VOWEL SIGN VOCALIC R, 0xSOYOMBO VOWEL LENGTH MARK
	0x11A8A, 0x11A96,  //  [13] SOYOMBO FINAL CONSONANT SIGN G, 0xSOYOMBO SIGN ANUSVARA
	0x11A98, 0x11A99,  //   [2] SOYOMBO GEMINATION MARK, 0xSOYOMBO SUBJOINER
	0x11C30, 0x11C36,  //   [7] BHAIKSUKI VOWEL SIGN I, 0xBHAIKSUKI VOWEL SIGN VOCALIC L
	0x11C38, 0x11C3D,  //   [6] BHAIKSUKI VOWEL SIGN E, 0xBHAIKSUKI SIGN ANUSVARA
	0x11C92, 0x11CA7,  //  [22] MARCHEN SUBJOINED LETTER KA, 0xMARCHEN SUBJOINED LETTER ZA
	0x11CAA, 0x11CB0,  //   [7] MARCHEN SUBJOINED LETTER RA, 0xMARCHEN VOWEL SIGN AA
	0x11CB2, 0x11CB3,  //   [2] MARCHEN VOWEL SIGN U, 0xMARCHEN VOWEL SIGN E
	0x11CB5, 0x11CB6,  //   [2] MARCHEN SIGN ANUSVARA, 0xMARCHEN SIGN CANDRABINDU
	0x11D31, 0x11D36,  //   [6] MASARAM GONDI VOWEL SIGN AA, 0xMASARAM GONDI VOWEL SIGN VOCALIC R
	0x11D3C, 0x11D3D,  //   [2] MASARAM GONDI VOWEL SIGN AI, 0xMASARAM GONDI VOWEL SIGN O
	0x11D3F, 0x11D45,  //   [7] MASARAM GONDI VOWEL SIGN AU, 0xMASARAM GONDI VIRAMA
	0x11D90, 0x11D91,  //   [2] GUNJALA GONDI VOWEL SIGN EE, 0xGUNJALA GONDI VOWEL SIGN AI
	0x11EF3, 0x11EF4,  //   [2] MAKASAR VOWEL SIGN I, 0xMAKASAR VOWEL SIGN U
	0x11F00, 0x11F01,  //   [2] KAWI SIGN CANDRABINDU, 0xKAWI SIGN ANUSVARA
	0x11F36, 0x11F3A,  //   [5] KAWI VOWEL SIGN I, 0xKAWI VOWEL SIGN VOCALIC R
	0x13447, 0x13455,  //  [15] EGYPTIAN HIEROGLYPH MODIFIER DAMAGED AT TOP START, 0xEGYPTIAN HIEROGLYPH MODIFIER DAMAGED
	0x16AF0, 0x16AF4,  //   [5] BASSA VAH COMBINING HIGH TONE, 0xBASSA VAH COMBINING HIGH-LOW TONE
	0x16B30, 0x16B36,  //   [7] PAHAWH HMONG MARK CIM TUB, 0xPAHAWH HMONG MARK CIM TAUM
	0x16F8F, 0x16F92,  //   [4] MIAO TONE RIGHT, 0xMIAO TONE BELOW
	0x1BC9D, 0x1BC9E,  //   [2] DUPLOYAN THICK LETTER SELECTOR, 0xDUPLOYAN DOUBLE MARK
	0x1CF00, 0x1CF2D,  //  [46] ZNAMENNY COMBINING MARK GORAZDO NIZKO S KRYZHEM ON LEFT, 0xZNAMENNY COMBINING MARK KRYZH ON LEFT
	0x1CF30, 0x1CF46,  //  [23] ZNAMENNY COMBINING TONAL RANGE MARK MRACHNO, 0xZNAMENNY PRIZNAK MODIFIER ROG
	0x1D167, 0x1D169,  //   [3] MUSICAL SYMBOL COMBINING TREMOLO-1, 0xMUSICAL SYMBOL COMBINING TREMOLO-3
	0x1D17B, 0x1D182,  //   [8] MUSICAL SYMBOL COMBINING ACCENT, 0xMUSICAL SYMBOL COMBINING LOURE
	0x1D185, 0x1D18B,  //   [7] MUSICAL SYMBOL COMBINING DOIT, 0xMUSICAL SYMBOL COMBINING TRIPLE TONGUE
	0x1D1AA, 0x1D1AD,  //   [4] MUSICAL SYMBOL COMBINING DOWN BOW, 0xMUSICAL SYMBOL COMBINING SNAP PIZZICATO
	0x1D242, 0x1D244,  //   [3] COMBINING GREEK MUSICAL TRISEME, 0xCOMBINING GREEK MUSICAL PENTASEME
	0x1DA00, 0x1DA36,  //  [55] SIGNWRITING HEAD RIM, 0xSIGNWRITING AIR SUCKING IN
	0x1DA3B, 0x1DA6C,  //  [50] SIGNWRITING MOUTH CLOSED NEUTRAL, 0xSIGNWRITING EXCITEMENT
	0x1DA9B, 0x1DA9F,  //   [5] SIGNWRITING FILL MODIFIER-2, 0xSIGNWRITING FILL MODIFIER-6
	0x1DAA1, 0x1DAAF,  //  [15] SIGNWRITING ROTATION MODIFIER-2, 0xSIGNWRITING ROTATION MODIFIER-16
	0x1E000, 0x1E006,  //   [7] COMBINING GLAGOLITIC LETTER AZU, 0xCOMBINING GLAGOLITIC LETTER ZHIVETE
	0x1E008, 0x1E018,  //  [17] COMBINING GLAGOLITIC LETTER ZEMLJA, 0xCOMBINING GLAGOLITIC LETTER HERU
	0x1E01B, 0x1E021,  //   [7] COMBINING GLAGOLITIC LETTER SHTA, 0xCOMBINING GLAGOLITIC LETTER YATI
	0x1E023, 0x1E024,  //   [2] COMBINING GLAGOLITIC LETTER YU, 0xCOMBINING GLAGOLITIC LETTER SMALL YUS
	0x1E026, 0x1E02A,  //   [5] COMBINING GLAGOLITIC LETTER YO, 0xCOMBINING GLAGOLITIC LETTER FITA
	0x1E130, 0x1E136,  //   [7] NYIAKENG PUACHUE HMONG TONE-B, 0xNYIAKENG PUACHUE HMONG TONE-D
	0x1E2EC, 0x1E2EF,  //   [4] WANCHO TONE TUP, 0xWANCHO TONE KOINI
	0x1E4EC, 0x1E4EF,  //   [4] NAG MUNDARI SIGN MUHOR, 0xNAG MUNDARI SIGN SUTUH
	0x1E8D0, 0x1E8D6,  //   [7] MENDE KIKAKUI COMBINING NUMBER TEENS, 0xMENDE KIKAKUI COMBINING NUMBER MILLIONS
	0x1E944, 0x1E94A,  //   [7] ADLAM ALIF LENGTHENER, 0xADLAM NUKTA
	0xE0100, 0xE01EF   // [240] VARIATION SELECTOR-17, 0xVARIATION SELECTOR-256
	};

	bool codepoint16_is_nonspacing(uint16_t c16) noexcept {
		for(uint32_t i = 0; i < sizeof(isolated_nonspacing) / sizeof(uint16_t); ++i) {
			if(isolated_nonspacing[i] == c16) {
				return true;
			}
		}
		for(uint32_t i = 0; i < sizeof(nonspacing_ranges) / sizeof(uint16_t); i += 2) {
			if(nonspacing_ranges[i] <= c16 && c16 <= nonspacing_ranges[i + 1]) {
				return true;
			}
		}
		return false;
	}

	bool codepoint32_is_nonspacing(uint32_t c) noexcept {
		for(uint32_t i = 0; i < sizeof(isolated_nonspacing_32) / sizeof(uint32_t); ++i) {
			if(isolated_nonspacing_32[i] == c) {
				return true;
			}
		}
		for(uint32_t i = 0; i < sizeof(nonspacing_ranges_32) / sizeof(uint32_t); i += 2) {
			if(nonspacing_ranges_32[i] <= c && c <= nonspacing_ranges_32[i + 1]) {
				return true;
			}
		}
		return false;
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
				return codepoint32_is_nonspacing(assemble_codepoint(at_position, trailing));
			} else { // non surrogate
				return codepoint16_is_nonspacing(at_position);
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
						if(!codepoint32_is_nonspacing(assemble_codepoint(str[i], str[i + 1])))
							++total;
					}
				} else { // non surrogate
					if(!codepoint16_is_nonspacing(str[i]))
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
	};

	void release_text_analysis_object(text_analysis_object* ptr) {
		delete ptr;
	}
	text_analysis_object* make_analysis_object() {
		return new text_analysis_object();
	}
	void impl_update_analyzed_text(text_analysis_object* ptr, std::wstring const& str, bool ltr, text_manager const& tm) {
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
		memset(ptr->char_attributes.data(), int32_t(sizeof(SCRIPT_LOGATTR) * str.length()), 0);
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
	}
	void update_analyzed_text(text_analysis_object* ptr, std::wstring const& str, bool ltr, text_manager const& tm) {
		impl_update_analyzed_text(ptr, str, ltr, tm);
	}

	int32_t left_visual_cursor_position(text_analysis_object* ptr, int32_t position, std::wstring const& str, bool ltr, text_manager const& tm) {
		auto is_ltr = position_is_ltr(ptr, position);
		auto default_pos = is_ltr ? get_previous_cursor_position(ptr, position) : get_next_cursor_position(ptr, position);
		auto default_ltr = position_is_ltr(ptr, default_pos);
		if(default_ltr == is_ltr)
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
		int32_t run_position = 0;
		std::vector<BYTE> run_embedding_levels(items_got);
		std::vector<int32_t> visual_to_logical(items_got);
		std::vector<int32_t> logical_to_visual(items_got);

		for(int32_t i = 0; i < items_got; ++i) {
			if(processed_items[i].iCharPos <= position && position < processed_items[i + 1].iCharPos) {
				run_position = i;
			}
			run_embedding_levels[i] = processed_items[i].a.s.uBidiLevel;
		}

		ScriptLayout(items_got, run_embedding_levels.data(), visual_to_logical.data(), logical_to_visual.data());
		auto visual_position_of_run = logical_to_visual[run_position];
		if(visual_position_of_run == 0) {
			return position;
		}
		auto logical_position_of_left_run = visual_to_logical[visual_position_of_run - 1];
		auto next_run_is_ltr = (processed_items[logical_position_of_left_run].a.s.uBidiLevel & 0x01) == 0;
		if(next_run_is_ltr) {
			// find rightmost char position by moving back from the run after it
			return get_previous_cursor_position(ptr, processed_items[logical_position_of_left_run + 1].iCharPos);
		} else {
			// rightmost char position is first char
			return processed_items[logical_position_of_left_run].iCharPos;
		}
	}
	int32_t right_visual_cursor_position(text_analysis_object* ptr, int32_t position, std::wstring const& str, bool ltr, text_manager const& tm) {
		auto is_ltr = position_is_ltr(ptr, position);
		auto default_pos = is_ltr ? get_next_cursor_position(ptr, position) : get_previous_cursor_position(ptr, position);
		auto default_ltr = position_is_ltr(ptr, default_pos);
		if(default_ltr == is_ltr)
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
		int32_t run_position = 0;
		std::vector<BYTE> run_embedding_levels(items_got);
		std::vector<int32_t> visual_to_logical(items_got);
		std::vector<int32_t> logical_to_visual(items_got);

		for(int32_t i = 0; i < items_got; ++i) {
			if(processed_items[i].iCharPos <= position && position < processed_items[i + 1].iCharPos) {
				run_position = i;
			}
			run_embedding_levels[i] = processed_items[i].a.s.uBidiLevel;
		}

		ScriptLayout(items_got, run_embedding_levels.data(), visual_to_logical.data(), logical_to_visual.data());
		auto visual_position_of_run = logical_to_visual[run_position];
		if(visual_position_of_run == items_got - 1) { // is already rightmost
			return position;
		}
		auto logical_position_of_left_run = visual_to_logical[visual_position_of_run + 1];
		auto next_run_is_ltr = (processed_items[logical_position_of_left_run].a.s.uBidiLevel & 0x01) == 0;
		if(next_run_is_ltr) {
			// leftmost char position is first char
			return processed_items[logical_position_of_left_run].iCharPos;
		} else {
			// find leftmost char position by moving back from the run after it
			return get_previous_cursor_position(ptr, processed_items[logical_position_of_left_run + 1].iCharPos);
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
				if(ptr->char_attributes[position].fWordStop) {
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
			if(ptr->char_attributes[position].fWordStop) {
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


			win32_text_services* ts = static_cast<win32_text_services*>(win.text_services_interface.get());
			if(ts) {
				ts->send_notifications = false;
				ei->insert_text(win, uint32_t(acpStart), uint32_t(acpRemovingEnd), std::wstring_view(pchText, cch));
				ts->send_notifications = true;
			}

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

			win32_text_services* ts = static_cast<win32_text_services*>(win.text_services_interface.get());
			if(ts) {
				ts->send_notifications = false;
				ei->insert_text(win, uint32_t(acpStart), uint32_t(acpEnd), std::wstring_view(pchText, cch));
				ts->send_notifications = true;
			}
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
						gathered_attributes.back().varValue.boolVal = win.dynamic_settings.primary_font.is_bold ? VARIANT_TRUE : VARIANT_FALSE;
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
						gathered_attributes.back().varValue.lVal = int32_t(to_font_weight(win.dynamic_settings.primary_font.is_bold));
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
							ei->update_analysis(win);
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
					ei->update_analysis(win);
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
						ei->update_analysis(win);
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

				auto window_rect = win.window_interface->get_window_location();
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

			if(win.window_interface->is_minimized()) {
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
			if(win.window_interface->is_minimized())
				return S_OK;

			auto& node = win.get_node(control->l_id);
			auto location = win.get_current_location(control->l_id);
			ui_rectangle temp;
			temp.x_position = uint16_t(location.x);
			temp.y_position = uint16_t(location.y);
			temp.width = uint16_t(location.width);
			temp.height = uint16_t(location.height);

			auto new_content_rect = screen_rectangle_from_layout_in_ui(win, node.left_margin(), 0, node.width - (node.left_margin() + node.right_margin()), 1, temp);

			auto window_rect = win.window_interface->get_window_location();

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

		hwnd_direct_access_base* b = static_cast<hwnd_direct_access_base*>(win.window_interface->get_os_access(os_handle_type::windows_hwnd));
		if(b) {
			ITfDocumentMgr* old_doc = nullptr;
			manager_ptr->AssociateFocus(b->m_hwnd, o ? o->document : nullptr, &old_doc);
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
}
