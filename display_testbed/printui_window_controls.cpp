#include "printui_utility.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <d2d1_3.h>
#include <dwrite_3.h>

namespace printui {
	title_bar_element::~title_bar_element() {
	}

	layout_node_type title_bar_element::get_node_type() {
		return layout_node_type::visible;
	}

	void title_bar_element::set_text_content(uint16_t text_id) {
		text.set_text(text_id);
		text.text_alignment = content_alignment::centered;
	}

	void title_bar_element::render_foreground(ui_rectangle const& rect, window_data& win) {
		text.relayout_text(win, screen_space_point{rect.width, rect.height});
		text.draw_text(win, rect.x_position, rect.y_position);
	}

	ui_rectangle title_bar_element::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle retvalue;

		retvalue.background_index = parent_foreground_index;
		retvalue.foreground_index = parent_background_index;
		retvalue.parent_object = this;

		return retvalue;
	}

	void title_bar_element::recreate_contents(window_data& , layout_node& ) {
	}

	simple_layout_specification title_bar_element::get_specification(window_data const&) {
		simple_layout_specification spec;

		spec.minimum_page_size = 1ui16;
		spec.minimum_line_size = 4ui16;

		spec.page_flags = size_flags::none;
		spec.line_flags = size_flags::fill_to_max;
		
		return spec;
	}

	vertical_2x2_max_icon::vertical_2x2_max_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"max_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;

		restore_ico.file_name = L"restore_i.svg";
		restore_ico.xsize = 2;
		restore_ico.ysize = 2;
		restore_ico.edge_padding = 0.5f;
	}
	void vertical_2x2_max_icon::on_click(window_data& win, uint32_t, uint32_t) {
		if(win.window_interface->is_maximized())
			win.window_interface->restore();
		else
			win.window_interface->maximize();
	}
	void vertical_2x2_max_icon::on_right_click(window_data&, uint32_t, uint32_t) {

	}
	void vertical_2x2_max_icon::render_foreground(ui_rectangle const& rect, window_data& win) {
		auto new_top_left = screen_topleft_from_layout_in_ui(win, 0, 1, 2, 2, rect);

		if(win.window_interface->is_maximized())
			restore_ico.present_image(float(new_top_left.x), float(new_top_left.y), win.d2d_device_context, win.dummy_brush);
		else
			ico.present_image(float(new_top_left.x), float(new_top_left.y), win.d2d_device_context, win.dummy_brush);

	}

	vertical_2x2_min_icon::vertical_2x2_min_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"min_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;
	}
	void vertical_2x2_min_icon::on_click(window_data& win, uint32_t, uint32_t) {
		win.window_interface->minimize();
	}
	void vertical_2x2_min_icon::on_right_click(window_data&, uint32_t, uint32_t) {

	}

	vertical_2x2_settings_icon::vertical_2x2_settings_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"settings_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;
	}
	void vertical_2x2_settings_icon::on_click(window_data& win, uint32_t, uint32_t) {
		if(win.window_bar.expanded_show_settings)
			win.hide_settings_panel();
		else
			win.show_settings_panel();

		
	}
	void vertical_2x2_settings_icon::on_right_click(window_data&, uint32_t, uint32_t) {

	}
	void vertical_2x2_info_icon::mark_for_update(window_data& win) {
		if(l_id != layout_reference_none && win.get_node(l_id).visible_rect != ui_reference_none) {
			auto const u_id = win.get_node(l_id).visible_rect;
			auto& ui_rects = win.get_ui_rects();

			RECT icon_rect{ ui_rects[u_id].x_position, ui_rects[u_id].y_position, ui_rects[u_id].x_position + ui_rects[u_id].width, ui_rects[u_id].y_position + ui_rects[u_id].height };

			win.window_interface->invalidate_window();
		}
	}
	void vertical_2x2_info_icon::render_composite(ui_rectangle const& r, window_data& win, bool under_mouse) {

		D2D1_RECT_F content_rect{
			float(r.x_position), float(r.y_position),
			float(r.x_position + r.width), float(r.y_position + r.height) };

		if(!win.pending_right_click) {
			render::background_rectangle(content_rect, win, r.display_flags, r.background_index, under_mouse);

			win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[r.foreground_index], D2D1_OPACITY_MASK_CONTENT_GRAPHICS, content_rect, content_rect);
		} else {
			render::background_rectangle(content_rect, win, r.display_flags & ~ui_rectangle::flag_skip_bg, r.foreground_index, under_mouse);

			win.d2d_device_context->FillOpacityMask(win.foreground, win.palette[r.background_index], D2D1_OPACITY_MASK_CONTENT_GRAPHICS, content_rect, content_rect);
		}
	}
	vertical_2x2_info_icon::vertical_2x2_info_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"alt_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;
	}
	void vertical_2x2_info_icon::on_click(window_data& win, uint32_t, uint32_t) {
		win.pending_right_click = !win.pending_right_click;
		mark_for_update(win);
	}
	void vertical_2x2_info_icon::on_right_click(window_data&, uint32_t, uint32_t) {

	}
	ui_rectangle vertical_2x2_info_icon::prototype_ui_rectangle(window_data const& win, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle revalue = vertical_2x2_icon_base::prototype_ui_rectangle(win, parent_foreground_index, parent_background_index);
		standard_fg = parent_foreground_index;
		standard_bg = parent_background_index;
		if(win.pending_right_click) {
			std::swap(revalue.background_index, revalue.foreground_index);
		}
		return revalue;
	}


	vertical_2x2_close_icon::vertical_2x2_close_icon() : vertical_2x2_icon_base() {
		ico.file_name = L"close_i.svg";
		ico.xsize = 2;
		ico.ysize = 2;
		ico.edge_padding = 0.5f;
	}
	void vertical_2x2_close_icon::on_click(window_data& win, uint32_t, uint32_t) {
		win.window_interface->close();
	}
	void vertical_2x2_close_icon::on_right_click(window_data&, uint32_t, uint32_t) {

	}

	window_bar_element::window_bar_element(window_data const& win, bool mn, bool mx, bool settings, std::vector<settings_menu_item> const& app_settings) : info_i(), close_i(), settings_pages(win, app_settings) {
		if(mn)
			min_i.emplace();
		if(mx)
			max_i.emplace();
		if(settings)
			setting_i.emplace();
	}
	layout_node_type window_bar_element::get_node_type() {
		return layout_node_type::container;
	}
	ui_rectangle window_bar_element::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.right_border = 1ui8;

		return retvalue;
	}
	layout_rect window_bar_element::get_content_rectangle(window_data& win) {
		if(!expanded_show_settings || settings_pages.l_id == layout_reference_none) {
			return layout_rect{ 0i16, 0i16, 0i16, 0i16 };
		} else {
			return layout_rect{ 0i16, 0i16, int16_t(win.get_node(settings_pages.l_id).width), 0i16 };
		}
	}
	simple_layout_specification window_bar_element::get_specification(window_data const& ) {
		simple_layout_specification spec;

		spec.minimum_page_size = 4ui16;
		spec.minimum_line_size = 2ui16;

		spec.page_flags = size_flags::fill_to_max_single_col;
		spec.line_flags = size_flags::none;

		return spec;
	}

	auto_layout_specification window_bar_element::get_auto_layout_specification(window_data const& ) {
		auto_layout_specification spec;

		spec.orientation = content_orientation::page;
		spec.internal_alignment = content_alignment::trailing;
		spec.group_alignment = content_alignment::trailing;

		return spec;
	}
	void window_bar_element::recreate_contents(window_data& win, layout_node& n) {
		std::vector<layout_interface*> buttons;
		buttons.reserve(5);

		auto const bar_height = n.height;

		if(setting_i.has_value()) {
			buttons.push_back(&(*setting_i));
		}
		buttons.push_back(&info_i);

		if(min_i.has_value()) {
			buttons.push_back(&(*min_i));
		}
		if(max_i.has_value()) {
			buttons.push_back(&(*max_i));
		}
		buttons.push_back(&close_i);

		default_recreate_container(win, this, &n, buttons);

		auto existing_sp_width = settings_pages.l_id != layout_reference_none ? win.get_node(settings_pages.l_id).width : 6;
		auto settings_pages_id = win.create_node(&settings_pages, existing_sp_width, bar_height, true);
		auto& pages_node = win.get_node(settings_pages_id);
		pages_node.parent = l_id;
		win.get_node(l_id).container_info()->children.push_back(settings_pages_id);
		pages_node.x = 2;
		pages_node.ignore = !expanded_show_settings;
	}
}
