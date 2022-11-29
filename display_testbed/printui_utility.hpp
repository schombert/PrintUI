#pragma once
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
#include "unordered_dense.h"

struct IDWriteFontFallback;
struct IDWriteFontSetBuilder2;
struct IDWriteFontFallbackBuilder;
struct IDWriteFontCollection1;
struct IDWriteFactory6;
struct IDWriteFont;
struct IDWriteTextLayout;
struct IDWriteTextFormat3;
struct ID2D1DeviceContext5;
struct ID2D1SolidColorBrush;
struct ID2D1StrokeStyle;
struct ID2D1Brush;
struct ID2D1Bitmap;
struct ID2D1Bitmap1;
struct ID2D1Factory6;
struct IDWriteFontCollection2;
struct IWICImagingFactory;
struct ID3D11Device;
struct IDXGIDevice1;
struct ID2D1Device5;
struct ID3D11DeviceContext;
struct IDXGISwapChain1;
struct IDWriteTypography;
struct IDXGIFactory2;
struct D2D_RECT_F;
struct DXGI_SWAP_CHAIN_DESC1;
struct IUnknown;

// for text ids common to the shared prinui controls
namespace text_id {
	constexpr uint16_t ui_settings_name = 0;
	constexpr uint16_t settings_header = 1;
	constexpr uint16_t orientation_label = 2;
	constexpr uint16_t orientation_ltr = 3;
	constexpr uint16_t orientation_rtl = 4;
	constexpr uint16_t orientation_vltr = 5;
	constexpr uint16_t orientation_vrtl = 6;
	constexpr uint16_t input_mode_label = 7;
	constexpr uint16_t input_mode_keyboard_only = 8;
	constexpr uint16_t input_mode_mouse_only = 9;
	constexpr uint16_t input_mode_controller_only = 10;
	constexpr uint16_t input_mode_controller_with_pointer = 11;
	constexpr uint16_t input_mode_mouse_and_keyboard = 12;
	constexpr uint16_t input_mode_follow_input = 13;
	constexpr uint16_t page_fraction = 14;
	constexpr uint16_t language_label = 15;

	constexpr uint16_t minimize_info = 16;
	constexpr uint16_t maximize_info = 17;
	constexpr uint16_t restore_info = 18;
	constexpr uint16_t settings_info = 19;
	constexpr uint16_t info_info = 20;
	constexpr uint16_t close_info = 21;

	constexpr uint16_t orientation_info = 22;
	constexpr uint16_t input_mode_info = 23;
	constexpr uint16_t input_mode_mouse_info = 24;
	constexpr uint16_t input_mode_automatic_info = 25;
	constexpr uint16_t input_mode_controller_info = 26;
	constexpr uint16_t input_mode_controller_hybrid_info = 27;
	constexpr uint16_t input_mode_keyboard_info = 28;
	constexpr uint16_t input_mode_mk_hybrid_info = 29;
	constexpr uint16_t language_info = 30;
	constexpr uint16_t ui_settings_info = 31;

	constexpr uint16_t first_free_id = 32;
}

namespace printui {
	enum class font_type {
		roman, sans, script, italic
	};
	enum class font_span {
		normal, condensed, wide
	};
	inline float to_font_weight(bool is_bold) {
		if(is_bold)
			return 700.0f;
		else
			return 400.0f;
	}
	inline float to_font_style(bool is_italic) {
		if(is_italic)
			return 1.0f;
		else
			return 0.0f;
	}
	inline float to_font_span(font_span i) {
		switch(i) {
			case font_span::normal: return 100.0f;
			case font_span::condensed: return 75.0f;
			case font_span::wide: return 125.0f;
			default: return 100.0f;
		}
	}

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

	struct font_description {
		std::wstring name;
		font_type type = font_type::roman;
		font_span span = font_span::normal;
		bool is_bold = false;
		bool is_oblique = false;
		int32_t top_leading = 0;
		int32_t bottom_leading = 0;

		float line_spacing = 0.0f;
		float baseline = 0.0f;
		float font_size = 0.0f;
		float vertical_baseline = 0.0f;
		float descender = 0.0f;
	};

	void parse_font_fallbacks_file(std::vector<printui::font_fallback>& result, char const* start, char const* end);

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

	enum class layout_orientation : uint8_t {
		horizontal_left_to_right, horizontal_right_to_left, vertical_left_to_right, vertical_right_to_left
	};

	struct launch_settings {
		font_description primary_font;
		font_description small_font;
		std::vector<font_description> named_fonts;
		std::vector<brush> brushes;
		float layout_base_size = 20.0f;
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
		layout_orientation preferred_orientation = layout_orientation::horizontal_left_to_right;
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
		void locale_settings_file(launch_settings& ls, std::unordered_map<std::string, uint32_t, text::string_hash, std::equal_to<>>& font_name_to_index, char const* start, char const* end);
	}

	struct window_data;

	template<class Interface>
	inline void safe_release(Interface*& i) {
		if(i) {
			i->Release();
			i = nullptr;
		}
	}

