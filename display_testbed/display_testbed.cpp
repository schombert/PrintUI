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

	/*IMPORTANT: MAKE ROOM FOR ALL DESIRED BRUSHES*/
	dynamic_settings.brushes.clear();
	dynamic_settings.brushes.push_back(printui::brush{ L"yellowpaper.png", printui::brush_color{0.95f,0.95f,0.95f}, true });
	dynamic_settings.brushes.push_back(printui::brush{ L"", printui::brush_color{0.0f,0.0f,0.0f}, false });
	
	dynamic_settings.texture_directory = L"textures";
	dynamic_settings.font_directory = L"fonts";
	dynamic_settings.icon_directory = L"icons";
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
