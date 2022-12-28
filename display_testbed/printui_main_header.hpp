#ifndef PRINTUI_MAIN_HEADER
#define PRINTUI_MAIN_HEADER

#include "printui_datatypes.hpp"
#include "printui_text_definitions.hpp"
#include "printui_accessibility_definitions.hpp"
#include "printui_files_definitions.hpp"
#include "printui_render_definitions.hpp"
#include "printui_windows_definitions.hpp"

#include "unordered_dense.h"
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <assert.h>
#include <intrin.h>
#include <memory>
#include <limits>
#include <variant>
#include <array>
#include <functional>
#include <chrono>

// for text ids common to the shared prinui controls
namespace text_id {
	inline constexpr uint16_t ui_settings_name = 0;
	inline constexpr uint16_t settings_header = 1;
	inline constexpr uint16_t orientation_label = 2;
	inline constexpr uint16_t orientation_ltr = 3;
	inline constexpr uint16_t orientation_rtl = 4;
	inline constexpr uint16_t orientation_vltr = 5;
	inline constexpr uint16_t orientation_vrtl = 6;
	inline constexpr uint16_t input_mode_label = 7;
	inline constexpr uint16_t input_mode_keyboard_only = 8;
	inline constexpr uint16_t input_mode_mouse_only = 9;
	inline constexpr uint16_t input_mode_controller_only = 10;
	inline constexpr uint16_t input_mode_controller_with_pointer = 11;
	inline constexpr uint16_t input_mode_mouse_and_keyboard = 12;
	inline constexpr uint16_t input_mode_follow_input = 13;
	inline constexpr uint16_t page_fraction = 14;
	inline constexpr uint16_t language_label = 15;
	inline constexpr uint16_t minimize_info = 16;
	inline constexpr uint16_t maximize_info = 17;
	inline constexpr uint16_t restore_info = 18;
	inline constexpr uint16_t settings_info = 19;
	inline constexpr uint16_t info_info = 20;
	inline constexpr uint16_t close_info = 21;
	inline constexpr uint16_t orientation_info = 22;
	inline constexpr uint16_t input_mode_info = 23;
	inline constexpr uint16_t input_mode_mouse_info = 24;
	inline constexpr uint16_t input_mode_automatic_info = 25;
	inline constexpr uint16_t input_mode_controller_info = 26;
	inline constexpr uint16_t input_mode_controller_hybrid_info = 27;
	inline constexpr uint16_t input_mode_keyboard_info = 28;
	inline constexpr uint16_t input_mode_mk_hybrid_info = 29;
	inline constexpr uint16_t language_info = 30;
	inline constexpr uint16_t ui_settings_info = 31;
	inline constexpr uint16_t minimize_name = 32;
	inline constexpr uint16_t maximize_name = 33;
	inline constexpr uint16_t restore_name = 34;
	inline constexpr uint16_t close_name = 35;
	inline constexpr uint16_t settings_name = 36;
	inline constexpr uint16_t info_name = 37;
	inline constexpr uint16_t info_name_on = 38;
	inline constexpr uint16_t window_bar_name = 39;
	inline constexpr uint16_t expandable_container_localized_name = 40;
	inline constexpr uint16_t settings_tabs_name = 41;
	inline constexpr uint16_t selection_list_localized_name = 42;
	inline constexpr uint16_t close_settings_name = 43;
	inline constexpr uint16_t close_menu_name = 44;
	inline constexpr uint16_t page_prev_name = 45;
	inline constexpr uint16_t page_next_name = 46;
	inline constexpr uint16_t page_prev_prev_name = 47;
	inline constexpr uint16_t page_next_next_name = 48;
	inline constexpr uint16_t page_footer_name = 49;
	inline constexpr uint16_t page_footer_info = 50;
	inline constexpr uint16_t generic_toggle_on = 51;
	inline constexpr uint16_t generic_toggle_off = 52;
	inline constexpr uint16_t ui_animations_label = 53;
	inline constexpr uint16_t ui_animations_info = 54;
	inline constexpr uint16_t ui_scale = 55;
	inline constexpr uint16_t ui_scale_edit_name = 56;
	inline constexpr uint16_t ui_scale_info = 57;
	inline constexpr uint16_t primary_font_label = 58;
	inline constexpr uint16_t primary_font_info = 59;
	inline constexpr uint16_t small_font_label = 60;
	inline constexpr uint16_t small_font_info = 61;
	inline constexpr uint16_t fonts_header = 62;
	inline constexpr uint16_t generic_toggle_yes = 63;
	inline constexpr uint16_t generic_toggle_no = 64;
	inline constexpr uint16_t font_weight = 65;
	inline constexpr uint16_t font_stretch = 66;
	inline constexpr uint16_t font_italic = 67;
	inline constexpr uint16_t font_weight_edit_name = 68;
	inline constexpr uint16_t font_stretch_edit_name = 69;
	inline constexpr uint16_t font_italic_info = 70;
	inline constexpr uint16_t font_weight_info = 71;
	inline constexpr uint16_t font_stretch_info = 72;
	inline constexpr uint16_t header_font_label = 73;
	inline constexpr uint16_t header_font_info = 74;
	inline constexpr uint16_t relative_size_label = 75;
	inline constexpr uint16_t relative_size_edit_name = 76;
	inline constexpr uint16_t relative_size_small_info = 77;
	inline constexpr uint16_t relative_size_header_info = 78;
	inline constexpr uint16_t top_lead_label = 79;
	inline constexpr uint16_t top_lead_edit_name = 80;
	inline constexpr uint16_t top_lead_edit_info = 81;
	inline constexpr uint16_t bottom_lead_label = 82;
	inline constexpr uint16_t bottom_lead_edit_name = 83;
	inline constexpr uint16_t bottom_lead_edit_info = 84;
	inline constexpr uint16_t keyboard_header = 85;
	inline constexpr uint16_t key_ord_name = 86;
	inline constexpr uint16_t scan_code = 87;
	inline constexpr uint16_t key_display_name = 88;
	inline constexpr uint16_t keyboard_arrangement = 89;
	inline constexpr uint16_t keyboard_left = 90;
	inline constexpr uint16_t keyboard_right = 91;
	inline constexpr uint16_t keyboard_tilted = 92;
	inline constexpr uint16_t keyboard_custom = 93;
	inline constexpr uint16_t keyboard_arragnement_info = 94;
	inline constexpr uint16_t key_escape_name = 95;
	inline constexpr uint16_t key_escape_info = 96;
	inline constexpr uint16_t key_info_name = 97;
	inline constexpr uint16_t key_info_info = 98;
	inline constexpr uint16_t info_key_is_sticky = 99;
	inline constexpr uint16_t keyboard_code_info = 100;
	inline constexpr uint16_t keyboard_display_name_info = 101;
	inline constexpr uint16_t keyboard_display_edit_name = 102;
	inline constexpr uint16_t info_key_sticky_info = 103;

	inline constexpr uint16_t first_free_id = 104;
}

namespace printui {
	

	class arranged_text_ptr {
		text::arranged_text* ptr = nullptr;
	public:
		arranged_text_ptr() noexcept {
		}
		arranged_text_ptr(text::arranged_text* ptr) noexcept : ptr(ptr) {
		}
		arranged_text_ptr(arranged_text_ptr&& o) noexcept {
			ptr = o.ptr;
			o.ptr = nullptr;
		}
		~arranged_text_ptr() {
			if(ptr)
				text::release_arranged_text(ptr);
			ptr = nullptr;
		}
		arranged_text_ptr& operator=(arranged_text_ptr const& o) = delete;
		arranged_text_ptr& operator=(arranged_text_ptr&& o) noexcept {
			if(ptr)
				text::release_arranged_text(ptr);
			ptr = o.ptr;
			o.ptr = nullptr;
			return *this;
		}
		arranged_text_ptr& operator=(text::arranged_text* o) noexcept {
			if(ptr)
				text::release_arranged_text(ptr);
			ptr = o;
			return *this;
		}
		operator bool() const noexcept {
			return ptr != nullptr;
		}
		operator text::arranged_text* () const noexcept {
			return ptr;
		}
	};

	class text_analysis_ptr {
		text::text_analysis_object* ptr = nullptr;
	public:
		text_analysis_ptr() noexcept {
		}
		text_analysis_ptr(text::text_analysis_object* ptr) noexcept : ptr(ptr) {
		}
		text_analysis_ptr(text_analysis_ptr&& o) noexcept {
			ptr = o.ptr;
			o.ptr = nullptr;
		}
		~text_analysis_ptr() {
			if(ptr)
				text::release_text_analysis_object(ptr);
			ptr = nullptr;
		}
		text_analysis_ptr& operator=(text_analysis_ptr const& o) = delete;
		text_analysis_ptr& operator=(text_analysis_ptr&& o) noexcept {
			if(ptr)
				text::release_text_analysis_object(ptr);
			ptr = o.ptr;
			o.ptr = nullptr;
			return *this;
		}
		text_analysis_ptr& operator=(text::text_analysis_object* o) noexcept {
			if(ptr)
				text::release_text_analysis_object(ptr);
			ptr = o;
			return *this;
		}
		operator bool() const noexcept {
			return ptr != nullptr;
		}
		operator text::text_analysis_object* () const noexcept {
			return ptr;
		}
	};

	class text_services_ptr {
		text::text_services_object* ptr = nullptr;
	public:
		text_services_ptr() noexcept {
		}
		text_services_ptr(text::text_services_object* ptr) noexcept : ptr(ptr) {
		}
		text_services_ptr(text_services_ptr&& o) noexcept {
			ptr = o.ptr;
			o.ptr = nullptr;
		}
		~text_services_ptr() {
			if(ptr)
				text::release_text_services_object(ptr);
			ptr = nullptr;
		}
		text_services_ptr& operator=(text_services_ptr const& o) = delete;
		text_services_ptr& operator=(text_services_ptr&& o) noexcept {
			if(ptr)
				text::release_text_services_object(ptr);
			ptr = o.ptr;
			o.ptr = nullptr;
			return *this;
		}
		text_services_ptr& operator=(text::text_services_object* o) noexcept {
			if(ptr)
				text::release_text_services_object(ptr);
			ptr = o;
			return *this;
		}
		operator bool() const noexcept {
			return ptr != nullptr;
		}
		operator text::text_services_object* () const noexcept {
			return ptr;
		}
	};

