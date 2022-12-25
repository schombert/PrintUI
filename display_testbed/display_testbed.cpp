#include "printui_datatypes.hpp"
#include "printui_text_definitions.hpp"
#include "printui_accessibility_definitions.hpp"
#include "printui_files_definitions.hpp"
#include "printui_render_definitions.hpp"
#include "printui_windows_definitions.hpp"
#include "printui_main_header.hpp"
#include "printui_accessibility.cpp"
#include "printui_common_controls.cpp"
#include "printui_files.cpp"
#include "printui_interactables.cpp"
#include "printui_layout.cpp"
#include "printui_parsing.cpp"
#include "printui_rendering.cpp"
#include "printui_settings_controls.cpp"
#include "printui_text.cpp"
#include "printui_utility.cpp"
#include "printui_window_controls.cpp"
#include "printui_window_functions.cpp"


void printui::window_data::client_on_dpi_change() {
}
void printui::window_data::client_on_resize(uint32_t, uint32_t) {
}
std::vector<printui::settings_menu_item> printui::window_data::get_settings_items() const {
	return std::vector<printui::settings_menu_item>{};
}

void printui::window_data::register_icons() {
	window_bar.settings_pages.section_bottom_decorations = rendering_interface.load_icon(L"1x1_087.svg", 0.1f, 1, 1);
	large_centered_header::left_decoration = rendering_interface.load_icon(L"S1x1_001Left.svg", 0.2f, 2, 2);
	large_centered_header::right_decoration = rendering_interface.load_icon(L"S1x1_001Right.svg", 0.2f, 2, 2);
}

void printui::window_data::load_default_dynamic_settings() {
	dynamic_settings.window_border = 3;
	dynamic_settings.layout_base_size = 22;

	/*IMPORTANT: MAKE ROOM FOR ALL DESIRED BRUSHES*/
	dynamic_settings.brushes.clear();
	dynamic_settings.brushes.push_back(printui::brush{ L"yellowpaper.png", printui::brush_color{0.95f,0.95f,0.95f}, true });
	dynamic_settings.brushes.push_back(printui::brush{ L"", printui::brush_color{0.0f,0.0f,0.0f}, false });
	dynamic_settings.brushes.push_back(printui::brush{ L"", printui::brush_color{0.811764f,0.184313f,0.086274f}, false });

	dynamic_settings.texture_directory = L"textures";
	dynamic_settings.font_directory = L"fonts";
	dynamic_settings.icon_directory = L"icons";
	dynamic_settings.text_directory = L"locale";
	dynamic_settings.primary_font.name = L"Libre Caslon Text";
	dynamic_settings.primary_font.type = printui::font_type::roman;
	dynamic_settings.primary_font.top_leading = 0;
	dynamic_settings.primary_font.bottom_leading = 1;

	dynamic_settings.dark_interactable_brush = 2;
	dynamic_settings.light_interactable_brush = 2;

	dynamic_settings.small_font.name = L"NewComputerModern10";
	dynamic_settings.small_font.type = printui::font_type::roman;

	dynamic_settings.header_font.name = L"Libre Caslon Text";
	dynamic_settings.header_font.type = printui::font_type::roman;
	dynamic_settings.header_font.top_leading = 0;
	dynamic_settings.header_font.bottom_leading = 1;

	// define shared animation effects
	printui::list_control::list_appearance = printui::animation_defintion{printui::animation_type::fade, 0.25f, true};
	printui::list_control::list_disappearance = printui::animation_defintion{ printui::animation_type::fade, 0.25f, false };

	printui::menu_control::list_appearance = printui::animation_defintion{ printui::animation_type::fade, 0.25f, true };
	printui::menu_control::list_disappearance = printui::animation_defintion{ printui::animation_type::fade, 0.25f, false };

	printui::page_footer::footer_appearance = printui::animation_defintion{ printui::animation_type::slide, 0.15f, true };
	printui::page_footer::footer_disappearance = printui::animation_defintion{ printui::animation_type::slide, 0.15f, false };


	printui::page_footer::page_turn_up = printui::animation_defintion{ printui::animation_type::flip, 0.15f, false };
	printui::page_footer::page_turn_down = printui::animation_defintion{ printui::animation_type::flip, 0.15f, true };
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
			printui::window_data app(true, true, true);

			app.text_services_interface.start_text_services();
			app.create_window();
			app.message_loop();
			app.text_services_interface.end_text_services();

		}
		CoUninitialize();
	}
	return 0;
}
