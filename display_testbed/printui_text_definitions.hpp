#ifndef PRINTUI_TEXT_HEADER
#define PRINTUI_TEXT_HEADER

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "printui_datatypes.hpp"

#include <msctf.h>
#include <usp10.h>
#include <dwrite_3.h>
#include <variant>

namespace printui {
	struct window_data;
}

namespace printui::text {
	struct format_marker;

	enum class extra_formatting : uint8_t {
		none, small_caps, italic, old_numbers, tabular_numbers, bold, superscript, subscript
	};
	struct font_id {
		uint8_t id = 0;
	};
	struct parameter_id {
		uint8_t id = 0;
	};
	struct substitution_mark {
		uint8_t id = 0;
	};

	using formatting_content = std::variant<std::monostate, font_id, parameter_id, substitution_mark, extra_formatting>;

	struct format_marker {
		uint16_t position;
		formatting_content format;

		bool operator<(const format_marker& o) const noexcept {
			return position < o.position;
		}
	};

	struct arranged_text;
	struct text_format_ptr;

	struct text_format {
		text_format_ptr* ptr;
		float baseline;
	};

	struct arrangement_result {
		arranged_text* ptr = nullptr;
		int32_t width_used = 0;
		int32_t lines_used = 0;
	};


	struct text_metrics {
		uint32_t textPosition;
		uint32_t length;
		float left;
		float top;
		float width;
		float height;
		uint32_t bidiLevel;
	};
	struct hit_test_metrics {
		text_metrics metrics;
		bool is_inside;
		bool is_trailing;
	};

	struct direct_write_text {
	private:
		IDWriteFactory6* dwrite_factory = nullptr;
		IDWriteFontCollection2* font_collection = nullptr;

		IDWriteTextFormat3* common_text_format = nullptr;
		IDWriteTextFormat3* small_text_format = nullptr;
		IDWriteTextFormat3* header_text_format = nullptr;

		IDWriteFontFallback* font_fallbacks = nullptr;
		IDWriteFontFallback* small_font_fallbacks = nullptr;
		IDWriteFontFallback* header_font_fallbacks = nullptr;

		IDWriteRenderingParams3* common_text_params = nullptr;
		IDWriteRenderingParams3* small_text_params = nullptr;
		IDWriteRenderingParams3* header_text_params = nullptr;
	public:
		direct_write_text();
		virtual ~direct_write_text() {
			safe_release(common_text_params);
			safe_release(small_text_params);
			safe_release(header_text_params);
			safe_release(common_text_format);
			safe_release(small_text_format);
			safe_release(header_text_format);
			safe_release(font_fallbacks);
			safe_release(small_font_fallbacks);
			safe_release(header_font_fallbacks);
			safe_release(font_collection);
			safe_release(dwrite_factory);
		}

		void initialize_fonts(window_data& win);
		void initialize_font_fallbacks(window_data& win);
		void create_font_collection(window_data& win);
		arrangement_result create_text_arragement(window_data const& win, std::wstring_view text, content_alignment text_alignment, text_size text_sz, bool single_line, int32_t max_width, std::vector<format_marker> const* formatting = nullptr) const;
		text_format create_text_format(window_data const& win, wchar_t const* name, int32_t capheight, bool italic, float stretch, int32_t weight, int32_t top_lead, int32_t bottom_lead) const;
		void release_text_format(text_format fmt) const;
		void* to_dwrite_format(text_format fmt) const;
		void* to_dwrite_layout(arranged_text* ptr) const;
		void* get_dwrite_factory() const;
		std::vector<std::wstring> ennumerate_fonts(std::wstring const& locale) const;
		void update_text_rendering_parameters(window_data& win);
		void* get_rendering_paramters(text_size sz) const;
	private:
		void update_font_metrics(font_description& desc, wchar_t const* locale, float target_pixels, float dpi_scale, IDWriteFont* font);
		void load_fallbacks_by_type(std::vector<font_fallback> const& fb, font_type type, IDWriteFontFallbackBuilder* bldr, IDWriteFontCollection1* collection);
		void load_fonts_from_directory(window_data const& win, std::wstring const& directory, IDWriteFontSetBuilder2* bldr);
		void apply_formatting(IDWriteTextLayout* target, std::vector<format_marker> const& formatting, std::vector<font_description> const& named_fonts) const;
	};

	struct win32_text_services {
	private:
		ITfThreadMgr* manager_ptr = nullptr;
		TfClientId client_id = 0;
		bool send_notifications = true;
	public:
		win32_text_services();
		~win32_text_services();
		void start_text_services();
		void end_text_services();
		text_services_object* create_text_service_object(window_data&, edit_interface& ei);
		void on_text_change(text_services_object*, uint32_t old_start, uint32_t old_end, uint32_t new_end);
		void on_selection_change(text_services_object*);
		void set_focus(window_data& win, text_services_object*);
		void suspend_keystroke_handling();
		void resume_keystroke_handling();
		bool send_mouse_event_to_tso(text_services_object* ts, int32_t x, int32_t y, uint32_t buttons);

		friend struct text_services_object;
	};
}

#endif