	template<typename I>
	class iunk_ptr {
		I* ptr = nullptr;
	public:
		iunk_ptr() noexcept { }
		iunk_ptr(I* ptr) noexcept : ptr(ptr) { }
		iunk_ptr(iunk_ptr<I> const& o) noexcept {
			ptr = o.ptr;
			if(ptr)
				ptr->AddRef();
		}
		iunk_ptr(iunk_ptr<I>&& o) noexcept {
			ptr = o.ptr;
			o.ptr = nullptr;
		}
		~iunk_ptr() {
			if(ptr)
				ptr->Release();
			ptr = nullptr;
		}
		iunk_ptr<I>& operator=(iunk_ptr<I> const& o) noexcept {
			if(ptr)
				ptr->Release();
			ptr = o.ptr;
			if(ptr)
				ptr->AddRef();
			return *this;
		}
		iunk_ptr<I>& operator=(iunk_ptr<I>&& o) noexcept {
			if(ptr)
				ptr->Release();
			ptr = o.ptr;
			o.ptr = nullptr;
			return *this;
		}
		I* operator->() const noexcept {
			return ptr;
		}
		operator bool() const noexcept {
			return ptr != nullptr;
		}
		operator I* () const noexcept {
			return ptr;
		}
		template<typename T>
		[[nodiscard]] iunk_ptr<T> query_interface() const noexcept {
			T* temp = nullptr;
			ptr->QueryInterface(__uuidof(T), reinterpret_cast<void**>(&temp));
			return temp;
		}
	};
	template<typename I, typename ... P>
	[[nodiscard]] iunk_ptr<I> make_iunk(P&&...p) {
		return iunk_ptr<I>(new I(std::forward<P>(p)...));
	}

	struct ui_rectangle;

	screen_space_rect screen_rectangle_from_layout(window_data const& win,
		int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height);
	screen_space_point screen_point_from_layout(layout_orientation o,
		int32_t x_pos, int32_t y_pos, ui_rectangle const& in_rect);
	screen_space_point screen_topleft_from_layout_in_ui(window_data const& win, int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height, ui_rectangle const& rect);
	screen_space_rect screen_rectangle_from_layout_in_ui(window_data const& win,
		int32_t line_position, int32_t page_position, int32_t line_width, int32_t page_height,
		ui_rectangle const& rect);
	screen_space_rect reverse_screen_space_orientation(window_data const& win, screen_space_rect source);
	screen_space_rect intersection(screen_space_rect a, screen_space_rect b);
	struct icon {
		std::wstring file_name;
		ID2D1Bitmap1* rendered_layer = nullptr;

		float edge_padding = 0.0f;

		int8_t xsize = 1;
		int8_t ysize = 1;

		~icon();
		void redraw_image(window_data const& win);
		void present_image(float x, float y, ID2D1DeviceContext5* context, ID2D1SolidColorBrush* dummy_brush);
	};

	struct standard_icons {
		constexpr static uint8_t header_minimize = 0;
		constexpr static uint8_t header_close = 1;
		constexpr static uint8_t control_button = 2;
		constexpr static uint8_t control_menu = 3;
		constexpr static uint8_t control_pages = 4;
		constexpr static uint8_t control_list = 5;
		constexpr static uint8_t control_next = 6;
		constexpr static uint8_t control_next_next = 7;
		constexpr static uint8_t control_prev = 8;
		constexpr static uint8_t control_prev_prev = 9;

		constexpr static uint32_t final_icon = 10;

		std::array<icon, final_icon> icons;

		standard_icons();
		void redraw_icons(window_data&);
	};

	void load_launch_settings(launch_settings& ls, std::unordered_map<std::string, uint32_t, text::string_hash, std::equal_to<>>& font_name_to_index);
	void load_locale_settings(std::wstring const& directory, launch_settings& ls, std::unordered_map<std::string, uint32_t, text::string_hash, std::equal_to<>>& font_name_to_index);

	std::optional<std::wstring> get_path(std::wstring const& file_name, wchar_t const* relative_path, wchar_t const* appdata_path);

	enum class size_flags : uint8_t {
		none, fill_to_max, match_content, fill_to_max_single_col
	};
	enum class content_orientation : uint8_t {
		line, page
	};
	enum class content_alignment : uint8_t {
		leading, trailing, centered, justified
	};
	enum class interactable_type : uint8_t {
		none, button
	};

	struct layout_interface;
	struct render_interface;

	struct page_layout_specification {
		layout_interface* header = nullptr;
		layout_interface* footer = nullptr;


		uint8_t page_leading_margin_min = 0;
		uint8_t page_leading_margin_max = 0;
		uint8_t page_trailing_margin_min = 0;
		uint8_t page_trailing_margin_max = 0;

		uint8_t line_leading_margin_min = 0;
		uint8_t line_leading_margin_max = 0;
		uint8_t line_trailing_margin_min = 0;
		uint8_t line_trailing_margin_max = 0;

		uint8_t min_column_line_size = 0;
		uint8_t max_column_line_size = 0;
		uint8_t max_columns = 0;
		content_orientation orientation = content_orientation::page;

		content_alignment column_content_alignment = content_alignment::leading;
		content_alignment column_in_page_alignment = content_alignment::leading;

		size_flags column_size_flags = size_flags::none;
	};

	enum class paragraph_break_behavior : uint8_t {
		standard, break_before, break_after, break_both, dont_break_after
	};

	struct simple_layout_specification {
		uint16_t minimum_page_size = 0;
		uint16_t minimum_line_size = 0;

		size_flags page_flags = size_flags::none;
		size_flags line_flags = size_flags::none;
		
		paragraph_break_behavior paragraph_breaking = paragraph_break_behavior::standard;
	};

	struct auto_layout_specification {
		int32_t list_size = -1;
		content_alignment internal_alignment = content_alignment::leading; // for each item in the list
		content_alignment group_alignment = content_alignment::leading; // for all of the items in the container
		content_orientation orientation = content_orientation::page;
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

