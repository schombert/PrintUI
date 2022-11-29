#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include "printui_utility.hpp"
#include "printui_accessibility_definitions.hpp"
#include "printui_windows_definitions.hpp"

class main_window : public printui::window_data {
public:
	virtual void load_default_dynamic_settings() override;
	virtual void load_primary_font_fallbacks(IDWriteFontFallbackBuilder*) override {
	}
	virtual void load_small_font_fallbacks(IDWriteFontFallbackBuilder*) override {
	}
	virtual void client_on_dpi_change() override {
	}
	virtual void client_on_resize(uint32_t, uint32_t) override {
	}
	std::vector<printui::settings_menu_item> get_settings_items() const {
		return std::vector<printui::settings_menu_item>{};
	}
	main_window() : printui::window_data(true, true, true, get_settings_items(), std::make_unique<printui::os_win32_wrapper>(), std::make_unique<printui::win32_accessibility>(*this)) {
	}
};

