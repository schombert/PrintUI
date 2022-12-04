#include "printui_utility.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

namespace printui {
	void settings_item_button::button_action(window_data& win) {
		auto pi = win.get_node(win.window_bar.settings_pages.l_id).page_info();
		if(pi) {
			win.window_bar.settings_pages.go_to_page(win, id, *pi);
		}
	}

	auto close_settings_pages_fn = [](window_data& win, layout_reference) {
		win.hide_settings_panel();
	};


	settings_page_container::settings_page_container(window_data& win, std::vector<settings_menu_item> const& app_settings) : page_header(text_id::close_settings_name, close_settings_pages_fn) {
		subpage_selection_list.name_id = text_id::settings_tabs_name;

		settings_items.reserve(app_settings.size() + 1);
		subpage_selection_list.options.reserve(app_settings.size() + 1);

		uint32_t count = 0;
		for(auto& i : app_settings) {
			settings_items.emplace_back(std::make_unique<settings_item_button>(count, i.settings_contents));
			settings_items.back()->set_text(win, uint16_t(i.text));
			settings_items.back()->set_alt_text(win, i.alt_text);
			settings_items.back()->interior_right_margin = 1;
			settings_items.back()->interior_left_margin = 2;
			settings_items.back()->set_text_alignment(content_alignment::trailing);
			++count;

			subpage_selection_list.options.push_back(settings_items.back().get());
		}

		settings_items.emplace_back(std::make_unique<settings_item_button>(count, (layout_interface*)(&win.window_bar.print_ui_settings)));
		settings_items.back()->set_text(win, text_id::ui_settings_name);
		settings_items.back()->set_alt_text(win, text_id::ui_settings_info);
		settings_items.back()->interior_right_margin = 1;
		settings_items.back()->interior_left_margin = 2;
		settings_items.back()->set_text_alignment(content_alignment::trailing);

		subpage_selection_list.options.push_back(settings_items.back().get());

		settings_items.front()->set_selected(win, true);

		page_header.text.set_text(text_id::settings_header);
	}

	ui_rectangle settings_page_container::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {
		ui_rectangle retvalue;

		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		retvalue.left_border = 1;
		return retvalue;
	}
	simple_layout_specification settings_page_container::get_specification(window_data& win) {
		simple_layout_specification spec;

		spec.minimum_page_size = uint16_t(5 + settings_items.size());
		spec.minimum_line_size = uint16_t(6 + subpage_selection_list.get_specification(win).minimum_line_size);
		spec.page_flags = size_flags::fill_to_max;
		spec.line_flags = size_flags::match_content;

		return spec;
	}
	page_layout_specification settings_page_container::get_page_layout_specification(window_data const&) {
		page_layout_specification retvalue;
		retvalue.header = &page_header;

		retvalue.page_leading_margin_min = 0ui8;
		retvalue.page_leading_margin_max = 0ui8;
		retvalue.page_trailing_margin_min = 0ui8;
		retvalue.page_trailing_margin_max = 0ui8;
		retvalue.min_column_line_size = 6ui8;

		return retvalue;
	}