	using layout_reference = uint16_t;
	using ui_reference = uint16_t;

	constexpr layout_reference layout_reference_none = std::numeric_limits<layout_reference>::max();
	constexpr ui_reference ui_reference_none = std::numeric_limits<ui_reference>::max();

	struct interface_or_layout_ref {
	private:
		render_interface* ptr = nullptr;
	public:
		interface_or_layout_ref() {}
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
		static constexpr uint8_t flag_grouping_only = 0x80;

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

		void rotate_borders(layout_orientation o);
	};

	struct page_information {
		layout_reference header = layout_reference_none;
		layout_reference footer = layout_reference_none;

		std::vector<layout_reference> columns;

		std::vector<uint16_t> subpage_divisions;
		uint16_t subpage_offset = 0;
	};

	struct container_information {
		std::vector<layout_reference> children;
		int32_t list_offset = -1;
	};

	struct layout_node {
		std::variant<
			std::unique_ptr<page_information>,
			std::unique_ptr<container_information>,
			std::monostate> contents = std::monostate{};

		layout_interface* l_interface = nullptr;

		layout_reference parent = layout_reference_none;

		ui_reference visible_rect = ui_reference_none;

		uint16_t x = 0; //in layout units
		uint16_t y = 0;
		uint16_t width = 0;
		uint16_t height = 0;

		bool layout_deferred = true;
		bool ignore = false;
		uint8_t generation = 0;

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
		visible, control, container, list_stub, page
	};

	struct layout_interface {
		layout_reference l_id = layout_reference_none;

		virtual ~layout_interface() {
		}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) = 0;
		virtual layout_node_type get_node_type() = 0;
		virtual simple_layout_specification get_specification(window_data const&) = 0;

		virtual layout_rect get_content_rectangle(window_data&) {
			return layout_rect{ 0i16, 0i16, 0i16, 0i16 };
		}
		void set_layout_id(layout_reference val) {
			l_id = val;
		}
		virtual page_layout_specification get_page_layout_specification(window_data const&) {
			return page_layout_specification{};
		}
		virtual auto_layout_specification get_auto_layout_specification(window_data const&) {
			return auto_layout_specification{};
		}
		virtual layout_interface* get_list_item(int32_t) {
			return nullptr;
		}
		virtual void recreate_contents(window_data&, layout_node&) {
		}
		virtual void on_focus(window_data&) {
		}
		virtual void on_lose_focus(window_data&) {
		}
		virtual void go_to_page(uint32_t pg, page_information& pi) {
			pi.subpage_offset = uint16_t(pg);
		}
		virtual IUnknown* get_accessibility_interface(window_data&) {
			return nullptr;
		}
	};


	struct interactable_state {
	private:
		uint8_t data = 0ui8;
		struct impl_key_type { };
		struct impl_group_type {};
	public:
		constexpr static impl_group_type group{};
		constexpr static impl_key_type key{};

		interactable_state() : data(0ui8) {
		}
		interactable_state(impl_group_type, uint8_t v) {
			data = uint8_t(v | 0x20);
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
			return (0xF0 & data) == 0x20;
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
		virtual int32_t interactable_count() {
			return 0;
		}
		virtual void set_interactable(int32_t, interactable_state) {
		}
		virtual bool data_is_ready() {
			return true;
		}
	};

	struct wrapped_text_instance {
		text::text_parameter stored_params[10] = {};
		uint16_t text_id = uint16_t(-1);
		uint8_t params_count = 0;
		uint8_t text_generation = 0;
	};

	struct stored_text {
	private:
		IDWriteTextLayout* formatted_text = nullptr;
		std::variant<std::monostate, std::wstring, wrapped_text_instance> text_content = std::monostate{};
	public:
		layout_position resolved_text_size{ 0,1 };
		content_alignment text_alignment = content_alignment::leading;
		bool draw_standard_size = true;
	public:
		~stored_text();
		void set_text(std::wstring const& v);
		void set_text(uint16_t text_id, text::text_parameter const* begin = nullptr, text::text_parameter const* end = nullptr);
		void set_text();
		void set_text(stored_text const& v);
		void prepare_text(window_data const& win);
		void relayout_text(window_data const& win, screen_space_point sz);
		void draw_text(window_data const& win, int32_t x, int32_t y) const;
		void invalidate();
		int32_t get_lines_height(window_data const& win) const;
	};

	struct title_bar_element : public render_interface {
		stored_text text;

		title_bar_element() {}
		virtual ~title_bar_element();

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;

		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		
		void set_text_content(uint16_t text_id);
	};


	struct vertical_2x2_icon_base : public render_interface {
		icon ico;
		interactable_state saved_state;

		vertical_2x2_icon_base() {}
		virtual ~vertical_2x2_icon_base() {}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;

		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual int32_t interactable_count() override {
			return 1;
		}
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void on_click(window_data&, uint32_t, uint32_t) override {
		}
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override {
		}
	};

	struct vertical_2x2_max_icon : public vertical_2x2_icon_base {
		icon restore_ico;
		
		vertical_2x2_max_icon();
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
	};

	struct vertical_2x2_min_icon : public vertical_2x2_icon_base {
		vertical_2x2_min_icon();
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
	};

	struct vertical_2x2_close_icon : public vertical_2x2_icon_base {
		vertical_2x2_close_icon();
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
	};

	struct vertical_2x2_settings_icon : public vertical_2x2_icon_base {
		vertical_2x2_settings_icon();
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
	};

