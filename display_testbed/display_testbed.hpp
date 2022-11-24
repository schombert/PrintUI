#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include "printui_utility.hpp"


class main_window : public printui::window_data {
public:
	virtual void load_default_dynamic_settings() override;
	virtual void load_primary_font_fallbacks(IDWriteFontFallbackBuilder*) override {
	}
	virtual void load_small_font_fallbacks(IDWriteFontFallbackBuilder*) override {
	}
	virtual void client_on_dpi_change() override {
	}
	virtual void client_on_resize(uint32_t, uint32_t) override {
	}
	std::vector<printui::settings_menu_item> get_settings_items() const {
		return std::vector<printui::settings_menu_item>{};
	}
	main_window() : printui::window_data(true, true, true, get_settings_items(), std::make_unique<printui::os_win32_wrapper>()) {
	}
};

/*
struct ID2D1Factory6;
struct IWICImagingFactory;
struct IDWriteFactory6;
struct IDWriteFontCollection2;
struct IDWriteFontFallback;
struct IDWriteTextFormat3;
struct ID3D11Device;
struct IDXGIDevice1;
struct ID2D1Device5;
struct ID3D11DeviceContext;
struct ID2D1DeviceContext5;
struct IDXGISwapChain1;
struct ID2D1Bitmap1;
struct ID2D1Brush;
struct ID2D1Bitmap;
struct ID2D1SolidColorBrush;
struct ID2D1StrokeStyle;
struct IDWriteFont;

namespace printui  {
	class main_window : public printui::window_data {
	public:
		main_window();
		~main_window();

		void create_window();
		void message_loop();

		static constexpr int32_t pane_count = 4;
		static constexpr int32_t palette_count = 4;

		printui::window_data win;
	private:
		void create_persistent_resources();
		void create_device_resources();
		void create_window_size_resources(uint32_t nWidth, uint32_t nHeight);

		void release_device_resources();
		void render();

		void on_resize(uint32_t width, uint32_t height);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		void create_panes();
		void create_palette();

		void release_panes();
		void release_palette();

		void redraw_ui_completely();
	private:
		IWICImagingFactory* wic_factory;
		

		// Fonts
		IDWriteFontCollection2* font_collection;
		IDWriteFontFallback* font_fallbacks;

		ID3D11Device* d3d_device;
		IDXGIDevice1* dxgi_device;
		ID2D1Device5* d2d_device;

		ID3D11DeviceContext* d3d_device_context;
		ID2D1DeviceContext5* d2d_device_context;
		IDXGISwapChain1* swap_chain;

		ID2D1Bitmap1* back_buffer_target;

		
		ID2D1Brush* palette[palette_count];
		ID2D1Bitmap* palette_bitmaps[palette_count];

		ID2D1StrokeStyle* plain_strokes;

		ID2D1Bitmap1* rendered_close_icon;

		//icon settings_icon;
	};
}
*/

