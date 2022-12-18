#ifndef PRINTUI_WINDOWING_HEADER
#define PRINTUI_WINDOWING_HEADER

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "printui_datatypes.hpp"
#include <Windows.h>

namespace printui {

	struct os_win32_wrapper {
	private:
		HWND m_hwnd = nullptr;
		bool cursor_visible = false;
	public:
		os_win32_wrapper();
		~os_win32_wrapper();
		void invalidate_window();
		screen_space_rect get_available_workspace() const;
		screen_space_rect get_window_placement() const;
		screen_space_rect get_window_location() const;
		void set_window_placement(screen_space_rect r);
		void hide_mouse_cursor();
		void show_mouse_cursor();
		bool is_mouse_cursor_visible() const;
		void reshow_mouse_cursor();
		int32_t get_key_state(uint32_t scan_code) const;
		bool is_shift_held_down() const;
		bool is_ctrl_held_down() const;
		void move_window(screen_space_rect r);
		uint32_t get_window_dpi() const;
		bool create_window(window_data& wd);
		void display_fatal_error_message(wchar_t const*);
		bool is_maximized() const;
		bool is_minimized() const;
		void maximize(window_data&);
		void minimize(window_data&);
		void restore(window_data&);
		void close(window_data&);
		void set_window_title(wchar_t const* t);
		bool window_has_focus() const;
		void* get_hwnd();
		void text_to_clipboard(std::wstring_view txt);
		std::wstring text_from_clipboard();
		void create_system_caret(int32_t width, int32_t height);
		void move_system_caret(int32_t x, int32_t y);
		void destroy_system_caret();
	};

	
}

#endif
