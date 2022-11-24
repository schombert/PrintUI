#include "printui_utility.hpp"

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

	uint32_t os_win32_wrapper::get_window_dpi() const {
		return GetDpiForWindow((HWND)(m_hwnd));
	}

	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	bool os_win32_wrapper::create_window(window_data& win) {

		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_OWNDC;
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
			if(win.dynamic_settings.imode == printui::input_mode::mouse_and_keyboard || win.dynamic_settings.imode == printui::input_mode::mouse_only || win.dynamic_settings.imode == printui::input_mode::follow_input || win.dynamic_settings.imode == printui::input_mode::controller_with_pointer) {
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
				static_cast<int>(ceil(float(win.dynamic_settings.window_x_size) * win.dpi / 96.f)),
				static_cast<int>(ceil(float(win.dynamic_settings.window_y_size) * win.dpi / 96.f)),
				SWP_NOMOVE | SWP_FRAMECHANGED);

			if(win.dynamic_settings.locale_lang.size() > 0) {
				win.text_data.change_locale(win.dynamic_settings.locale_lang, win.dynamic_settings.locale_region, win);
			} else {
				win.text_data.default_locale(win);
			}

			auto window_title_id = win.text_data.text_id_from_name("window_title").id;
			win.title_bar.set_text_content(uint16_t(window_title_id));

			ShowWindow((HWND)(m_hwnd), SW_SHOWNORMAL);
			UpdateWindow((HWND)(m_hwnd));

			return true;
		}
		return false;
	}

	screen_space_rect os_win32_wrapper::get_window_location() const {
		RECT rcClient;
		GetClientRect((HWND)(m_hwnd), &rcClient);
		return screen_space_rect{ rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top };
	}

	void os_win32_wrapper::display_fatal_error_message(wchar_t const* msg) {
		MessageBox((HWND)(m_hwnd), msg, L"Fatal Error", MB_OK | MB_ICONERROR);
	}

	long os_win32_wrapper::create_swap_chain(IDXGIFactory2* fac, ID3D11Device* dev, DXGI_SWAP_CHAIN_DESC1 const* desc, IDXGISwapChain1** out) 		{
		return fac->CreateSwapChainForHwnd(dev, (HWND)(m_hwnd), desc, nullptr, nullptr, out);

	}

	bool os_win32_wrapper::is_maximized() const {
		return IsZoomed((HWND)(m_hwnd));
	}
	void os_win32_wrapper::maximize() {
		ShowWindow((HWND)(m_hwnd), SW_MAXIMIZE);
	}
	void os_win32_wrapper::minimize() {
		ShowWindow((HWND)(m_hwnd), SW_MINIMIZE);
	}
	void os_win32_wrapper::restore() {
		ShowWindow((HWND)(m_hwnd), SW_SHOWNORMAL);
	}
	void os_win32_wrapper::close() {
		PostMessage((HWND)(m_hwnd), WM_CLOSE, 0, 0);
	}
	
	void os_win32_wrapper::set_window_title(wchar_t const* t) {
		SetWindowText((HWND)(m_hwnd), t);
	}

	void os_win32_wrapper::set_text_rendering_parameters(ID2D1DeviceContext5* dc, IDWriteFactory6* fac) {
		IDWriteRenderingParams* rparams = nullptr;
		auto monitor_handle = MonitorFromWindow((HWND)(m_hwnd), MONITOR_DEFAULTTOPRIMARY);
		fac->CreateMonitorRenderingParams(monitor_handle, &rparams);
		if(rparams) {
			dc->SetTextRenderingParams(rparams);
			safe_release(rparams);
		}
	}

	window_data::window_data(bool mn, bool mx, bool settings, std::vector<settings_menu_item> const& setting_items, std::unique_ptr<window_wrapper>&& wi) : window_bar(*this, mn, mx, settings, setting_items), window_interface(std::move(wi)) {
		horizontal_interactable_bg.file_name = L"left_select_i.svg";
		horizontal_interactable_bg.edge_padding = 0.0f;
		vertical_interactable_bg.file_name = L"top_select_i.svg";
		vertical_interactable_bg.edge_padding = 0.0f;
	}

	ui_rectangle const* interface_under_point(std::vector<ui_rectangle> const& rects, int32_t x, int32_t y) {
		ui_rectangle const* found = nullptr;
		for(auto& r : rects) {
			if(x >= r.x_position && x <= r.x_position + r.width && y >= r.y_position && y <= r.y_position + r.height) {
				if((r.display_flags & ui_rectangle::flag_clear_rect) != 0) {
					found = nullptr;
				} else if((r.display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					return found;
				} else if((r.display_flags & ui_rectangle::flag_grouping_only) == 0) {
					if(r.parent_object.get_render_interface())
						found = &r;
				}
			}
		}
		return found;
	}
	ui_reference reference_under_point(std::vector<ui_rectangle> const& rects, int32_t x, int32_t y) {
		ui_reference found = std::numeric_limits<ui_reference>::max();
		for(ui_reference i = 0; i < rects.size(); ++i) {
			if(x >= rects[i].x_position && x <= rects[i].x_position + rects[i].width && y >= rects[i].y_position && y <= rects[i].y_position + rects[i].height) {
				if((rects[i].display_flags & ui_rectangle::flag_clear_rect) != 0) {
					found = std::numeric_limits<ui_reference>::max();
				} else if((rects[i].display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					return found;
				} else if(((rects[i].display_flags & ui_rectangle::flag_grouping_only) == 0) &&
					((rects[i].display_flags & ui_rectangle::flag_frame) == 0 || rects[i].parent_object.get_render_interface() != nullptr)) {
					found = i;
				}
			}
		}
		return found;
	}
	layout_reference layout_reference_under_point(std::vector<ui_rectangle> const& rects, int32_t x, int32_t y) {
		layout_reference found = std::numeric_limits<layout_reference>::max();
		for(layout_reference i = 0; i < rects.size(); ++i) {

			if(x >= rects[i].x_position && x <= rects[i].x_position + rects[i].width && y >= rects[i].y_position && y <= rects[i].y_position + rects[i].height) {

				if((rects[i].display_flags & ui_rectangle::flag_clear_rect) != 0) {
					found = layout_reference_none;
				} else if((rects[i].display_flags & ui_rectangle::flag_preserve_rect) != 0) {
					return found;
				} else if((rects[i].display_flags & ui_rectangle::flag_frame) != 0) {
				} else {
					found = rects[i].parent_object.get_layout_reference();
				}
			}
		}
		return found;
	}

	window_data::~window_data() {
		safe_release(common_text_format);
		safe_release(small_text_format);
		safe_release(font_fallbacks);
		safe_release(small_font_fallbacks);
		safe_release(d2d_factory);
		safe_release(dwrite_factory);
		safe_release(wic_factory);

		safe_release(light_selected);
		safe_release(light_line);
		safe_release(light_selected_line);
		safe_release(dark_selected);
		safe_release(dark_line);
		safe_release(dark_selected_line);
		safe_release(dummy_brush);
		safe_release(plain_strokes);

		safe_release(foreground);

		safe_release(animation_foreground);
		safe_release(animation_background);

		safe_release(font_collection);

		for(uint32_t i = 0; i < 12; ++i) {
			safe_release(horizontal_interactable[i]);
			safe_release(vertical_interactable[i]);
		}
		for(auto& i : palette) {
			safe_release(i);
		}
		for(auto& i : palette_bitmaps) {
			safe_release(i);
		}

		safe_release(d3d_device);
		safe_release(dxgi_device);
		safe_release(d2d_device);

		safe_release(d3d_device_context);
		safe_release(d2d_device_context);
		safe_release(swap_chain);

		safe_release(back_buffer_target);
	}

	void window_data::create_highlight_brushes() {
		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.08f),
			&light_selected
		);
		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.16f),
			&light_line
		);
		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.24f),
			&light_selected_line
		);

		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.07f),
			&dark_selected
		);
		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.14f),
			&dark_line
		);
		d2d_device_context->CreateSolidColorBrush(
			D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.21f),
			&dark_selected_line
		);
	}

	void window_data::intitialize_fonts() {
		layout_size = int32_t(std::round(float(dynamic_settings.layout_base_size) * dpi / 96.0f));
		window_border = int32_t(std::round(float(dynamic_settings.window_border) * dpi / 96.0f));

		// metrics
		{
			IDWriteFont3* f = nullptr;
			IDWriteFont3* f2 = nullptr;

			IDWriteFontList2* fl;
			DWRITE_FONT_AXIS_VALUE fax[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					to_font_weight(dynamic_settings.primary_font.is_bold)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					to_font_span(dynamic_settings.primary_font.span) } ,
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC,
					to_font_style(dynamic_settings.primary_font.is_oblique) } };

			font_collection->GetMatchingFonts(dynamic_settings.primary_font.name.c_str(), fax, 3, &fl);
			fl->GetFont(0, &f);
			safe_release(fl);

			IDWriteFontList2* fl2;
			DWRITE_FONT_AXIS_VALUE fax2[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					to_font_weight(dynamic_settings.small_font.is_bold)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					to_font_span(dynamic_settings.small_font.span) } ,
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC,
					to_font_style(dynamic_settings.small_font.is_oblique) } };

			font_collection->GetMatchingFonts(dynamic_settings.small_font.name.c_str(), fax2, 3, &fl2);
			fl2->GetFont(0, &f2);
			safe_release(fl2);

			text::update_font_metrics(dynamic_settings.primary_font, dwrite_factory, text_data.locale_string(), float(layout_size), dpi / 96.0f, f);
			text::update_font_metrics(dynamic_settings.small_font, dwrite_factory, text_data.locale_string(), std::round(float(layout_size) * 3.0f / 4.0f), dpi / 96.0f, f2);

			safe_release(f2);
			safe_release(f);
		}

		// text formats
		{
			safe_release(common_text_format);

			DWRITE_FONT_AXIS_VALUE fax[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					to_font_weight(dynamic_settings.small_font.is_bold)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					to_font_span(dynamic_settings.small_font.span) } ,
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC,
					to_font_style(dynamic_settings.small_font.is_oblique) } };

			dwrite_factory->CreateTextFormat(dynamic_settings.small_font.name.c_str(), font_collection, fax, 3, dynamic_settings.primary_font.font_size, L"", &common_text_format);
			common_text_format->SetFontFallback(font_fallbacks);
			common_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			common_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, dynamic_settings.primary_font.line_spacing, dynamic_settings.primary_font.baseline);
		}
		{
			safe_release(small_text_format);

			DWRITE_FONT_AXIS_VALUE fax[] = {
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WEIGHT,
					to_font_weight(dynamic_settings.small_font.is_bold)},
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_WIDTH,
					to_font_span(dynamic_settings.small_font.span) } ,
				DWRITE_FONT_AXIS_VALUE{DWRITE_FONT_AXIS_TAG_ITALIC,
					to_font_style(dynamic_settings.small_font.is_oblique) } };

			dwrite_factory->CreateTextFormat(dynamic_settings.small_font.name.c_str(), font_collection, fax, 3, dynamic_settings.small_font.font_size, L"", &small_text_format);
			small_text_format->SetFontFallback(small_font_fallbacks);

			small_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			small_text_format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, dynamic_settings.small_font.line_spacing, dynamic_settings.small_font.baseline);
		}
	}

	void window_data::on_dpi_change() {
		intitialize_fonts();

		//todo: icons.
		init_layout_graphics();
		common_icons.redraw_icons(*this);
		create_interactiable_tags();

		client_on_dpi_change();
	}


	void window_data::create_persistent_resources() {
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory);
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&wic_factory));
		DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(dwrite_factory), reinterpret_cast<IUnknown**>(&dwrite_factory));

		load_launch_settings(dynamic_settings, text_data.font_name_to_index);

		{
			D2D1_STROKE_STYLE_PROPERTIES style_prop;
			style_prop.startCap = D2D1_CAP_STYLE_FLAT;
			style_prop.endCap = D2D1_CAP_STYLE_FLAT;
			style_prop.dashCap = D2D1_CAP_STYLE_FLAT;
			style_prop.lineJoin = D2D1_LINE_JOIN_MITER;
			style_prop.miterLimit = 10.0f;
			style_prop.dashStyle = D2D1_DASH_STYLE_SOLID;
			style_prop.dashOffset = 0.0f;
			d2d_factory->CreateStrokeStyle(style_prop, nullptr, 0, &plain_strokes);
		}

		//fonts
		text::create_font_collection(*this, dynamic_settings.font_directory);

		//fallbacks
		auto global_fb = text::get_global_font_fallbacks();

		{
			IDWriteFontFallbackBuilder* bldr = nullptr;
			dwrite_factory->CreateFontFallbackBuilder(&bldr);

			load_primary_font_fallbacks(bldr);

			text::load_fallbacks_by_type(global_fb, dynamic_settings.primary_font.type, bldr, font_collection, dwrite_factory);
			bldr->CreateFontFallback(&font_fallbacks);
			safe_release(bldr);
		}
		{
			IDWriteFontFallbackBuilder* bldr = nullptr;
			dwrite_factory->CreateFontFallbackBuilder(&bldr);

			load_small_font_fallbacks(bldr);

			text::load_fallbacks_by_type(global_fb, dynamic_settings.small_font.type, bldr, font_collection, dwrite_factory);
			bldr->CreateFontFallback(&font_fallbacks);
			safe_release(bldr);
		}

	}

	void window_data::expand_to_fit_content() {
		auto workspace = window_interface->get_available_workspace();
		auto current_placement = window_interface->get_window_placement();

		screen_space_rect new_placement{ 0 ,0, int32_t(std::max(minimum_ui_width, ui_width)), int32_t(std::max(minimum_ui_height, ui_height)) };

		auto const change_width = new_placement.width - (current_placement.width);
		auto const change_height = new_placement.height - (current_placement.height);
		new_placement.x = std::clamp(current_placement.x - change_width / 2, 0, workspace.width - new_placement.width);
		new_placement.y = std::clamp(current_placement.y - change_height / 2, 0, workspace.height - new_placement.height);

		window_interface->set_window_placement(new_placement);
	}

	bool window_data::on_mouse_move(uint32_t x, uint32_t y) {
		if(dynamic_settings.imode == input_mode::follow_input) {
			if(!window_interface->is_mouse_cursor_visible()) {
				window_interface->show_mouse_cursor();
				set_prompt_visibility(printui::prompt_mode::hidden);
				window_interface->invalidate_window();
			}
		} else if(dynamic_settings.imode != input_mode::mouse_only && dynamic_settings.imode != input_mode::mouse_and_keyboard) {
			return false;
		}

		last_cursor_x_position = x;
		last_cursor_y_position = y;

		auto& rects = get_ui_rects();
		auto new_under_cursor = printui::reference_under_point(rects, last_cursor_x_position, last_cursor_y_position);

		auto new_l_ref = layout_reference_under_point(rects, last_cursor_x_position, last_cursor_y_position);

		set_window_focus_from_mouse(new_l_ref);

		if(last_under_cursor != new_under_cursor) {
			bool last_is_highlighted = last_under_cursor < rects.size() ? (rects[last_under_cursor].display_flags & printui::ui_rectangle::flag_interactable) != 0 : false;

			bool new_is_highlighted = new_under_cursor < rects.size() ? (rects[new_under_cursor].display_flags & printui::ui_rectangle::flag_interactable) != 0 : false;

			last_under_cursor = new_under_cursor;

			if(last_is_highlighted || new_is_highlighted) {
				window_interface->invalidate_window();
			}
		}

		return true;
	}

	bool window_data::on_key_down(uint32_t scan_code, uint32_t /* key_code */) {
		if(scan_code == primary_right_click_modifier_sc || scan_code == secondary_right_click_modifier_sc) {
			pending_right_click = true;
			window_bar.info_i.mark_for_update(*this);
			return true;
		}

		if(dynamic_settings.imode == input_mode::follow_input) {
			if(window_interface->is_mouse_cursor_visible()) {
				window_interface->hide_mouse_cursor();
				set_prompt_visibility(printui::prompt_mode::keyboard);
				window_interface->invalidate_window();
				return true; // eat the key so that we don't do something bad
			}

		} else if(dynamic_settings.imode != input_mode::keyboard_only && dynamic_settings.imode != input_mode::mouse_and_keyboard) {
			return false;
		}

		if(scan_code == primary_escape_sc || scan_code == secondary_escape_sc) {
			// go up
			execute_focus_action(focus_actions.escape);
		} else {
			for(int32_t i = 0; i < 12; i++) {
				if(sc_values[i] == scan_code) {
					// do something with ith key
					execute_focus_action(focus_actions.button_actions[i]);
					break;
				}
			}
		}

		return true;
	}

	bool window_data::on_key_up(uint32_t scan_code, uint32_t /*key_code*/) {
		if(scan_code == primary_right_click_modifier_sc || scan_code == secondary_right_click_modifier_sc) {
			pending_right_click = false;
			window_bar.info_i.mark_for_update(*this);
		}

		if(dynamic_settings.imode == input_mode::follow_input) {

		} else if(dynamic_settings.imode != input_mode::keyboard_only && dynamic_settings.imode != input_mode::mouse_and_keyboard) {
			return false;
		}

		return true;
	}

	bool window_data::on_mouse_left_down(uint32_t x, uint32_t y) {
		if(dynamic_settings.imode == input_mode::follow_input) {
			if(!window_interface->is_mouse_cursor_visible()) {
				window_interface->show_mouse_cursor();
				set_prompt_visibility(printui::prompt_mode::hidden);
				window_interface->invalidate_window();
			}
		} else if(dynamic_settings.imode != input_mode::mouse_only && dynamic_settings.imode != input_mode::mouse_and_keyboard) {
			return false;
		}

		auto under_cursor = printui::interface_under_point(get_ui_rects(), x, y);
		if(under_cursor) {
			if(pending_right_click) {
				auto presult = window_interface->get_key_state(primary_right_click_modifier_sc);
				auto sresult = window_interface->get_key_state(secondary_right_click_modifier_sc);
				if((presult == 0 && sresult == 0) || dynamic_settings.imode == input_mode::mouse_only) {
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
			if(!window_interface->is_mouse_cursor_visible()) {
				window_interface->show_mouse_cursor();
				set_prompt_visibility(printui::prompt_mode::hidden);
				window_interface->invalidate_window();
			}
		} else if(dynamic_settings.imode != input_mode::mouse_only && dynamic_settings.imode != input_mode::mouse_and_keyboard) {
			return false;
		}

		auto under_cursor = printui::interface_under_point(get_ui_rects(), x, y);
		if(under_cursor) {
			if(pending_right_click) {
				auto presult = window_interface->get_key_state(primary_right_click_modifier_sc);
				auto sresult = window_interface->get_key_state(secondary_right_click_modifier_sc);
				if((presult == 0 && sresult == 0) || dynamic_settings.imode == input_mode::mouse_only) {
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
						app->stop_ui_animations();
						app->on_dpi_change();
						SetWindowPos(hwnd, nullptr, lprcNewScale->left, lprcNewScale->top,
							lprcNewScale->right - lprcNewScale->left, lprcNewScale->bottom - lprcNewScale->top,
							SWP_NOZORDER | SWP_NOACTIVATE);
						break;
					}
					case WM_SETCURSOR:
					{
						if(LOWORD(lParam) == HTCLIENT) {
							app->window_interface->reshow_mouse_cursor();
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
						if(app->on_mouse_move(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
							return 0;
						else
							break;
					}
					case WM_KEYDOWN:
					{
						if((HIWORD(lParam) & KF_REPEAT) != 0)
							return 0;

						// IF text box has focus, process only secondary escape, else ...

						if(app->on_key_down(0xFF & MapVirtualKey(UINT(wParam), MAPVK_VK_TO_VSC), UINT(wParam))) {
							return 0;
						} else {
							break;
						}
					}
					case WM_KEYUP:
					{
						// IF text box has focus, process only secondary escape, else ...

						if(app->on_key_up(0xFF & MapVirtualKey(UINT(wParam), MAPVK_VK_TO_VSC), UINT(wParam))) {
							return 0;
						} else {
							break;
						}
					}
					case WM_CHAR:
					{
						// only route to text box, if it has focus

						return 0;
					}
					case WM_RBUTTONDOWN:
					{
						if(app->on_mouse_right_down(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
							return 0;
						else
							break;
					}
					case WM_LBUTTONDOWN:
					{
						if(app->on_mouse_left_down(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
							return 0;
						else
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
										if(printui::interface_under_point(app->get_ui_rects(), ptMouse.x - rcWindow.left, ptMouse.y - rcWindow.top) == nullptr) {
											return HTCAPTION;
										}
									}
								} else {
									if(ptMouse.x <= rcWindow.right && ptMouse.x >= rcWindow.right - app->window_border - 2 * app->layout_size) {
										if(printui::interface_under_point(app->get_ui_rects(), ptMouse.x - rcWindow.left, ptMouse.y - rcWindow.top) == nullptr) {
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
									if(printui::interface_under_point(app->get_ui_rects(), ptMouse.x - rcWindow.left, ptMouse.y - rcWindow.top) == nullptr) {
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
						app->render();
						EndPaint(hwnd, &ps);
						return 0;
					}
					case WM_DESTROY:
						app->release_all();
						PostQuitMessage(0);
						return 1;
					default:
						break;
				}
			}
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	}

	void window_data::create_window() {
		dpi = float(window_interface->get_window_dpi());
		load_default_dynamic_settings();
		create_persistent_resources();

		if(dynamic_settings.imode == input_mode::system_default) {
			dynamic_settings.imode = printui::get_system_default_input_mode();
		}

		if(dynamic_settings.imode == input_mode::keyboard_only || dynamic_settings.imode == input_mode::mouse_and_keyboard)
			prompts = prompt_mode::keyboard;
		else if(dynamic_settings.imode == input_mode::controller_only || dynamic_settings.imode == input_mode::controller_with_pointer)
			prompts = prompt_mode::controller;
		else
			prompts = prompt_mode::hidden;

		window_interface->create_window(*this);
	}

	bool window_data::on_resize(resize_type type, uint32_t width, uint32_t height) {

		stop_ui_animations();
		if(type == resize_type::maximize) {
			auto workspace = window_interface->get_available_workspace();

			window_saved_border = std::max(window_border, window_saved_border);
			window_border = 0;

			window_interface->move_window(workspace);

			if(!d3d_device) {
				create_device_resources();
			} else {
				create_window_size_resources(width, height);
			}
			client_on_resize(width, height);

			window_interface->invalidate_window();

			return true;
		} else if(type == resize_type::resize) {
			window_border = std::max(window_border, window_saved_border);

			if(!d3d_device) {
				create_device_resources();
			} else {
				create_window_size_resources(width, height);
			}
			client_on_resize(width, height);

			if(minimum_ui_width > ui_width || minimum_ui_height > ui_height) {
				expand_to_fit_content();
			} else {
				window_interface->invalidate_window();
			}
			return true;
		}
		return false;
	}

	void window_data::create_device_resources() {
		HRESULT hr = S_OK;

		if(!d3d_device) {
			IDXGIAdapter* pAdapter = nullptr;
			IDXGIFactory2* pDXGIFactory = nullptr;

			auto loc = window_interface->get_window_location();

			auto nWidth = loc.width;
			auto nHeight = loc.height;

			D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

			hr = D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
				levels, 4,
				D3D11_SDK_VERSION,
				&d3d_device,
				nullptr,
				&d3d_device_context
			);

			if(FAILED(hr)) {
				hr = D3D11CreateDevice(nullptr,
					D3D_DRIVER_TYPE_WARP,
					nullptr,
					D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
					levels, 4,
					D3D11_SDK_VERSION,
					&d3d_device,
					nullptr,
					&d3d_device_context
				);
			}

			if(SUCCEEDED(hr)) {
				hr = d3d_device->QueryInterface(IID_PPV_ARGS(&dxgi_device));
			}
			if(SUCCEEDED(hr)) {
				hr = d2d_factory->CreateDevice(dxgi_device, &d2d_device);
			}
			if(SUCCEEDED(hr)) {
				hr = d2d_device->CreateDeviceContext(
					D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
					&d2d_device_context
				);
			}
			if(SUCCEEDED(hr)) {
				hr = d3d_device->QueryInterface(&dxgi_device);
			}
			if(SUCCEEDED(hr)) {
				hr = dxgi_device->GetAdapter(&pAdapter);
			}
			if(SUCCEEDED(hr)) {
				hr = pAdapter->GetParent(IID_PPV_ARGS(&pDXGIFactory));
			}
			if(SUCCEEDED(hr)) {
				DXGI_SWAP_CHAIN_DESC1  swapDesc;
				::ZeroMemory(&swapDesc, sizeof(swapDesc));

				swapDesc.Width = nWidth;
				swapDesc.Height = nHeight;
				swapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				swapDesc.Stereo = FALSE;
				swapDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1,0 };
				swapDesc.SampleDesc.Count = 1;
				swapDesc.SampleDesc.Quality = 0;
				swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapDesc.BufferCount = 2;
				swapDesc.Scaling = DXGI_SCALING_NONE;
				swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
				swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
				swapDesc.Flags = 0;

				hr = window_interface->create_swap_chain(pDXGIFactory, d3d_device, &swapDesc, &swap_chain);
			}
			if(SUCCEEDED(hr)) {
				hr = dxgi_device->SetMaximumFrameLatency(1);
			}

			dpi = float(window_interface->get_window_dpi());

			create_palette();

			if(SUCCEEDED(hr)) {
				create_window_size_resources(nWidth, nHeight);
			} else {
				window_interface->display_fatal_error_message(L"Could not create direct X devices, exiting");
				std::terminate();
			}

			safe_release(pAdapter);
			safe_release(pDXGIFactory);
		}
	}

	void window_data::release_device_resources() {
		safe_release(d3d_device);
		safe_release(d3d_device_context);
		safe_release(d2d_device);
		safe_release(d2d_device_context);
		safe_release(dxgi_device);
		safe_release(swap_chain);

		release_palette();
	}


	void window_data::create_window_size_resources(uint32_t nWidth, uint32_t nHeight) {
		HRESULT hr = S_OK;
		IDXGISurface* back_buffer = nullptr;

		if(ui_width == nWidth && ui_height == nHeight) return;

		ui_width = nWidth;
		ui_height = nHeight;
		dpi = float(window_interface->get_window_dpi());

		recreate_layout();

		if(!swap_chain)
			return;

		safe_release(back_buffer_target);
		safe_release(foreground);

		stop_ui_animations();
		safe_release(animation_foreground);
		safe_release(animation_background);

		// Resize render target buffers
		d2d_device_context->SetTarget(nullptr);
		hr = swap_chain->ResizeBuffers(0, nWidth, nHeight, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

		if(SUCCEEDED(hr)) {
			hr = swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
		}
		if(SUCCEEDED(hr)) {
			D2D1_BITMAP_PROPERTIES1 bitmapProperties =
				D2D1::BitmapProperties1(
					D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
					dpi,
					dpi
				);
			hr = d2d_device_context->CreateBitmapFromDxgiSurface(
				back_buffer,
				&bitmapProperties,
				&back_buffer_target
			);
		}

		safe_release(back_buffer);

		if(!back_buffer_target)
			std::abort();

		if(SUCCEEDED(hr)) {
			d2d_device_context->CreateBitmap(D2D1_SIZE_U{ ui_width, ui_height }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(
						DXGI_FORMAT_A8_UNORM,
						D2D1_ALPHA_MODE_PREMULTIPLIED),
					dpi, dpi,
					D2D1_BITMAP_OPTIONS_TARGET , nullptr },
				&foreground);

			redraw_completely_pending = true;

			d2d_device_context->CreateBitmap(D2D1_SIZE_U{ ui_width, ui_height }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(
						DXGI_FORMAT_B8G8R8A8_UNORM ,
						D2D1_ALPHA_MODE_PREMULTIPLIED),
					dpi, dpi,
					D2D1_BITMAP_OPTIONS_TARGET , nullptr },
				& animation_foreground);
			d2d_device_context->CreateBitmap(D2D1_SIZE_U{ ui_width, ui_height }, nullptr, 0,
				D2D1_BITMAP_PROPERTIES1{
					D2D1::PixelFormat(
						DXGI_FORMAT_B8G8R8A8_UNORM ,
						D2D1_ALPHA_MODE_PREMULTIPLIED),
					dpi, dpi,
					D2D1_BITMAP_OPTIONS_TARGET , nullptr },
				& animation_background);
		}
	}

	void window_data::message_loop() {
		MSG msg;

		while(GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	input_mode get_system_default_input_mode() {
		return input_mode::follow_input;
	}

	void window_data::switch_input_mode(input_mode new_mode) {
		if(new_mode == input_mode::keyboard_only || new_mode == input_mode::mouse_and_keyboard)
			prompts = prompt_mode::keyboard;
		else if(new_mode == input_mode::controller_only || new_mode == input_mode::controller_with_pointer)
			prompts = prompt_mode::controller;
		else
			prompts = prompt_mode::hidden;

		if(window_interface->is_mouse_cursor_visible() && (new_mode == input_mode::keyboard_only || new_mode == input_mode::controller_only)) {
			window_interface->hide_mouse_cursor();
			focus_stack.clear();
			last_under_cursor = ui_reference_none;
			last_layout_under_cursor = nullptr;
			repopulate_key_actions();
		}

		if(!window_interface->is_mouse_cursor_visible() && (new_mode == input_mode::mouse_only || new_mode == input_mode::mouse_and_keyboard || new_mode == input_mode::controller_with_pointer)) {
			window_interface->show_mouse_cursor();
			focus_stack.clear();
			repopulate_key_actions();
		}

		dynamic_settings.imode = new_mode;
		window_interface->invalidate_window();
	}

	void window_data::set_prompt_visibility(prompt_mode mode) {
		if(prompts != mode) {
			prompts = mode;
			window_interface->invalidate_window();
		}
	}

	void window_data::stop_ui_animations() {
		animation_status.is_running = false;
	}
	void window_data::prepare_ui_animation() {
		stop_ui_animations();

		refresh_foregound();

		d2d_device_context->SetTarget(animation_background);
		d2d_device_context->BeginDraw();

		d2d_device_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
		d2d_device_context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

		render::to_display(get_layout(), *this);

		d2d_device_context->EndDraw();
	}

	void window_data::start_ui_animation(animation_description description) {
		animation_status.description = description;

		switch(orientation) {
			case layout_orientation::horizontal_left_to_right:
				break;
			case layout_orientation::horizontal_right_to_left:
				if(description.direction == animation_direction::left)
					animation_status.description.direction = animation_direction::right;
				else if(description.direction == animation_direction::right)
					animation_status.description.direction = animation_direction::left;
				break;
			case layout_orientation::vertical_left_to_right:
				if(description.direction == animation_direction::left)
					animation_status.description.direction = animation_direction::top;
				else if(description.direction == animation_direction::right)
					animation_status.description.direction = animation_direction::bottom;
				else if(description.direction == animation_direction::top)
					animation_status.description.direction = animation_direction::left;
				else if(description.direction == animation_direction::bottom)
					animation_status.description.direction = animation_direction::right;
				break;
			case layout_orientation::vertical_right_to_left:
				if(description.direction == animation_direction::left)
					animation_status.description.direction = animation_direction::top;
				else if(description.direction == animation_direction::right)
					animation_status.description.direction = animation_direction::bottom;
				else if(description.direction == animation_direction::top)
					animation_status.description.direction = animation_direction::right;
				else if(description.direction == animation_direction::bottom)
					animation_status.description.direction = animation_direction::left;
				break;
		}

		animation_status.is_running = true;
		animation_status.start_time = std::chrono::steady_clock::now();
		window_interface->invalidate_window();
	}

	void window_data::show_settings_panel() {
		prepare_ui_animation();

		auto panelsize_x = get_node(window_bar.settings_pages.l_id).width;
		auto panelsize_y = get_node(window_bar.l_id).height;
		auto vertical_offset = get_node(window_bar.l_id).y;
		auto horizontal_offset = get_node(window_bar.l_id).width;

		auto screen_rect = screen_rectangle_from_layout(*this, horizontal_offset, vertical_offset, panelsize_x, panelsize_y);

		start_ui_animation(animation_description{ screen_rect, animation_type::flip, animation_direction::left, 0.5f, true});

		window_bar.expanded_show_settings = true;
		get_node(window_bar.settings_pages.l_id).ignore = false;
		redraw_ui();
	}

	void window_data::hide_settings_panel() {
		prepare_ui_animation();

		auto panelsize_x = get_node(window_bar.settings_pages.l_id).width;
		auto panelsize_y = get_node(window_bar.l_id).height;
		auto vertical_offset = get_node(window_bar.l_id).y;
		auto horizontal_offset = get_node(window_bar.l_id).width;

		auto screen_rect = screen_rectangle_from_layout(*this, horizontal_offset, vertical_offset, panelsize_x, panelsize_y);

		start_ui_animation(animation_description{ screen_rect, animation_type::flip, animation_direction::left, 0.5f, false });

		window_bar.expanded_show_settings = false;
		get_node(window_bar.settings_pages.l_id).ignore = true;
		redraw_ui();
	}
}
