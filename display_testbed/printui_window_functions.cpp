#include "printui_main_header.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

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
#include <ole2.h>
#include <UIAutomation.h>
#include "printui_windows_definitions.hpp"
#include "printui_accessibility_definitions.hpp"
#include "printui_text_definitions.hpp"

#define XINPUT_ON_GAMEINPUT_NO_XINPUTENABLE

#include "xinput.h"
#include <hidsdi.h>
#include <hidpi.h>

#pragma comment(lib, "Xinput.lib")
#pragma comment(lib, "Xinput9_1_0.lib")

namespace printui {

	os_win32_wrapper::os_win32_wrapper() {
		SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	}
	void os_win32_wrapper::invalidate_window() {
		InvalidateRect((HWND)(m_hwnd), nullptr, FALSE);
	}
	screen_space_rect os_win32_wrapper::get_available_workspace() const {
		auto monitor_handle = MonitorFromWindow((HWND)(m_hwnd), MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(monitor_handle, &mi);

		return screen_space_rect{ mi.rcWork.left, mi.rcWork.top, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top };
	}
	screen_space_rect os_win32_wrapper::get_window_placement() const {
		WINDOWPLACEMENT placement;
		placement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement((HWND)(m_hwnd), &placement);

		return screen_space_rect{ placement.rcNormalPosition.left, placement.rcNormalPosition.top, placement.rcNormalPosition.right - placement.rcNormalPosition.left, placement.rcNormalPosition.bottom - placement.rcNormalPosition.top };
	}
	void os_win32_wrapper::set_window_placement(screen_space_rect r) {

		WINDOWPLACEMENT placement;
		placement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement((HWND)(m_hwnd), &placement);

		RECT new_normal;

		new_normal.left = r.x;
		new_normal.top = r.y;
		new_normal.right = r.x + r.width;
		new_normal.bottom = r.y + r.height;
		placement.rcNormalPosition = new_normal;

		SetWindowPlacement((HWND)(m_hwnd), &placement);
	}


	void os_win32_wrapper::create_system_caret(int32_t width, int32_t height) {
		CreateCaret(m_hwnd, nullptr,  width, height);
	}
	void os_win32_wrapper::move_system_caret(int32_t x, int32_t y) {
		SetCaretPos(x, y);
	}
	void os_win32_wrapper::destroy_system_caret() {
		DestroyCaret();
	}

	void os_win32_wrapper::hide_mouse_cursor() {
		SetCursor(nullptr);
		cursor_visible = false;
	}
	void os_win32_wrapper::show_mouse_cursor() {
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
		cursor_visible = true;
	}
	bool os_win32_wrapper::is_mouse_cursor_visible() const {
		return cursor_visible;
	}
	void os_win32_wrapper::reshow_mouse_cursor() {
		if(cursor_visible) {
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
		} else {
			SetCursor(nullptr);
		}
	}

	void os_win32_wrapper::move_window(screen_space_rect r) {
		SetWindowPos((HWND)(m_hwnd), nullptr, r.x, r.y, r.width, r.height, SWP_NOZORDER | SWP_NOACTIVATE);
	}

	int32_t os_win32_wrapper::get_key_state(uint32_t scan_code) const {
		return GetKeyState(MapVirtualKey(scan_code, MAPVK_VSC_TO_VK)) & ~1;
	}

	bool os_win32_wrapper::is_shift_held_down() const {
		return (GetKeyState(VK_SHIFT) & 0x80) != 0;
		
	}
	bool os_win32_wrapper::is_ctrl_held_down() const {
		return (GetKeyState(VK_CONTROL) & 0x80) != 0;
	}

	uint32_t os_win32_wrapper::get_window_dpi() const {
		return GetDpiForWindow((HWND)(m_hwnd));
	}

	void os_win32_wrapper::text_to_clipboard(std::wstring_view txt) {
		if(OpenClipboard(m_hwnd)) {
			if(EmptyClipboard()) {
				size_t byteSize = sizeof(wchar_t) * (txt.length() + 1);
				HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE | GMEM_ZEROINIT, byteSize);

				if(hClipboardData != nullptr) {
					void* memory = GlobalLock(hClipboardData);  // [byteSize] in bytes

					if(memory != nullptr) {
						memcpy(memory, txt.data(), byteSize);
						GlobalUnlock(hClipboardData);

						if(SetClipboardData(CF_UNICODETEXT, hClipboardData) != nullptr) {
							hClipboardData = nullptr; // system now owns the clipboard, so don't touch it.
						}
					}
					GlobalFree(hClipboardData); // free if failed
				}
			}
			CloseClipboard();
		}
	}
	std::wstring os_win32_wrapper::text_from_clipboard() {
		std::wstring return_value;
		if(OpenClipboard(m_hwnd)) {
			HGLOBAL hClipboardData = GetClipboardData(CF_UNICODETEXT);

			if(hClipboardData != NULL) {
				size_t byteSize = GlobalSize(hClipboardData);
				void* memory = GlobalLock(hClipboardData);
				if(memory != NULL) {
					const wchar_t* text = reinterpret_cast<const wchar_t*>(memory);
					return_value = std::wstring(text, text + byteSize / sizeof(wchar_t));
					GlobalUnlock(hClipboardData);
					if(return_value.length() > 0 && return_value.back() == 0) {
						return_value.pop_back();
					}
				}
			}
			CloseClipboard();
		}
		return return_value;
	}


	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	bool os_win32_wrapper::create_window(window_data& win) {

		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_OWNDC | CS_DBLCLKS;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = GetModuleHandle(nullptr);
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.hCursor = nullptr;
		wcex.lpszClassName = L"printui_class";

		if(RegisterClassEx(&wcex) == 0) {
			std::abort();
		}

		m_hwnd = CreateWindowEx(
			0,
			L"printui_class",
			L"",
			WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			NULL,
			NULL,
			GetModuleHandle(nullptr),
			&win
		);

		if(m_hwnd) {
			win.double_click_ms = GetDoubleClickTime();
			win.caret_blink_ms = GetCaretBlinkTime() * 2;
			if(GetCaretBlinkTime() == INFINITE)
				win.dynamic_settings.caret_blink = false;

			win.last_double_click = std::chrono::steady_clock::now();

			if(win.dynamic_settings.imode == printui::input_mode::mouse_and_keyboard
				|| win.dynamic_settings.imode == printui::input_mode::mouse_only
				|| (win.dynamic_settings.imode == printui::input_mode::follow_input && win.prompts == prompt_mode::hidden)
				|| win.dynamic_settings.imode == printui::input_mode::controller_with_pointer) {

				SetCursor(LoadCursor(NULL, IDC_ARROW));
				cursor_visible = true;
			} else {
				cursor_visible = false;
			}

			win.dpi = float(GetDpiForWindow((HWND)(m_hwnd)));

			SetWindowPos(
				(HWND)(m_hwnd),
				NULL,
				NULL,
				NULL,
				static_cast<int>(ceil(float(win.dynamic_settings.window_x_size) * win.dpi * win.dynamic_settings.global_size_multiplier / 96.f)),
				static_cast<int>(ceil(float(win.dynamic_settings.window_y_size) * win.dpi * win.dynamic_settings.global_size_multiplier / 96.f)),
				SWP_NOMOVE | SWP_FRAMECHANGED);

			win.file_system.load_settings(win);

			auto window_title_id = win.text_data.text_id_from_name("window_title").id;
			win.title_bar.set_text_content(uint16_t(window_title_id));

			ShowWindow((HWND)(m_hwnd), SW_SHOWNORMAL);
			UpdateWindow((HWND)(m_hwnd));

			if(UiaHasServerSideProvider(m_hwnd))
				OutputDebugStringA("provider found\n"); // this was done to force the root window provider to load early

			RAWINPUTDEVICE deviceList[2];
			deviceList[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
			deviceList[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;
			deviceList[0].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
			deviceList[0].hwndTarget = m_hwnd;
			deviceList[1] = deviceList[0];
			deviceList[1].usUsage = HID_USAGE_GENERIC_JOYSTICK;
			RegisterRawInputDevices(deviceList, 2, sizeof(RAWINPUTDEVICE));

			return true;
		}
		return false;
	}

	screen_space_rect os_win32_wrapper::get_window_location() const {
		RECT rcClient;
		GetWindowRect((HWND)(m_hwnd), &rcClient);
		return screen_space_rect{ rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top };
	}

	void os_win32_wrapper::display_fatal_error_message(wchar_t const* msg) {
		MessageBox((HWND)(m_hwnd), msg, L"Fatal Error", MB_OK | MB_ICONERROR);
	}

	bool os_win32_wrapper::window_has_focus() const {
		return GetForegroundWindow() == (HWND)(m_hwnd);
	}
	bool os_win32_wrapper::is_maximized() const {
		return IsZoomed((HWND)(m_hwnd));
	}
	bool os_win32_wrapper::is_minimized() const {
		return IsIconic((HWND)(m_hwnd));
	}
	void os_win32_wrapper::maximize(window_data& win) {
		ShowWindow((HWND)(m_hwnd), SW_MAXIMIZE);
		win.accessibility_interface.notify_window_state_change(resize_type::maximize);
	}
	void os_win32_wrapper::minimize(window_data& win) {
		ShowWindow((HWND)(m_hwnd), SW_MINIMIZE);
		win.accessibility_interface.notify_window_state_change(resize_type::minimize);
	}
	void os_win32_wrapper::restore(window_data& win) {
		ShowWindow((HWND)(m_hwnd), SW_SHOWNORMAL);
		win.accessibility_interface.notify_window_state_change(resize_type::resize);
	}
	void os_win32_wrapper::close(window_data& win) {
		win.accessibility_interface.notify_window_closed();
		PostMessage((HWND)(m_hwnd), WM_CLOSE, 0, 0);
	}
	
	void os_win32_wrapper::set_window_title(wchar_t const* t) {
		SetWindowText((HWND)(m_hwnd), t);
	}


	window_data::window_data(bool mn, bool mx, bool settings) : window_bar(*this, mn, mx, settings, get_settings_items()), accessibility_interface(*this) {
		dynamic_settings.keys.type = keyboard_type::left_hand;
		populate_key_mappings_by_type(dynamic_settings.keys);
		register_icons();
	}

	ui_rectangle const* interface_under_point(window_data const& win, std::vector<ui_rectangle> const& rects, int32_t x, int32_t y, bool ignore_overlay) {
		ui_rectangle const* found = nullptr;
		for(auto& r : rects) {
			auto test_rect = render::extend_rect_to_edges(r, win);

			if(x >= test_rect.x && x <= test_rect.x + test_rect.width && y >= test_rect.y && y <= test_rect.y + test_rect.height) {
				if((r.display_flags & ui_rectangle::flag_clear_rect) != 0) {
					found = nullptr;
				} else if((r.display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					return found;
				} else if(!ignore_overlay || (r.display_flags & ui_rectangle::flag_overlay) == 0) {
					if(r.parent_object.get_render_interface())
						found = &r;
				}
			}
		}
		return found;
	}
	ui_reference reference_under_point(window_data const& win, std::vector<ui_rectangle> const& rects, int32_t x, int32_t y, bool ignore_overlay) {
		ui_reference found = ui_reference_none;
		for(ui_reference i = 0; i < rects.size(); ++i) {
			auto test_rect = render::extend_rect_to_edges(rects[i], win);
			if(x >= test_rect.x && x <= test_rect.x + test_rect.width && y >= test_rect.y && y <= test_rect.y + test_rect.height) {
				if((rects[i].display_flags & ui_rectangle::flag_clear_rect) != 0) {
					found = ui_reference_none;
				} else if((rects[i].display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					return found;
				} else if( (!ignore_overlay || (rects[i].display_flags & ui_rectangle::flag_overlay) == 0) &&
					((rects[i].display_flags & ui_rectangle::flag_frame) == 0 || rects[i].parent_object.get_render_interface() != nullptr)) {
					found = i;
				}
			}
		}
		return found;
	}
	layout_reference layout_reference_under_point(window_data const& win, std::vector<ui_rectangle> const& rects, int32_t x, int32_t y, bool ignore_overlay) {
		layout_reference found = layout_reference_none;
		for(layout_reference i = 0; i < rects.size(); ++i) {

			auto test_rect = render::extend_rect_to_edges(rects[i], win);
			if(x >= test_rect.x && x <= test_rect.x + test_rect.width && y >= test_rect.y && y <= test_rect.y + test_rect.height) {

				if((rects[i].display_flags & ui_rectangle::flag_clear_rect) != 0) {
					found = layout_reference_none;
				} else if((rects[i].display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					return found;
				} else if((rects[i].display_flags & ui_rectangle::flag_frame) != 0 && rects[i].parent_object.get_render_interface() == nullptr) {
				} else if(!ignore_overlay || (rects[i].display_flags & ui_rectangle::flag_overlay) == 0) {
					found = rects[i].parent_object.get_layout_reference();
				}
			}
		}
		return found;
	}

	window_data::~window_data() {
		if(dynamic_settings.settings_changed) {
			file_system.save_settings(*this);
			dynamic_settings.settings_changed = false;
		}

	}


	void window_data::on_dpi_change() {
		layout_size = int32_t(std::round(dynamic_settings.global_size_multiplier * float(dynamic_settings.layout_base_size) * dpi / 96.0f));
		window_border = int32_t(std::round(float(dynamic_settings.window_border) * dpi / 96.0f));

		rendering_interface.stop_ui_animations(*this);
		
		rendering_interface.recreate_dpi_dependent_resource(*this);
		rendering_interface.create_window_size_resources(*this);
		text_interface.initialize_fonts(*this);
		change_orientation(dynamic_settings.preferred_orientation);
		rendering_interface.mark_for_complete_redraw();

		client_on_dpi_change();

		window_interface.invalidate_window();
	}


	void window_data::expand_to_fit_content() {
		auto workspace = window_interface.get_available_workspace();
		auto current_placement = window_interface.get_window_placement();

		screen_space_rect new_placement{ 0 ,0, int32_t(std::max(minimum_ui_width, ui_width)), int32_t(std::max(minimum_ui_height, ui_height)) };

		auto const change_width = new_placement.width - (current_placement.width);
		auto const change_height = new_placement.height - (current_placement.height);
		new_placement.x = std::clamp(current_placement.x - change_width / 2, 0, workspace.width - new_placement.width);
		new_placement.y = std::clamp(current_placement.y - change_height / 2, 0, workspace.height - new_placement.height);

		window_interface.set_window_placement(new_placement);
	}

	bool window_data::on_mouse_move(uint32_t x, uint32_t y) {
		if(dynamic_settings.imode == input_mode::follow_input) {
			if(!window_interface.is_mouse_cursor_visible()) {
				window_interface.show_mouse_cursor();
				set_prompt_visibility(printui::prompt_mode::hidden);
				window_interface.invalidate_window();
			}
		} else if(dynamic_settings.imode != input_mode::mouse_only && dynamic_settings.imode != input_mode::mouse_and_keyboard && dynamic_settings.imode != input_mode::controller_with_pointer) {
			return false;
		}

		last_cursor_x_position = x;
		last_cursor_y_position = y;

		if(keyboard_target && selecting_edit_text != edit_selection_mode::none) {
			keyboard_target->move_cursor_by_screen_pt(*this, screen_space_point{int32_t(x), int32_t(y) }, true);
			return true;
		}

		auto& rects = get_ui_rects();
		auto new_under_cursor = printui::reference_under_point(*this, rects, last_cursor_x_position, last_cursor_y_position, true);

		auto new_l_ref = layout_reference_under_point(*this, rects, last_cursor_x_position, last_cursor_y_position, true);

		set_window_focus_from_mouse(new_l_ref);

		if(last_under_cursor != new_under_cursor) {
			bool last_is_highlighted = last_under_cursor < rects.size() ? (rects[last_under_cursor].display_flags & printui::ui_rectangle::flag_interactable) != 0 : false;

			bool new_is_highlighted = new_under_cursor < rects.size() ? (rects[new_under_cursor].display_flags & printui::ui_rectangle::flag_interactable) != 0 : false;

			last_under_cursor = new_under_cursor;

			if(last_is_highlighted || new_is_highlighted) {
				window_interface.invalidate_window();
			}
		}

		if(info_popup.currently_visible && info_popup.seeking_rect.width > 0) {
			auto margin = int32_t(5 * dpi / 96.0f);
			auto& info_node = get_node(info_popup.l_id);

			if(int32_t(x) < info_popup.seeking_rect.x || int32_t(y) < info_popup.seeking_rect.y || int32_t(x) > info_popup.seeking_rect.x + info_popup.seeking_rect.width || int32_t(y) > info_popup.seeking_rect.y + info_popup.seeking_rect.height) {
				info_popup.close(*this, false);
			} else {
				auto screen_location = screen_rectangle_from_layout(*this, info_node.x, info_node.y, info_node.width, info_node.height);
				if(int32_t(last_cursor_x_position) < screen_location.x) {
					screen_location.width += (screen_location.x - last_cursor_x_position);
					screen_location.x = last_cursor_x_position;
				} else if(int32_t(last_cursor_x_position) > screen_location.x + screen_location.width) {
					screen_location.width = (last_cursor_x_position - screen_location.x);
				}
				if(int32_t(last_cursor_y_position) < screen_location.y) {
					screen_location.height += (screen_location.y - last_cursor_y_position);
					screen_location.y = last_cursor_y_position;
				} else if(int32_t(last_cursor_y_position) > screen_location.y + screen_location.height) {
					screen_location.height = (last_cursor_y_position - screen_location.y);
				}
				screen_location.x -= margin;
				screen_location.y -= margin;
				screen_location.width += margin * 2;
				screen_location.height += margin * 2;

				info_popup.seeking_rect = intersection(info_popup.seeking_rect, screen_location);
			}
		}

		return true;
	}

	bool window_data::on_key_down(uint32_t scan_code, uint32_t key_code) {
		if(capture_key >= 0) {
			if(capture_key < 12) {
				dynamic_settings.keys.main_keys[capture_key].scancode = scan_code;

				WCHAR keyname[64];
				auto length = GetKeyNameTextW((scan_code << 16) | 1ui32, keyname, 64);
				if(length > 0)
					dynamic_settings.keys.main_keys[capture_key].display_name = std::wstring(size_t(1), keyname[0]);
				else
					dynamic_settings.keys.main_keys[capture_key].display_name = L"";

			} else if(capture_key == 12) {
				dynamic_settings.keys.primary_escape.scancode = scan_code;

				WCHAR keyname[64];
				auto length = GetKeyNameTextW((scan_code << 16) | 1ui32, keyname, 64);
				if(length > 0)
					dynamic_settings.keys.primary_escape.display_name = std::wstring(keyname);
				else
					dynamic_settings.keys.primary_escape.display_name = L"";
			} else if(capture_key == 13) {
				dynamic_settings.keys.info_key.scancode = scan_code;

				WCHAR keyname[64];
				auto length = GetKeyNameTextW((scan_code << 16) | 1ui32, keyname, 64);
				if(length > 0)
					dynamic_settings.keys.info_key.display_name = std::wstring(keyname);
				else
					dynamic_settings.keys.info_key.display_name = L"";
			}

			window_bar.print_ui_settings.update_with_keyboard_settings(*this);
			rendering_interface.create_interactiable_tags(*this);
			window_interface.invalidate_window();
			dynamic_settings.settings_changed = true;

			capture_key = -1;
			return true;
		}
		if(keyboard_target && scan_code != secondary_escape_sc) {
			bool shift_held = window_interface.is_shift_held_down();
			bool ctrl_held = window_interface.is_ctrl_held_down();

			if(key_code == VK_RETURN) {
				keyboard_target->command(*this, edit_command::new_line, false);
			} else if(key_code == VK_BACK) {
				if(ctrl_held)
					keyboard_target->command(*this, edit_command::backspace_word, false);
				else
					keyboard_target->command(*this, edit_command::backspace, false);
			} else if(key_code == VK_DELETE) {
				if(ctrl_held)
					keyboard_target->command(*this, edit_command::delete_word, false);
				else
					keyboard_target->command(*this, edit_command::delete_char, false);
			} else if(key_code == VK_TAB) {
				keyboard_target->command(*this, edit_command::tab, false);
			} else if(key_code == VK_LEFT) {
				if(horizontal(orientation)) {
					if(ctrl_held) {
						keyboard_target->command(*this, edit_command::cursor_left_word, shift_held);
					} else {
						keyboard_target->command(*this, edit_command::cursor_left, shift_held);
					}
				} else if(orientation == layout_orientation::vertical_left_to_right) {
					keyboard_target->command(*this, edit_command::cursor_up, shift_held);
				} else {
					keyboard_target->command(*this, edit_command::cursor_down, shift_held);
				}
			} else if(key_code == VK_RIGHT) {
				if(horizontal(orientation)) {
					if(ctrl_held)
						keyboard_target->command(*this, edit_command::cursor_right_word, shift_held);
					else
						keyboard_target->command(*this, edit_command::cursor_right, shift_held);
				} else if(orientation == layout_orientation::vertical_left_to_right) {
					keyboard_target->command(*this, edit_command::cursor_down, shift_held);
				} else {
					keyboard_target->command(*this, edit_command::cursor_up, shift_held);
				}
			} else if(key_code == VK_UP) {
				if(horizontal(orientation)) {
					keyboard_target->command(*this, edit_command::cursor_up, shift_held);
				} else {
					if(ctrl_held) {
						keyboard_target->command(*this, edit_command::cursor_left_word, shift_held);
					} else {
						keyboard_target->command(*this, edit_command::cursor_left, shift_held);
					}
				}
			} else if(key_code == VK_DOWN) {
				if(horizontal(orientation)) {
					keyboard_target->command(*this, edit_command::cursor_down, shift_held);
				} else {
					if(ctrl_held) {
						keyboard_target->command(*this, edit_command::cursor_right_word, shift_held);
					} else {
						keyboard_target->command(*this, edit_command::cursor_right, shift_held);
					}
				}
			} else if(key_code == VK_HOME) {
				if(ctrl_held)
					keyboard_target->command(*this, edit_command::to_text_start, shift_held);
				else
					keyboard_target->command(*this, edit_command::to_line_start, shift_held);
			} else if(key_code == VK_END) {
				if(ctrl_held)
					keyboard_target->command(*this, edit_command::to_text_end, shift_held);
				else
					keyboard_target->command(*this, edit_command::to_line_end, shift_held);
			} else if(key_code == VK_PRIOR) {
				keyboard_target->command(*this, edit_command::to_text_start, shift_held);
			} else if(key_code == VK_NEXT) {
				keyboard_target->command(*this, edit_command::to_text_end, shift_held);
			} else if(key_code == VK_INSERT) {
				if(ctrl_held)
					keyboard_target->command(*this, edit_command::copy, false);
				else if(shift_held)
					keyboard_target->command(*this, edit_command::paste, false);
			} else if(key_code == 'A') {
				if(ctrl_held) {
					keyboard_target->command(*this, edit_command::select_all, false);
				}
			} else if(key_code == 'C') {
				if(ctrl_held) {
					keyboard_target->command(*this, edit_command::copy, false);
				}
			} else if(key_code == 'X') {
				if(ctrl_held) {
					keyboard_target->command(*this, edit_command::cut, false);
				}
			} else if(key_code == 'V') {
				if(ctrl_held) {
					keyboard_target->command(*this, edit_command::paste, false);
				}
			} else if(key_code == 'Z') {
				if(ctrl_held) {
					keyboard_target->command(*this, edit_command::undo, false);
				}
			} else if(key_code == 'Y') {
				if(ctrl_held) {
					keyboard_target->command(*this, edit_command::redo, false);
				}
			}
			info_popup.close(*this, false);
			return true;
		}

		if(scan_code == dynamic_settings.keys.info_key.scancode) {
			if(dynamic_settings.keys.info_key_is_sticky)
				pending_right_click = !pending_right_click;
			else
				pending_right_click = true;
			window_bar.info_i.mark_for_update(*this);
			return true;
		}

		if(dynamic_settings.imode == input_mode::follow_input) {
			if(window_interface.is_mouse_cursor_visible() || prompts != printui::prompt_mode::keyboard) {
				window_interface.hide_mouse_cursor();
				set_prompt_visibility(printui::prompt_mode::keyboard);
				set_window_focus(nullptr);
				window_interface.invalidate_window();
				return true; // eat the key so that we don't do something bad
			}

		} else if(dynamic_settings.imode != input_mode::keyboard_only && dynamic_settings.imode != input_mode::mouse_and_keyboard) {
			return false;
		}

		if(scan_code == dynamic_settings.keys.primary_escape.scancode || scan_code == secondary_escape_sc) {
			// go up
			if(keyboard_target) {
				set_keyboard_focus(nullptr);
			} else {
				execute_focus_action(focus_actions.escape);
			}
		} else {
			for(int32_t i = 0; i < 12; i++) {
				if(dynamic_settings.keys.main_keys[i].scancode == scan_code) {
					// do something with ith key
					execute_focus_action(focus_actions.button_actions[i]);
					break;
				}
			}
		}
		info_popup.close(*this, false);
		return true;
	}

	void window_data::on_device_change(uint64_t /*status*/, void* /*handle*/) {
		auto old_number = controller_number_plugged_in;
		controller_number_plugged_in = 0;

		for(uint32_t i = 0; i < 4; ++i) {
			XINPUT_STATE state;
			ZeroMemory(&state, sizeof(XINPUT_STATE));

			if(XInputGetState(i, &state) == ERROR_SUCCESS) {
				controller_number_plugged_in |= (1 << i);
			}
		}
		if(controller_number_plugged_in != old_number) {
			if(auto p = window_bar.print_ui_settings.input_mode_list.get_option(size_t(input_mode::controller_with_pointer)); p)
				p->set_disabled(*this, controller_number_plugged_in == 0);
			if(auto p = window_bar.print_ui_settings.input_mode_list.get_option(size_t(input_mode::controller_only)); p)
				p->set_disabled(*this, controller_number_plugged_in == 0);
			window_interface.invalidate_window();
		}

		/* -- FOR ACTUAL HID CONTROLLERS 
		
		WCHAR deviceName[1024] = { 0 };
		UINT deviceNameLength = 1024;
		bool gotName = GetRawInputDeviceInfo((HANDLE)handle, RIDI_DEVICENAME, deviceName, &deviceNameLength) > 0;
		

		if(gotName) {
			if(status == GIDC_ARRIVAL) {
				// if we handled HID joysticks
			} else if(status == GIDC_REMOVAL) {
				// if we handled HID joysticks
			}
		}
		*/
	}

	

	uint32_t to_bitmask(controller_button b) {
		switch(b) {
			case controller_button::y:
				return controller_button_state::button_y;
			case controller_button::x:
				return controller_button_state::button_x;
			case controller_button::b:
				return controller_button_state::button_b;
			case controller_button::a:
				return controller_button_state::button_a;
			case controller_button::lb:
				return controller_button_state::button_lb;
			case controller_button::rb:
				return controller_button_state::button_rb;
			case controller_button::start:
				return controller_button_state::button_start;
			case controller_button::back:
				return controller_button_state::button_back;
			case controller_button::tleft:
				return controller_button_state::thumb_left;
			case controller_button::tright:
				return controller_button_state::thumb_right;
			case controller_button::dup:
				return controller_button_state::dpad_up;
			case controller_button::ddown:
				return controller_button_state::dpad_down;
			case controller_button::dleft:
				return controller_button_state::dpad_left;
			case controller_button::dright:
				return controller_button_state::dpad_right;
			case controller_button::no_button:
				return 0;
			default:
				return 0;
		}
	}

	uint32_t to_xinput_bitmask(controller_button b) {
		switch(b) {
			case controller_button::y:
				return XINPUT_GAMEPAD_Y;
			case controller_button::x:
				return XINPUT_GAMEPAD_X;
			case controller_button::b:
				return XINPUT_GAMEPAD_B;
			case controller_button::a:
				return XINPUT_GAMEPAD_A;
			case controller_button::lb:
				return XINPUT_GAMEPAD_LEFT_SHOULDER;
			case controller_button::rb:
				return XINPUT_GAMEPAD_RIGHT_SHOULDER;
			case controller_button::start:
				return XINPUT_GAMEPAD_START;
			case controller_button::back:
				return XINPUT_GAMEPAD_BACK;
			case controller_button::tleft:
				return XINPUT_GAMEPAD_LEFT_THUMB;
			case controller_button::tright:
				return XINPUT_GAMEPAD_RIGHT_THUMB;
			case controller_button::dup:
				return XINPUT_GAMEPAD_DPAD_UP;
			case controller_button::ddown:
				return XINPUT_GAMEPAD_DPAD_DOWN;
			case controller_button::dleft:
				return XINPUT_GAMEPAD_DPAD_LEFT;
			case controller_button::dright:
				return XINPUT_GAMEPAD_DPAD_RIGHT;
			case controller_button::no_button:
				return 0;
			default:
				return 0;
		}
	}

	void test_for_button(uint32_t& new_buttons_pressed, uint32_t& new_buttons_released, uint32_t& button_state, WORD gamepadstate, controller_button b) {

		if((gamepadstate & to_xinput_bitmask(b)) != 0) {
			if((button_state & to_bitmask(b)) == 0) {
				button_state |= to_bitmask(b);
				new_buttons_pressed |= to_bitmask(b);
			}
		} else {
			if((button_state & to_bitmask(b)) != 0) {
				button_state &= ~to_bitmask(b);
				new_buttons_released |= to_bitmask(b);
			}
		}
	}

	void test_for_all_buttons(uint32_t& buttons_pressed, uint32_t& buttons_released, uint32_t& button_state, WORD gamepadstate) {

		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::lb);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::rb);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::y);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::x);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::b);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::a);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::tleft);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::tright);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::start);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::back);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::dleft);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::dright);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::dup);
		test_for_button(buttons_pressed, buttons_released, button_state, gamepadstate, controller_button::ddown);
	}

	bool window_data::controller_info_held_down() {
		if(dynamic_settings.controller.info.second_button == controller_button::no_button) {
			if((controller_buttons.val & to_bitmask(dynamic_settings.controller.info.first_button)) != 0) {
				return true;
			}
		} else {
			if(
				(controller_buttons.val & to_bitmask(dynamic_settings.controller.info.first_button)) != 0 &&
				(controller_buttons.val & to_bitmask(dynamic_settings.controller.info.second_button)) != 0) { // both are held down

				return true;
			}
			
		}
		return false;
	}
	void window_data::reset_controller_info_keys() {
		controller_sticky_buttons.val &= ~(to_bitmask(dynamic_settings.controller.info.first_button) | to_bitmask(dynamic_settings.controller.info.second_button));
	}

	void window_data::on_controller_input() {

		//capture test
		if(capture_button >= 0) {

			for(int32_t i = 0; i < 4; ++i) {
				if(((1 << i) & controller_number_plugged_in) != 0) {
					XINPUT_STATE state;
					ZeroMemory(&state, sizeof(XINPUT_STATE));

					if(XInputGetState(i, &state) == ERROR_SUCCESS) {
						uint32_t buttons_pressed = 0;
						uint32_t buttons_released = 0;
						test_for_all_buttons(buttons_pressed, buttons_released, controller_buttons.val, state.Gamepad.wButtons);

						controller_button captured = controller_button::no_button;
						if((buttons_pressed & controller_button_state::button_y) != 0)
							captured = controller_button::y;
						else if((buttons_pressed & controller_button_state::button_x) != 0)
							captured = controller_button::x;
						else if((buttons_pressed & controller_button_state::button_a) != 0)
							captured = controller_button::a;
						else if((buttons_pressed & controller_button_state::button_b) != 0)
							captured = controller_button::b;
						else if((buttons_pressed & controller_button_state::button_lb) != 0)
							captured = controller_button::lb;
						else if((buttons_pressed & controller_button_state::button_rb) != 0)
							captured = controller_button::rb;
						else if((buttons_pressed & controller_button_state::button_start) != 0)
							captured = controller_button::start;
						else if((buttons_pressed & controller_button_state::button_back) != 0)
							captured = controller_button::back;
						else if((buttons_pressed & controller_button_state::dpad_down) != 0)
							captured = controller_button::ddown;
						else if((buttons_pressed & controller_button_state::dpad_left) != 0)
							captured = controller_button::dleft;
						else if((buttons_pressed & controller_button_state::dpad_right) != 0)
							captured = controller_button::dright;
						else if((buttons_pressed & controller_button_state::dpad_up) != 0)
							captured = controller_button::dup;
						else if((buttons_pressed & controller_button_state::thumb_left) != 0)
							captured = controller_button::tleft;
						else if((buttons_pressed & controller_button_state::thumb_right) != 0)
							captured = controller_button::tright;

						if(captured != controller_button::no_button) {
							switch(capture_button) {
								case 1:
									dynamic_settings.controller.button1 = captured;
									break;
								case 2:
									dynamic_settings.controller.button2 = captured;
									break;
								case 3:
									dynamic_settings.controller.button3 = captured;
									break;
								case 4:
									dynamic_settings.controller.button4 = captured;
									break;
								case 5:
									dynamic_settings.controller.first_group = captured;
									break;
								case 6:
									dynamic_settings.controller.second_group = captured;
									break;
								case 7:
									dynamic_settings.controller.escape.first_button = captured;
									if(dynamic_settings.controller.escape.first_button == dynamic_settings.controller.escape.second_button)
										dynamic_settings.controller.escape.second_button = controller_button::no_button;
									break;
								case 8:
									dynamic_settings.controller.escape.second_button = captured;
									if(dynamic_settings.controller.escape.first_button == dynamic_settings.controller.escape.second_button)
										dynamic_settings.controller.escape.second_button = controller_button::no_button;
									break;
								case 9:
									dynamic_settings.controller.info.first_button = captured;
									if(dynamic_settings.controller.info.first_button == dynamic_settings.controller.info.second_button)
										dynamic_settings.controller.info.second_button = controller_button::no_button;
									break;
								case 10:
									dynamic_settings.controller.info.second_button = captured;
									if(dynamic_settings.controller.info.first_button == dynamic_settings.controller.info.second_button)
										dynamic_settings.controller.info.second_button = controller_button::no_button;
									
									break;
							}
							window_bar.print_ui_settings.update_with_controller_settings(*this);
							rendering_interface.create_interactiable_tags(*this);
							capture_button = -1;
						}
					}
				}
			}
			return;
		}

		// end capture test

		if(dynamic_settings.imode == input_mode::follow_input) {
			// switch below
		} else if(dynamic_settings.imode != input_mode::controller_only && dynamic_settings.imode != input_mode::controller_with_pointer) {
			return;
		}

		for(int32_t i = 0; i < 4; ++i) {
			if(((1 << i) & controller_number_plugged_in) != 0) {
				XINPUT_STATE state;
				ZeroMemory(&state, sizeof(XINPUT_STATE));

				if(XInputGetState(i, &state) == ERROR_SUCCESS) {
					// Controller is connected
					

					if(dynamic_settings.imode == input_mode::controller_with_pointer && (i == controller_mouse.for_controller || !controller_mouse.running)) {
						double LX = dynamic_settings.controller.left_thumbstick ? state.Gamepad.sThumbLX : state.Gamepad.sThumbRX;
						double LY = dynamic_settings.controller.left_thumbstick ? state.Gamepad.sThumbLY : state.Gamepad.sThumbRY;

						//determine how far the controller is pushed
						double magnitude = sqrt(LX * LX + LY * LY);
						double norm_lx = LX / magnitude;
						double norm_ly = -LY / magnitude;
						magnitude = std::min(magnitude, 32767.0) / 32767.0;

						//check if the controller is outside a circular dead zone
						if(magnitude >= dynamic_settings.controller.deadzone) {
							if(!controller_mouse.running) {
								controller_mouse.start_time = std::chrono::steady_clock::now();
								controller_mouse.running = true;
								controller_mouse.for_controller = i;

								POINT p{};
								GetCursorPos(&p);

								controller_mouse.current_x = p.x;
								controller_mouse.current_y = p.y;
							} else {
								auto new_time = std::chrono::steady_clock::now();
								auto duration = new_time - controller_mouse.start_time;
								controller_mouse.start_time = new_time;
								auto in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

								auto scaled_magnitude = (magnitude * magnitude * magnitude) * 1.5;
								controller_mouse.current_x += double(dynamic_settings.controller.sensitivity * in_ms.count()) * norm_lx * scaled_magnitude;
								controller_mouse.current_y += double(dynamic_settings.controller.sensitivity * in_ms.count()) * norm_ly * scaled_magnitude;

								auto winrect = window_interface.get_window_location();
								auto temp_x = int32_t(controller_mouse.current_x);
								auto temp_y = int32_t(controller_mouse.current_y);
								temp_x = std::clamp(temp_x, winrect.x + window_border, winrect.x + winrect.width - window_border);
								temp_y = std::clamp(temp_y, winrect.y + window_border, winrect.y + winrect.height - window_border);

								SetCursorPos(temp_x, temp_y);
							}

							
						} else {
							controller_mouse.running = false;
						}
					}

					uint32_t buttons_pressed = 0;
					uint32_t buttons_released = 0;

					test_for_all_buttons(buttons_pressed, buttons_released, controller_buttons.val, state.Gamepad.wButtons);

					if(buttons_pressed || buttons_released) {
						if(dynamic_settings.imode == input_mode::follow_input) {
							if(window_interface.is_mouse_cursor_visible() || prompts != printui::prompt_mode::controller) {
								window_interface.hide_mouse_cursor();
								set_prompt_visibility(printui::prompt_mode::controller);
								set_window_focus(nullptr);
								window_interface.invalidate_window();
								return; // eat the key so that we don't do something bad
							}
						}

						controller_sticky_buttons.val ^= buttons_pressed;

						if((buttons_released & (to_bitmask(dynamic_settings.controller.first_group) | to_bitmask(dynamic_settings.controller.second_group))) != 0) {
							window_interface.invalidate_window();
						}
						if((buttons_pressed & (to_bitmask(dynamic_settings.controller.first_group) | to_bitmask(dynamic_settings.controller.second_group))) != 0) {
							window_interface.invalidate_window();
						}

						if(dynamic_settings.controller.escape.second_button == controller_button::no_button) {
							if((buttons_pressed & to_bitmask(dynamic_settings.controller.escape.first_button)) != 0) {
								execute_focus_action(focus_actions.escape);
							}
						} else {
							if(dynamic_settings.controller.escape.sticky) {
								if((buttons_pressed & to_bitmask(dynamic_settings.controller.escape.first_button)) != 0 ||
									(buttons_pressed & to_bitmask(dynamic_settings.controller.escape.second_button)) != 0) { // at least one has just been pushed
									if(
										(controller_sticky_buttons.val & to_bitmask(dynamic_settings.controller.escape.first_button)) != 0
										&& (controller_sticky_buttons.val & to_bitmask(dynamic_settings.controller.escape.second_button)) != 0) { // both are sticky down

										controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.escape.first_button);
										controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.escape.second_button);
										execute_focus_action(focus_actions.escape);
									}
								}
							} else {
								if((buttons_pressed & to_bitmask(dynamic_settings.controller.escape.first_button)) != 0 ||
									(buttons_pressed & to_bitmask(dynamic_settings.controller.escape.second_button)) != 0) { // at least one has just been pushed
									if(
										(controller_buttons.val & to_bitmask(dynamic_settings.controller.escape.first_button)) != 0
										&& (controller_buttons.val & to_bitmask(dynamic_settings.controller.escape.second_button)) != 0) { // both are held down

										execute_focus_action(focus_actions.escape);
									}
								}
							}
						}

						if(dynamic_settings.controller.info.second_button == controller_button::no_button) {
							if(dynamic_settings.controller.info.sticky) {
								if((buttons_pressed & to_bitmask(dynamic_settings.controller.info.first_button)) != 0) {
									if((controller_sticky_buttons.val & to_bitmask(dynamic_settings.controller.info.first_button)) != 0) {
										pending_right_click = true;
									} else {
										pending_right_click = false;
									}
									window_interface.invalidate_window();
								}
							} else {
								if((buttons_released & to_bitmask(dynamic_settings.controller.info.first_button)) != 0) {
									pending_right_click = false;
									window_interface.invalidate_window();
								}
								if((buttons_pressed & to_bitmask(dynamic_settings.controller.info.first_button)) != 0) {
									pending_right_click = true;
									window_interface.invalidate_window();
								}
							}
						} else {
							if(dynamic_settings.controller.info.sticky) {
								if((buttons_pressed & to_bitmask(dynamic_settings.controller.info.first_button)) != 0 || (buttons_pressed & to_bitmask(dynamic_settings.controller.info.second_button)) != 0) {

									if(
										(controller_sticky_buttons.val & to_bitmask(dynamic_settings.controller.info.first_button)) != 0 &&
										(controller_sticky_buttons.val & to_bitmask(dynamic_settings.controller.info.second_button)) != 0) { // both are held down

										pending_right_click = true;
									} else {
										pending_right_click = false;
									}
								
									window_interface.invalidate_window();
								}
							} else {
								if((buttons_pressed & to_bitmask(dynamic_settings.controller.info.first_button)) != 0 || (buttons_pressed & to_bitmask(dynamic_settings.controller.info.second_button)) != 0 || (buttons_released & to_bitmask(dynamic_settings.controller.info.first_button)) != 0 || (buttons_released & to_bitmask(dynamic_settings.controller.info.second_button)) != 0) {
									if(
										(controller_buttons.val & to_bitmask(dynamic_settings.controller.info.first_button)) != 0 &&
										(controller_buttons.val & to_bitmask(dynamic_settings.controller.info.second_button)) != 0) { // both are held down

										pending_right_click = true;
									} else {
										pending_right_click = false;
									}
									window_interface.invalidate_window();
								}
							}
						}

						int32_t action_offset = 0;
						if(((dynamic_settings.controller.first_group_sticky ? controller_sticky_buttons.val : controller_buttons.val) & to_bitmask(dynamic_settings.controller.first_group)) != 0) {
							action_offset = 4;
						} else if(((dynamic_settings.controller.second_group_sticky ? controller_sticky_buttons.val : controller_buttons.val) & to_bitmask(dynamic_settings.controller.second_group)) != 0) {
							action_offset = 8;
						}

						if((buttons_pressed & to_bitmask(dynamic_settings.controller.button1)) != 0) {
							controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.first_group);
							controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.second_group);
							execute_focus_action(focus_actions.button_actions[action_offset + 0]);
						} else if((buttons_pressed & to_bitmask(dynamic_settings.controller.button2)) != 0) {
							controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.first_group);
							controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.second_group);
							execute_focus_action(focus_actions.button_actions[action_offset + 1]);
						} else if((buttons_pressed & to_bitmask(dynamic_settings.controller.button3)) != 0) {
							controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.first_group);
							controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.second_group);
							execute_focus_action(focus_actions.button_actions[action_offset + 2]);
						} else if((buttons_pressed & to_bitmask(dynamic_settings.controller.button4)) != 0) {
							controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.first_group);
							controller_sticky_buttons.val &= ~to_bitmask(dynamic_settings.controller.second_group);
							execute_focus_action(focus_actions.button_actions[action_offset + 3]);
						}

						return; // only process one controller
					} // end if some button was pressed
				} else { // else -- controller could not be read, stop trying it
					controller_number_plugged_in &= ~(1 << i);
				}
			}
		} // end loop over xinput controllers
	}

	bool window_data::on_key_up(uint32_t scan_code, uint32_t /*key_code*/) {
		if(scan_code == dynamic_settings.keys.info_key.scancode) {
			if(!dynamic_settings.keys.info_key_is_sticky) {
				pending_right_click = false;
				window_bar.info_i.mark_for_update(*this);
			}
			return true;
		}

		if(dynamic_settings.imode == input_mode::follow_input) {

		} else if(dynamic_settings.imode != input_mode::keyboard_only && dynamic_settings.imode != input_mode::mouse_and_keyboard) {
			return false;
		}

		return true;
	}

	bool window_data::on_mouse_wheel(int32_t quantity) {
		info_popup.close(*this, false);

		if(keyboard_target) {
			if(quantity > 20) {
				keyboard_target->command(*this, edit_command::cursor_up, window_interface.is_shift_held_down());
			} else if(quantity < -20) {
				keyboard_target->command(*this, edit_command::cursor_down, window_interface.is_shift_held_down());
			}
			return true;
		}

		auto under_cursor = printui::layout_reference_under_point(*this, get_ui_rects(), last_cursor_x_position, last_cursor_y_position, true);
		if(under_cursor != layout_reference_none) {
			auto page_container = get_node(under_cursor).page_info() == nullptr || get_node(under_cursor).page_info()->header == layout_reference_none ? get_containing_proper_page(under_cursor) : under_cursor;
			if(page_container != layout_reference_none) {
				auto li = get_node(page_container).l_interface;
				auto pi = get_node(page_container).page_info();
				if(li) {
					auto sp = pi->subpage_offset;
					if(quantity > 20) {
						if(sp > 0)
							--sp;
					} else if(quantity < -20) {
						if(sp < pi->subpage_divisions.size()) {
							++sp;
						}
					}

					if(sp != pi->subpage_offset) {
						if((sp > pi->subpage_offset ? page_footer::page_turn_up.type : page_footer::page_turn_down.type) != animation_type::none)
							rendering_interface.prepare_layered_ui_animation(*this);
						auto old_page = pi->subpage_offset;

						li->go_to_page(*this, sp, *pi);
						redraw_ui();

						page_footer::start_page_turn_animation(*this, page_container, sp > old_page);
					}
					return true;
				}
			}
		}
		return false;
	}

	bool window_data::on_mouse_left_down(uint32_t x, uint32_t y) {
		if(dynamic_settings.imode == input_mode::follow_input) {
			if(!window_interface.is_mouse_cursor_visible()) {
				window_interface.show_mouse_cursor();
				set_prompt_visibility(printui::prompt_mode::hidden);
				window_interface.invalidate_window();
			}
		} else if(dynamic_settings.imode != input_mode::mouse_only && dynamic_settings.imode != input_mode::mouse_and_keyboard) {
			return false;
		}

		if(keyboard_target) {
			if(keyboard_target->move_cursor_by_screen_pt(*this, screen_space_point{ int32_t(x), int32_t(y) }, false)) {
				selecting_edit_text = edit_selection_mode::standard;

				info_popup.close(*this, false);
				return true;
			} else {
				set_keyboard_focus(nullptr);
			}
		}

		if(!pending_right_click)
			info_popup.close(*this, false);

		auto under_cursor = printui::interface_under_point(*this, get_ui_rects(), x, y, true);
		if(under_cursor) {
			if(pending_right_click) {
				auto presult = window_interface.get_key_state(dynamic_settings.keys.info_key.scancode);
				if(presult == 0 || dynamic_settings.imode == input_mode::mouse_only) {
					pending_right_click = false; // right click modifier not held down
					window_bar.info_i.mark_for_update(*this);
				}

				under_cursor->parent_object->on_right_click(*this, uint32_t(x - under_cursor->x_position), uint32_t(y - under_cursor->y_position));
			} else {
				under_cursor->parent_object->on_click(*this, uint32_t(x - under_cursor->x_position), uint32_t(y - under_cursor->y_position));
			}
			return true;
		}

		return false;
	}
	bool window_data::on_mouse_right_down(uint32_t x, uint32_t y) {
		if(dynamic_settings.imode == input_mode::follow_input) {
			if(!window_interface.is_mouse_cursor_visible()) {
				window_interface.show_mouse_cursor();
				set_prompt_visibility(printui::prompt_mode::hidden);
				window_interface.invalidate_window();
			}
		} else if(dynamic_settings.imode != input_mode::mouse_only && dynamic_settings.imode != input_mode::mouse_and_keyboard) {
			return false;
		}

		auto under_cursor = printui::interface_under_point(*this, get_ui_rects(), x, y, false);
		if(under_cursor) {
			if(pending_right_click) {
				auto presult = window_interface.get_key_state(dynamic_settings.keys.info_key.scancode);
				if(presult == 0 || dynamic_settings.imode == input_mode::mouse_only) {
					pending_right_click = false; // right click modifier not held down
					window_bar.info_i.mark_for_update(*this);
				}
			}
			under_cursor->parent_object->on_right_click(*this, uint32_t(x - under_cursor->x_position), uint32_t(y - under_cursor->y_position));
			return true;
		}

		return false;
	}


	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		if(message == WM_CREATE) {
			LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
			window_data* app = (window_data*)pcs->lpCreateParams;

			::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));

			return 1;
		} else {
			window_data* app = reinterpret_cast<window_data*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

			if(app) {
				switch(message) {
					case WM_DPICHANGED:
					{
						app->dpi = float(HIWORD(wParam));
						auto lprcNewScale = reinterpret_cast<RECT*>(lParam);
						
						app->on_dpi_change();
						SetWindowPos(hwnd, nullptr, lprcNewScale->left, lprcNewScale->top,
							lprcNewScale->right - lprcNewScale->left, lprcNewScale->bottom - lprcNewScale->top,
							SWP_NOZORDER | SWP_NOACTIVATE);
						break;
					}
					case WM_SETCURSOR:
					{
						if(LOWORD(lParam) == HTCLIENT) {
							app->window_interface.reshow_mouse_cursor();
							return TRUE;
						}
						break;
					}
					case WM_NCMOUSEMOVE:
					{
						RECT rcWindow;
						GetWindowRect(hwnd, &rcWindow);
						auto x = GET_X_LPARAM(lParam);
						auto y = GET_Y_LPARAM(lParam);

						POINTS adj{ SHORT(x - rcWindow.left), SHORT(y - rcWindow.top) };
						memcpy(&lParam, &adj, sizeof(LPARAM));
					} // fallthrough; keep above WM_MOUSEMOVE
					case WM_MOUSEMOVE:
					{
						if(app->keyboard_target && app->keyboard_target->consume_mouse_event(*app, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), uint32_t(wParam))) {
							return 0;
						} else if(app->on_mouse_move(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
							return 0;
						else
							break;
					}
					case WM_SYSKEYDOWN:
					case WM_KEYDOWN:
					{
						if((HIWORD(lParam) & KF_REPEAT) != 0 && app->keyboard_target == nullptr)
							return 0;

						if(app->on_key_down(0xFF & MapVirtualKey(UINT(wParam), MAPVK_VK_TO_VSC), UINT(wParam))) {
							return 0;
						} else {
							break;
						}
					}
					case WM_SYSKEYUP:
					case WM_KEYUP:
					{
						if(app->on_key_up(0xFF & MapVirtualKey(UINT(wParam), MAPVK_VK_TO_VSC), UINT(wParam))) {
							return 0;
						} else {
							break;
						}
					}
					case WM_CHAR:
					{
						// only route to text box, if it has focus
						if(app->keyboard_target && wParam >= 0x20) {
							app->keyboard_target->insert_codepoint(*app, uint32_t(wParam));
						}
						return 0;
					}
					case WM_RBUTTONDOWN:
					{
						if(app->keyboard_target && app->keyboard_target->consume_mouse_event(*app, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), uint32_t(wParam))) {
							return 0;
						} else if(app->on_mouse_right_down(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
							return 0;
						} else {
							break;
						}
					}
					case WM_RBUTTONDBLCLK:
						if(app->keyboard_target && app->keyboard_target->consume_mouse_event(*app, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), uint32_t(wParam))) {
							return 0;
						}
						break;
					case WM_LBUTTONDOWN:
					{
						SetCapture(hwnd);
						if(app->keyboard_target) {
							if(app->keyboard_target->consume_mouse_event(*app, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), uint32_t(wParam))) {
								return 0;
							}
							auto duration = std::chrono::steady_clock::now() - app->last_double_click;
							auto in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
							if(in_ms.count() <= app->double_click_ms) {
								app->keyboard_target->command(*app, edit_command::select_current_section, false);
								app->selecting_edit_text = edit_selection_mode::line;
								return 0;
							}
						}
						if(app->on_mouse_left_down(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
							return 0;
						} else {
							break;
						}
					}
					case WM_LBUTTONDBLCLK:
					{
						if(app->keyboard_target) {
							if(app->keyboard_target->consume_mouse_event(*app, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), uint32_t(wParam))) {
								return 0;
							}
							app->keyboard_target->command(*app, edit_command::select_current_word, false);
							app->selecting_edit_text = edit_selection_mode::word;
							app->last_double_click = std::chrono::steady_clock::now();
							return 0;
						}
						if(app->on_mouse_left_down(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
							return 0;
						} else {
							break;
						}
					}
					case WM_LBUTTONUP:
						ReleaseCapture();
						app->selecting_edit_text = edit_selection_mode::none;
						return 0;
					case WM_MOUSEWHEEL:
						if(app->on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wParam))) {
							return 0;
						} else {
							break;
						}
					case WM_NCCALCSIZE:
						if(wParam == TRUE)
							return 0;
						break;
					case WM_NCHITTEST:
					{
						POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
						RECT rcWindow;
						GetWindowRect(hwnd, &rcWindow);

						USHORT uRow = 1;
						USHORT uCol = 1;

						if(ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + 5) {
							uRow = 0;
						} else if(ptMouse.y < rcWindow.bottom && ptMouse.y >= rcWindow.bottom - 5) {
							uRow = 2;
						}

						// Determine if the point is at the left or right of the window.
						if(ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + 5) {
							uCol = 0; // left side
						} else if(ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - 5) {
							uCol = 2; // right side
						}

						// Hit test (HTTOPLEFT, ... HTBOTTOMRIGHT)
						LRESULT hitTests[3][3] =
						{
							{ HTTOPLEFT,    HTTOP ,     HTTOPRIGHT },
							{ HTLEFT,       HTNOWHERE,  HTRIGHT },
							{ HTBOTTOMLEFT, HTBOTTOM,   HTBOTTOMRIGHT },
						};

						if(hitTests[uRow][uCol] == HTNOWHERE) {
							
							if(app->orientation == layout_orientation::horizontal_left_to_right || app->orientation == layout_orientation::horizontal_right_to_left) {
								if(app->visible_window_title() && ptMouse.y < rcWindow.top + app->window_border + app->layout_size) {
									return HTCAPTION;
								}

								if(app->orientation == layout_orientation::horizontal_left_to_right) {
									if(ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + app->window_border + 2 * app->layout_size) {
										if(printui::interface_under_point(*app, app->get_ui_rects(), ptMouse.x - rcWindow.left, ptMouse.y - rcWindow.top, true) == nullptr) {
											return HTCAPTION;
										}
									}
								} else {
									if(ptMouse.x <= rcWindow.right && ptMouse.x >= rcWindow.right - app->window_border - 2 * app->layout_size) {
										if(printui::interface_under_point(*app, app->get_ui_rects(), ptMouse.x - rcWindow.left, ptMouse.y - rcWindow.top, true) == nullptr) {
											return HTCAPTION;
										}
									}
								}
							} else {
								if(app->orientation == layout_orientation::vertical_left_to_right) {
									if(app->visible_window_title() && ptMouse.x < rcWindow.left + app->window_border + app->layout_size) {
										return HTCAPTION;
									}
								} else {
									if(app->visible_window_title() && ptMouse.x > rcWindow.right - app->window_border - app->layout_size) {
										return HTCAPTION;
									}
								}

								if(ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + app->window_border + 2 * app->layout_size) {
									if(printui::interface_under_point(*app, app->get_ui_rects(), ptMouse.x - rcWindow.left, ptMouse.y - rcWindow.top, true) == nullptr) {
										return HTCAPTION;
									}
								}
							}
						} else {
							return hitTests[uRow][uCol];
						}
							
						break;
					}
					case WM_ENTERSIZEMOVE:
						app->is_sizeing = true;
						return 0;
					case WM_EXITSIZEMOVE:
						app->is_sizeing = false;
						return 0;
					case WM_SIZE:
					{
						resize_type t = resize_type::resize;

						if(wParam == SIZE_MAXIMIZED) {
							t = resize_type::maximize;
						} else if(wParam == SIZE_MINIMIZED) {
							t = resize_type::minimize;
						}  else if(wParam == SIZE_RESTORED) {
							t = resize_type::resize;
						} else {
							break;
						}

						if(app->on_resize(t, LOWORD(lParam), HIWORD(lParam)))
							return 0;
						else
							break;
					}
					case WM_GETMINMAXINFO:
					{
						MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
						pMinMaxInfo->ptMinTrackSize.x = static_cast<UINT>(app->minimum_ui_width);
						pMinMaxInfo->ptMinTrackSize.y = static_cast<UINT>(app->minimum_ui_height);
						return 0;
					}
					case WM_PAINT:
					case WM_DISPLAYCHANGE:
					{
						PAINTSTRUCT ps;
						BeginPaint(hwnd, &ps);
						app->rendering_interface.render(*app);
						EndPaint(hwnd, &ps);
						return 0;
					}
					case WM_DESTROY:
						app->release_all();
						app->accessibility_interface.release_root_provider();
						PostQuitMessage(0);
						return 1;
					case WM_GETOBJECT:
					{

						//if(InSendMessageEx(nullptr) != 0)

						if(static_cast<long>(lParam) == static_cast<long>(UiaRootObjectId)) {
							return UiaReturnRawElementProvider(hwnd, wParam, lParam,
								app->accessibility_interface.get_root_window_provider());
						}
						break;
					}
					case WM_ACTIVATEAPP:
						if(wParam == TRUE) {
							// being activated
							// XInputEnable(TRUE);
						} else {
							// being deactivated
							// XInputEnable(FALSE);
						}
						return 0;
					case WM_INPUT:
						app->on_controller_input();
						return 0;
					case WM_INPUT_DEVICE_CHANGE:
						app->on_device_change(wParam, (HANDLE)lParam);
						return 0;
					case WM_APPCOMMAND:
					{
						auto cmd = GET_APPCOMMAND_LPARAM(lParam);
						if(cmd == APPCOMMAND_COPY) {
							if(app->keyboard_target)
								app->keyboard_target->command(*app, edit_command::copy, false);
							return TRUE;
						} else if(cmd == APPCOMMAND_CUT) {
							if(app->keyboard_target)
								app->keyboard_target->command(*app, edit_command::cut, false);
							return TRUE;
						} else if(cmd == APPCOMMAND_PASTE) {
							if(app->keyboard_target)
								app->keyboard_target->command(*app, edit_command::paste, false);
							return TRUE;
						} else if(cmd == APPCOMMAND_REDO) {
							if(app->keyboard_target)
								app->keyboard_target->command(*app, edit_command::redo, false);
							return TRUE;
						} else if(cmd == APPCOMMAND_UNDO) {
							if(app->keyboard_target)
								app->keyboard_target->command(*app, edit_command::undo, false);
							return TRUE;
						}
						break;
					}
					case WM_KILLFOCUS:
						app->selecting_edit_text = edit_selection_mode::none;
						if(app->keyboard_target)
							app->keyboard_target->set_cursor_visibility(*app, false);
						return 0;
					case WM_SETFOCUS:
						if(app->keyboard_target)
							app->keyboard_target->set_cursor_visibility(*app, true);
						return 0;
					default:
						break;
				}
			}
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	}

	void window_data::create_window() {
		//dpi = float(window_interface.get_window_dpi());
		load_default_dynamic_settings();
		text_interface.create_font_collection(*this);

		if(dynamic_settings.imode == input_mode::system_default) {
			dynamic_settings.imode = input_mode::follow_input;
		}

		bool kb_preference = accessibility_interface.has_keyboard_preference();

		if(dynamic_settings.imode == input_mode::keyboard_only || dynamic_settings.imode == input_mode::mouse_and_keyboard || kb_preference)
			prompts = prompt_mode::keyboard;
		else if(dynamic_settings.imode == input_mode::controller_only || dynamic_settings.imode == input_mode::controller_with_pointer)
			prompts = prompt_mode::controller;
		else
			prompts = prompt_mode::hidden;

		window_interface.create_window(*this);
	}

	bool window_data::on_resize(resize_type type, uint32_t width, uint32_t height) {

		rendering_interface.stop_ui_animations(*this);

		if(ui_width == width && ui_height == height) return false;

		info_popup.currently_visible = false;

		ui_width = width;
		ui_height = height;
		//dpi = float(window_interface.get_window_dpi());

		recreate_layout();

		if(type == resize_type::maximize) {
			auto workspace = window_interface.get_available_workspace();

			window_saved_border = std::max(window_border, window_saved_border);
			window_border = 0;

			window_interface.move_window(workspace);

			rendering_interface.create_window_size_resources(*this);

			client_on_resize(width, height);

			window_interface.invalidate_window();

			return true;
		} else if(type == resize_type::resize) {
			window_border = std::max(window_border, window_saved_border);

			rendering_interface.create_window_size_resources(*this);

			client_on_resize(width, height);

			if(minimum_ui_width > ui_width || minimum_ui_height > ui_height) {
				expand_to_fit_content();
			} else {
				window_interface.invalidate_window();
			}
			return true;
		}
		return false;
	}

	

	void window_data::message_loop() {
		MSG msg;

		while(GetMessage(&msg, NULL, 0, 0)) {
			if(keyboard_target)
				TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	void window_data::switch_input_mode(input_mode new_mode) {
		if(new_mode == input_mode::keyboard_only || new_mode == input_mode::mouse_and_keyboard)
			prompts = prompt_mode::keyboard;
		else if(new_mode == input_mode::controller_only || new_mode == input_mode::controller_with_pointer)
			prompts = prompt_mode::controller;
		else
			prompts = prompt_mode::hidden;

		if(window_interface.is_mouse_cursor_visible() && (new_mode == input_mode::keyboard_only || new_mode == input_mode::controller_only)) {
			window_interface.hide_mouse_cursor();
			focus_stack.clear();
			last_under_cursor = ui_reference_none;
			last_layout_under_cursor = nullptr;
			repopulate_key_actions();
		}

		if(!window_interface.is_mouse_cursor_visible() && (new_mode == input_mode::mouse_only || new_mode == input_mode::mouse_and_keyboard || new_mode == input_mode::controller_with_pointer)) {
			window_interface.show_mouse_cursor();
			focus_stack.clear();
			repopulate_key_actions();
		}

		dynamic_settings.imode = new_mode;
		window_interface.invalidate_window();
	}

	void window_data::set_prompt_visibility(prompt_mode mode) {
		if(prompts != mode) {
			prompts = mode;
			window_interface.invalidate_window();
		}
	}

	void window_data::show_settings_panel() {
		rendering_interface.prepare_ui_animation(*this);

		auto panelsize_x = get_node(window_bar.settings_pages.l_id).width;
		auto panelsize_y = get_node(window_bar.l_id).height;
		auto vertical_offset = get_node(window_bar.l_id).y;
		auto horizontal_offset = get_node(window_bar.l_id).width;

		auto screen_rect = screen_rectangle_from_layout(*this, horizontal_offset, vertical_offset, panelsize_x, panelsize_y);

		rendering_interface.start_ui_animation(animation_description{ screen_rect, animation_type::flip, animation_direction::left, 0.5f, true}, *this);

		window_bar.expanded_show_settings = true;
		get_node(window_bar.settings_pages.l_id).set_ignore(false);
		redraw_ui();

		if(window_bar.acc_obj) {
			accessibility_interface.on_contents_changed(window_bar.acc_obj);
		}
	}

	void window_data::hide_settings_panel() {
		rendering_interface.prepare_ui_animation(*this);

		auto panelsize_x = get_node(window_bar.settings_pages.l_id).width;
		auto panelsize_y = get_node(window_bar.l_id).height;
		auto vertical_offset = get_node(window_bar.l_id).y;
		auto horizontal_offset = get_node(window_bar.l_id).width;

		auto screen_rect = screen_rectangle_from_layout(*this, horizontal_offset, vertical_offset, panelsize_x, panelsize_y);

		rendering_interface.start_ui_animation(animation_description{ screen_rect, animation_type::flip, animation_direction::left, 0.5f, false }, *this);

		window_bar.expanded_show_settings = false;
		get_node(window_bar.settings_pages.l_id).set_ignore(true);
		redraw_ui();

		if(window_bar.acc_obj) {
			accessibility_interface.on_contents_changed(window_bar.acc_obj);
		}

		if(dynamic_settings.settings_changed) {
			file_system.save_settings(*this);
			dynamic_settings.settings_changed = false;
		}
	}

	os_win32_wrapper::~os_win32_wrapper() {

	}

	void* os_win32_wrapper::get_hwnd() {
		return (void*)m_hwnd;
	}

	void window_data::set_keyboard_focus(edit_interface* i) {
		if(keyboard_target == i)
			return;

		if(keyboard_target != nullptr) {
			keyboard_target->on_finalize(*this);
			selecting_edit_text = edit_selection_mode::none;
			if(!i)
				text_services_interface.suspend_keystroke_handling();
		} else {
			if(i)
				text_services_interface.resume_keystroke_handling();
		}
		keyboard_target = i;
		if(i) {
			i->on_initialize(*this);
		} else {
			accessibility_interface.on_focus_returned_to_root();
			text_services_interface.set_focus(*this, nullptr);
		}
		window_interface.invalidate_window();
	}

	void window_data::load_locale_settings(std::wstring const& directory) {
		auto filen = file_system.find_matching_file_name(directory + L"\\*.dat");
		if(filen.length() > 0) {
			file_system.with_file_content(directory + L"\\" + filen, [&](std::string_view content) {
				parse::settings_file(dynamic_settings, text_data.font_name_to_index, content.data(), content.data() + content.length());
				});
		}
	}

	void window_data::load_locale_fonts(std::wstring const& directory) {
		auto filen = file_system.find_matching_file_name(directory + L"\\*.dat");
		if(filen.length() > 0) {
			file_system.with_file_content(directory + L"\\" + filen, [&](std::string_view content) {
				parse::custom_fonts_only(dynamic_settings, text_data.font_name_to_index, content.data(), content.data() + content.length());
				});
		}
	}

	void populate_key_mappings_by_type(key_mappings& k) {
		switch(k.type) {
			case keyboard_type::left_hand:
				k.main_keys[0].scancode = 0x10; // Q
				k.main_keys[1].scancode = 0x11; // W
				k.main_keys[2].scancode = 0x12; // E
				k.main_keys[3].scancode = 0x13; // R
				k.main_keys[4].scancode = 0x1E; // A
				k.main_keys[5].scancode = 0x1F; // S
				k.main_keys[6].scancode = 0x20; // D
				k.main_keys[7].scancode = 0x21; // F
				k.main_keys[8].scancode = 0x2C; // Z
				k.main_keys[9].scancode = 0x2D; // X
				k.main_keys[10].scancode = 0x2E; // C
				k.main_keys[11].scancode = 0x2F; // V
				k.primary_escape.scancode = 0x39; // SPACE
				k.info_key.scancode = 0x2A; // SHFIT
				break;
			case keyboard_type::right_hand:
				k.main_keys[0].scancode = 0x16; // U
				k.main_keys[1].scancode = 0x17; // I
				k.main_keys[2].scancode = 0x18; // O
				k.main_keys[3].scancode = 0x19; // P
				k.main_keys[4].scancode = 0x24; // J
				k.main_keys[5].scancode = 0x25; // K
				k.main_keys[6].scancode = 0x26; // L
				k.main_keys[7].scancode = 0x27; // :
				k.main_keys[8].scancode = 0x32; // M
				k.main_keys[9].scancode = 0x33; // ,
				k.main_keys[10].scancode = 0x34; // .
				k.main_keys[11].scancode = 0x35; // /
				k.primary_escape.scancode = 0x39; // SPACE
				k.info_key.scancode = 0x2A; // SHFIT
				break;
			case keyboard_type::right_hand_tilted:
				k.main_keys[0].scancode = 0x17; // I
				k.main_keys[1].scancode = 0x18; // O
				k.main_keys[2].scancode = 0x19; // P
				k.main_keys[3].scancode = 0x1A; // {
				k.main_keys[4].scancode = 0x24; // J
				k.main_keys[5].scancode = 0x25; // K
				k.main_keys[6].scancode = 0x26; // L
				k.main_keys[7].scancode = 0x27; // :
				k.main_keys[8].scancode = 0x31; // N
				k.main_keys[9].scancode = 0x32; // M
				k.main_keys[10].scancode = 0x33; // ,
				k.main_keys[11].scancode = 0x34; // .
				k.primary_escape.scancode = 0x39; // SPACE
				k.info_key.scancode = 0x2A; // SHFIT
				break;
			default:
				return;
		}

		for(uint32_t i = 0; i < 12; ++i) {
			WCHAR keyname[64];
			auto length = GetKeyNameTextW((uint32_t(k.main_keys[i].scancode) << 16) | 1ui32, keyname, 64);
			if(length > 0)
				k.main_keys[i].display_name = std::wstring(size_t(1), keyname[0]);
			else
				k.main_keys[i].display_name = L"";
		}
		{
			WCHAR keyname[64];
			auto length = GetKeyNameTextW((uint32_t(k.primary_escape.scancode) << 16) | 1ui32, keyname, 64);
			k.primary_escape.display_name = std::wstring(keyname, size_t(length));
		}
		{
			WCHAR keyname[64];
			auto length = GetKeyNameTextW((uint32_t(k.info_key.scancode) << 16) | 1ui32, keyname, 64);
			k.info_key.display_name = std::wstring(keyname, size_t(length));
		}
	}
}