	accessibility_object* settings_page_container::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface->make_container_accessibility_interface(win, this, text_id::settings_header);
		}
		return acc_obj;
	}

	void settings_page_container::recreate_contents(window_data& win, layout_node& n) {
		auto pi = n.page_info();
		int32_t resolved_height = n.height;
		int32_t resolved_width = n.width;

		auto open_list_spec = subpage_selection_list.get_specification(win);

		auto column_container_id = win.create_node(&subpage_selection_list, open_list_spec.minimum_line_size, open_list_spec.minimum_page_size, false);
		win.get_node(column_container_id).parent = l_id;

		int32_t column_height = win.get_node(column_container_id).height;
		int32_t column_width = win.get_node(column_container_id).width;
		resolved_height = std::max(column_height + 1, resolved_height);
		resolved_width = std::max(column_width, resolved_width);

		//create each settings page

		int32_t max_child_height = 0;
		int32_t max_child_width = 0;
		for(auto& si : settings_items) {

			auto node_id = win.create_node(si->settings_contents, resolved_width - column_width, resolved_height - 1, true);

			win.immediate_add_child(l_id, node_id);

			auto& child_page = win.get_node(node_id);
			child_page.x = uint16_t(column_width);
			child_page.y = 1ui16;

			max_child_width = std::max(max_child_width, int32_t(child_page.width));
			max_child_height = std::max(max_child_height, int32_t(child_page.height));
		}

		{
			auto& self = win.get_node(l_id);
			self.height = uint16_t(std::max(max_child_height + 1, column_height + 2));
			resolved_height = self.height;
			self.width = std::max(self.width, uint16_t(max_child_width + column_width));
			resolved_width = self.width;
		}

		for(auto node_id : pi->view_columns()) {
			auto& child_page = win.get_node(node_id);
			win.immediate_resize(child_page, resolved_width - column_width, resolved_height - 1);
		}

		{
			auto& column_container = win.get_node(column_container_id);
			column_container.x = 0ui16;
			column_container.y = uint16_t(1 + ((resolved_height - 1) - column_container.height) / 2);
		}

		pi->modify_columns().insert(pi->view_columns().begin() + pi->subpage_offset, column_container_id);
		{
			auto& self = win.get_node(l_id);
			pi->header = win.create_node(&page_header, self.width, 1, false);
			win.get_node(pi->header).parent = l_id;
		}
	}

	void settings_page_container::go_to_page(window_data& win, uint32_t i, page_information& pi) {
		if(pi.subpage_offset != i) {
			for(auto& c : settings_items) {
				if(c->id == i)
					c->set_selected(win, true);
				else
					c->set_selected(win, false);
			}
			auto old_sub_off = pi.subpage_offset == 0 ? 0 : pi.subpage_divisions[pi.subpage_offset - 1];
			auto new_sub_off = i == 0 ? 0 : pi.subpage_divisions[i - 1];
			if(pi.subpage_offset < i) {
				std::rotate(pi.modify_columns().data() + old_sub_off, pi.modify_columns().data() + old_sub_off + 1,
					pi.modify_columns().data() + new_sub_off);
				for(auto j = pi.subpage_offset; j < i; ++j) {
					pi.subpage_divisions[j] -= 1;
				}
			} else {
				std::rotate(pi.modify_columns().data() + new_sub_off, pi.modify_columns().data() + old_sub_off,
					pi.modify_columns().data() + old_sub_off + 1);
				for(auto j = i; j < pi.subpage_offset; ++j) {
					pi.subpage_divisions[j] += 1;
				}
			}

			pi.subpage_offset = uint16_t(i);
		}

		if(acc_obj) {
			win.accessibility_interface->on_contents_changed(acc_obj);
		}
	}

	common_printui_settings::common_printui_settings() : language_label(text_id::language_label, content_alignment::leading), orientation_label(text_id::orientation_label, content_alignment::leading), input_mode_label(text_id::input_mode_label, content_alignment::leading) {

		lang_menu.open_button.set_text_alignment(content_alignment::trailing);
		lang_menu.page_size = 1;
		lang_menu.line_size = 10;

		orientation_list.text_alignment = content_alignment::trailing;

		input_mode_list.text_alignment = content_alignment::trailing;

		language_label.alt_text = text_id::language_info;
		lang_menu.alt_text = text_id::language_info;

		orientation_label.alt_text = text_id::orientation_info;
		orientation_list.alt_text_id = text_id::orientation_info;

		input_mode_label.alt_text = text_id::input_mode_info;
		input_mode_list.alt_text_id = text_id::input_mode_info;
	}

	accessibility_object* common_printui_settings::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface->make_container_accessibility_interface(win, this, text_id::ui_settings_name);
		}
		return acc_obj;
	}

	void common_printui_settings::go_to_page(window_data& win, uint32_t i, page_information& pi) {
		layout_interface::go_to_page(win, i, pi);
		if(acc_obj) {
			win.accessibility_interface->on_contents_changed(acc_obj);
		}
	}
	ui_rectangle common_printui_settings::prototype_ui_rectangle(window_data const&, uint8_t parent_foreground_index, uint8_t parent_background_index) {

		ui_rectangle retvalue;
		retvalue.left_border = 0;
		retvalue.background_index = parent_background_index;
		retvalue.foreground_index = parent_foreground_index;
		return retvalue;
	}
	simple_layout_specification common_printui_settings::get_specification(window_data&) {
		simple_layout_specification spec;

		spec.minimum_page_size = 5ui16;
		spec.minimum_line_size = 8ui16;
		spec.page_flags = size_flags::fill_to_max;
		spec.line_flags = size_flags::fill_to_max;

		return spec;
	}
	page_layout_specification common_printui_settings::get_page_layout_specification(window_data const&) {
		page_layout_specification retvalue;
		retvalue.header = &header;
		retvalue.footer = &footer;

		retvalue.page_leading_margin_min = 0ui8;
		retvalue.page_leading_margin_max = 0ui8;
		retvalue.page_trailing_margin_min = 0ui8;
		retvalue.page_trailing_margin_max = 1ui8;
		retvalue.min_column_line_size = 6ui8;
		retvalue.column_content_alignment = content_alignment::justified;
		retvalue.column_in_page_alignment = content_alignment::leading;

		return retvalue;
	}
	void common_printui_settings::recreate_contents(window_data& win, layout_node& n) {
		std::vector<layout_interface*> contents;
		contents.push_back(&language_label);
		contents.push_back(&lang_menu);
		contents.push_back(&orientation_label);
		contents.push_back(&orientation_list);
		contents.push_back(&input_mode_label);
		contents.push_back(&input_mode_list);
		default_recreate_page(win, this, &n, contents);
	}

	void settings_orientation_list::on_select(window_data& win, size_t v) {
		win.change_orientation(layout_orientation(v));
	}
	list_option_description settings_orientation_list::describe_option(window_data const&, uint32_t i) {
		switch(i) {
			case 0:
				return list_option_description{ size_t(layout_orientation::horizontal_left_to_right), text_id::orientation_ltr, uint16_t(-1) };
			case 1:
				return list_option_description{ size_t(layout_orientation::horizontal_right_to_left), text_id::orientation_rtl, uint16_t(-1) };
			case 2:
				return list_option_description{ size_t(layout_orientation::vertical_left_to_right), text_id::orientation_vltr, uint16_t(-1) };
			case 3:
				return list_option_description{ size_t(layout_orientation::vertical_right_to_left), text_id::orientation_vrtl, uint16_t(-1) };
			default:
				return list_option_description{0, uint16_t(-1), uint16_t(-1) };
		}
	}

	void settings_input_mode_list::on_select(window_data& win, size_t v) {
		win.switch_input_mode(input_mode(v));
	}
	list_option_description settings_input_mode_list::describe_option(window_data const&, uint32_t i) {
		switch(i) {
			case 0:
				return list_option_description{ size_t(input_mode::follow_input), text_id::input_mode_follow_input, text_id::input_mode_automatic_info };
			case 1:
				return list_option_description{ size_t(input_mode::mouse_and_keyboard), text_id::input_mode_mouse_and_keyboard, text_id::input_mode_mk_hybrid_info };
			case 2:
				return list_option_description{ size_t(input_mode::mouse_only), text_id::input_mode_mouse_only, text_id::input_mode_mouse_info };
			case 3:
				return list_option_description{ size_t(input_mode::keyboard_only), text_id::input_mode_keyboard_only, text_id::input_mode_keyboard_info };
			case 4:
				return list_option_description{ size_t(input_mode::controller_only), text_id::input_mode_controller_only, text_id::input_mode_controller_info };
			case 5:
				return list_option_description{ size_t(input_mode::controller_with_pointer), text_id::input_mode_controller_with_pointer, text_id::input_mode_controller_hybrid_info };
			default:
				return list_option_description{ 0, uint16_t(-1), uint16_t(-1) };
		}
	}
	void settings_input_mode_list::on_create(window_data& win) {
		get_option(size_t(input_mode::controller_with_pointer))->set_disabled(win, true);
		get_option(size_t(input_mode::controller_only))->set_disabled(win, true);
	}

	void language_button::button_action(window_data& win) {
		win.text_data.change_locale(lang, region, win);
		win.window_bar.print_ui_settings.lang_menu.close_menu(win, true);
	}

	std::vector<layout_interface*> language_menu::get_options(window_data& win) {
		std::vector<layout_interface*> result;

		if(lbuttons.size() == 0) {
			auto langs = text::ennumerate_languages(win);
			for(auto& l : langs) {
				auto ptr = std::make_unique<language_button>();
				ptr->lang = l.language;
				ptr->region = l.region;
				ptr->set_text(win, l.display_name);
				ptr->interior_left_margin = 2;
				ptr->interior_right_margin = 1;
				ptr->set_text_alignment(content_alignment::leading);
				ptr->set_selected(win, win.text_data.is_current_locale(l.language, l.region));
				lbuttons.push_back(std::move(ptr));
			}
		}

		for(auto& ptr : lbuttons) {
			result.push_back(ptr.get());
		}
		return result;
	}
	void language_menu::on_open(window_data& win) {
		for(auto& ptr : win.window_bar.print_ui_settings.lang_menu.lbuttons) {
			language_button* b = static_cast<language_button*>(ptr.get());
			b->set_selected(win, win.text_data.is_current_locale(b->lang, b->region));
		}
	}
	void language_menu::on_close(window_data&) {

	}
}