	struct vertical_2x2_info_icon : public vertical_2x2_icon_base {
		uint8_t standard_fg = 0ui8;
		uint8_t standard_bg = 0ui8;

		vertical_2x2_info_icon();
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
		virtual int32_t interactable_count() override {
			return 0;
		}
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual void render_composite(ui_rectangle const& r, window_data& win, bool under_mouse) override;

		void mark_for_update(window_data& win);
	};

	struct button_control_base : public render_interface {
		stored_text button_text;

		uint16_t alt_text = uint16_t(-1);

		interactable_state saved_state;
		uint8_t interior_left_margin = 2;
		uint8_t interior_right_margin = 2;
		uint8_t icon = standard_icons::control_button;

		bool disabled = false;
		bool selected = false;

		button_control_base() {
		}
		virtual ~button_control_base();

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void recreate_contents(window_data&, layout_node&) override;

		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual int32_t interactable_count() override {
			if(!disabled)
				return 1;
			else
				return 0;
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override {
		}
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
	};

	struct label_control : public render_interface {
		stored_text label_text;
		uint16_t alt_text = uint16_t(-1);

		uint8_t interior_left_margin = 2;
		uint8_t interior_right_margin = 2;

		label_control() {
		}
		virtual ~label_control() {
		}

		virtual layout_node_type get_node_type() override {
			return layout_node_type::visible;
		};
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override;
	};

	struct list_control;

	struct list_option : public button_control_base {
		size_t data = 0;
		list_option() {
		}
		virtual ~list_option() {
		}
		virtual void on_click(window_data&, uint32_t, uint32_t);
	};
	struct list_open : public button_control_base {
		list_open() {
		}
		virtual ~list_open() {
		}
		virtual void on_click(window_data&, uint32_t, uint32_t);
	};

	struct list_option_description {
		size_t data;
		uint16_t text_id;
		uint16_t alt_text_id;
	};

	struct list_control : public layout_interface {
		static animation_defintion list_appearance;
		static animation_defintion list_disappearance;

		std::vector<list_option> options;
		list_open open_button;

		int32_t currently_selected = 0;
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
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual void on_lose_focus(window_data&) override;
		virtual layout_rect get_content_rectangle(window_data&) override;

		void open_list(window_data&, bool move_focus);
		void close_list(window_data&, bool move_focus);
		void populate_list(window_data const&);
		void quiet_select_option_by_value(size_t);
		void select_option_by_value(window_data&, size_t);
		button_control_base* get_option(size_t);

		virtual void on_select(window_data&, size_t) = 0;
		virtual list_option_description describe_option(window_data const&, uint32_t) = 0;
		virtual void on_create(window_data const&) { }
	};


	struct page_header_button : public render_interface {
		std::function<void(window_data&, layout_reference)> close_action;
		interactable_state saved_state;

		page_header_button(std::function<void(window_data&, layout_reference)>&& a) : close_action(std::move(a)) {
		}
		virtual ~page_header_button() {
		}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override {
			return layout_node_type::control;
		}
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void render_foreground(ui_rectangle const&, window_data&) override {
		};

		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual int32_t interactable_count() override {
			return 1;
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override {
		}
	};

	struct page_footer_button : public render_interface {
		stored_text fraction_text;

		interactable_state saved_state;
		uint32_t stored_page = 0;
		uint32_t stored_page_total = 0;

