#ifndef PRINTUI_DATATYPES_HEADER
#define PRINTUI_DATATYPES_HEADER

#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace printui {
	enum class font_type {
		roman, sans, script, italic
	};
	enum class input_mode {
		keyboard_only, mouse_only, controller_only, controller_with_pointer, mouse_and_keyboard, system_default, follow_input
	};
	enum class prompt_mode {
		hidden, keyboard, controller
	};
	enum class animation_type : uint8_t {
		fade, slide, flip, none
	};
	enum class animation_direction : uint8_t {
		top, right, left, bottom
	};
	struct screen_space_rect {
		int32_t x;
		int32_t y;
		int32_t width;
		int32_t height;
	};

	struct screen_space_point {
		int32_t x;
		int32_t y;
	};

	struct layout_position {
		int16_t x;
		int16_t y;
	};

	struct layout_rect {
		int16_t x;
		int16_t y;
		int16_t width;
		int16_t height;
	};

	enum class layout_orientation : uint8_t {
		horizontal_left_to_right, horizontal_right_to_left, vertical_left_to_right, vertical_right_to_left
	};

	enum class size_flags : uint8_t {
		none, fill_to_max, match_content, fill_to_max_single_col
	};
	enum class content_orientation : uint8_t {
		line, page
	};
	enum class content_alignment : uint8_t {
		leading, trailing, centered, justified
	};

	enum class text_size : uint8_t {
		standard, note, header
	};

	enum class interactable_type : uint8_t {
		none, button
	};

	struct font_description {
		std::wstring name;
		font_type type = font_type::roman;
		float span = 100.0f;
		int32_t weight = 400;
		bool is_oblique = false;
		int32_t top_leading = 0;
		int32_t bottom_leading = 0;

		float line_spacing = 0.0f;
		float baseline = 0.0f;
		float font_size = 0.0f;
		float vertical_baseline = 0.0f;
		float descender = 0.0f;
	};

	struct unicode_range {
		uint32_t start = 0;
		uint32_t end = 0;
	};

	struct font_fallback {
		std::wstring name;
		std::vector<unicode_range> ranges;
		float scale = 1.0f;
		font_type type = font_type::roman;
	};

	template<class Interface>
	inline void safe_release(Interface*& i) {
		if(i) {
			i->Release();
			i = nullptr;
		}
	}

	struct accessibility_object;
	void release_accessibility_object(accessibility_object* o);

	namespace text {
		struct text_analysis_object;
		void release_text_analysis_object(text_analysis_object* ptr);

		struct text_services_object;
		void release_text_services_object(text_services_object* ptr);

		struct arranged_text;
		void release_arranged_text(arranged_text* ptr);
	}

	struct edit_interface;

	class root_window_provider;

	enum class resize_type : uint8_t {
		resize, maximize, minimize
	};

	struct button_control_base;
	struct button_control_toggle;
	struct icon_button_base;
	struct open_list_control;
	struct layout_interface;
	struct generic_expandable;
	struct editable_numeric_range;
	struct stored_text;
	struct generic_selection_container;
	struct launch_settings;
	struct ui_rectangle;

	struct standard_icons {
		constexpr static uint8_t header_minimize = 0;
		constexpr static uint8_t header_close = 1;

		constexpr static uint8_t window_settings = 2;
		constexpr static uint8_t window_max = 3;
		constexpr static uint8_t window_min = 4;
		constexpr static uint8_t window_restore = 5;
		constexpr static uint8_t window_info = 6;
		constexpr static uint8_t window_close = 7;

		constexpr static uint8_t control_button = 8;
		constexpr static uint8_t control_menu = 9;
		constexpr static uint8_t control_pages = 10;
		constexpr static uint8_t control_list = 11;
		constexpr static uint8_t control_next = 12;
		constexpr static uint8_t control_next_next = 13;
		constexpr static uint8_t control_prev = 14;
		constexpr static uint8_t control_prev_prev = 15;
		constexpr static uint8_t control_toggle = 16;
		constexpr static uint8_t control_text = 17;

		constexpr static uint32_t final_icon = 18;
	};

	struct interactable_state {
	private:
		uint8_t data = 0ui8;
		struct impl_key_type {
		};
		struct impl_group_type {
		};
		struct impl_group_start_type {
		};
	public:
		constexpr static impl_group_type group{};
		constexpr static impl_group_start_type group_start{};
		constexpr static impl_key_type key{};

		interactable_state() : data(0ui8) {
		}
		interactable_state(impl_group_type, uint8_t v) {
			data = uint8_t(v | 0x20);
		}
		interactable_state(impl_group_start_type, uint8_t v) {
			data = uint8_t(v | 0x30);
		}
		interactable_state(impl_key_type, uint8_t v) {
			data = uint8_t(v | 0x10);
		}
		int32_t get_key() {
			return (0x0F & data);
		}
		bool holds_key() {
			return (0xF0 & data) == 0x10;
		}
		bool holds_group() {
			return (0x20 & data) == 0x20;
		}
		bool is_group_start() {
			return (0xF0 & data) == 0x30;
		}
	};

	struct animation_description {
		screen_space_rect animated_region;
		animation_type type = animation_type::none;
		animation_direction direction = animation_direction::top;
		float duration_seconds = 1.0f;
		bool animate_in = true;
	};

	struct animation_defintion {
		animation_type type = animation_type::none;
		float duration_seconds = 1.0f;
		bool animate_in = true;
	};

	struct animation_status_struct {
		animation_description description;

		std::chrono::time_point<std::chrono::steady_clock> start_time;
		bool is_running = false;

	};
}

#endif
