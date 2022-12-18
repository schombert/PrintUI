#ifndef PRINTUI_RENDERING_HEADER
#define PRINTUI_RENDERING_HEADER

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "printui_datatypes.hpp"
#include <algorithm>
#include <charconv>
#include <Windowsx.h>
#include <Shlobj.h>
#include <dxgi1_2.h>
#include <d3d11_3.h>
#include <DirectXMath.h>
#include <d2d1_3.h>
#include <d2d1helper.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <shlwapi.h>
#include <array>


namespace printui::render {

	struct direct2d_rendering;

	struct icon {
		std::wstring file_name;
		ID2D1Bitmap1* rendered_layer = nullptr;

		float edge_padding = 0.0f;

		int8_t xsize = 1;
		int8_t ysize = 1;

		~icon();
		void redraw_image(window_data const& win, direct2d_rendering& ri);
		void present_image(float x, float y, direct2d_rendering& ri, ID2D1Brush* br);
	};

	struct direct2d_rendering {
	private:
		ID2D1Factory6* d2d_factory = nullptr;
		IWICImagingFactory* wic_factory = nullptr;

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

		ID2D1Bitmap1* horizontal_controller_interactable[7] = { nullptr };
		ID2D1Bitmap1* vertical_controller_interactable[7] = { nullptr };

		ID3D11Device* d3d_device = nullptr;
		IDXGIDevice1* dxgi_device = nullptr;
		ID2D1Device5* d2d_device = nullptr;

		ID3D11DeviceContext* d3d_device_context = nullptr;
		ID2D1DeviceContext5* d2d_device_context = nullptr;
		IDXGISwapChain1* swap_chain = nullptr;

		ID2D1Bitmap1* back_buffer_target = nullptr;

		std::vector<ID2D1Brush*> palette;
		std::vector<ID2D1Bitmap*> palette_bitmaps;

		std::array<icon, standard_icons::final_icon> icons;

		bool redraw_completely_pending = true;
		bool is_suspended = false;

		bool running_in_place_animation = false;
		bool previous_frame_in_place_animation = false;
		decltype(std::chrono::steady_clock::now()) in_place_animation_start;
		animation_status_struct animation_status;

	public:
		direct2d_rendering();
		virtual ~direct2d_rendering();

		void render(window_data& win);
		void background_rectangle(screen_space_rect content_rect, uint8_t display_flags, uint8_t brush, bool under_mouse, window_data const& win);
		void interactable_or_icon(window_data const& win, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical, uint8_t ico);
		void interactable_or_foreground(window_data const& win, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical);
		void interactable(window_data const& win, screen_space_point location, interactable_state state, uint8_t fg_brush, bool vertical);
		void text(window_data const& win, ::printui::text::arranged_text*, int32_t x, int32_t y);
		void fill_from_foreground(screen_space_rect location, uint8_t fg_brush, bool optimize_for_text);
		void create_palette(window_data const& win);
		void mark_for_complete_redraw();
		void stop_ui_animations(window_data const& win);
		void prepare_ui_animation(window_data& win);
		void start_ui_animation(animation_description description, window_data& win);
		void register_in_place_animation();
		int64_t in_place_animation_running_ms() const;
		void recreate_dpi_dependent_resource(window_data& win);
		void create_window_size_resources(window_data& win);
		void set_brush_opacity(uint8_t b, float o);
		void fill_rectangle(screen_space_rect location, uint8_t b);
		void draw_icon(int32_t x, int32_t y, uint8_t ico, uint8_t br);
		void draw_icon_to_foreground(int32_t x, int32_t y, uint8_t ico);
		layout_position get_icon_size(uint8_t ico);
	private:
		void create_device_resources(window_data& win);
		void to_display(std::vector<ui_rectangle> const& uirects, window_data& win);
		void foregrounds(std::vector<ui_rectangle>& uirects, window_data& win);
		void update_foregrounds(std::vector<ui_rectangle>& uirects, window_data& win);
		void composite_animation(window_data& win, uint32_t ui_width, uint32_t ui_height);
		void release_palette();
		void create_highlight_brushes();
		void setup_icon_transform(window_data const& win, float edge_padding, int32_t xsize, int32_t ysize);
		void create_interactiable_tags(window_data& win);
		void refresh_foregound(window_data& win);
		void redraw_icons(window_data& win);
		void release_device_resources();
		void update_preserved_rects(ui_rectangle const& r, direct2d_rendering& d2dri, ID2D1RectangleGeometry*& screen_rect_geom, ID2D1GeometryGroup*& with_preserved_rects, ID2D1Geometry*& old_rects, int32_t ui_width, int32_t ui_height);

		friend struct icon;
	};
}

#endif
