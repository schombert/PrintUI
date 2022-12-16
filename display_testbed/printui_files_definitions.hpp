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
		virtual std::wstring find_matching_file_name(std::wstring const& directory_and_filter) override;
		virtual void for_each_file(std::wstring const& directory, std::function<void(std::wstring const&)> const& fn) override;
		virtual void for_each_directory(std::wstring const& directory, std::function<void(std::wstring const&)> const& fn) override;
		virtual void for_each_filtered_file(std::wstring const& directory_and_filter, std::function<void(std::wstring const&)> const& fn) override;
		virtual void with_file_content(std::wstring const& file_name, std::function<void(std::string_view)> const& fn) override;
		virtual bool file_exists(std::wstring const& file_name) override;
		virtual bool directory_exists(std::wstring const& dir_name) override;
		virtual std::optional<std::wstring> resolve_file_path(std::wstring const& file_name, std::wstring const& subdirectory) override;
		virtual std::wstring get_root_directory() override;
		virtual std::wstring get_common_printui_directory() override;
	};
}
