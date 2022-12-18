#ifndef PRINTUI_FILES_HEADER
#define PRINTUI_FILES_HEADER

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "printui_datatypes.hpp"
#include <Windows.h>

namespace printui {
	struct win32_file_system {
		void load_settings(window_data& win);
		void save_settings(window_data& win);
		void load_global_font_fallbacks(launch_settings& ls);
		std::wstring find_matching_file_name(std::wstring const& directory_and_filter) const;
		void for_each_file(std::wstring const& directory, std::function<void(std::wstring const&)> const& fn) const;
		void for_each_directory(std::wstring const& directory, std::function<void(std::wstring const&)> const& fn) const;
		void for_each_filtered_file(std::wstring const& directory_and_filter, std::function<void(std::wstring const&)> const& fn) const;
		void with_file_content(std::wstring const& file_name, std::function<void(std::string_view)> const& fn) const;
		bool file_exists(std::wstring const& file_name) const;
		bool directory_exists(std::wstring const& dir_name) const;
		std::optional<std::wstring> resolve_file_path(std::wstring const& file_name, std::wstring const& subdirectory) const;
		std::wstring get_root_directory() const;
		std::wstring get_common_printui_directory() const;
	};
}

#endif
