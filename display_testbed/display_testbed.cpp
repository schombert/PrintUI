#include "display_testbed.hpp"
#include <Windowsx.h>
#include <shlwapi.h>
#include <cmath>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <dxgi1_2.h>
#include <d3d11_3.h>
#include <DirectXMath.h>
#include <d2d1_3.h>
#include <d2d1helper.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <algorithm>

void main_window::load_default_dynamic_settings() {
	dynamic_settings.window_border = 3;
	dynamic_settings.layout_base_size = 22;
	dynamic_settings.brushes.push_back(printui::brush{ L"yellowpaper.png", printui::brush_color{0.95f,0.95f,0.95f}, true });
	dynamic_settings.brushes.push_back(printui::brush{ L"", printui::brush_color{0.0f,0.0f,0.0f}, false });
	
	dynamic_settings.text_directory = L"locale";
	dynamic_settings.primary_font.name = L"Libre Caslon Text";
	dynamic_settings.primary_font.type = printui::font_type::roman;
	dynamic_settings.primary_font.top_leading = 0;
	dynamic_settings.primary_font.bottom_leading = 1;

	dynamic_settings.small_font.name = L"NewComputerModern10";
	dynamic_settings.small_font.type = printui::font_type::roman;

	// define shared animation effects
	printui::list_control::list_appearance = printui::animation_defintion{printui::animation_type::fade, 0.25f, true};
	printui::list_control::list_disappearance = printui::animation_defintion{ printui::animation_type::fade, 0.25f, false };

	printui::page_footer::footer_appearance = printui::animation_defintion{ printui::animation_type::slide, 0.15f, true };
	printui::page_footer::footer_disappearance = printui::animation_defintion{ printui::animation_type::slide, 0.15f, false };

	// pre assign fixed text indices
	text_data.register_name("ui_settings_name", text_id::ui_settings_name);
	text_data.register_name("settings_header", text_id::settings_header);

	text_data.register_name("orientation_label", text_id::orientation_label);
	text_data.register_name("orientation_ltr", text_id::orientation_ltr);
	text_data.register_name("orientation_rtl", text_id::orientation_rtl);
	text_data.register_name("orientation_vltr", text_id::orientation_vltr);
	text_data.register_name("orientation_vrtl", text_id::orientation_vrtl);

	text_data.register_name("input_mode_label", text_id::input_mode_label);
	text_data.register_name("input_mode_keyboard_only", text_id::input_mode_keyboard_only);
	text_data.register_name("input_mode_mouse_only", text_id::input_mode_mouse_only);
	text_data.register_name("input_mode_controller_only", text_id::input_mode_controller_only);
	text_data.register_name("input_mode_controller_with_pointer", text_id::input_mode_controller_with_pointer);
	text_data.register_name("input_mode_mouse_and_keyboard", text_id::input_mode_mouse_and_keyboard);
	text_data.register_name("input_mode_follow_input", text_id::input_mode_follow_input);

	text_data.register_name("language_label", text_id::language_label);

	text_data.register_name("page_fraction", text_id::page_fraction);

	text_data.register_name("minimize_info", text_id::minimize_info);
	text_data.register_name("maximize_info", text_id::maximize_info);
	text_data.register_name("restore_info", text_id::restore_info);
	text_data.register_name("settings_info", text_id::settings_info);
	text_data.register_name("info_info", text_id::info_info);
	text_data.register_name("close_info", text_id::close_info);

	text_data.register_name("orientation_info", text_id::orientation_info);
	text_data.register_name("input_mode_info", text_id::input_mode_info);
	text_data.register_name("input_mode_mouse_info", text_id::input_mode_mouse_info);
	text_data.register_name("input_mode_automatic_info", text_id::input_mode_automatic_info);
	text_data.register_name("input_mode_controller_info", text_id::input_mode_controller_info);
	text_data.register_name("input_mode_controller_hybrid_info", text_id::input_mode_controller_hybrid_info);
	text_data.register_name("input_mode_keyboard_info", text_id::input_mode_keyboard_info);
	text_data.register_name("input_mode_mk_hybrid_info", text_id::input_mode_mk_hybrid_info);
	text_data.register_name("language_info", text_id::language_info);
	text_data.register_name("ui_settings_info", text_id::ui_settings_info);

	text_data.register_name("minimize_name", text_id::minimize_name);
	text_data.register_name("maximize_name", text_id::maximize_name);
	text_data.register_name("restore_name", text_id::restore_name);
	text_data.register_name("close_name", text_id::close_name);
	text_data.register_name("settings_name", text_id::settings_name);
	text_data.register_name("info_name", text_id::info_name);
	text_data.register_name("info_name_on", text_id::info_name_on);
	text_data.register_name("window_bar_name", text_id::window_bar_name);
	text_data.register_name("expandable_container_localized_name", text_id::expandable_container_localized_name);
	text_data.register_name("settings_tabs_name", text_id::settings_tabs_name);
	text_data.register_name("selection_list_localized_name", text_id::selection_list_localized_name);
	text_data.register_name("close_settings_name", text_id::close_settings_name);
	text_data.register_name("close_menu_name", text_id::close_menu_name);
	text_data.register_name("page_prev_name", text_id::page_prev_name);
	text_data.register_name("page_next_name", text_id::page_next_name);
	text_data.register_name("page_prev_prev_name", text_id::page_prev_prev_name);
	text_data.register_name("page_next_next_name", text_id::page_next_next_name);
	text_data.register_name("page_footer_name", text_id::page_footer_name);
	text_data.register_name("page_footer_info", text_id::page_footer_info);

	text_data.register_name("generic_toggle_on", text_id::generic_toggle_on);
	text_data.register_name("generic_toggle_off", text_id::generic_toggle_off);
	text_data.register_name("ui_animations_label", text_id::ui_animations_label);
	text_data.register_name("ui_animations_info", text_id::ui_animations_info);
}

int WINAPI wWinMain(
	HINSTANCE /*hInstance*/,
	HINSTANCE /*hPrevInstance*/,
	LPWSTR /*lpCmdLine*/,
	int /*nCmdShow*/
) {
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if(SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
		{
			std::shared_ptr<printui::text::text_services_wrapper> ts = std::make_shared<printui::text::win32_text_services>();
			ts->start_text_services();

			{
				main_window app(ts);

				app.create_window();
				app.message_loop();
			}

			ts->end_text_services();
		}
		CoUninitialize();
	}
	return 0;
}
