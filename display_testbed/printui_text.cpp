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

	std::vector<font_fallback> get_global_font_fallbacks() {
		HANDLE file_handle = nullptr;
		HANDLE mapping_handle = nullptr;
		char const* mapped_bytes = nullptr;
		std::vector<font_fallback> result;

		wchar_t* local_path_out = nullptr;
		if(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_path_out) != S_OK) {
			CoTaskMemFree(local_path_out);
			return result;
		}

		std::wstring fallback_def_file = std::wstring(local_path_out) = L"\\printui\\font_fallbacks.txt";
		CoTaskMemFree(local_path_out);

		file_handle = CreateFileW(fallback_def_file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
			parse_font_fallbacks_file(result, mapped_bytes, mapped_bytes + pvalue.QuadPart);
		}


		if(mapped_bytes)
			UnmapViewOfFile(mapped_bytes);
		if(mapping_handle)
			CloseHandle(mapping_handle);
		if(file_handle)
			CloseHandle(file_handle);

		return result;
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

	void text_manager::update_with_new_locale(window_data& win) {
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

			load_locale_settings(locale_path, win.dynamic_settings, win.text_data.font_name_to_index);

			auto locale_name = get_locale_name(locale_path);
			win.window_bar.print_ui_settings.lang_menu.open_button.button_text.set_text(locale_name.length() > 0 ? locale_name : full_compound);

			win.intitialize_fonts();
		}
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


	void text_manager::change_locale(std::wstring const& lang, std::wstring const& region, window_data& win) {
		app_lang = lang;
		app_region = region;

		std::wstring full_compound = app_region.size() != 0 ? app_lang + L"-" + app_region : app_lang;

		if(IsValidLocaleName(full_compound.c_str())) {
			os_locale = full_compound;
			os_locale_is_default = false;
		} else {
			os_locale_is_default = true;
		}

		update_with_new_locale(win);
	}

	void text_manager::default_locale(window_data& win) {

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


		update_with_new_locale(win);
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

			if(t.content.length() == 1 && t.content[0] == '}' && static_format_storage.empty()) {
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
									uint32_t v = val - 0x10000;
									uint32_t h = ((v >> 10) & 0x03FF) + 0xD800;
									uint32_t l = (v & 0x03FF) + 0xDC00;

									codepoint_storage += wchar_t(h);
									codepoint_storage += wchar_t(l);
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
}