	class accessibility_object_ptr {
		accessibility_object* ptr = nullptr;
	public:
		accessibility_object_ptr() noexcept {
		}
		accessibility_object_ptr(accessibility_object* ptr) noexcept : ptr(ptr) {
		}
		accessibility_object_ptr(accessibility_object_ptr&& o) noexcept {
			ptr = o.ptr;
			o.ptr = nullptr;
		}
		~accessibility_object_ptr() {
			if(ptr)
				release_accessibility_object(ptr);
			ptr = nullptr;
		}
		accessibility_object_ptr& operator=(accessibility_object_ptr const& o) = delete;
		accessibility_object_ptr& operator=(accessibility_object_ptr&& o) noexcept {
			if(ptr)
				release_accessibility_object(ptr);
			ptr = o.ptr;
			o.ptr = nullptr;
			return *this;
		}
		accessibility_object_ptr& operator=(accessibility_object* o) noexcept {
			if(ptr)
				release_accessibility_object(ptr);
			ptr = o;
			return *this;
		}
		operator bool() const noexcept {
			return ptr != nullptr;
		}
		operator accessibility_object* () const noexcept {
			return ptr;
		}
	};

	


	struct launch_settings {
		font_description primary_font;
		font_description small_font;
		font_description header_font;
		std::vector<font_description> named_fonts;
		std::vector<font_fallback> fallbacks;
		std::vector<brush> brushes;
		float layout_base_size = 20.0f;
		float small_size_multiplier = 0.75f;
		float heading_size_multiplier = 1.375f;
		int32_t line_width = 25;
		int32_t small_width = 15;
		int32_t window_border = 1;
		input_mode imode = input_mode::system_default;
		int32_t window_x_size = 640;
		int32_t window_y_size = 480;
		int32_t light_interactable_brush = -1;
		int32_t dark_interactable_brush = -1;
		std::wstring icon_directory;
		std::wstring font_directory;
		std::wstring texture_directory;
		std::wstring text_directory;
		std::wstring locale_region;
		std::wstring locale_lang;
		bool locale_is_default = true;

		layout_orientation preferred_orientation = layout_orientation::horizontal_left_to_right;
		float animation_speed_multiplier = 1.0f;
		float global_size_multiplier = 1.0f;
		bool uianimations = true;
		bool caret_blink = true;

		key_mappings keys;

		bool settings_changed = false;
		
	};

	namespace text {
		struct string_hash {
			using is_transparent = void;
			[[nodiscard]] size_t operator()(const char* txt) const {
				return std::hash<std::string_view>{}(txt);
			}
			[[nodiscard]] size_t operator()(std::string_view txt) const {
				return std::hash<std::string_view>{}(txt);
			}
			[[nodiscard]] size_t operator()(const std::string& txt) const {
				return std::hash<std::string_view>{}(std::string_view(txt));
			}
		};
		struct text_id {
			uint32_t id = 0;
		};
		struct fp_param {
			double value = 0.0;
			uint8_t decimal_places = 0;
		};
		struct int_param {
			int64_t value = 0;
			uint8_t decimal_places = 0;
		};
		using text_parameter = std::variant<int_param, fp_param, text_id>;
	}

	namespace parse {
		void settings_file(launch_settings& ls, std::unordered_map<std::string, uint32_t, text::string_hash, std::equal_to<>>& font_name_to_index, char const* start, char const* end);
		void custom_fonts_only(launch_settings& ls, std::unordered_map<std::string, uint32_t, text::string_hash, std::equal_to<>>& font_name_to_index, char const* start, char const* end);
		std::string create_settings_file(launch_settings const& ls);
		void font_fallbacks_file(std::vector<printui::font_fallback>& result, char const* start, char const* end);
	}

	struct window_data;

	struct ui_rectangle;

	screen_space_rect screen_rectangle_from_layout(window_data const& win,
		int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height);
	screen_space_point screen_point_from_layout(layout_orientation o,
		int32_t x_pos, int32_t y_pos, screen_space_rect const& in_rect);
	screen_space_point screen_topleft_from_layout_in_ui(window_data const& win, int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height, screen_space_rect const& rect);
	screen_space_rect screen_rectangle_from_layout_in_ui(window_data const& win,
		int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height,
		screen_space_rect const& rect);
	screen_space_rect screen_rectangle_from_relative_rect_in_ui(window_data const& win,
		screen_space_rect const& rect_in, screen_space_rect const& rect);
	screen_space_rect reverse_screen_space_orientation(window_data const& win, screen_space_rect source);
	screen_space_rect intersection(screen_space_rect a, screen_space_rect b);

	struct layout_interface;
	struct render_interface;

	

	struct page_information {
	private:
		std::vector<layout_reference> columns;
	public:
		std::vector<uint16_t> subpage_divisions;

		layout_reference header = layout_reference_none;
		layout_reference footer = layout_reference_none;
		uint16_t subpage_offset = 0;

		std::vector<layout_reference> const& view_columns() const {
			return columns;
		}
		std::vector<layout_reference>& modify_columns() {
			return columns;
		}
		void clear_columns() {
			columns.clear();
		}
		void add(layout_reference r) {
			columns.push_back(r);
		}
	};

	struct container_information {
	private:
		std::vector<layout_reference> children;
	public:
		std::vector<layout_reference> const& view_children() const {
			return children;
		}
		void clear_children() {
			children.clear();
		}
		std::vector<layout_reference>& modify_children() {
			return children;
		}
		void add(layout_reference r) {
			children.push_back(r);
		}
	};

	struct decoration_id {
		uint8_t id;
		uint8_t brush;
	};

	struct layout_node {
		std::variant<
			std::unique_ptr<page_information>,
			std::unique_ptr<container_information>,
			decoration_id,
			std::monostate> contents = std::monostate{};

		layout_interface* l_interface = nullptr;
		layout_reference parent = layout_reference_none;
		ui_reference visible_rect = ui_reference_none;

		uint16_t x = 0; //in layout units
		uint16_t y = 0;
		uint16_t width = 0;
		uint16_t height = 0;

		//bool layout_deferred = true;
		//bool ignore = false;

		uint8_t flags = 0;
		uint8_t l_margin = 0;
		uint8_t r_margin = 0;

		constexpr static uint8_t flag_generation_mask = 0x07;
		constexpr static uint8_t flag_overlay = 0x08;
		constexpr static uint8_t flag_layout_deferred = 0x10;
		constexpr static uint8_t flag_ignore = 0x20;
		constexpr static uint8_t flag_highlight = 0x40;
		constexpr static uint8_t flag_freed = 0x80;

		int32_t left_margin() const {
			return l_margin;
		}
		int32_t right_margin() const {
			return r_margin;
		}
		bool ignore() const { 
			return (flags & flag_ignore) != 0;
		}
		uint8_t generation() const {
			return uint8_t(flags & flag_generation_mask);
		}
		bool is_deferred() const {
			return (flags & flag_layout_deferred) != 0;
		}
		bool highlight() const {
			return (flags & flag_highlight) != 0;
		}
		void set_ignore(bool v) {
			flags = uint8_t((flags & ~flag_ignore) | (v ? flag_ignore : 0));
		}
		void set_deferred(bool v) {
			flags = uint8_t((flags & ~flag_layout_deferred) | (v ? flag_layout_deferred : 0));
		}
		void set_highlight(bool v) {
			flags = uint8_t((flags & ~flag_highlight) | (v ? flag_highlight : 0));
		}
		void set_generation(uint8_t v) {
			flags = uint8_t((flags & ~flag_generation_mask) | (v & flag_generation_mask));
		}
		void set_margins(int32_t left, int32_t right) {
			l_margin = uint8_t(left);
			r_margin = uint8_t(right);
		}

		void reset_node();

		page_information* page_info() const {
			if(std::holds_alternative<std::unique_ptr<page_information>>(contents)) {
				return std::get<std::unique_ptr<page_information>>(contents).get();
			}
			return nullptr;
		}
		container_information* container_info() const {
			if(std::holds_alternative<std::unique_ptr<container_information>>(contents)) {
				return std::get<std::unique_ptr<container_information>>(contents).get();
			}
			return nullptr;
		}
		std::vector<layout_reference> const* direct_children() const {
			if(std::holds_alternative<std::unique_ptr<page_information>>(contents)) {
				return &(std::get<std::unique_ptr<page_information>>(contents).get()->view_columns());
			} else if(std::holds_alternative<std::unique_ptr<container_information>>(contents)) {
				return &(std::get<std::unique_ptr<container_information>>(contents).get()->view_children());
			} else {
				return nullptr;
			}
		}
	};

	class layout_node_storage {
	private:
		std::vector<layout_node> node_storage;
		layout_reference last_free = std::numeric_limits<layout_reference>::max();

		uint8_t current_generation = 0;
	public:
		void reset();
		void begin_new_generation();
		void release_node(layout_reference id);
		void update_generation(layout_node& n);
		void garbage_collect();
		layout_reference allocate_node();
		layout_reference allocate_node(layout_interface* li);
		layout_node& get_node(layout_reference id);
		layout_node const& get_node(layout_reference id) const;
	};

	enum class layout_node_type {
		visible, control, container, page
	};

	struct layout_interface {
		layout_reference l_id = layout_reference_none;

		virtual ~layout_interface() {
		}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) = 0;
		virtual layout_node_type get_node_type() = 0;
		virtual simple_layout_specification get_specification(window_data&) = 0;

