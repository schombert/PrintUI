#pragma once
#include "printui_utility.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

namespace printui {
	struct hwnd_direct_access_base : public os_direct_access_base {
		HWND m_hwnd = nullptr;
	};

	struct os_win32_wrapper : public window_wrapper {
		hwnd_direct_access_base stored_direct_access;

		HWND m_hwnd = nullptr;
		bool cursor_visible = false;

		os_win32_wrapper();
		virtual ~os_win32_wrapper();
		virtual void invalidate_window() override;
		virtual screen_space_rect get_available_workspace() const override;
		virtual screen_space_rect get_window_placement() const override;
		virtual screen_space_rect get_window_location() const override;
		virtual void set_window_placement(screen_space_rect r) override;
		virtual void hide_mouse_cursor() override;
		virtual void show_mouse_cursor() override;
		virtual bool is_mouse_cursor_visible() const override;
		virtual void reshow_mouse_cursor() override;
		virtual int32_t get_key_state(uint32_t scan_code) const override;
		virtual bool is_shift_held_down() const override;
		virtual bool is_ctrl_held_down() const override;
		virtual void move_window(screen_space_rect r) override;
		virtual uint32_t get_window_dpi() const override;
		virtual bool create_window(window_data& wd) override;
		virtual void display_fatal_error_message(wchar_t const*) override;
		virtual long create_swap_chain(IDXGIFactory2* fac, ID3D11Device* dev, DXGI_SWAP_CHAIN_DESC1 const* desc, IDXGISwapChain1** out) override;
		virtual bool is_maximized() const override;
		virtual bool is_minimized() const override;
		virtual void maximize(window_data&) override;
		virtual void minimize(window_data&) override;
		virtual void restore(window_data&) override;
		virtual void close(window_data&) override;
		virtual void set_text_rendering_parameters(ID2D1DeviceContext5* dc, IDWriteFactory6* fac) override;
		virtual void set_window_title(wchar_t const* t) override;
		virtual bool window_has_focus() const override;
		virtual os_direct_access_base* get_os_access(os_handle_type) override;
		virtual void text_to_clipboard(std::wstring_view txt) override;
		virtual std::wstring text_from_clipboard() override;
		virtual void create_system_caret(int32_t width, int32_t height) override;
		virtual void move_system_caret(int32_t x, int32_t y) override;
		virtual void destroy_system_caret() override;
	};

	
}
