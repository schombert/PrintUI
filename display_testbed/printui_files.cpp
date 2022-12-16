#include "printui_utility.hpp"
#include "printui_files_definitions.hpp"

#include <Shlobj.h>

namespace printui {
	void win32_file_system::load_settings(window_data& win) {
		win.load_default_dynamic_settings();

		WCHAR module_name[MAX_PATH] = {};
		
		int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);

		int32_t last_path = path_used;
		for(; last_path >= 0 && module_name[last_path] != L'\\'; --last_path) {
		}
		std::wstring app_name(module_name + last_path + 1);
		if(app_name.ends_with(L".exe") || app_name.ends_with(L".dll")) {
			app_name.pop_back(); app_name.pop_back(); app_name.pop_back(); app_name.pop_back();
		}
		module_name[last_path + 1] = 0;

		wchar_t* local_path_out = nullptr;
		std::wstring local_settings;
		if(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_path_out) == S_OK) {
			local_settings = std::wstring(local_path_out) + L"\\printui\\" + app_name + L"\\settings.txt";
		}
		CoTaskMemFree(local_path_out);

		if(local_settings.length() > 0 && GetFileAttributes(local_settings.c_str()) != INVALID_FILE_ATTRIBUTES) {
			HANDLE file_handle = CreateFile(local_settings.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
			if(file_handle != INVALID_HANDLE_VALUE) {
				HANDLE mapping_handle = CreateFileMapping(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
				if(mapping_handle) {
					char const* mapped_bytes = (char const*)MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
					if(mapped_bytes) {
						_LARGE_INTEGER pvalue;
						GetFileSizeEx(file_handle, &pvalue);
						parse::settings_file(win.dynamic_settings, win.text_data.font_name_to_index, mapped_bytes, mapped_bytes + pvalue.QuadPart);

						UnmapViewOfFile(mapped_bytes);
					}
					CloseHandle(mapping_handle);
				}
				CloseHandle(file_handle);

				if(win.dynamic_settings.locale_is_default) {
					win.text_data.default_locale(win, false);
				} else {
					win.text_data.change_locale(win.dynamic_settings.locale_lang, win.dynamic_settings.locale_region, win, false);
				}
			} 
		} else {
			win.text_data.default_locale(win, true);
		}
	}

	void win32_file_system::save_settings(window_data& win) {

		WCHAR module_name[MAX_PATH] = {};

		int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);

		int32_t last_path = path_used;
		for(; last_path >= 0 && module_name[last_path] != L'\\'; --last_path) {
		}
		std::wstring app_name(module_name + last_path + 1);
		if(app_name.ends_with(L".exe") || app_name.ends_with(L".dll")) {
			app_name.pop_back(); app_name.pop_back(); app_name.pop_back(); app_name.pop_back();
		}

		wchar_t* local_path_out = nullptr;
		std::wstring base_path;
		if(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_path_out) == S_OK) {
			base_path = std::wstring(local_path_out) + L"\\printui";
		}
		CoTaskMemFree(local_path_out);

		
		std::thread file_writing([file_content = parse::create_settings_file(win.dynamic_settings), base_path, app_name, locale_name = win.text_data.locale_name()]() {
			CreateDirectory(base_path.c_str(), nullptr);
			auto app_path = base_path + L"\\" + app_name;
			CreateDirectory(app_path.c_str(), nullptr);
			auto file_name = app_path + L"\\settings.txt";

			HANDLE file_handle = CreateFile(file_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
			if(file_handle != INVALID_HANDLE_VALUE) {
				WriteFile(file_handle, file_content.c_str(), DWORD(file_content.length()), nullptr, nullptr);
				SetEndOfFile(file_handle);
				CloseHandle(file_handle);
			}
		});
		file_writing.detach();
	}

	void win32_file_system::load_global_font_fallbacks(launch_settings& ls) {
		wchar_t* local_path_out = nullptr;
		if(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_path_out) != S_OK) {
			CoTaskMemFree(local_path_out);
			return;
		}

		std::wstring fallback_def_file = std::wstring(local_path_out) = L"\\printui\\font_fallbacks.txt";
		CoTaskMemFree(local_path_out);

		HANDLE file_handle = CreateFileW(fallback_def_file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(file_handle != INVALID_HANDLE_VALUE) {
			HANDLE mapping_handle = CreateFileMapping(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if(mapping_handle) {
				char const* mapped_bytes = (char const*)MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);

				if(mapped_bytes) {
					_LARGE_INTEGER pvalue;
					GetFileSizeEx(file_handle, &pvalue);
					parse::font_fallbacks_file(ls.fallbacks, mapped_bytes, mapped_bytes + pvalue.QuadPart);
					UnmapViewOfFile(mapped_bytes);
				}
					
				CloseHandle(mapping_handle);
			}
			CloseHandle(file_handle);
		}

	}

	std::wstring win32_file_system::find_matching_file_name(std::wstring const& directory_and_filter) {
		WIN32_FIND_DATAW fdata;
		HANDLE search_handle = FindFirstFile(directory_and_filter.c_str(), &fdata);
		if(search_handle != INVALID_HANDLE_VALUE) {
			FindClose(search_handle);
			return std::wstring(fdata.cFileName);
		} else {
			return std::wstring();
		}
	}

	void win32_file_system::for_each_filtered_file(std::wstring const& directory_and_filter, std::function<void(std::wstring const&)> const& fn) {

		WIN32_FIND_DATAW fdata;
		HANDLE search_handle = FindFirstFile(directory_and_filter.c_str(), &fdata);
		if(search_handle != INVALID_HANDLE_VALUE) {
			do {
				fn(std::wstring(fdata.cFileName));
			} while(FindNextFile(search_handle, &fdata));
			FindClose(search_handle);
		}
	}

	void win32_file_system::with_file_content(std::wstring const& file_name, std::function<void(std::string_view)> const& fn) {

		HANDLE file_handle = CreateFileW(file_name.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
		if(file_handle != INVALID_HANDLE_VALUE) {
			HANDLE mapping_handle = CreateFileMapping(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if(mapping_handle) {
				char const* mapped_bytes = (char const*)MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
				if(mapped_bytes) {
					_LARGE_INTEGER pvalue;
					GetFileSizeEx(file_handle, &pvalue);
					fn(std::string_view(mapped_bytes, mapped_bytes + pvalue.QuadPart));
					UnmapViewOfFile(mapped_bytes);
				}
				CloseHandle(mapping_handle);
			}
			CloseHandle(file_handle);
		}
	}

	bool win32_file_system::file_exists(std::wstring const& file_name) {
		auto a = GetFileAttributes(file_name.c_str());
		return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}
	bool win32_file_system::directory_exists(std::wstring const& dir_name) {
		auto a = GetFileAttributes(dir_name.c_str());
		return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	void win32_file_system::for_each_file(std::wstring const& directory, std::function<void(std::wstring const&)> const& fn) {
		std::wstring wfilter = directory + L"\\*";
		WIN32_FIND_DATAW fdata;
		HANDLE search_handle = FindFirstFile(wfilter.c_str(), &fdata);
		if(search_handle != INVALID_HANDLE_VALUE) {
			do {
				if((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					fn(std::wstring(fdata.cFileName));
			} while(FindNextFile(search_handle, &fdata));
			FindClose(search_handle);
		}
	}

	void win32_file_system::for_each_directory(std::wstring const& directory, std::function<void(std::wstring const&)> const& fn) {
		std::wstring wfilter = directory + L"\\*";
		WIN32_FIND_DATAW fdata;
		HANDLE search_handle = FindFirstFile(wfilter.c_str(), &fdata);
		if(search_handle != INVALID_HANDLE_VALUE) {
			do {
				if((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && fdata.cFileName != std::wstring(L".") && fdata.cFileName != std::wstring(L"..")) {
					fn(std::wstring(fdata.cFileName));
				}
			} while(FindNextFile(search_handle, &fdata));
			FindClose(search_handle);
		}
	}

	std::optional<std::wstring> win32_file_system::resolve_file_path(std::wstring const& file_name, std::wstring const& subdirectory) {
		{
			WCHAR module_name[MAX_PATH] = {};
			int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);
			while(path_used >= 0 && module_name[path_used] != L'\\') {
				module_name[path_used] = 0;
				--path_used;
			}
			std::wstring rel_name = std::wstring(module_name) + subdirectory + L"\\" + file_name;

			if(GetFileAttributes(rel_name.c_str()) != INVALID_FILE_ATTRIBUTES)
				return rel_name;
		}
		{
			wchar_t* local_path_out = nullptr;
			std::wstring app_name;
			if(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_path_out) == S_OK) {
				app_name = std::wstring(local_path_out) + L"\\printui\\" + subdirectory + L"\\" + file_name;
			}
			CoTaskMemFree(local_path_out);

			if(app_name.length() > 0 && GetFileAttributes(app_name.c_str()) != INVALID_FILE_ATTRIBUTES)
				return app_name;
		}

		return std::optional<std::wstring>{};
	}

	std::wstring win32_file_system::get_root_directory() {
		WCHAR module_name[MAX_PATH] = {};
		int32_t path_used = GetModuleFileName(nullptr, module_name, MAX_PATH);
		while(path_used >= 0 && module_name[path_used] != L'\\') {
			module_name[path_used] = 0;
			--path_used;
		}
		return module_name;
	}

	std::wstring win32_file_system::get_common_printui_directory() {
		wchar_t* local_path_out = nullptr;
		std::wstring result;
		if(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_path_out) == S_OK) {
			result = std::wstring(local_path_out) + L"\\printui\\";
		}
		CoTaskMemFree(local_path_out);
		return result;
	}
}
