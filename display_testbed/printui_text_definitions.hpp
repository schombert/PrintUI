#pragma once
#include "printui_utility.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <msctf.h>
#include <usp10.h>
#include <dwrite_3.h>

namespace printui::text {
	struct direct_write_text : public wrapper {
		IDWriteFactory6* dwrite_factory = nullptr;
		IDWriteFontCollection2* font_collection = nullptr;

		IDWriteTextFormat3* common_text_format = nullptr;
		IDWriteTextFormat3* small_text_format = nullptr;

		IDWriteFontFallback* font_fallbacks = nullptr;
		IDWriteFontFallback* small_font_fallbacks = nullptr;

		direct_write_text();
		virtual ~direct_write_text() {
			safe_release(common_text_format);
			safe_release(small_text_format);
			safe_release(font_fallbacks);
			safe_release(small_font_fallbacks);
			safe_release(font_collection);
			safe_release(dwrite_factory);
		}

		virtual void initialize_fonts(window_data& win) override;
		virtual void initialize_font_fallbacks(window_data& win) override;
		virtual void create_font_collection(window_data& win) override;
		virtual arrangement_result create_text_arragement(window_data const& win, std::wstring_view text, content_alignment text_alignment, bool large_text, bool single_line, int32_t max_width, std::vector<format_marker> const* formatting) override;
		virtual text_format create_text_format(wchar_t const* name, int32_t capheight) override;
		virtual void release_text_format(text_format fmt) override;

		void update_font_metrics(font_description& desc, wchar_t const* locale, float target_pixels, float dpi_scale, IDWriteFont* font);
		void load_fallbacks_by_type(std::vector<font_fallback> const& fb, font_type type, IDWriteFontFallbackBuilder* bldr, IDWriteFontCollection1* collection);
		void load_fonts_from_directory(window_data const& win, std::wstring const& directory, IDWriteFontSetBuilder2* bldr);
		void apply_formatting(IDWriteTextLayout* target, std::vector<format_marker> const& formatting, std::vector<font_description> const& named_fonts);
	};

	struct win32_text_services : public text_services_wrapper {
		ITfThreadMgr* manager_ptr = nullptr;
		TfClientId client_id = 0;
		bool send_notifications = true;

		win32_text_services();
		virtual ~win32_text_services();
		virtual void start_text_services() override;
		virtual void end_text_services() override;
		virtual text_services_object* create_text_service_object(window_data&, edit_interface& ei) override;
		virtual void on_text_change(text_services_object*, uint32_t old_start, uint32_t old_end, uint32_t new_end) override;
		virtual void on_selection_change(text_services_object*) override;
		virtual void set_focus(window_data& win, text_services_object*) override;
		virtual void suspend_keystroke_handling() override;
		virtual void resume_keystroke_handling() override;
		virtual bool send_mouse_event_to_tso(text_services_object* ts, int32_t x, int32_t y, uint32_t buttons) override;
	};
}
