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
		standard, note, header, label
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

	struct brush_color {
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
	};

	struct brush {
		std::wstring texture;
		brush_color rgb;
		bool is_light_color;
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

	enum class column_break_behavior : uint8_t {
		normal, column_header, section_header, dont_break_after
	};
	enum class item_type : uint8_t {
		normal, single_space, double_space, item_start, item_end, single_item, decoration_footer,
		decoration_space
	};

	using layout_reference = uint16_t;
	using ui_reference = uint16_t;


	struct page_content {
		layout_interface* item = nullptr;
		column_break_behavior brk = column_break_behavior::normal;
		item_type type = item_type::normal;
		layout_interface* label = nullptr;
	};

	struct page_layout_specification {
		layout_interface* header = nullptr;
		layout_interface* footer = nullptr;

		page_content const* begin = nullptr;
		page_content const* end = nullptr;

		uint8_t ex_page_left_margin = 0;
		uint8_t ex_page_right_margin = 0;
		uint8_t ex_inter_column_margin = 0;

		uint8_t ex_page_top_margin = 0; // will be added to space for header
		uint8_t ex_page_bottom_margin = 0; // will be added to space for footer

		uint8_t column_left_margin = 2;
		uint8_t column_right_margin = 2;

		uint8_t min_columns = 1;
		uint8_t max_columns = 255;

		uint8_t min_column_horizontal_size = 0;
		uint8_t max_column_horizontal_size = 255;

		uint8_t decoration_brush = uint8_t(-1);
		uint8_t section_footer_decoration = uint8_t(-1);
		uint8_t spacing_decoration = uint8_t(-1);

		bool uniform_column_width = true;
		bool additional_space_to_outer_margins = true; // if false, to inter-column spaces

		content_alignment vertical_column_alignment = content_alignment::leading;
		content_alignment horizontal_columns_alignment = content_alignment::centered; // where additional outer margin space goes

		bool horz_shrink_page_to_content = true; // if set, any extra horizontal space will be removed
		bool vert_shrink_page_to_content = false;
	};


	struct simple_layout_specification {
		uint16_t minimum_page_size = 0;
		uint16_t minimum_line_size = 0;

		size_flags page_flags = size_flags::none;
		size_flags line_flags = size_flags::none;
	};
	
	inline constexpr layout_reference layout_reference_none = std::numeric_limits<layout_reference>::max();
	inline constexpr ui_reference ui_reference_none = std::numeric_limits<ui_reference>::max();

	struct render_interface;

	struct interface_or_layout_ref {
	private:
		render_interface* ptr = nullptr;
	public:
		interface_or_layout_ref() {
		}
		interface_or_layout_ref(render_interface* r) : ptr(r) {
		}
		interface_or_layout_ref(layout_reference r) : ptr(reinterpret_cast<render_interface*>((size_t(r) << 1) | 1)) {
		}

		render_interface* operator->() const {
			if((reinterpret_cast<size_t>(ptr) & 0x01) == 0)
				return ptr;
			else
				return nullptr;
		}
		layout_reference get_layout_reference() const;
		render_interface* get_render_interface() const;
	};

	struct ui_rectangle {
	public:
		static constexpr uint8_t flag_interactable = 0x01;
		static constexpr uint8_t flag_line_highlight = 0x02;
		static constexpr uint8_t flag_frame = 0x04;
		static constexpr uint8_t flag_skip_bg = 0x08;
		static constexpr uint8_t flag_clear_rect = 0x10;
		static constexpr uint8_t flag_preserve_rect = 0x20;
		static constexpr uint8_t flag_needs_update = 0x40;
		static constexpr uint8_t flag_overlay = 0x80;

		interface_or_layout_ref parent_object; // +8 bytes

		uint16_t x_position = 0; // 2 -- in pixels
		uint16_t y_position = 0; // 4 -- in pixels
		uint16_t width = 0; // 6 -- in pixels
		uint16_t height = 0; // 8 -- in pixels

		uint8_t foreground_index = 1; //9
		uint8_t background_index = 0; //10

		uint8_t left_border = 0; //11
		uint8_t right_border = 0; //12
		uint8_t top_border = 0; //13
		uint8_t bottom_border = 0; //14

		uint8_t display_flags = 0; //15

		ui_rectangle() {
		};
		ui_rectangle(uint8_t fg, uint8_t bg) : foreground_index(fg), background_index(bg) {
		};

		operator screen_space_rect() const {
			return screen_space_rect{ x_position, y_position, width, height };
		}
		void rotate_borders(layout_orientation o);
	};

	enum class keyboard_type : uint8_t {
		left_hand, right_hand, right_hand_tilted, custom
	};

	struct keyboard_key_descriptor {
		uint32_t scancode = 0;
		std::wstring display_name;
	};

	struct key_mappings {
		keyboard_type type = keyboard_type::left_hand;
		keyboard_key_descriptor main_keys[12] = {};
		keyboard_key_descriptor primary_escape;
		keyboard_key_descriptor info_key;
		bool info_key_is_sticky = false;
	};

	struct controller_button_state {
		uint32_t val = 0;

		constexpr static uint32_t button_y = 0x0001;
		constexpr static uint32_t button_x = 0x0002;
		constexpr static uint32_t button_b = 0x0004;
		constexpr static uint32_t button_a = 0x0008;
		constexpr static uint32_t button_lb = 0x0010;
		constexpr static uint32_t button_rb = 0x0020;
		constexpr static uint32_t button_start = 0x0040;
		constexpr static uint32_t button_back = 0x0080;

		constexpr static uint32_t thumb_left = 0x0100;
		constexpr static uint32_t thumb_right = 0x0200;

		constexpr static uint32_t dpad_up = 0x0400;
		constexpr static uint32_t dpad_down = 0x0800;
		constexpr static uint32_t dpad_left = 0x1000;
		constexpr static uint32_t dpad_right = 0x2000;
	};


	enum class controller_button : uint8_t {
		y, x, b, a, lb, rb, start, back, tleft, tright, dup, ddown, dleft, dright, no_button
	};

	struct button_combination {
		controller_button first_button;
		controller_button second_button;
		bool sticky;
	};

	struct controller_mappings {
		double deadzone = 0.02;
		float sensitivity = 1.0f;

		controller_button button1 = controller_button::y;
		controller_button button2 = controller_button::x;
		controller_button button3 = controller_button::b;
		controller_button button4 = controller_button::a;

		controller_button first_group = controller_button::lb;
		bool first_group_sticky = false;
		controller_button second_group = controller_button::rb;
		bool second_group_sticky = false;

		button_combination escape = button_combination{ controller_button::lb,controller_button::rb,false};
		button_combination info = button_combination{ controller_button::no_button,controller_button::no_button,true };

		bool left_thumbstick = true;
	};
}

#endif