		page_footer_button() {
			fraction_text.text_alignment = content_alignment::centered;
		}
		virtual ~page_footer_button() {
		}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void render_foreground(ui_rectangle const&, window_data&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;

		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual int32_t interactable_count() override {
			return 1;
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override {
		}
		void update_page(window_data& win);
	};

	struct small_icon_button : public render_interface {
		uint8_t icon_id = 0;
		interactable_state saved_state;

		small_icon_button() {
		}
		virtual ~small_icon_button() {
		}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_node_type get_node_type() override {
			return layout_node_type::control;
		}
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void render_foreground(ui_rectangle const&, window_data&) override {
		};
		virtual void set_interactable(int32_t, interactable_state v) override {
			saved_state = v;
		}
		virtual int32_t interactable_count() override {
			return 1;
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override {
		}
		virtual void on_right_click(window_data&, uint32_t, uint32_t) override {
		}
		virtual bool is_disabled(window_data const&) {
			return false;
		}
	};

	struct page_back_button : public small_icon_button {
		page_back_button() {
			icon_id = standard_icons::control_prev;
		}
		virtual ~page_back_button() {
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual bool is_disabled(window_data const& win) override;
	};
	struct page_forward_button : public small_icon_button {
		page_forward_button() {
			icon_id = standard_icons::control_next;
		}
		virtual ~page_forward_button() {
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual bool is_disabled(window_data const& win) override;
	};
	struct page_jump_forward_button : public small_icon_button {
		page_jump_forward_button() {
			icon_id = standard_icons::control_next_next;
		}
		virtual ~page_jump_forward_button() {
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual bool is_disabled(window_data const& win) override;
	};
	struct page_jump_back_button : public small_icon_button {
		page_jump_back_button() {
			icon_id = standard_icons::control_prev_prev;
		}
		virtual ~page_jump_back_button() {
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
		virtual bool is_disabled(window_data const& win) override;
	};

	struct page_footer : public layout_interface {
		static animation_defintion footer_appearance;
		static animation_defintion footer_disappearance;

		static animation_defintion page_turn_up;
		static animation_defintion page_turn_down;

		page_footer_button primary_display;
		page_back_button back;
		page_forward_button forward;
		page_jump_back_button jump_back;
		page_jump_forward_button jump_forward;

		bool is_open = false;

		page_footer() { }
		virtual ~page_footer() {
		}
		
		virtual layout_node_type get_node_type() override {
			return layout_node_type::container;
		};

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual layout_rect get_content_rectangle(window_data&) override;
		virtual void on_focus(window_data&) override;
		virtual void on_lose_focus(window_data&) override;

		void open_footer(window_data&, bool move_focus);
		void close_footer(window_data&, bool move_focus);
	};

	struct close_info_window : public small_icon_button {
		close_info_window() {
			icon_id = standard_icons::header_close;
		}
		virtual ~close_info_window() {
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
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
			text.draw_standard_size = false;
		}
		virtual ~info_window() { }
		virtual layout_node_type get_node_type() override {
			return layout_node_type::container;
		}
		virtual int32_t interactable_count() override {
			return 0;
		}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data const&) override;
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
		interactable_state saved_state;

		single_line_centered_header(std::function<void(window_data&, layout_reference)>&& a) : close_button(std::move(a)) {
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
		virtual int32_t interactable_count() override {
			return 0;
		}
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void render_foreground(ui_rectangle const& rect, window_data& win) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
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
		virtual int32_t interactable_count() override {
			return 0;
		}

		virtual void render_foreground(ui_rectangle const&, window_data&) override {
		}
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void render_composite(ui_rectangle const& rect, window_data& win, bool under_mouse) override;
	};

	struct menu_control;

	struct menu_open : public button_control_base {
		menu_open() {
		}
		virtual ~menu_open() {
		}
		virtual void on_click(window_data&, uint32_t, uint32_t);
	};

	struct menu_control : public layout_interface {
		single_line_empty_header header_space;
		page_footer menu_footer;

		static animation_defintion list_appearance;
		static animation_defintion list_disappearance;

		menu_open open_button;
		int32_t page_size = 1;
		int32_t line_size = 6;

		content_alignment text_alignment = content_alignment::leading;
		bool list_is_open = false;
		bool vertically_cover_parent = false;
		uint16_t alt_text = uint16_t(-1);

		menu_control() {
		}
		virtual ~menu_control() {
		}

		int32_t get_menu_height(window_data const&) const;

		virtual layout_node_type get_node_type() override {
			return layout_node_type::page;
		};
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual page_layout_specification get_page_layout_specification(window_data const&) override;
		virtual simple_layout_specification get_specification(window_data const&) override;

		virtual void recreate_contents(window_data&, layout_node&) override;
		virtual void on_lose_focus(window_data&) override;
		virtual layout_rect get_content_rectangle(window_data&) override;

		void open_menu(window_data&, bool move_focus);
		void close_menu(window_data&, bool move_focus);

		virtual std::vector<layout_interface*> get_options(window_data&) = 0;
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
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
	};

	struct language_button : public button_control_base {
		std::wstring lang;
		std::wstring region;

		language_button() {
			button_text.text_alignment = content_alignment::trailing;
		}
		virtual ~language_button() {
		}
		virtual void on_click(window_data&, uint32_t, uint32_t) override;
	};

	struct language_menu : public menu_control {
		std::vector<std::unique_ptr<layout_interface>> lbuttons;

		virtual ~language_menu() {
		}

		virtual std::vector<layout_interface*> get_options(window_data&);
		virtual void on_open(window_data&);
		virtual void on_close(window_data&);
	};

	struct settings_orientation_list : public list_control {
		virtual ~settings_orientation_list() { }
		virtual void on_select(window_data&, size_t) override;
		virtual list_option_description describe_option(window_data const&, uint32_t) override;
	};
	struct settings_input_mode_list : public list_control {
		virtual ~settings_input_mode_list() {
		}
		virtual void on_select(window_data&, size_t) override;
		virtual list_option_description describe_option(window_data const&, uint32_t) override;
		virtual void on_create(window_data const&) override;
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

		common_printui_settings();
		virtual ~common_printui_settings() { }

		virtual layout_node_type get_node_type() override {
			return layout_node_type::page;
		};

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual page_layout_specification get_page_layout_specification(window_data const&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
	};

	struct settings_page_container : public layout_interface {
		std::vector<settings_item_button> settings_items;

		single_line_centered_header page_header;
		int32_t settings_item_selected = 0;

		settings_page_container(window_data const& win, std::vector<settings_menu_item> const& setting_items);
		virtual ~settings_page_container() { }

		virtual layout_node_type get_node_type() override {
			return layout_node_type::page;
		};
		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual page_layout_specification get_page_layout_specification(window_data const&) override;

		virtual void recreate_contents(window_data&, layout_node&) override;

		virtual void go_to_page(uint32_t i, page_information& pi) override;
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

		window_bar_element(window_data const& win, bool mn, bool mx, bool settings, std::vector<settings_menu_item> const& setting_items);

		virtual ~window_bar_element() {}

		virtual ui_rectangle prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) override;
		virtual layout_rect get_content_rectangle(window_data& win) override;
		virtual auto_layout_specification get_auto_layout_specification(window_data const&) override;
		virtual layout_node_type get_node_type() override;
		virtual simple_layout_specification get_specification(window_data const&) override;
		virtual void recreate_contents(window_data&, layout_node&) override;
	};

	
	struct column_content {
		std::vector<layout_reference> contents;

		int32_t along_column_size = 0;
		int32_t across_column_max = 0;
	};

	struct formatting_values {
		int32_t with_orientation_lmn = 0;
		int32_t with_orientation_lmx = 0;
		int32_t with_orientation_tmn = 0;
		int32_t with_orientation_tmx = 0;

		int32_t across_orientation_lmn = 0;
		int32_t across_orientation_lmx = 0;
		int32_t across_orientation_tmn = 0;
		int32_t across_orientation_tmx = 0;

		content_orientation o = content_orientation::page;

		content_alignment column_in_page = content_alignment::leading;
		content_alignment content_in_column = content_alignment::leading;
		size_flags column_size = size_flags::none;

		int32_t header_size = 0;
		int32_t footer_size = 0;
	};
	struct column_break_condition {
		bool limit_to_max_size = true;
		bool only_glued = false;
		bool obey_break_requests = true;
		bool column_must_not_be_empty = true;
	};
	column_content make_column(std::vector<layout_interface*> const& source, window_data& win, int32_t& index_into_source, content_orientation o, int32_t max_space, int32_t across_column_min, int32_t across_column_max, int32_t& running_list_position, column_break_condition const& conditions);
	void format_columns(page_information& containing_page_info, window_data& layout_nodes,
		int32_t max_v_size, int32_t max_h_size, formatting_values const& values);
	void default_recreate_container(window_data& win, layout_interface* l_interface, layout_node* retvalue, std::vector<layout_interface*> const& children);
	void default_recreate_page(window_data& win, layout_interface* l_interface, layout_node* retvalue, std::vector<layout_interface*> const& children);
	void default_relayout_page_columns(window_data& win, layout_interface* l_interface, layout_node* retvalue);

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
		void load_fonts_from_directory(std::wstring const& directory, IDWriteFontSetBuilder2* bldr);
		std::vector<font_fallback> get_global_font_fallbacks();
		void load_fallbacks_by_type(std::vector<font_fallback> const& fb, font_type type, IDWriteFontFallbackBuilder* bldr, IDWriteFontCollection1* collection, IDWriteFactory6* dwrite_factory);
		void update_font_metrics(font_description& desc, IDWriteFactory6* dwrite_factory, wchar_t const* locale, float target_pixels, float dpi_scale, IDWriteFont* font);
		void create_font_collection(window_data& win, std::wstring font_directory);

		enum class extra_formatting : uint8_t {
			none, small_caps, italic, old_numbers, tabular_numbers, bold
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

		class text_manager;
		struct text_data_storage;

		using attribute_type = int8_t;

		using formatting_content = std::variant<std::monostate, font_id, parameter_id, substitution_mark, extra_formatting>;

		// extra display formatting to apply to a range of characters
		struct format_marker {
			uint16_t position;
			formatting_content format;

			bool operator<(const format_marker& o) const noexcept {
				return position < o.position;
			}
		};

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

		
		constexpr attribute_type zero = 0;
		constexpr attribute_type one = 1;
		constexpr attribute_type two = 2;
		constexpr attribute_type few = 3;
		constexpr attribute_type many = 4;
		constexpr attribute_type other = 5;

		constexpr attribute_type ord_zero = 6;
		constexpr attribute_type ord_one = 7;
		constexpr attribute_type ord_two = 8;
		constexpr attribute_type ord_few = 9;
		constexpr attribute_type ord_many = 10;
		constexpr attribute_type ord_other = 11;

		constexpr attribute_type last_predefined = 11;
		

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

			void load_text_from_directory(std::wstring const& directory);
			replaceable_instance parameter_to_text(text_parameter p) const;

			replaceable_instance format_int(int64_t value, uint32_t decimal_places) const;
			replaceable_instance format_double(double value, uint32_t decimal_places) const;
		public:
			uint8_t text_generation = 0;

			void update_with_new_locale(window_data& win);
			void populate_text_content(window_data const& win);
			void register_name(std::string_view n, uint16_t id);

			std::unordered_map<std::string, uint32_t, string_hash, std::equal_to<>> font_name_to_index;

			text_manager();

			text_id text_id_from_name(std::string_view name) const;

			friend matched_pattern;
			friend text_function;
			friend text_with_formatting;

			void change_locale(std::wstring const& lang, std::wstring const& region, window_data& win);
			void default_locale(window_data& win);

			double text_to_double(wchar_t* start, uint32_t count) const;
			int64_t text_to_int(wchar_t* start, uint32_t count) const;
			wchar_t const* locale_string() const;
			bool is_current_locale(std::wstring const& lang, std::wstring const& region) const;
			
			replaceable_instance instantiate_text(uint16_t id, text_parameter const* s = nullptr, text_parameter const* e = nullptr) const;
			replaceable_instance instantiate_text(std::string_view key, text_parameter const* s = nullptr, text_parameter const* e = nullptr) const;
		};

		void apply_default_vertical_options(IDWriteTypography* t);
		void apply_default_ltr_options(IDWriteTypography* t);
		void apply_default_rtl_options(IDWriteTypography* t);

		void apply_old_style_figures_options(IDWriteTypography* t);
		void apply_lining_figures_options(IDWriteTypography* t);
		void apply_small_caps_options(IDWriteTypography* t);

		void apply_formatting(IDWriteTextLayout* target, std::vector<format_marker> const& formatting, std::vector<font_description> const& named_fonts, IDWriteFactory6* dwrite_factory);

		struct language_description {
			std::wstring language;
			std::wstring region;
			std::wstring display_name;
		};
		std::vector<language_description> ennumerate_languages(window_data const& win);
	}
	
	struct animation_status_struct {
		animation_description description;

		std::chrono::time_point<std::chrono::steady_clock> start_time;
		bool is_running = false;
		
	};

	class root_window_provider;

	enum class os_handle_type {
		windows_hwnd
	};

	struct os_direct_access_base {
	};

	struct window_wrapper {
		virtual ~window_wrapper() { }
		virtual void invalidate_window() = 0;
		virtual screen_space_rect get_available_workspace() const = 0;
		virtual screen_space_rect get_window_placement() const = 0;
		virtual screen_space_rect get_window_location() const = 0;
		virtual void set_window_placement(screen_space_rect r) = 0;
		virtual void hide_mouse_cursor() = 0;
		virtual void show_mouse_cursor() = 0;
		virtual bool is_mouse_cursor_visible() const = 0;
		virtual void reshow_mouse_cursor() = 0;
		virtual int32_t get_key_state(uint32_t scan_code) const = 0;
		virtual void move_window(screen_space_rect r) = 0;
		virtual uint32_t get_window_dpi() const = 0;
		virtual bool create_window(window_data& wd) = 0;
		virtual void display_fatal_error_message(wchar_t const*) = 0;
		virtual long create_swap_chain(IDXGIFactory2* fac, ID3D11Device* dev, DXGI_SWAP_CHAIN_DESC1 const* desc, IDXGISwapChain1** out) = 0;
		virtual bool is_maximized() const = 0;
		virtual bool is_minimized() const = 0;
		virtual void maximize(window_data&) = 0;
		virtual void minimize(window_data&) = 0;
		virtual void restore(window_data&) = 0;
		virtual void close(window_data&) = 0;
		virtual void set_text_rendering_parameters(ID2D1DeviceContext5* dc, IDWriteFactory6* fac) = 0;
		virtual void set_window_title(wchar_t const* t) = 0;
		virtual bool window_has_focus() const = 0;
		virtual os_direct_access_base* get_os_access(os_handle_type) = 0;
	};

	enum class resize_type : uint8_t {
		resize, maximize, minimize
	};

	struct accessibility_framework_wrapper {
		virtual ~accessibility_framework_wrapper() {}
		virtual bool has_keyboard_preference() = 0;
		virtual void notify_window_state_change(resize_type r) = 0;
		virtual void notify_window_moved(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
		virtual void notify_window_closed() = 0;
		virtual root_window_provider* get_root_window_provider() = 0;
		virtual void release_root_provider() = 0;
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
		bool redraw_completely_pending = false;

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

		ID2D1Factory6* d2d_factory = nullptr;
		IDWriteFactory6* dwrite_factory = nullptr;
		IWICImagingFactory* wic_factory = nullptr;

		IDWriteFontCollection2* font_collection = nullptr;

		IDWriteTextFormat3* common_text_format = nullptr;
		IDWriteTextFormat3* small_text_format = nullptr;

		IDWriteFontFallback* font_fallbacks = nullptr;
		IDWriteFontFallback* small_font_fallbacks = nullptr;

		ID2D1SolidColorBrush* dummy_brush = nullptr;
		ID2D1StrokeStyle* plain_strokes = nullptr;

		ID2D1SolidColorBrush* light_selected = nullptr;
		ID2D1SolidColorBrush* light_line = nullptr;
		ID2D1SolidColorBrush* light_selected_line = nullptr;

		ID2D1SolidColorBrush* dark_selected = nullptr;
		ID2D1SolidColorBrush* dark_line = nullptr;
		ID2D1SolidColorBrush* dark_selected_line = nullptr;

		ID2D1Bitmap1* foreground = nullptr;

		ID2D1Bitmap1* animation_foreground = nullptr;
		ID2D1Bitmap1* animation_background = nullptr;

		icon horizontal_interactable_bg;
		icon vertical_interactable_bg;

		ID2D1Bitmap1* horizontal_interactable[12] = { nullptr };
		ID2D1Bitmap1* vertical_interactable[12] = { nullptr };

		ID3D11Device* d3d_device = nullptr;
		IDXGIDevice1* dxgi_device = nullptr;
		ID2D1Device5* d2d_device = nullptr;

		ID3D11DeviceContext* d3d_device_context = nullptr;
		ID2D1DeviceContext5* d2d_device_context = nullptr;
		IDXGISwapChain1* swap_chain = nullptr;

		ID2D1Bitmap1* back_buffer_target = nullptr;

		std::vector<ID2D1Brush*> palette;
		std::vector<ID2D1Bitmap*> palette_bitmaps;

		std::unique_ptr<window_wrapper> window_interface;
		std::unique_ptr<accessibility_framework_wrapper> accessibility_interface;

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

		std::vector<stored_focus> focus_stack;
		std::array<grouping_range, 12> current_focus_groupings;
		int32_t current_groupings_size = 0;
		key_actions focus_actions;

		prompt_mode prompts = prompt_mode::keyboard;
		bool display_interactable_type = true;
		

		wchar_t prompt_labels[12] = { L'Q', L'W', L'E', L'R', L'A', L'S', L'D', L'F', L'Z', L'X', L'C', L'V' };
		uint8_t sc_values[12] = {
			0x10ui8, 0x11ui8, 0x12ui8, 0x13ui8,   // Q W E R
			0x1Eui8, 0x1Fui8, 0x20ui8, 0x21ui8,   // A S D F
			0x2Cui8, 0x2Dui8, 0x2Eui8, 0x2Fui8 }; // Z X C V

		uint8_t primary_escape_sc = 0x39ui8; // space bar
		uint8_t secondary_escape_sc = 0x01ui8; // esc

		uint8_t primary_right_click_modifier_sc = 0x1Dui8; // left control
		uint8_t secondary_right_click_modifier_sc = 0x1Dui8; // right control (both are the same if we take only the lower byte)
		uint8_t primary_right_click_modifier_sc_up = 0x9Dui8; // left control
		uint8_t secondary_right_click_modifier_sc_up = 0x9Dui8; // right control (both are the same if we take only the lower byte)

		bool is_sizeing = false;
		bool is_suspended = false;
		bool pending_right_click = false;

		animation_status_struct animation_status;

		window_data(bool mn, bool mx, bool settings, std::vector<settings_menu_item> const& setting_items, std::unique_ptr<window_wrapper>&& wi, std::unique_ptr<accessibility_framework_wrapper>&& ai);
		virtual ~window_data();

		virtual void load_default_dynamic_settings() = 0;
		virtual void load_primary_font_fallbacks(IDWriteFontFallbackBuilder* bldr) = 0;
		virtual void load_small_font_fallbacks(IDWriteFontFallbackBuilder* bldr) = 0;
		virtual void client_on_dpi_change() = 0;
		virtual void client_on_resize(uint32_t width, uint32_t height) = 0;

		// called directly on message reciept

		bool on_mouse_move(uint32_t x, uint32_t y);
		bool on_key_down(uint32_t scan_code, uint32_t key_code);
		bool on_key_up(uint32_t scan_code, uint32_t key_code);
		bool on_mouse_left_down(uint32_t x, uint32_t y);
		bool on_mouse_right_down(uint32_t x, uint32_t y);
		bool on_resize(resize_type type, uint32_t width, uint32_t height);

		// OTHER

		void create_persistent_resources();
		void message_loop();

		void create_window();
		void on_dpi_change();
		

		void create_device_resources();
		void release_device_resources();

		void create_window_size_resources(uint32_t nWidth, uint32_t nHeight);

		void create_palette();
		void release_palette();
		
		void refresh_foregound();
		void render();

		void composite_animation();
		void stop_ui_animations();
		void prepare_ui_animation();
		void start_ui_animation(animation_description description);

		void show_settings_panel();
		void hide_settings_panel();

		void expand_to_fit_content();
		void intitialize_fonts();

		void remove_references_to(layout_interface* l);
		bool is_title_set() const {
			return has_window_title;
		}
		// layout functions

		void recreate_contents(layout_interface* l_interface, layout_node* node);
		void immediate_resize(layout_node& node, int32_t new_width, int32_t new_height);
		void recreate_container_contents(layout_interface* l_interface, layout_node* node);
		void recreate_simple_container_contents(layout_interface* l_interface, layout_node* node);
		void recreate_list_contents(layout_interface* l_interface, layout_node* node);
		void recreate_page_contents(layout_interface* l_interface, layout_node* node);

		window_bar_element window_bar;
		title_bar_element title_bar;
		info_window info_popup;

		layout_reference create_node(layout_interface* l_interface, int32_t max_width, int32_t max_height, bool force_create_children, bool force_create = false);

		uint32_t content_window_x = 0;
		uint32_t content_window_y = 0;

		void change_orientation(layout_orientation o);
		void recreate_layout();
		void resize_item(layout_reference id, int32_t new_width, int32_t new_height);
		void set_window_title(std::wstring const& title);
		wchar_t const* get_window_title() const;
		void init_layout_graphics();

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

		void create_interactiable_tags();
		void create_highlight_brushes();

		screen_space_rect get_current_location(layout_reference r) const;
		screen_space_rect get_layout_rect_in_current_location(layout_rect const& rect, layout_reference r) const;

		void safely_clear_vector(std::vector<std::unique_ptr<layout_interface>>& v);
		void safely_release_interface(layout_interface* v);
	};

	
	ui_rectangle const* interface_under_point(std::vector<ui_rectangle> const& rects, int32_t x, int32_t y);
	ui_reference reference_under_point(std::vector<ui_rectangle> const& rects, int32_t x, int32_t y);
	layout_reference layout_reference_under_point(std::vector<ui_rectangle> const& rects, int32_t x, int32_t y);


	int32_t reading_direction_from_orientation(layout_orientation o);
	int32_t flow_direction_from_orientation(layout_orientation o);
	inline bool horizontal(layout_orientation o) {
		return o == layout_orientation::horizontal_left_to_right || o == layout_orientation::horizontal_right_to_left;
	}
	uint32_t content_alignment_to_text_alignment(content_alignment align);
	

	namespace render {
		void to_display(std::vector<ui_rectangle> const& uirects, window_data& win);
		void foregrounds(std::vector<ui_rectangle>& uirects, window_data& win);
		void update_foregrounds(std::vector<ui_rectangle>& uirects, window_data& win);
		void background_rectangle(D2D_RECT_F const& content_rect, window_data const& win, uint8_t display_flags, uint8_t brush, bool under_mouse);

		void interactable_or_icon(window_data& win, ID2D1DeviceContext5* dc, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical, icon const& ico);
		void interactable_or_foreground(window_data& win, ID2D1DeviceContext5* dc, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical);
		void interactable(window_data& win, ID2D1DeviceContext5* dc, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical);
	}
}