		virtual layout_rect get_content_rectangle(window_data&) {
			return layout_rect{ 0i16, 0i16, 0i16, 0i16 };
		}
		void set_layout_id(layout_reference val) {
			l_id = val;
		}
		virtual void recreate_contents(window_data&, layout_node&) {
		}
		virtual void on_focus(window_data&) {
		}
		virtual void on_lose_focus(window_data&) {
		}
		virtual void go_to_page(window_data&, uint32_t pg, page_information& pi) {
			pi.subpage_offset = uint16_t(pg);
		}
		virtual accessibility_object* get_accessibility_interface(window_data&) {
			return nullptr;
		}
	};


	struct render_interface : public layout_interface {
		virtual ~render_interface() {
		}

		virtual void render_foreground(ui_rectangle const& rect, window_data& win) = 0;

		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse);
		virtual void on_click(window_data&, uint32_t, uint32_t) {
		}
		virtual void on_click(window_data& p, uint32_t) { // for direct use with an interactable
			on_click(p, 0, 0);
		}
		virtual void on_right_click(window_data&, uint32_t, uint32_t) {
		}
		virtual void on_right_click(window_data& p, uint32_t) { // for direct use with an interactable
			on_right_click(p, 0, 0);
		}
		virtual int32_t interactable_count(window_data const&) {
			return 0;
		}
		virtual void set_interactable(int32_t, interactable_state) {
		}
		virtual bool data_is_ready() {
			return true;
		}
	};

	enum class edit_command : uint8_t {
		new_line, backspace, delete_char, backspace_word, delete_word, tab, cursor_down, cursor_up, cursor_left, cursor_right, cursor_left_word, cursor_right_word, to_line_start, to_line_end, to_text_start, to_text_end, cut, copy, paste, select_all, undo, redo, select_current_word, select_current_section, delete_selection
	};

	enum class edit_contents : uint8_t {
		generic_text, number, single_char, email, date, time, url
	};

	struct mouse_test_result {
		uint32_t position;
		uint32_t quadrent;
	};

	struct edit_interface {
		virtual ~edit_interface() { }

		// commands sent to control
		virtual void insert_codepoint(window_data&, uint32_t codepoint) = 0;
		virtual void clear(window_data&) = 0;
		virtual void move_cursor(window_data&, uint32_t position, bool extend_selection) = 0;
		virtual void set_selection(window_data&, uint32_t start, uint32_t end) = 0;
		virtual bool move_cursor_by_screen_pt(window_data&, screen_space_point pt, bool extend_selection) = 0; // returns if cursor was captured;
		virtual void command(window_data&, edit_command cmd, bool extend_selection) = 0;
		virtual void set_temporary_selection(window_data&, uint32_t start, uint32_t end) = 0;
		virtual void clear_temporary_contents(window_data&) = 0;
		virtual void register_composition_result(window_data&) = 0;
		virtual void register_conversion_target_change(window_data&) = 0;
		virtual void set_cursor_visibility(window_data&, bool is_visible) = 0;
		virtual void insert_text(window_data&, uint32_t position_start, uint32_t position_end, std::wstring_view content) = 0;
		virtual bool consume_mouse_event(window_data& win, int32_t x, int32_t y, uint32_t buttons) = 0;
		virtual void make_line_visible(window_data& win, int32_t line) = 0;

		// retrieve information from control
		virtual uint32_t get_cursor() const = 0;
		virtual uint32_t get_selection_anchor() const = 0;
		virtual uint32_t get_text_length() const = 0;
		virtual std::wstring get_text() const = 0;
		virtual screen_space_rect get_edit_bounds(window_data&) const = 0;
		virtual screen_space_rect get_character_bounds(window_data&, uint32_t position) const = 0;
		virtual uint32_t get_temporary_position() const = 0;
		virtual uint32_t get_temporary_length() const = 0;
		virtual layout_interface* get_layout_interface() = 0;
		virtual uint32_t get_position_from_screen_point(window_data&, screen_space_point pt) = 0;
		virtual mouse_test_result get_detailed_position(window_data&, screen_space_point pt) = 0; //in window coordinates
		virtual void populate_selection_rectangles(window_data&, std::vector<screen_space_rect>& rects) = 0;
		virtual void get_range_bounds(window_data&, uint32_t position_start, uint32_t position_end, std::vector<screen_space_rect>& rects) = 0;
		virtual bool is_read_only() const = 0;
		virtual content_alignment get_alignment() const = 0;
		virtual bool position_is_ltr(uint32_t position) = 0;
		virtual edit_contents get_type() const = 0;
		virtual bool is_word_position(uint32_t v) = 0;
		virtual bool is_valid_cursor_position(uint32_t v) = 0;
		virtual uint32_t previous_word_position(uint32_t v) = 0;
		virtual uint32_t next_word_position(uint32_t v) = 0;
		virtual uint32_t previous_valid_cursor_position(uint32_t v) = 0;
		virtual uint32_t next_valid_cursor_position(uint32_t v) = 0;
		virtual uint32_t position_visually_above(window_data& win, uint32_t v) = 0;
		virtual uint32_t position_visually_below(window_data& win, uint32_t v) = 0;
		virtual bool is_start_of_line(uint32_t v) = 0;
		virtual uint32_t start_of_line(uint32_t v) = 0;
		virtual uint32_t line_of_position(uint32_t v) = 0;
		virtual uint32_t number_of_text_lines() = 0;
		virtual uint32_t first_visible_line() = 0;

		// notify control of event
		virtual void on_finalize(window_data&) = 0;
		virtual void on_initialize(window_data&) = 0;
	};

	struct wrapped_text_instance {
		text::text_parameter stored_params[10] = {};
		uint16_t text_id = uint16_t(-1);
		uint8_t params_count = 0;
		uint8_t text_generation = 0;
	};

	struct stored_text {
	private:
		arranged_text_ptr formatted_text;
		std::variant<std::monostate, std::wstring, wrapped_text_instance> text_content = std::monostate{};
	public:
		layout_position resolved_text_size{ 0,1 };
		content_alignment text_alignment = content_alignment::leading;
		text_size text_sz = text_size::standard;
	public:
		~stored_text();
		void set_text(std::wstring const& v);
		void set_text(uint16_t text_id, text::text_parameter const* begin = nullptr, text::text_parameter const* end = nullptr);
		void set_text();
		void set_text(stored_text const& v);
		void prepare_text(window_data const& win);
		void relayout_text(window_data const& win, screen_space_point sz);
		void draw_text(window_data& win, int32_t x, int32_t y) const;
		void invalidate();
		int32_t get_lines_height(window_data const& win) const;
		int32_t get_text_width(window_data const& win) const;
		std::wstring get_raw_text(window_data const& win) const;
	};

	struct title_bar_element : public render_interface {
		stored_text text;
		accessibility_object_ptr acc_obj;

		title_bar_element() {}
		virtual ~title_bar_element();

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;

		void set_text_content(uint16_t text_id);
	};


	enum class button_category : uint8_t {
		action_button, selection_button, toggle_button
	};

	struct button_control_base : public render_interface {
	protected:
		accessibility_object_ptr acc_obj;
	private:
		bool disabled = false;
		bool selected = false;
		stored_text button_text;
		uint16_t alt_text = uint16_t(-1);

	public:
		interactable_state saved_state;

		uint8_t icon = standard_icons::control_button;
		button_category category = button_category::action_button;

		button_control_base() {
		}
		button_control_base(uint16_t def_text, content_alignment ta, uint16_t alt_text) : alt_text(alt_text) {
			button_text.set_text(def_text);
			button_text.text_alignment = ta;
		}
		virtual ~button_control_base();

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void recreate_contents(window_data&, layout_node&) override;

		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual int32_t interactable_count(window_data const&) override {
			return 1;
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) final override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
		virtual void button_action(window_data&) = 0;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
		void set_disabled(window_data& win, bool v);
		void set_selected(window_data& win, bool v);
		void set_alt_text(window_data& win, uint16_t alt);
		uint16_t get_alt_text() const;
		void set_text(window_data& win, uint16_t val, text::text_parameter* b = nullptr, text::text_parameter* e = nullptr);
		void set_text(window_data& win, std::wstring const& val);
		void set_text(window_data& win);
		void set_text(window_data& win, stored_text const& t);
		void set_text_alignment(content_alignment align);
		std::wstring get_raw_text(window_data const& win) const;
		stored_text const& get_stored_text() const {
			return button_text;
		}
		bool is_disabled() const {
			return disabled;
		}
		bool is_selected() const {
			return selected;
		}
		void invalidate_text() {
			button_text.invalidate();
		}
	};

	struct button_control_toggle : public button_control_base {
		uint16_t toggle_on_text = uint16_t(-1);
		uint16_t toggle_off_text = uint16_t(-1);

		button_control_toggle(uint16_t on_text, uint16_t off_text, content_alignment ta, uint16_t alt_text) : 
			button_control_base(on_text, ta, alt_text),
			toggle_on_text(on_text), toggle_off_text(off_text) {
			icon = standard_icons::control_toggle;
			category = button_category::toggle_button;
		}
		bool toggle_is_on = true;
		virtual void button_action(window_data&) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
		virtual void toggle_action(window_data&, bool toggle_state) = 0;

		void change_toggle_state(window_data&, bool toggle_state);
	};

	struct icon_button_base : public render_interface {
	private:
		accessibility_object_ptr acc_obj;
		uint16_t name_text = uint16_t(-1);
		uint16_t alt_text = uint16_t(-1);

	public:
		interactable_state saved_state;
		uint8_t ico = standard_icons::control_button;
		bool display_vertically = false;
		button_category category = button_category::action_button;

		icon_button_base() { }
		icon_button_base(uint16_t name_text, uint16_t alt_text) : name_text(name_text), alt_text(alt_text) { }
		virtual ~icon_button_base() { }

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;

		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual int32_t interactable_count(window_data const&) override {
			return 1;
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) final override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
		virtual void button_action(window_data&) = 0;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
		void set_alt_text(window_data& win, uint16_t alt);
		uint16_t get_alt_text() const;
		void set_name_text(window_data& win, uint16_t alt);
		uint16_t get_name_text() const;
		virtual bool is_disabled(window_data const&) const {
			return false;
		}
		virtual bool is_toggled() const {
			return false;
		}
		virtual uint16_t get_toggled_text() const {
			return uint16_t(-1);
		}
		virtual std::wstring get_name(window_data const&);
	};

	enum class edit_selection_mode : uint8_t {
		none, standard, word, line
	};

	struct simple_editable_text : public render_interface, public edit_interface {
		struct selection_run {
			int32_t start = 0;
			int32_t end = 0;
			int32_t line = 0;
		};
	protected:
		std::vector<selection_run> cached_selection_region;
		accessibility_object_ptr acc_obj;
		text_services_ptr ts_obj;
		text_analysis_ptr analysis_obj;
		arranged_text_ptr formatted_text;

		std::wstring text;
		
		int32_t anchor_position = 0;
		int32_t cursor_position = 0;
		int32_t mouse_entry_position = 0;
		int32_t line_offset = 0;
		int32_t temp_text_position = 0;
		int32_t temp_text_length = 0;
		int32_t cached_cursor_postion = 0;

		uint16_t name;
		uint16_t alt_text;

		uint8_t minimum_layout_space;
		uint8_t layout_lines = 1;
		content_alignment text_alignment;
		

		bool disabled = false;
		bool changes_made = false;
		bool cursor_visible = false;
		bool selection_out_of_date = false;

		void prepare_selection_regions(window_data const& win);
		void internal_on_text_changed(window_data&);
		void internal_on_selection_changed(window_data&);
		void internal_move_cursor_to_point(window_data&, int32_t x, int32_t y, bool extend_selection);
	public:
		interactable_state saved_state;
		edit_contents edit_type = edit_contents::generic_text;

		simple_editable_text(content_alignment text_alignment, uint16_t name, uint16_t alt_text, uint8_t minimum_layout_space, uint8_t lines);
		virtual ~simple_editable_text();

		// render_interface
		virtual layout_node_type get_node_type() {
			return layout_node_type::control;
		}
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void on_click(window_data&, uint32_t, uint32_t) final override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) final override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;

		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual int32_t interactable_count(window_data const&) override {
			if(!disabled)
				return 1;
			else
				return 0;
		}
		
		//edit_interface
		// commands sent to control
		virtual void insert_codepoint(window_data&, uint32_t codepoint) override;
		virtual void clear(window_data&) override;
		virtual void move_cursor(window_data&, uint32_t position, bool extend_selection) override;
		virtual void set_selection(window_data&, uint32_t start, uint32_t end) override;
		virtual bool move_cursor_by_screen_pt(window_data&, screen_space_point pt, bool extend_selection) override; // returns if cursor was captured;
		virtual void command(window_data&, edit_command cmd, bool extend_selection) override;
		virtual void set_temporary_selection(window_data&, uint32_t start, uint32_t end) override;
		virtual void clear_temporary_contents(window_data&) override;
		virtual void register_composition_result(window_data&) override;
		virtual void register_conversion_target_change(window_data&) override;
		virtual void set_cursor_visibility(window_data&, bool is_visible) override;
		virtual void insert_text(window_data&, uint32_t position_start, uint32_t position_end, std::wstring_view content) override;
		virtual bool consume_mouse_event(window_data& win, int32_t x, int32_t y, uint32_t buttons) override;
		virtual void make_line_visible(window_data& win, int32_t line);

		// retrieve information from control
		virtual uint32_t get_cursor() const override;
		virtual uint32_t get_selection_anchor() const override;
		virtual uint32_t get_text_length() const override;
		virtual uint32_t get_temporary_position() const override;
		virtual uint32_t get_temporary_length() const override;
		virtual std::wstring get_text() const override;
		virtual screen_space_rect get_edit_bounds(window_data&) const override;
		virtual screen_space_rect get_character_bounds(window_data&, uint32_t position) const override;
		virtual uint32_t get_position_from_screen_point(window_data&, screen_space_point pt) override;
		virtual mouse_test_result get_detailed_position(window_data&, screen_space_point pt) override; //in window coordinates
		virtual void populate_selection_rectangles(window_data&, std::vector<screen_space_rect>& rects) override;
		virtual void get_range_bounds(window_data&, uint32_t position_start, uint32_t position_end, std::vector<screen_space_rect>& rects) override;
		virtual layout_interface* get_layout_interface() override {
			return this;
		}
		virtual bool position_is_ltr(uint32_t position) override;
		virtual edit_contents get_type() const override {
			return edit_type;
		}
		virtual bool is_word_position(uint32_t v) override;
		virtual bool is_valid_cursor_position(uint32_t v) override;
		virtual uint32_t previous_word_position(uint32_t v) override;
		virtual uint32_t next_word_position(uint32_t v) override;
		virtual uint32_t previous_valid_cursor_position(uint32_t v) override;
		virtual uint32_t next_valid_cursor_position(uint32_t v) override;
		virtual uint32_t position_visually_above(window_data& win, uint32_t v) override;
		virtual uint32_t position_visually_below(window_data& win, uint32_t v) override;
		virtual bool is_start_of_line(uint32_t v) override;
		virtual uint32_t start_of_line(uint32_t v) override;
		virtual uint32_t line_of_position(uint32_t v) override;
		virtual uint32_t number_of_text_lines() override;
		virtual uint32_t first_visible_line() override;

		// notify control of event
		virtual void on_finalize(window_data&) override;
		virtual void on_initialize(window_data&) override;

		// exposed properties
		void set_disabled(window_data& win, bool v);
		void set_alt_text(window_data& win, uint16_t alt);
		void set_text(window_data& win, std::wstring const&);
		void quiet_set_text(window_data& win, std::wstring const&);
		uint16_t get_alt_text() const;
		uint16_t get_name() const;
		virtual content_alignment get_alignment() const override {
			return text_alignment;
		}
		bool is_disabled() const {
			return disabled;
		}
		bool is_read_only() const override {
			return disabled;
		}

		// for rendering
		bool prepare_text(window_data const& win);
		bool relayout_text(window_data const& win, screen_space_point sz);
		void draw_text(window_data& win, int32_t x, int32_t y) const;

		// for implementations
		virtual void on_text_changed(window_data&, std::wstring const&) { }
		virtual void on_edit_finished(window_data&, std::wstring const&) { }

	};

	struct editable_numeric_range : public simple_editable_text {
		float minimum = 0.0f;
		float maximum = 0.0f;
		int8_t precision = 0;

		editable_numeric_range(content_alignment text_alignment, uint16_t name, uint16_t alt_text, uint8_t minimum_layout_space, uint8_t lines, float minimum, float maximum, int8_t precision);
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
		void set_value(window_data& win, float v);
		float get_value(window_data& win) const;
		virtual void command(window_data&, edit_command cmd, bool extend_selection) override;
	};

	struct vertical_2x2_max_icon : public icon_button_base {
		vertical_2x2_max_icon();
		virtual void button_action(window_data&) override;
		void update_window_state(window_data&);
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
	};

	struct vertical_2x2_min_icon : public icon_button_base {
		vertical_2x2_min_icon();
		virtual void button_action(window_data&) override;
	};

	struct vertical_2x2_close_icon : public icon_button_base {
		vertical_2x2_close_icon();
		virtual void button_action(window_data&) override;
	};

	struct vertical_2x2_settings_icon : public icon_button_base {
		vertical_2x2_settings_icon();
		virtual void button_action(window_data&) override;
	};

	struct vertical_2x2_info_icon : public icon_button_base {
		window_data& win;
		vertical_2x2_info_icon(window_data& win);
		virtual void button_action(window_data&) override;
		virtual int32_t interactable_count(window_data const&) override {
			return 0;
		}
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual bool is_toggled() const override;
		void mark_for_update(window_data& win);
	};

	struct label_control : public render_interface {
	private:
		stored_text label_text;
		accessibility_object_ptr acc_obj;
	public:
		uint16_t alt_text = uint16_t(-1);



		label_control(content_alignment a = content_alignment::leading) {
			label_text.text_alignment = a;
		}
		label_control(uint16_t t, content_alignment a, uint16_t alt_text) : alt_text(alt_text) {
			label_text.set_text(t);
			label_text.text_alignment = a;
		}
		virtual ~label_control() {
		}

		virtual layout_node_type get_node_type() override {
			return layout_node_type::visible;
		};
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;

		stored_text const& get_stored_text() const {
			return label_text;
		}
		void set_text(window_data&);
		void set_text(window_data&, uint16_t, text::text_parameter const* begin = nullptr, text::text_parameter const* end = nullptr);
		void set_text(window_data&, std::wstring const&);

		void quiet_set_text(uint16_t, text::text_parameter const* begin = nullptr, text::text_parameter const* end = nullptr);
	};

	struct list_control;

	struct list_option : public button_control_base {
		size_t data = 0;
		list_option() {
		}
		virtual ~list_option() {
		}
		virtual void button_action(window_data&);
	};
	struct list_open : public button_control_base {
		list_open() {
		}
		virtual ~list_open() {
		}
		virtual void button_action(window_data&);
	};

	struct list_option_description {
		size_t data;
		uint16_t text_id;
		uint16_t alt_text_id;
	};

	struct generic_expandable : public layout_interface {
		virtual bool is_open() const = 0;
		virtual void open(window_data&, bool move_focus) = 0;
		virtual void close(window_data&, bool move_focus) = 0;
	};

	struct generic_selection_container {
		virtual layout_interface* selected_item() const = 0;
	};

	struct list_control : public generic_expandable, public generic_selection_container {
		static animation_defintion list_appearance;
		static animation_defintion list_disappearance;

		std::vector<std::unique_ptr<list_option>> options;
		list_open open_button;
		accessibility_object_ptr acc_obj;

		int32_t currently_selected = 0;
		uint16_t name = uint16_t(-1);
		uint16_t alt_text_id = uint16_t(-1);
		content_alignment text_alignment = content_alignment::leading;
		bool list_is_open = false;

		list_control() {
		}
		virtual ~list_control() {
		}

		virtual layout_node_type get_node_type() override {
			return layout_node_type::container;
		};
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual void on_lose_focus(window_data&) override;
		virtual layout_rect get_content_rectangle(window_data&) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;

		virtual bool is_open() const override {
			return list_is_open;
		}
		virtual void open(window_data&, bool move_focus) override;
		virtual void close(window_data&, bool move_focus) override;
		virtual layout_interface* selected_item() const override;
		void populate_list(window_data&);
		void quiet_select_option_by_value(window_data& win, size_t);
		void select_option_by_value(window_data&, size_t);
		button_control_base* get_option(size_t);

		virtual void on_select(window_data&, size_t) = 0;
		virtual list_option_description describe_option(window_data const&, uint32_t) = 0;
		virtual void on_create(window_data&) { }
	};

	struct open_list_control : public layout_interface, public generic_selection_container {
		std::vector<button_control_base*> options;
		accessibility_object_ptr acc_obj;
		int32_t left_margin = 2;
		int32_t right_margin = 2;

		uint16_t name_id = uint16_t(-1);

		open_list_control() {
		}
		virtual ~open_list_control() {
		}

		virtual layout_node_type get_node_type() override {
			return layout_node_type::container;
		};
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
		virtual layout_interface* selected_item() const override;
		void center_contents(window_data&);
	};

	struct page_header_button : public icon_button_base {
		std::function<void(window_data&, layout_reference)> close_action;
		interactable_state saved_state;

		page_header_button(uint16_t name, std::function<void(window_data&, layout_reference)>&& a) : icon_button_base(name, uint16_t(-1)), close_action(std::move(a)) {
			ico = standard_icons::header_close;
		}
		virtual ~page_header_button() {
		}
		virtual void button_action(window_data&) override;
	};

	struct page_footer_button : public button_control_base {
		uint32_t stored_page = 0;
		uint32_t stored_page_total = 0;

		page_footer_button() {
			set_text_alignment(content_alignment::centered);
			icon = standard_icons::control_pages;
		}
		virtual ~page_footer_button() {
		}

		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void button_action(window_data&);
		virtual int32_t interactable_count(window_data const&) override;
		void update_page(window_data const& win);
	};

	struct page_back_button : public icon_button_base {
		page_back_button() : icon_button_base(text_id::page_prev_name, uint16_t(-1)) {
			ico = standard_icons::control_prev;
		}
		virtual ~page_back_button() {
		}
		virtual void button_action(window_data&) override;
		virtual bool is_disabled(window_data const& win) const override;
	};
	struct page_forward_button : public icon_button_base {
		page_forward_button() : icon_button_base(text_id::page_next_name, uint16_t(-1)) {
			ico = standard_icons::control_next;
		}
		virtual ~page_forward_button() {
		}
		virtual void button_action(window_data&) override;
		virtual bool is_disabled(window_data const& win) const override;
	};
	struct page_jump_forward_button : public icon_button_base {
		page_jump_forward_button() {
			ico = standard_icons::control_next_next;
		}
		virtual ~page_jump_forward_button() {
		}
		virtual void button_action(window_data&) override;
		virtual bool is_disabled(window_data const& win) const override;
		virtual std::wstring get_name(window_data const& win) override;
		int32_t jump_size(window_data const& win) const;
	};
	struct page_jump_back_button : public icon_button_base {
		page_jump_back_button() {
			ico = standard_icons::control_prev_prev;
		}
		virtual ~page_jump_back_button() {
		}
		virtual void button_action(window_data&) override;
		virtual bool is_disabled(window_data const& win) const override;
		virtual std::wstring get_name(window_data const& win) override;
		int32_t jump_size(window_data const& win) const;
	};

	struct page_footer : public generic_expandable {
		static animation_defintion footer_appearance;
		static animation_defintion footer_disappearance;

		static animation_defintion page_turn_up;
		static animation_defintion page_turn_down;

		page_footer_button primary_display;
		page_back_button back;
		page_forward_button forward;
		page_jump_back_button jump_back;
		page_jump_forward_button jump_forward;
		accessibility_object_ptr acc_obj;

		bool footer_is_open = false;

		page_footer() { }
		virtual ~page_footer() {
		}
		
		virtual layout_node_type get_node_type() override {
			return layout_node_type::container;
		};

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual layout_rect get_content_rectangle(window_data&) override;
		virtual void on_focus(window_data&) override;
		virtual void on_lose_focus(window_data&) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;

		virtual bool is_open() const override {
			return footer_is_open;
		}
		virtual void open(window_data&, bool move_focus) override;
		virtual void close(window_data&, bool move_focus) override;

		static void start_page_turn_animation(window_data& win, layout_reference page_container, bool increasing);
	};

	struct close_info_window : public icon_button_base {
		close_info_window() {
			ico = standard_icons::header_close;
		}
		virtual ~close_info_window() {
		}
		virtual void button_action(window_data&) override;
	};

	struct info_window : public render_interface {
		struct parameters {
			layout_reference attached_to = layout_reference_none;
			int32_t left_margin = 0;
			int32_t right_margin = 0;
			int32_t width_value = 6;
			bool appear_on_left = false;
			bool reverse_fg_bg = false;
			uint16_t text_id = uint16_t(-1);
			text::text_parameter const* b = nullptr;
			text::text_parameter const* e = nullptr;

			parameters(layout_reference attached_to) : attached_to(attached_to) { }

			parameters left(bool v = true) {
				appear_on_left = v;
				return *this;
			}
			parameters right(bool v = false) {
				appear_on_left = !v;
				return *this;
			}
			parameters internal_margins(int32_t left, int32_t right) {
				left_margin = left;
				right_margin = right;
				return *this;
			}
			parameters width(int32_t o) {
				width_value = o;
				return *this;
			}
			parameters reverse_palette(bool v = true) {
				reverse_fg_bg = v;
				return *this;
			}
			parameters text(uint16_t t, text::text_parameter const* f = nullptr, text::text_parameter const* g = nullptr) {
				text_id = t; b = f; e = g;
				return *this;
			}
		};

		static animation_defintion info_appearance;
		static animation_defintion info_disappearance;

		close_info_window close_button;
		stored_text text;
		
		screen_space_rect seeking_rect{0,0,0,0};

		bool currently_visible = false;
		animation_direction appearance_direction = animation_direction::left;

		int16_t connected_to_line = 0;

		uint8_t foreground = 1;
		uint8_t background = 0;

		info_window() {
			text.text_alignment = content_alignment::leading;
			text.text_sz = text_size::note;
		}
		virtual ~info_window() { }
		virtual layout_node_type get_node_type() override {
			return layout_node_type::container;
		}
		virtual int32_t interactable_count(window_data const&) override {
			return 0;
		}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual void on_lose_focus(window_data&) override;
		virtual void on_focus(window_data&) override;

		void open(window_data& win, parameters const& params);
		void close(window_data& win, bool move_focus);
	};

	struct single_line_centered_header : public render_interface {
		page_header_button close_button;
		stored_text text;
		accessibility_object_ptr acc_obj;
		interactable_state saved_state;

		single_line_centered_header(uint16_t close_text, std::function<void(window_data&, layout_reference)>&& a) : close_button(close_text, std::move(a)) {
			text.text_alignment = content_alignment::centered;
		}
		virtual ~single_line_centered_header();

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override {
			return layout_node_type::container;
		};
		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
	};

	struct large_centered_header : public render_interface {
		page_header_button close_button;
		stored_text text;
		accessibility_object_ptr acc_obj;
		interactable_state saved_state;

		static uint8_t left_decoration;
		static uint8_t right_decoration;
		static uint8_t decoration_brush;

		large_centered_header(uint16_t close_text, std::function<void(window_data&, layout_reference)>&& a) : close_button(close_text, std::move(a)) {
			text.text_alignment = content_alignment::centered;
			text.text_sz = text_size::header;
		}
		virtual ~large_centered_header();

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override {
			return layout_node_type::container;
		};
		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
	};

	struct single_line_empty_header : public render_interface {

		interactable_state saved_state;

		single_line_empty_header() {
		}
		virtual ~single_line_empty_header() {
		}

		virtual layout_node_type get_node_type() override {
			return layout_node_type::visible;
		};
		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}

		virtual void render_foreground(ui_rectangle const&, window_data&) override {
		}
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
	};

	struct menu_control;

	struct menu_open : public button_control_base {
		menu_open() {
		}
		virtual ~menu_open() {
		}
		virtual void button_action(window_data&);
	};

	struct menu_control : public generic_expandable {
		single_line_empty_header header_space;
		page_footer menu_footer;

		static animation_defintion list_appearance;
		static animation_defintion list_disappearance;

		accessibility_object_ptr acc_obj;
		menu_open open_button;
		int32_t page_size = 1;
		int32_t line_size = 6;

		content_alignment text_alignment = content_alignment::leading;
		bool list_is_open = false;
		bool vertically_cover_parent = false;
		uint16_t alt_text = uint16_t(-1);
		uint16_t name = uint16_t(-1);

		menu_control() {
		}
		virtual ~menu_control() {
		}

		int32_t get_menu_height(window_data const&) const;

		virtual layout_node_type get_node_type() override {
			return layout_node_type::page;
		};
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;

		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual void on_lose_focus(window_data&) override;
		virtual layout_rect get_content_rectangle(window_data&) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;

		virtual void open(window_data&, bool move_focus) override;
		virtual void close(window_data&, bool move_focus) override;
		virtual bool is_open() const override {
			return list_is_open;
		}

		virtual std::vector<page_content> get_options(window_data&) = 0;
		virtual void on_open(window_data&) {
		}
		virtual void on_close(window_data&) {
		}
	};


	struct settings_menu_item {
		layout_interface* settings_contents = nullptr;
		uint16_t text = uint16_t(-1);
		uint16_t alt_text = uint16_t(-1);
	};

	struct settings_item_button : public button_control_base {
		layout_interface* settings_contents = nullptr;
		uint32_t id;

		settings_item_button(uint32_t i, layout_interface* f) : id(i), settings_contents(f) {

		}

		virtual ~settings_item_button() { }
		virtual void button_action(window_data&) override;
	};

	struct ui_scale_edit : public editable_numeric_range {
		ui_scale_edit() : editable_numeric_range(content_alignment::trailing, text_id::ui_scale_edit_name, text_id::ui_scale_info, 3, 1, 0.5f, 3.0f, 2) { }
		virtual void on_edit_finished(window_data& win, std::wstring const&) override;
	};

	struct language_button : public button_control_base {
		std::wstring lang;
		std::wstring region;

		language_button() {
			set_text_alignment(content_alignment::trailing);
		}
		virtual ~language_button() {
		}
		virtual void button_action(window_data&) override;
	};

	struct language_menu : public menu_control, public generic_selection_container {
		std::vector<std::unique_ptr<layout_interface>> lbuttons;

		virtual ~language_menu() {
		}

		virtual std::vector<page_content> get_options(window_data&);
		virtual void on_open(window_data&);
		virtual void on_close(window_data&);
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
		virtual layout_interface* selected_item() const override;
	};

	struct font_button : public button_control_base {
		std::wstring name;
		text_size text_sz;

		font_button(text_size text_sz) : text_sz(text_sz) {
			set_text_alignment(content_alignment::trailing);
		}
		virtual ~font_button() {
		}
		virtual void button_action(window_data&) override;
	};

	struct font_menu : public menu_control, public generic_selection_container {
		std::vector<std::unique_ptr<layout_interface>> lbuttons;
		text_size text_sz;

		font_menu(text_size text_sz) : text_sz(text_sz) { }
		virtual ~font_menu() { }

		virtual std::vector<page_content> get_options(window_data&);
		virtual void on_open(window_data&);
		virtual void on_close(window_data&);
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
		virtual layout_interface* selected_item() const override;
	};

	struct font_stretch_edit : public editable_numeric_range {
		text_size text_sz;

		font_stretch_edit(text_size text_sz) : editable_numeric_range(content_alignment::trailing, text_id::font_stretch_edit_name, text_id::font_stretch_info, 3, 1, 50.0f, 200.0f, 0), text_sz(text_sz) {
		}
		virtual void on_edit_finished(window_data& win, std::wstring const&) override;
	};

	struct font_weight_edit : public editable_numeric_range {
		text_size text_sz;

		font_weight_edit(text_size text_sz) : editable_numeric_range(content_alignment::trailing, text_id::font_weight_edit_name, text_id::font_weight_info, 3, 1, 100.0f, 900.0f, 0), text_sz(text_sz) {
		}
		virtual void on_edit_finished(window_data& win, std::wstring const&) override;
	};

	struct font_top_lead_edit : public editable_numeric_range {
		text_size text_sz;

		font_top_lead_edit(text_size text_sz) : editable_numeric_range(content_alignment::trailing, text_id::top_lead_edit_name, text_id::top_lead_edit_info, 3, 1, -20.0f, 20.0f, 0), text_sz(text_sz) {
		}
		virtual void on_edit_finished(window_data& win, std::wstring const&) override;
	};
	struct font_bottom_lead_edit : public editable_numeric_range {
		text_size text_sz;

		font_bottom_lead_edit(text_size text_sz) : editable_numeric_range(content_alignment::trailing, text_id::bottom_lead_edit_name, text_id::bottom_lead_edit_info, 3, 1, -20.0f, 20.0f, 0), text_sz(text_sz) {
		}
		virtual void on_edit_finished(window_data& win, std::wstring const&) override;
	};
	struct small_relative_size : public editable_numeric_range {
		small_relative_size() : editable_numeric_range(content_alignment::trailing, text_id::relative_size_edit_name, text_id::relative_size_small_info, 3, 1, 0.5f, 1.0f, 3) {
		}
		virtual void on_edit_finished(window_data& win, std::wstring const&) override;
	};
	struct header_relative_size : public editable_numeric_range {
		header_relative_size() : editable_numeric_range(content_alignment::trailing, text_id::relative_size_edit_name, text_id::relative_size_header_info, 3, 1, 1.0f, 2.0f, 3) {
		}
		virtual void on_edit_finished(window_data& win, std::wstring const&) override;
	};

	struct font_italic_toggle_button : public button_control_toggle {
		text_size text_sz;

		font_italic_toggle_button(text_size text_sz) : button_control_toggle(text_id::generic_toggle_yes, text_id::generic_toggle_no, content_alignment::trailing, text_id::font_italic_info), text_sz(text_sz) {
		}
		virtual void toggle_action(window_data&, bool toggle_state) override;
	};

	struct settings_orientation_list : public list_control {
		virtual void on_select(window_data&, size_t) override;
		virtual list_option_description describe_option(window_data const&, uint32_t) override;
	};
	struct settings_input_mode_list : public list_control {
		virtual void on_select(window_data&, size_t) override;
		virtual list_option_description describe_option(window_data const&, uint32_t) override;
		virtual void on_create(window_data&) override;
	};
	struct settings_keyboard_arragement_list : public list_control {
		virtual void on_select(window_data&, size_t) override;
		virtual list_option_description describe_option(window_data const&, uint32_t) override;
	};
	struct ui_animation_toggle_button : public button_control_toggle {
		ui_animation_toggle_button() : button_control_toggle(text_id::generic_toggle_on, text_id::generic_toggle_off, content_alignment::trailing, text_id::ui_animations_info) {
		}
		virtual void toggle_action(window_data&, bool toggle_state) override;
	};

	struct key_code_button : public button_control_base {
		int32_t key = 0;

		key_code_button(int32_t key) : key(key), button_control_base(uint16_t(-1), content_alignment::trailing, text_id::keyboard_code_info) { }
		virtual void button_action(window_data&) override;
	};

	struct key_name_edit : public simple_editable_text {
		int32_t key = 0;

		key_name_edit(int32_t key) : key(key), simple_editable_text(content_alignment::trailing, text_id::keyboard_display_edit_name, text_id::keyboard_display_name_info, 6, 1) {
		}

		virtual void on_edit_finished(window_data&, std::wstring const&) override;
	};

	struct common_printui_settings : public layout_interface {
		single_line_empty_header header;
		page_footer footer;

		label_control language_label;
		language_menu lang_menu;

		label_control orientation_label;
		settings_orientation_list orientation_list;

		label_control input_mode_label;
		settings_input_mode_list input_mode_list;

		label_control toggle_animations_label;
		ui_animation_toggle_button toggle_animations;

		label_control ui_scale_label;
		ui_scale_edit ui_scale_e;
		
		label_control fonts_header;

		//primary font
		label_control primary_font_name_label;
		font_menu primary_font_menu;
		label_control primary_font_weight_label;
		font_weight_edit primary_font_weight_e;
		label_control primary_font_stretch_label;
		font_stretch_edit primary_font_stretch_e;
		label_control primary_font_italic_label;
		font_italic_toggle_button primary_font_italic_toggle;
		label_control primary_top_lead_label;
		font_top_lead_edit primary_top_lead_e;
		label_control primary_bottom_lead_label;
		font_bottom_lead_edit primary_bottom_lead_e;

		//small font
		label_control small_font_name_label;
		font_menu small_font_menu;
		label_control small_relative_label;
		small_relative_size small_relative_e;
		label_control small_font_weight_label;
		font_weight_edit small_font_weight_e;
		label_control small_font_stretch_label;
		font_stretch_edit small_font_stretch_e;
		label_control small_font_italic_label;
		font_italic_toggle_button small_font_italic_toggle;
		label_control small_top_lead_label;
		font_top_lead_edit small_top_lead_e;
		label_control small_bottom_lead_label;
		font_bottom_lead_edit small_bottom_lead_e;

		//header font
		label_control header_font_name_label;
		font_menu header_font_menu;
		label_control header_relative_label;
		header_relative_size header_relative_e;
		label_control header_font_weight_label;
		font_weight_edit header_font_weight_e;
		label_control header_font_stretch_label;
		font_stretch_edit header_font_stretch_e;
		label_control header_font_italic_label;
		font_italic_toggle_button header_font_italic_toggle;
		label_control header_top_lead_label;
		font_top_lead_edit header_top_lead_e;
		label_control header_bottom_lead_label;
		font_bottom_lead_edit header_bottom_lead_e;

		//keyboard settings
		label_control keyboard_header;

		label_control kb_arrangement_label;
		settings_keyboard_arragement_list kb_arrangement_list;

		label_control key1_label;
		label_control key1_code_label;
		key_code_button key1_code_button;
		label_control key1_name_label;
		key_name_edit key1_edit;

		label_control key2_label;
		label_control key2_code_label;
		key_code_button key2_code_button;
		label_control key2_name_label;
		key_name_edit key2_edit;

		label_control key3_label;
		label_control key3_code_label;
		key_code_button key3_code_button;
		label_control key3_name_label;
		key_name_edit key3_edit;
		
		label_control key4_label;
		label_control key4_code_label;
		key_code_button key4_code_button;
		label_control key4_name_label;
		key_name_edit key4_edit;

		label_control key5_label;
		label_control key5_code_label;
		key_code_button key5_code_button;
		label_control key5_name_label;
		key_name_edit key5_edit;

		label_control key6_label;
		label_control key6_code_label;
		key_code_button key6_code_button;
		label_control key6_name_label;
		key_name_edit key6_edit;

		label_control key7_label;
		label_control key7_code_label;
		key_code_button key7_code_button;
		label_control key7_name_label;
		key_name_edit key7_edit;

		label_control key8_label;
		label_control key8_code_label;
		key_code_button key8_code_button;
		label_control key8_name_label;
		key_name_edit key8_edit;

		label_control key9_label;
		label_control key9_code_label;
		key_code_button key9_code_button;
		label_control key9_name_label;
		key_name_edit key9_edit;

		label_control key10_label;
		label_control key10_code_label;
		key_code_button key10_code_button;
		label_control key10_name_label;
		key_name_edit key10_edit;

		label_control key11_label;
		label_control key11_code_label;
		key_code_button key11_code_button;
		label_control key11_name_label;
		key_name_edit key11_edit;

		label_control key12_label;
		label_control key12_code_label;
		key_code_button key12_code_button;
		label_control key12_name_label;
		key_name_edit key12_edit;

		accessibility_object_ptr acc_obj;

		std::vector<page_content> content_description;

		common_printui_settings();
		virtual ~common_printui_settings() { }

		virtual layout_node_type get_node_type() override {
			return layout_node_type::page;
		};

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
		virtual void go_to_page(window_data&, uint32_t i, page_information& pi) override;

		void update_with_keyboard_settings(window_data& win);
	};

	struct settings_page_container : public layout_interface {
		std::vector<std::unique_ptr<settings_item_button>> settings_items;

		large_centered_header page_header;
		int32_t settings_item_selected = 0;
		open_list_control subpage_selection_list;
		uint8_t section_bottom_decorations = uint8_t(-1);
		uint8_t section_separation_decorations = uint8_t(-1);

		accessibility_object_ptr acc_obj;

		settings_page_container(window_data& win, std::vector<settings_menu_item> const& setting_items);
		virtual ~settings_page_container() { }

		virtual layout_node_type get_node_type() override {
			return layout_node_type::page;
		};
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data&) override;

		virtual void recreate_contents(window_data&, layout_node&) override;

		virtual void go_to_page(window_data&, uint32_t i, page_information& pi) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
	};

	struct window_bar_element : public layout_interface {
		std::optional<vertical_2x2_min_icon> min_i;
		std::optional<vertical_2x2_max_icon> max_i;
		std::optional<vertical_2x2_settings_icon> setting_i;
		vertical_2x2_info_icon info_i;
		vertical_2x2_close_icon close_i;
		
		settings_page_container settings_pages;
		common_printui_settings print_ui_settings;
		bool expanded_show_settings = false;

		accessibility_object_ptr acc_obj;

		window_bar_element(window_data& win, bool mn, bool mx, bool settings, std::vector<settings_menu_item> const& setting_items);

		virtual ~window_bar_element() {}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_rect get_content_rectangle(window_data& win) override;
		virtual layout_node_type get_node_type() override;
		virtual simple_layout_specification get_specification(window_data&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual accessibility_object* get_accessibility_interface(window_data&) override;
	};


	namespace render {
		screen_space_rect extend_rect_to_edges(screen_space_rect content_rect, window_data const& win);
	}

	void default_recreate_page(window_data& win, layout_interface* l_interface, page_layout_specification const& spec);

	struct focus_tracker {
		layout_reference node = layout_reference_none;
		int32_t child_offset = 0;
		int32_t child_offset_end = 0;
	};
	struct stored_focus {
		layout_interface* l_interface = nullptr;
		int32_t child_offset = 0;
		int32_t child_offset_end = 0;
	};
	struct interactable {
		render_interface* ptr = nullptr;
		int32_t child_offset = 0;
	};
	struct move_focus_to_node {
		layout_reference to_node = layout_reference_none;
	};
	struct grouping_range {
		int32_t start;
		int32_t end;
	};
	struct go_up {
	};

	using key_action = std::variant<std::monostate, focus_tracker, move_focus_to_node, interactable, go_up>;

	struct key_actions {
		key_action escape;
		std::array<key_action, 12> button_actions;
		int32_t valid_key_action_count = 0;
	};

	namespace text {
		

		class text_manager;
		struct text_data_storage;

		using attribute_type = int8_t;


		// extra display formatting to apply to a range of characters
		

		int32_t get_height(window_data const& win, arranged_text* txt, text_size sz);
		int32_t get_width(window_data const& win, arranged_text* txt);
		bool appropriate_directionality(window_data const& win, arranged_text* txt);
		bool adjust_layout_region(arranged_text* txt, int32_t width, int32_t height);
		std::vector<text_metrics> get_metrics_for_range(arranged_text* txt, uint32_t position, uint32_t length);
		text_metrics get_metrics_at_position(arranged_text* txt, uint32_t position);
		hit_test_metrics hit_test_text(arranged_text* txt, int32_t x, int32_t y);

		struct replaceable_instance;
		struct static_text_with_formatting;

		// contains a string plus indices where formatting must be applied
		struct text_with_formatting {
			std::wstring text;
			std::vector<format_marker> formatting;

			text_with_formatting& operator+=(text_with_formatting const& other);
			void append(text_data_storage const& tm, static_text_with_formatting const& other);
			void make_substitutions(replaceable_instance const* start, replaceable_instance const* end);
		};

		// as above, but formatting and text itself are fixed after load
		struct static_text_with_formatting {
			uint32_t code_points_start;
			uint16_t code_points_count;
			uint16_t formatting_start;
			uint16_t formatting_end;
		};

		// result of calling a text function, holds attributes
		struct replaceable_instance {
			static constexpr uint32_t max_attributes = 8;

			text_with_formatting text_content;
			std::array<attribute_type, max_attributes> attributes = { -1i8, -1i8, -1i8, -1i8, -1i8, -1i8, -1i8, -1i8 };
		};

		struct param_attribute_pair {
			attribute_type attribute = -1i8;
			uint8_t parameter_id = 0;
		};

		struct matched_pattern {
			static_text_with_formatting base_text;
			uint16_t match_key_start = 0;
			uint8_t num_keys = 0;
			uint8_t group = 0;

			bool matches_parameters(text_data_storage const& tm, replaceable_instance const* param_start, replaceable_instance const* param_end) const;
		};


		// takes parameters -> fn or pattern match -> replaceable instance
		struct text_function {
			uint16_t begin_patterns = 0;
			uint16_t pattern_count = 0;

			// attributes assigend to results
			std::array<attribute_type, replaceable_instance::max_attributes> attributes = { -1i8, -1i8, -1i8, -1i8, -1i8, -1i8, -1i8, -1i8 };

			replaceable_instance instantiate(text_data_storage const& tm, replaceable_instance const* start, replaceable_instance const* end) const;
		};


		inline constexpr attribute_type zero = 0;
		inline constexpr attribute_type one = 1;
		inline constexpr attribute_type two = 2;
		inline constexpr attribute_type few = 3;
		inline constexpr attribute_type many = 4;
		inline constexpr attribute_type other = 5;

		inline constexpr attribute_type ord_zero = 6;
		inline constexpr attribute_type ord_one = 7;
		inline constexpr attribute_type ord_two = 8;
		inline constexpr attribute_type ord_few = 9;
		inline constexpr attribute_type ord_many = 10;
		inline constexpr attribute_type ord_other = 11;

		inline constexpr attribute_type last_predefined = 11;


		using cardinal_plural_fn = attribute_type(*)(int64_t, int64_t, int32_t);
		using ordinal_plural_fn = attribute_type(*)(int64_t);

		struct text_data_storage {
			std::wstring codepoint_storage;
			std::vector<format_marker> static_format_storage;
			std::vector<matched_pattern> static_matcher_storage;
			std::vector<param_attribute_pair> match_keys_storage;
			std::vector<text_function> stored_functions;

			ankerl::unordered_dense::map<std::string, uint16_t> internal_text_name_map;
			std::unordered_map<std::string, int8_t, string_hash, std::equal_to<>> attribute_name_to_id;
			int8_t last_attribute_id_mapped = last_predefined;

			std::string_view parse_match_conditions(std::string_view in);
			std::string_view assemble_entry_content(std::string_view body, std::unordered_map<std::string, uint32_t, string_hash, std::equal_to<>> const& font_name_to_index, bool allow_match);
			std::array<attribute_type, replaceable_instance::max_attributes> parse_attributes(std::string_view in);
			std::string_view consume_single_entry(std::string_view body, std::unordered_map<std::string, uint32_t, string_hash, std::equal_to<>> const& font_name_to_index);
			void consume_text_file(std::string_view body, std::unordered_map<std::string, uint32_t, string_hash, std::equal_to<>> const& font_name_to_index);
		};

		void impl_update_analyzed_text(text_analysis_object* ptr, arranged_text* txt, std::wstring const& str, bool ltr, text_manager const& tm);
		void update_analyzed_text(text_analysis_object* ptr, arranged_text* txt, std::wstring const& str, bool ltr, text_manager const& tm);
		int32_t left_visual_cursor_position(text_analysis_object* ptr, int32_t position, std::wstring const& str, bool ltr, text_manager const& tm);
		int32_t right_visual_cursor_position(text_analysis_object* ptr, int32_t position, std::wstring const& str, bool ltr, text_manager const& tm);

		class text_manager {
		private:
			text_data_storage text_data;

			std::wstring os_locale;
			std::wstring app_lang;
			std::wstring app_region;
			bool os_locale_is_default = true;

			// for filling in number format
			uint32_t  LeadingZero;
			uint32_t  Grouping;
			wchar_t lpDecimalSep[5];
			wchar_t lpThousandSep[5];
			uint32_t  NegativeOrder;
			uint32_t lcid;


			std::unordered_map<std::wstring_view, cardinal_plural_fn> cardinal_functions;
			std::unordered_map<std::wstring_view, ordinal_plural_fn> ordinal_functions;

			cardinal_plural_fn cardinal_classification = nullptr;
			ordinal_plural_fn ordinal_classification = nullptr;

			void load_text_from_directory(window_data const& win, std::wstring const& directory);
			replaceable_instance parameter_to_text(text_parameter p) const;
		public:
			uint8_t text_generation = 0;

			void update_with_new_locale(window_data& win, bool update_settings);
			void populate_text_content(window_data const& win);
			void register_name(std::string_view n, uint16_t id);

			std::unordered_map<std::string, uint32_t, string_hash, std::equal_to<>> font_name_to_index;

			text_manager();

			text_id text_id_from_name(std::string_view name) const;

			friend matched_pattern;
			friend text_function;
			friend text_with_formatting;

			void change_locale(std::wstring const& lang, std::wstring const& region, window_data& win, bool update_settings);
			void default_locale(window_data& win, bool update_settings);
			void update_fonts(window_data& win);

			double text_to_double(wchar_t const* start, uint32_t count) const;
			int64_t text_to_int(wchar_t const* start, uint32_t count) const;
			replaceable_instance format_int(int64_t value, uint32_t decimal_places) const;
			replaceable_instance format_double(double value, uint32_t decimal_places) const;

			wchar_t const* locale_string() const;
			std::wstring locale_name() const;
			std::wstring locale_display_name(window_data const& win) const;
			bool is_locale_default() const;
			bool is_current_locale(std::wstring const& lang, std::wstring const& region) const;

			replaceable_instance instantiate_text(uint16_t id, text_parameter const* s = nullptr, text_parameter const* e = nullptr) const;
			replaceable_instance instantiate_text(std::string_view key, text_parameter const* s = nullptr, text_parameter const* e = nullptr) const;

			friend void impl_update_analyzed_text(text_analysis_object* ptr, arranged_text* txt, std::wstring const& str, bool ltr, text_manager const& tm);
			friend int32_t left_visual_cursor_position(text_analysis_object* ptr, int32_t position, std::wstring const& str, bool ltr, text_manager const& tm);
			friend int32_t right_visual_cursor_position(text_analysis_object* ptr, int32_t position, std::wstring const& str, bool ltr, text_manager const& tm);
		};

		struct language_description {
			std::wstring language;
			std::wstring region;
			std::wstring display_name;
		};
		std::vector<language_description> ennumerate_languages(window_data const& win);

		struct surrogate_pair {
			uint16_t high = 0; // aka leading
			uint16_t low = 0; // aka trailing
		};

		uint32_t assemble_codepoint(uint16_t high, uint16_t low) noexcept;
		surrogate_pair make_surrogate_pair(uint32_t val) noexcept;
		bool is_low_surrogate(uint16_t char_code) noexcept;
		bool is_high_surrogate(uint16_t char_code) noexcept;
		int32_t num_logical_chars_in_range(std::wstring_view str);
		bool cursor_ignorable16(uint16_t at_position, uint16_t trailing);
		bool is_space(uint32_t c) noexcept;

		text_analysis_object* make_analysis_object();
		
		int32_t number_of_cursor_positions_in_range(text_analysis_object* ptr, int32_t start, int32_t count);
		int32_t get_previous_cursor_position(text_analysis_object* ptr, int32_t position);
		int32_t get_next_cursor_position(text_analysis_object* ptr, int32_t position);
		int32_t get_previous_word_position(text_analysis_object* ptr, int32_t position);
		int32_t get_next_word_position(text_analysis_object* ptr, int32_t position);
		bool is_cursor_position(text_analysis_object* ptr, int32_t position);
		bool is_word_position(text_analysis_object* ptr, int32_t position);
		bool position_is_ltr(text_analysis_object* ptr, int32_t position);
		int32_t left_visual_word_position(text_analysis_object* ptr, int32_t position);
		int32_t right_visual_word_position(text_analysis_object* ptr, int32_t position);
		int32_t number_of_lines(text_analysis_object* ptr);
		int32_t line_of_position(text_analysis_object* ptr, int32_t position);
		int32_t start_of_line(text_analysis_object* ptr, int32_t line);
		int32_t end_of_line(text_analysis_object* ptr, int32_t line);
	}
	
	struct undo_item {
		edit_interface* from = nullptr;
		std::wstring contents;
		int32_t anchor = 0;
		int32_t cursor = 0;

		bool operator==(undo_item const& o) const noexcept {
			return from == o.from && contents == o.contents;
		}
		bool operator!=(undo_item const& o) const noexcept {
			return !(*this == o);
		}
	};

	struct undo_buffer {
		constexpr static int32_t total_size = 16;

		undo_item interal_buffer[total_size] = {};
		int32_t buffer_position = 0;

		std::optional<undo_item> undo(undo_item current_state);
		std::optional<undo_item> redo(undo_item current_state);
		void push_state(undo_item state);
	};

	
	struct window_data {
	private:
		layout_node_storage layout_nodes;
		std::vector<ui_rectangle> prepared_layout;
		std::wstring window_title;

		layout_interface* top_node = nullptr;
		layout_reference top_node_id = layout_reference_none;
		layout_interface* bottom_node = nullptr;
		layout_reference bottom_node_id = layout_reference_none;
		layout_interface* left_node = nullptr;
		layout_reference left_node_id = layout_reference_none;
		layout_interface* right_node = nullptr;
		layout_reference right_node_id = layout_reference_none;

		bool has_window_title = false;
		bool layout_out_of_date = true;
		bool ui_rects_out_of_date = false;
		

		void repopulate_ui_rects(layout_reference n, layout_position base, uint8_t parent_foreground, uint8_t parent_background, bool highlight_line = false, bool skip_bg = false);
		void repopulate_ui_rects();
		std::vector<layout_reference> size_children(layout_interface* l_interface, int32_t max_width, int32_t max_height);
		void internal_recreate_layout();

		void update_generation(layout_reference id);
		void run_garbage_collector();

		void propogate_layout_change_upwards(layout_reference id);

	public:
		standard_icons common_icons;
		text::text_manager text_data;

		launch_settings dynamic_settings;

		os_win32_wrapper window_interface;
		win32_accessibility accessibility_interface;
		text::win32_text_services text_services_interface;
		win32_file_system file_system;
		text::direct_write_text text_interface;
		render::direct2d_rendering rendering_interface;

		uint32_t ui_width = 0;
		uint32_t ui_height = 0;

		uint32_t minimum_ui_width = 10;
		uint32_t minimum_ui_height = 10;

		uint32_t layout_width = 1; // in layout units
		uint32_t layout_height = 1;

		float dpi = 96.0f;

		int32_t layout_size = 28;
		int32_t window_border = 2;
		int32_t window_saved_border = 0;

		layout_orientation orientation = layout_orientation::horizontal_left_to_right;

		ui_reference last_under_cursor = ui_reference_none;
		layout_interface* last_layout_under_cursor = nullptr;

		uint32_t last_cursor_x_position = 0;
		uint32_t last_cursor_y_position = 0;

		edit_interface* keyboard_target = nullptr;
		undo_buffer edit_undo_buffer;
		edit_selection_mode selecting_edit_text = edit_selection_mode::none;
		int32_t double_click_ms = 5;
		int32_t caret_blink_ms = 1024;
		decltype(std::chrono::steady_clock::now()) last_double_click;

		struct {
			uint32_t val = 0;
			constexpr static uint32_t button_y = 0x001;
			constexpr static uint32_t button_x = 0x002;
			constexpr static uint32_t button_b = 0x004;
			constexpr static uint32_t button_a = 0x008;
			constexpr static uint32_t button_lb = 0x010;
			constexpr static uint32_t button_rb = 0x020;
			constexpr static uint32_t button_start = 0x040;
			constexpr static uint32_t button_back = 0x080;
		} controller_buttons;
		struct {
			decltype(std::chrono::steady_clock::now()) start_time;
			bool running = false;
			double current_x = 0.0;
			double current_y = 0.0;
			int32_t for_controller = 0;
		} controller_mouse;
		uint32_t controller_number_plugged_in = 0;

		std::vector<stored_focus> focus_stack;
		std::array<grouping_range, 12> current_focus_groupings;
		int32_t current_groupings_size = 0;
		key_actions focus_actions;

		prompt_mode prompts = prompt_mode::keyboard;
		bool display_interactable_type = true;
		
		uint8_t secondary_escape_sc = 0x01ui8; // esc

		bool is_sizeing = false;
		bool pending_right_click = false;

		window_data(bool mn, bool mx, bool settings);
		~window_data();

		void load_default_dynamic_settings();
		void client_on_dpi_change();
		void client_on_resize(uint32_t width, uint32_t height);
		std::vector<printui::settings_menu_item> get_settings_items() const;
		void register_icons();

		// called directly on message reciept

		bool on_mouse_move(uint32_t x, uint32_t y);
		bool on_key_down(uint32_t scan_code, uint32_t key_code);
		bool on_key_up(uint32_t scan_code, uint32_t key_code);
		bool on_mouse_left_down(uint32_t x, uint32_t y);
		bool on_mouse_right_down(uint32_t x, uint32_t y);
		bool on_resize(resize_type type, uint32_t width, uint32_t height);
		void on_controller_input();
		void on_device_change(uint64_t status, void* handle);
		bool on_mouse_wheel(int32_t quantity);

		// OTHER

		void message_loop();

		void create_window();
		void on_dpi_change();

		void show_settings_panel();
		void hide_settings_panel();

		void expand_to_fit_content();

		void remove_references_to(layout_interface* l);
		bool is_title_set() const {
			return has_window_title;
		}
		// layout functions

		void recreate_contents(layout_interface* l_interface, layout_node* node);
		void immediate_resize(layout_node& node, int32_t new_width, int32_t new_height);
		void recreate_container_contents(layout_interface* l_interface, layout_node* node);
		void recreate_simple_container_contents(layout_interface* l_interface, layout_node* node);
		void recreate_page_contents(layout_interface* l_interface, layout_node* node);

		window_bar_element window_bar;
		title_bar_element title_bar;
		info_window info_popup;

		layout_reference create_node(layout_interface* l_interface, int32_t max_width, int32_t max_height, bool force_create_children, bool force_create = false);

		uint32_t content_window_x = 0;
		uint32_t content_window_y = 0;

		void change_orientation(layout_orientation o);
		void change_size_multiplier(float v);
		void recreate_layout();
		void resize_item(layout_reference id, int32_t new_width, int32_t new_height);
		void set_window_title(std::wstring const& title);
		wchar_t const* get_window_title() const;

		void release_all() {
			prepared_layout.clear();
			layout_nodes.reset();
		}
		void clear_prepared_layout();
		void reset_layout();

		void set_top_node(layout_interface* l_interface);
		void set_bottom_node(layout_interface* l_interface);
		void set_left_node(layout_interface* l_interface);
		void set_right_node(layout_interface* l_interface);

		layout_reference get_top_node() {
			return top_node_id;
		}
		layout_reference get_bottom_node() {
			return bottom_node_id;
		}
		layout_reference get_left_node() {
			return left_node_id;
		}
		layout_reference get_right_node() {
			return right_node_id;
		}

		std::vector<ui_rectangle>& get_layout();
		std::vector<ui_rectangle>& get_ui_rects() {
			return prepared_layout;
		}
		std::vector<ui_rectangle> const& get_ui_rects() const {
			return prepared_layout;
		}
		bool visible_window_title() {
			return has_window_title;
		}
		layout_reference find_common_root(layout_reference a, layout_reference b) const;
		void change_focus(layout_reference old_focus, layout_reference new_focus);
		layout_reference get_minimimal_visible(layout_reference leaf) const;
		bool is_child_of(layout_reference parent, layout_reference child) const;
		layout_reference get_containing_page(layout_reference r) const;
		layout_reference get_containing_proper_page(layout_reference r) const;
		layout_reference get_containing_page_or_container(layout_reference r) const;
		layout_reference get_enclosing_node(layout_reference r);
		bool is_rendered(layout_reference r) const;
		int32_t get_containing_page_number(layout_reference page_id, page_information& pi, layout_reference c);
		animation_direction get_default_animation_direction(layout_reference id) const;

		layout_node const& get_node(layout_reference r) const {
			return layout_nodes.get_node(r);
		}
		layout_node& get_node(layout_reference r) {
			return layout_nodes.get_node(r);
		}
		layout_reference allocate_node() {
			return layout_nodes.allocate_node();
		}
		void release_node(layout_reference r) {
			layout_nodes.release_node(r);
		}
		void redraw_ui();

		bool is_visible(layout_reference r) const;

		void switch_input_mode(input_mode new_mode);
		void set_prompt_visibility(prompt_mode p);
		void update_window_focus();
		void set_window_focus(layout_interface* r);
		void set_window_focus(focus_tracker r);
		void set_window_focus_from_mouse(layout_reference new_lr);
		void back_focus_out_of(layout_interface* l);
		void execute_focus_action(key_action a);
		render_interface* get_interactable_render_holder(layout_reference r) const;
		std::optional<int32_t> interactables_at_node_over_threshold(layout_node const& n, int32_t threshold_count) const;

		int32_t get_item_offset(layout_node& n, layout_reference target) const;
		focus_tracker get_parent_group_or_node(focus_tracker r) const;

		void repopulate_key_actions();
		void repopulate_interactable_statuses();
		void remove_interactable_statuses();

		void flag_for_update_from_interface(render_interface const* i);
		void flag_for_update_from_layout_reference(layout_reference i);

		screen_space_rect get_current_location(layout_reference r) const;
		screen_space_rect get_layout_rect_in_current_location(layout_rect const& rect, layout_reference r) const;

		void safely_clear_vector(std::vector<std::unique_ptr<layout_interface>>& v);
		void safely_release_interface(layout_interface* v);

		accessibility_object* get_parent_accessibility_object(layout_reference r);
		accessibility_object* get_previous_sibling_accessibility_object(layout_reference r);
		accessibility_object* get_next_sibling_accessibility_object(layout_reference r);
		accessibility_object* get_first_child_accessibility_object(layout_reference r);
		accessibility_object* get_last_child_accessibility_object(layout_reference r);

		void immediate_add_child(layout_reference parent, layout_reference child);

		void set_keyboard_focus(edit_interface* i);

		void load_locale_settings(std::wstring const& directory);
		void load_locale_fonts(std::wstring const& directory);
	};

	
	ui_rectangle const* interface_under_point(window_data const& win, std::vector<ui_rectangle> const& rects, int32_t x, int32_t y, bool ignore_overlay);
	ui_reference reference_under_point(window_data const& win, std::vector<ui_rectangle> const& rects, int32_t x, int32_t y, bool ignore_overlay);
	layout_reference layout_reference_under_point(window_data const& win, std::vector<ui_rectangle> const& rects, int32_t x, int32_t y, bool ignore_overlay);


	int32_t reading_direction_from_orientation(layout_orientation o);
	int32_t flow_direction_from_orientation(layout_orientation o);
	inline bool horizontal(layout_orientation o) {
		return o == layout_orientation::horizontal_left_to_right || o == layout_orientation::horizontal_right_to_left;
	}
	uint32_t content_alignment_to_text_alignment(content_alignment align);
	void populate_key_mappings_by_type(key_mappings& k);
	
}

#endif PRINTUI_MAIN_HEADER
