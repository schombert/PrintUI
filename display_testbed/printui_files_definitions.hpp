#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "printui_utility.hpp"
#include <Windows.h>

namespace printui {
	struct win32_file_system : public file_system_wrapper {
		virtual void load_settings(window_data& win) override;
		virtual void save_settings(window_data& win) override;
		virtual void load_global_font_fallbacks(launch_settings& ls) override;
	};
}
