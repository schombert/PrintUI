#include "printui_main_header.hpp"

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

		subpage_selection_list.right_margin = 1;

		uint32_t count = 0;
		for(auto& i : app_settings) {
			settings_items.emplace_back(std::make_unique<settings_item_button>(count, i.settings_contents));
			settings_items.back()->set_text(win, uint16_t(i.text));
			settings_items.back()->set_alt_text(win, i.alt_text);
			settings_items.back()->set_text_alignment(content_alignment::trailing);
			++count;

			subpage_selection_list.options.push_back(settings_items.back().get());
		}

		settings_items.emplace_back(std::make_unique<settings_item_button>(count, (layout_interface*)(&win.window_bar.print_ui_settings)));
		settings_items.back()->set_text(win, text_id::ui_settings_name);
		settings_items.back()->set_alt_text(win, text_id::ui_settings_info);
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

	accessibility_object* settings_page_container::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_container_accessibility_interface(win, this, text_id::settings_header);
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
		resolved_height = std::max(column_height + 2, resolved_height);
		resolved_width = std::max(column_width, resolved_width);

		//create each settings page

		int32_t max_child_height = 0;
		int32_t max_child_width = 0;
		for(auto& si : settings_items) {

			auto node_id = win.create_node(si->settings_contents, resolved_width - column_width, resolved_height - 2, true);

			win.immediate_add_child(l_id, node_id);

			auto& child_page = win.get_node(node_id);
			child_page.x = uint16_t(column_width);
			child_page.y = 2ui16;

			max_child_width = std::max(max_child_width, int32_t(child_page.width));
			max_child_height = std::max(max_child_height, int32_t(child_page.height));
		}

		{
			auto& self = win.get_node(l_id);
			self.height = uint16_t(std::max(max_child_height + 2, column_height + 2));
			resolved_height = self.height;
			self.width = std::max(self.width, uint16_t(max_child_width + column_width));
			resolved_width = self.width;
		}

		for(auto node_id : pi->view_columns()) {
			auto& child_page = win.get_node(node_id);
			win.immediate_resize(child_page, resolved_width - column_width, resolved_height - 2);
		}

		{
			auto& column_container = win.get_node(column_container_id);
			column_container.x = 0ui16;
			column_container.y = uint16_t(2 + ((resolved_height - 2) - column_container.height) / 2);
		}

		pi->modify_columns().insert(pi->view_columns().begin() + pi->subpage_offset, column_container_id);
		{
			auto& self = win.get_node(l_id);
			pi->header = win.create_node(&page_header, self.width, 2, false);
			win.immediate_add_child(l_id, pi->header);
			win.get_node(pi->header).y = 0;
			win.get_node(pi->header).x = 0;
		}
	}

	void settings_page_container::go_to_page(window_data& win, uint32_t i, page_information& pi) {
		if(pi.subpage_offset != i) {
			auto old_page = pi.subpage_offset;

			layout_interface::go_to_page(win, i, pi);

			for(auto& c : settings_items) {
				if(c->id == i)
					c->set_selected(win, true);
				else
					c->set_selected(win, false);
			}
			auto old_sub_off = old_page == 0 ? 0 : pi.subpage_divisions[old_page - 1];
			auto new_sub_off = i == 0 ? 0 : pi.subpage_divisions[i - 1];
			if(old_page < i) {
				std::rotate(pi.modify_columns().data() + old_sub_off, pi.modify_columns().data() + old_sub_off + 1,
					pi.modify_columns().data() + new_sub_off);
				for(auto j = old_page; j < i; ++j) {
					pi.subpage_divisions[j] -= 1;
				}
			} else {
				std::rotate(pi.modify_columns().data() + new_sub_off, pi.modify_columns().data() + old_sub_off,
					pi.modify_columns().data() + old_sub_off + 1);
				for(auto j = i; j < old_page; ++j) {
					pi.subpage_divisions[j] += 1;
				}
			}
		}

		if(acc_obj) {
			win.accessibility_interface.on_contents_changed(acc_obj);
		}
	}

	/*
	label_control primary_font_weight_label;
		font_weight_edit primary_font_weight_e;
		label_control primary_font_stretch_label;
		font_stretch_edit primary_font_stretch_e;
		label_control primary_font_italic_label;
		font_italic_toggle_button primary_font_italic_toggle;
	*/
	common_printui_settings::common_printui_settings() :
		language_label(text_id::language_label, content_alignment::leading, text_id::language_info),
		orientation_label(text_id::orientation_label, content_alignment::leading, text_id::orientation_info),
		input_mode_label(text_id::input_mode_label, content_alignment::leading, text_id::input_mode_info),
		toggle_animations_label(text_id::ui_animations_label, content_alignment::leading, text_id::ui_animations_info),
		ui_scale_label(text_id::ui_scale, content_alignment::leading, text_id::ui_scale_info),
		fonts_header(text_id::fonts_header, content_alignment::centered, uint16_t(-1)),
		primary_font_name_label(text_id::primary_font_label, content_alignment::leading, text_id::primary_font_info),
		primary_font_menu(text_size::standard),
		primary_font_weight_label(text_id::font_weight, content_alignment::leading, text_id::font_weight_info),
		primary_font_weight_e(text_size::standard),
		primary_font_stretch_label(text_id::font_stretch, content_alignment::leading, text_id::font_stretch_info),
		primary_font_stretch_e(text_size::standard),
		primary_font_italic_label(text_id::font_italic, content_alignment::leading, text_id::font_italic_info),
		primary_font_italic_toggle(text_size::standard),
		primary_top_lead_label(text_id::top_lead_label, content_alignment::leading, text_id::top_lead_edit_info),
		primary_top_lead_e(text_size::standard),
		primary_bottom_lead_label(text_id::bottom_lead_label, content_alignment::leading, text_id::bottom_lead_edit_info),
		primary_bottom_lead_e(text_size::standard),
		small_font_name_label(text_id::small_font_label, content_alignment::leading, text_id::small_font_info),
		small_font_menu(text_size::note),
		small_relative_label(text_id::relative_size_label, content_alignment::leading, text_id::relative_size_small_info),
		small_font_weight_label(text_id::font_weight, content_alignment::leading, text_id::font_weight_info),
		small_font_weight_e(text_size::note),
		small_font_stretch_label(text_id::font_stretch, content_alignment::leading, text_id::font_stretch_info),
		small_font_stretch_e(text_size::note),
		small_font_italic_label(text_id::font_italic, content_alignment::leading, text_id::font_italic_info),
		small_font_italic_toggle(text_size::note),
		small_top_lead_label(text_id::top_lead_label, content_alignment::leading, text_id::top_lead_edit_info),
		small_top_lead_e(text_size::note),
		small_bottom_lead_label(text_id::bottom_lead_label, content_alignment::leading, text_id::bottom_lead_edit_info),
		small_bottom_lead_e(text_size::note),
		header_font_name_label(text_id::header_font_label, content_alignment::leading, text_id::header_font_info),
		header_font_menu(text_size::header),
		header_relative_label(text_id::relative_size_label, content_alignment::leading, text_id::relative_size_header_info),
		header_font_weight_label(text_id::font_weight, content_alignment::leading, text_id::font_weight_info),
		header_font_weight_e(text_size::header),
		header_font_stretch_label(text_id::font_stretch, content_alignment::leading, text_id::font_stretch_info),
		header_font_stretch_e(text_size::header),
		header_font_italic_label(text_id::font_italic, content_alignment::leading, text_id::font_italic_info),
		header_font_italic_toggle(text_size::header),
		header_top_lead_label(text_id::top_lead_label, content_alignment::leading, text_id::top_lead_edit_info),
		header_top_lead_e(text_size::header),
		header_bottom_lead_label(text_id::bottom_lead_label, content_alignment::leading, text_id::bottom_lead_edit_info),
		header_bottom_lead_e(text_size::header),
		keyboard_header(text_id::keyboard_header, content_alignment::centered, uint16_t(-1)),
		kb_arrangement_label(text_id::keyboard_arrangement, content_alignment::leading, text_id::keyboard_arragnement_info),
		key1_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key1_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key1_edit(0),
		key1_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key1_code_button(0),
		key2_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key2_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key2_edit(1),
		key2_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key2_code_button(1),
		key3_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key3_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key3_edit(2),
		key3_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key3_code_button(2),
		key4_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key4_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key4_edit(3),
		key4_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key4_code_button(3),
		key5_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key5_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key5_edit(4),
		key5_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key5_code_button(4),
		key6_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key6_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key6_edit(5),
		key6_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key6_code_button(5),
		key7_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key7_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key7_edit(6),
		key7_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key7_code_button(6),
		key8_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key8_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key8_edit(7),
		key8_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key8_code_button(7),
		key9_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key9_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key9_edit(8),
		key9_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key9_code_button(8),
		key10_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key10_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key10_edit(9),
		key10_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key10_code_button(9),
		key11_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key11_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key11_edit(10),
		key11_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key11_code_button(10),
		key12_label(text_id::key_ord_name, content_alignment::leading, uint16_t(-1)),
		key12_name_label(text_id::key_display_name, content_alignment::leading, text_id::keyboard_display_name_info),
		key12_edit(11),
		key12_code_label(text_id::scan_code, content_alignment::leading, text_id::keyboard_code_info),
		key12_code_button(11)
	{

		lang_menu.open_button.set_text_alignment(content_alignment::trailing);
		lang_menu.page_size = 1;
		lang_menu.line_size = 10;

		orientation_list.text_alignment = content_alignment::trailing;

		input_mode_list.text_alignment = content_alignment::trailing;

		lang_menu.alt_text = text_id::language_info;
		lang_menu.name = text_id::language_label;

		orientation_list.alt_text_id = text_id::orientation_info;
		orientation_list.name = text_id::orientation_label;

		input_mode_list.alt_text_id = text_id::input_mode_info;
		input_mode_list.name = text_id::input_mode_label;

		primary_font_menu.open_button.set_text_alignment(content_alignment::trailing);
		primary_font_menu.page_size = 1;
		primary_font_menu.line_size = 10;
		primary_font_menu.alt_text = text_id::primary_font_info;
		primary_font_menu.name = text_id::primary_font_label;

		small_font_menu.open_button.set_text_alignment(content_alignment::trailing);
		small_font_menu.page_size = 1;
		small_font_menu.line_size = 10;
		small_font_menu.alt_text = text_id::small_font_info;
		small_font_menu.name = text_id::small_font_label;

		header_font_menu.open_button.set_text_alignment(content_alignment::trailing);
		header_font_menu.page_size = 1;
		header_font_menu.line_size = 10;
		header_font_menu.alt_text = text_id::header_font_info;
		header_font_menu.name = text_id::header_font_label;

		kb_arrangement_list.text_alignment = content_alignment::trailing;
		kb_arrangement_list.alt_text_id = text_id::keyboard_arragnement_info;
		kb_arrangement_list.name = text_id::keyboard_arrangement;
		{
			text::text_parameter tp = text::int_param{ 1,0 };
			key1_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 2,0 };
			key2_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 3,0 };
			key3_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 4,0 };
			key4_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 5,0 };
			key5_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 6,0 };
			key6_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 7,0 };
			key7_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 8,0 };
			key8_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 9,0 };
			key9_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 10,0 };
			key10_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 11,0 };
			key11_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}
		{
			text::text_parameter tp = text::int_param{ 12,0 };
			key12_label.quiet_set_text(text_id::key_ord_name, &tp, &tp + 1);
		}

		content_description.push_back(page_content{ &language_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &lang_menu, column_break_behavior::normal, item_type::item_end, nullptr });
		content_description.push_back(page_content{ &orientation_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &orientation_list, column_break_behavior::normal, item_type::item_end, nullptr });
		content_description.push_back(page_content{ &input_mode_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &input_mode_list, column_break_behavior::normal, item_type::item_end, nullptr });
		content_description.push_back(page_content{ &toggle_animations, column_break_behavior::normal, item_type::single_item, &toggle_animations_label });
		content_description.push_back(page_content{ &ui_scale_e, column_break_behavior::normal, item_type::single_item, &ui_scale_label });

		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::single_space, nullptr });
		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::decoration_footer, nullptr });
		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::single_space, nullptr });

		content_description.push_back(page_content{ &fonts_header, column_break_behavior::dont_break_after, item_type::normal, nullptr });
		content_description.push_back(page_content{ nullptr, column_break_behavior::dont_break_after, item_type::single_space, nullptr });

		content_description.push_back(page_content{ &primary_font_name_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &primary_font_menu, column_break_behavior::normal, item_type::item_end, nullptr });
		content_description.push_back(page_content{ &primary_font_weight_e, column_break_behavior::normal, item_type::single_item, &primary_font_weight_label });
		content_description.push_back(page_content{ &primary_font_stretch_e, column_break_behavior::normal, item_type::single_item, &primary_font_stretch_label });
		content_description.push_back(page_content{ &primary_font_italic_toggle, column_break_behavior::normal, item_type::single_item, &primary_font_italic_label });
		content_description.push_back(page_content{ &primary_top_lead_e, column_break_behavior::normal, item_type::single_item, &primary_top_lead_label });
		content_description.push_back(page_content{ &primary_bottom_lead_e, column_break_behavior::normal, item_type::single_item, &primary_bottom_lead_label });

		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::decoration_space, nullptr });

		content_description.push_back(page_content{ &small_font_name_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &small_font_menu, column_break_behavior::normal, item_type::item_end, nullptr });
		content_description.push_back(page_content{ &small_relative_e, column_break_behavior::normal, item_type::single_item, &small_relative_label });
		content_description.push_back(page_content{ &small_font_weight_e, column_break_behavior::normal, item_type::single_item, &small_font_weight_label });
		content_description.push_back(page_content{ &small_font_stretch_e, column_break_behavior::normal, item_type::single_item, &small_font_stretch_label });
		content_description.push_back(page_content{ &small_font_italic_toggle, column_break_behavior::normal, item_type::single_item, &small_font_italic_label });
		content_description.push_back(page_content{ &small_top_lead_e, column_break_behavior::normal, item_type::single_item, &small_top_lead_label });
		content_description.push_back(page_content{ &small_bottom_lead_e, column_break_behavior::normal, item_type::single_item, &small_bottom_lead_label });

		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::decoration_space, nullptr });

		content_description.push_back(page_content{ &header_font_name_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &header_font_menu, column_break_behavior::normal, item_type::item_end, nullptr });
		content_description.push_back(page_content{ &header_relative_e, column_break_behavior::normal, item_type::single_item, &header_relative_label });
		content_description.push_back(page_content{ &header_font_weight_e, column_break_behavior::normal, item_type::single_item, &header_font_weight_label });
		content_description.push_back(page_content{ &header_font_stretch_e, column_break_behavior::normal, item_type::single_item, &header_font_stretch_label });
		content_description.push_back(page_content{ &header_font_italic_toggle, column_break_behavior::normal, item_type::single_item, &header_font_italic_label });
		content_description.push_back(page_content{ &header_top_lead_e, column_break_behavior::normal, item_type::single_item, &header_top_lead_label });
		content_description.push_back(page_content{ &header_bottom_lead_e, column_break_behavior::normal, item_type::single_item, &header_bottom_lead_label });

		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::single_space, nullptr });
		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::decoration_footer, nullptr });
		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::single_space, nullptr });

		content_description.push_back(page_content{ &keyboard_header, column_break_behavior::dont_break_after, item_type::normal, nullptr });
		content_description.push_back(page_content{ nullptr, column_break_behavior::dont_break_after, item_type::single_space, nullptr });

		content_description.push_back(page_content{ &kb_arrangement_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &kb_arrangement_list, column_break_behavior::normal, item_type::item_end, nullptr });

		content_description.push_back(page_content{ &key1_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key1_edit, column_break_behavior::dont_break_after, item_type::normal, &key1_name_label });
		content_description.push_back(page_content{ &key1_code_button, column_break_behavior::normal, item_type::item_end, &key1_code_label });

		content_description.push_back(page_content{ &key2_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key2_edit, column_break_behavior::dont_break_after, item_type::normal, &key2_name_label });
		content_description.push_back(page_content{ &key2_code_button, column_break_behavior::normal, item_type::item_end, &key2_code_label });

		content_description.push_back(page_content{ &key3_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key3_edit, column_break_behavior::dont_break_after, item_type::normal, &key3_name_label });
		content_description.push_back(page_content{ &key3_code_button, column_break_behavior::normal, item_type::item_end, &key3_code_label });

		content_description.push_back(page_content{ &key4_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key4_edit, column_break_behavior::dont_break_after, item_type::normal, &key4_name_label });
		content_description.push_back(page_content{ &key4_code_button, column_break_behavior::normal, item_type::item_end, &key4_code_label });

		content_description.push_back(page_content{ &key5_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key5_edit, column_break_behavior::dont_break_after, item_type::normal, &key5_name_label });
		content_description.push_back(page_content{ &key5_code_button, column_break_behavior::normal, item_type::item_end, &key5_code_label });

		content_description.push_back(page_content{ &key6_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key6_edit, column_break_behavior::dont_break_after, item_type::normal, &key6_name_label });
		content_description.push_back(page_content{ &key6_code_button, column_break_behavior::normal, item_type::item_end, &key6_code_label });

		content_description.push_back(page_content{ &key7_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key7_edit, column_break_behavior::dont_break_after, item_type::normal, &key7_name_label });
		content_description.push_back(page_content{ &key7_code_button, column_break_behavior::normal, item_type::item_end, &key7_code_label });

		content_description.push_back(page_content{ &key8_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key8_edit, column_break_behavior::dont_break_after, item_type::normal, &key8_name_label });
		content_description.push_back(page_content{ &key8_code_button, column_break_behavior::normal, item_type::item_end, &key8_code_label });

		content_description.push_back(page_content{ &key9_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key9_edit, column_break_behavior::dont_break_after, item_type::normal, &key9_name_label });
		content_description.push_back(page_content{ &key9_code_button, column_break_behavior::normal, item_type::item_end, &key9_code_label });

		content_description.push_back(page_content{ &key10_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key10_edit, column_break_behavior::dont_break_after, item_type::normal, &key10_name_label });
		content_description.push_back(page_content{ &key10_code_button, column_break_behavior::normal, item_type::item_end, &key10_code_label });

		content_description.push_back(page_content{ &key11_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key11_edit, column_break_behavior::dont_break_after, item_type::normal, &key11_name_label });
		content_description.push_back(page_content{ &key11_code_button, column_break_behavior::normal, item_type::item_end, &key11_code_label });

		content_description.push_back(page_content{ &key12_label, column_break_behavior::dont_break_after, item_type::item_start, nullptr });
		content_description.push_back(page_content{ &key12_edit, column_break_behavior::dont_break_after, item_type::normal, &key12_name_label });
		content_description.push_back(page_content{ &key12_code_button, column_break_behavior::normal, item_type::item_end, &key12_code_label });

		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::single_space, nullptr });
		content_description.push_back(page_content{ nullptr, column_break_behavior::normal, item_type::decoration_footer, nullptr });
	}

	accessibility_object* common_printui_settings::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_container_accessibility_interface(win, this, text_id::ui_settings_name);
		}
		return acc_obj;
	}

	void common_printui_settings::go_to_page(window_data& win, uint32_t i, page_information& pi) {
		layout_interface::go_to_page(win, i, pi);
		if(acc_obj) {
			win.accessibility_interface.on_contents_changed(acc_obj);
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

	void common_printui_settings::recreate_contents(window_data& win, layout_node&) {

		if(toggle_animations.toggle_is_on != win.dynamic_settings.uianimations) {
			toggle_animations.change_toggle_state(win, win.dynamic_settings.uianimations);
		}
		lang_menu.open_button.set_text(win, win.text_data.locale_display_name(win));
		orientation_list.quiet_select_option_by_value(win, size_t(win.orientation));
		input_mode_list.quiet_select_option_by_value(win, size_t(win.dynamic_settings.imode));

		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.global_size_multiplier, 2);
			ui_scale_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		primary_font_menu.open_button.set_text(win, win.dynamic_settings.primary_font.name);
		small_font_menu.open_button.set_text(win, win.dynamic_settings.small_font.name);
		header_font_menu.open_button.set_text(win, win.dynamic_settings.header_font.name);

		if(primary_font_italic_toggle.toggle_is_on != win.dynamic_settings.primary_font.is_oblique) {
			primary_font_italic_toggle.change_toggle_state(win, win.dynamic_settings.primary_font.is_oblique);
		}
		if(small_font_italic_toggle.toggle_is_on != win.dynamic_settings.small_font.is_oblique) {
			small_font_italic_toggle.change_toggle_state(win, win.dynamic_settings.small_font.is_oblique);
		}
		if(header_font_italic_toggle.toggle_is_on != win.dynamic_settings.header_font.is_oblique) {
			header_font_italic_toggle.change_toggle_state(win, win.dynamic_settings.header_font.is_oblique);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.primary_font.weight, 0);
			primary_font_weight_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.primary_font.span, 0);
			primary_font_stretch_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.small_font.weight, 0);
			small_font_weight_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.small_font.span, 0);
			small_font_stretch_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.header_font.weight, 0);
			header_font_weight_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.header_font.span, 0);
			header_font_stretch_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.primary_font.top_leading, 0);
			primary_top_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.primary_font.bottom_leading, 0);
			primary_bottom_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.small_font.top_leading, 0);
			small_top_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.small_font.bottom_leading, 0);
			small_bottom_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.header_font.top_leading, 0);
			header_top_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_int(win.dynamic_settings.header_font.bottom_leading, 0);
			header_bottom_lead_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.small_size_multiplier, 3);
			small_relative_e.quiet_set_text(win, result_to_str.text_content.text);
		}
		{
			auto result_to_str = win.text_data.format_double(win.dynamic_settings.heading_size_multiplier, 3);
			header_relative_e.quiet_set_text(win, result_to_str.text_content.text);
		}

		update_with_keyboard_settings(win);

		page_layout_specification page_spec;
		page_spec.header = &header;
		page_spec.footer = &footer;
		page_spec.begin = content_description.data();
		page_spec.end = content_description.data() + content_description.size();
		page_spec.column_left_margin = 2;
		page_spec.column_right_margin = 2;
		page_spec.ex_inter_column_margin = 0;
		page_spec.ex_page_top_margin = 0;
		page_spec.ex_page_bottom_margin = 1;
		page_spec.ex_page_left_margin = 0;
		page_spec.ex_page_right_margin = 0;
		page_spec.horz_shrink_page_to_content = true;
		page_spec.min_column_horizontal_size = 6;
		page_spec.max_column_horizontal_size = uint8_t(win.dynamic_settings.line_width);
		page_spec.uniform_column_width = true;
		page_spec.vert_shrink_page_to_content = false;
		page_spec.vertical_column_alignment = content_alignment::leading;
		page_spec.horizontal_columns_alignment = content_alignment::centered;
		page_spec.additional_space_to_outer_margins = true;
		page_spec.section_footer_decoration = win.window_bar.settings_pages.section_bottom_decorations;
		page_spec.spacing_decoration = win.window_bar.settings_pages.section_separation_decorations;

		page_spec.decoration_brush = uint8_t(-1);

		default_recreate_page(win, this, page_spec);
	}

	void key_code_button::button_action(window_data&) {
		// todo: trigger key code update
	}

	void key_name_edit::on_edit_finished(window_data& win, std::wstring const& txt) {
		auto first_char_length = next_valid_cursor_position(0);
		if(first_char_length > 0) {
			win.dynamic_settings.keys.main_keys[key].display_name = txt.substr(0, first_char_length);
			win.window_bar.print_ui_settings.update_with_keyboard_settings(win);
			win.rendering_interface.create_interactiable_tags(win);
			win.window_interface.invalidate_window();
			win.dynamic_settings.settings_changed = true;
		}
	}

	void settings_keyboard_arragement_list::on_select(window_data& win, size_t v) {
		win.dynamic_settings.keys.type = keyboard_type(v);
		populate_key_mappings_by_type(win.dynamic_settings.keys);
		win.window_bar.print_ui_settings.update_with_keyboard_settings(win);
		win.rendering_interface.create_interactiable_tags(win);
		win.window_interface.invalidate_window();
		win.dynamic_settings.settings_changed = true;
	}
	list_option_description settings_keyboard_arragement_list::describe_option(window_data const&, uint32_t i) {
		switch(i) {
			case 0:
				return list_option_description{ size_t(keyboard_type::left_hand), text_id::keyboard_left, uint16_t(-1) };
			case 1:
				return list_option_description{ size_t(keyboard_type::right_hand), text_id::keyboard_right, uint16_t(-1) };
			case 2:
				return list_option_description{ size_t(keyboard_type::right_hand_tilted), text_id::keyboard_tilted, uint16_t(-1) };
			case 3:
				return list_option_description{ size_t(keyboard_type::custom), text_id::keyboard_custom, uint16_t(-1) };
			default:
				return list_option_description{ 0, uint16_t(-1), uint16_t(-1) };
		}
	}

	void settings_orientation_list::on_select(window_data& win, size_t v) {
		win.change_orientation(layout_orientation(v));
		win.dynamic_settings.settings_changed = true;
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
		win.dynamic_settings.settings_changed = true;
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
		get_option(size_t(input_mode::controller_with_pointer))->set_disabled(win, win.controller_number_plugged_in == 0);
		get_option(size_t(input_mode::controller_only))->set_disabled(win, win.controller_number_plugged_in == 0);
	}


	void font_italic_toggle_button::toggle_action(window_data& win, bool toggle_state) {
		if(text_sz == text_size::standard) {
			if(toggle_state != win.dynamic_settings.primary_font.is_oblique) {
				win.dynamic_settings.primary_font.is_oblique = toggle_state;
				win.dynamic_settings.settings_changed = true;
				win.text_data.update_fonts(win);
			}
		} else if(text_sz == text_size::note) {
			if(toggle_state != win.dynamic_settings.small_font.is_oblique) {
				win.dynamic_settings.small_font.is_oblique = toggle_state;
				win.dynamic_settings.settings_changed = true;
				win.text_data.update_fonts(win);
			}
		} else {
			if(toggle_state != win.dynamic_settings.header_font.is_oblique) {
				win.dynamic_settings.header_font.is_oblique = toggle_state;
				win.dynamic_settings.settings_changed = true;
				win.text_data.update_fonts(win);
			}
		}

		win.get_layout();
		auto containing_pi = win.get_node(win.window_bar.print_ui_settings.l_id).page_info();
		int pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, l_id);
		if(pn >= 0) {
			win.window_bar.print_ui_settings.go_to_page(win, uint32_t(pn), *containing_pi);
			win.redraw_ui();
		}
	}

	void font_top_lead_edit::on_edit_finished(window_data& win, std::wstring const& txt) {
		auto extracted_value = win.text_data.text_to_int(txt.data(), uint32_t(txt.length()));
		if(extracted_value > 0) {
			extracted_value = std::clamp(extracted_value, 0i64, 20i64);

			if(text_sz == text_size::standard)
				win.dynamic_settings.primary_font.top_leading = int32_t(extracted_value);
			else if(text_sz == text_size::note)
				win.dynamic_settings.small_font.top_leading = int32_t(extracted_value);
			else
				win.dynamic_settings.header_font.top_leading = int32_t(extracted_value);

			win.dynamic_settings.settings_changed = true;
			win.text_data.update_fonts(win);

			win.get_layout();
			auto containing_pi = win.get_node(win.window_bar.print_ui_settings.l_id).page_info();
			int pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, l_id);
			if(pn >= 0) {
				win.window_bar.print_ui_settings.go_to_page(win, uint32_t(pn), *containing_pi);
				win.redraw_ui();
			}
		}
		auto result_to_str = win.text_data.format_int(extracted_value, 0);
		quiet_set_text(win, result_to_str.text_content.text);
	}

	void font_bottom_lead_edit::on_edit_finished(window_data& win, std::wstring const& txt) {
		auto extracted_value = win.text_data.text_to_int(txt.data(), uint32_t(txt.length()));
		if(extracted_value > 0) {
			extracted_value = std::clamp(extracted_value, 0i64, 20i64);

			if(text_sz == text_size::standard)
				win.dynamic_settings.primary_font.bottom_leading = int32_t(extracted_value);
			else if(text_sz == text_size::note)
				win.dynamic_settings.small_font.bottom_leading = int32_t(extracted_value);
			else
				win.dynamic_settings.header_font.bottom_leading = int32_t(extracted_value);

			win.dynamic_settings.settings_changed = true;
			win.text_data.update_fonts(win);

			win.get_layout();
			auto containing_pi = win.get_node(win.window_bar.print_ui_settings.l_id).page_info();
			int pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, l_id);
			if(pn >= 0) {
				win.window_bar.print_ui_settings.go_to_page(win, uint32_t(pn), *containing_pi);
				win.redraw_ui();
			}
		}
		auto result_to_str = win.text_data.format_int(extracted_value, 0);
		quiet_set_text(win, result_to_str.text_content.text);
	}

	void font_weight_edit::on_edit_finished(window_data& win, std::wstring const& txt) {
		auto extracted_value = win.text_data.text_to_int(txt.data(), uint32_t(txt.length()));
		if(extracted_value > 0) {
			extracted_value = std::clamp(extracted_value, 100i64, 900i64);
			
			if(text_sz == text_size::standard)
				win.dynamic_settings.primary_font.weight = int32_t(extracted_value);
			else if(text_sz == text_size::note)
				win.dynamic_settings.small_font.weight = int32_t(extracted_value);
			else
				win.dynamic_settings.header_font.weight = int32_t(extracted_value);

			win.dynamic_settings.settings_changed = true;
			win.text_data.update_fonts(win);

			win.get_layout();
			auto containing_pi = win.get_node(win.window_bar.print_ui_settings.l_id).page_info();
			int pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, l_id);
			if(pn >= 0) {
				win.window_bar.print_ui_settings.go_to_page(win, uint32_t(pn), *containing_pi);
				win.redraw_ui();
			}
		}
		auto result_to_str = win.text_data.format_int(extracted_value, 0);
		quiet_set_text(win, result_to_str.text_content.text);
	}

	void small_relative_size::on_edit_finished(window_data& win, std::wstring const& txt) {
		auto extracted_value = win.text_data.text_to_double(txt.data(), uint32_t(txt.length()));
		if(extracted_value > 0) {
			extracted_value = std::clamp(extracted_value, 0.5, 1.0);

			win.dynamic_settings.small_size_multiplier = float(extracted_value);

			win.dynamic_settings.settings_changed = true;
			win.text_data.update_fonts(win);

			win.get_layout();
			auto containing_pi = win.get_node(win.window_bar.print_ui_settings.l_id).page_info();
			int pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, l_id);
			if(pn >= 0) {
				win.window_bar.print_ui_settings.go_to_page(win, uint32_t(pn), *containing_pi);
				win.redraw_ui();
			}
		}
		auto result_to_str = win.text_data.format_double(extracted_value, 3);
		quiet_set_text(win, result_to_str.text_content.text);
	}

	void header_relative_size::on_edit_finished(window_data& win, std::wstring const& txt) {
		auto extracted_value = win.text_data.text_to_double(txt.data(), uint32_t(txt.length()));
		if(extracted_value > 0) {
			extracted_value = std::clamp(extracted_value, 1.0, 2.0);

			win.dynamic_settings.heading_size_multiplier = float(extracted_value);

			win.dynamic_settings.settings_changed = true;
			win.text_data.update_fonts(win);

			win.get_layout();
			auto containing_pi = win.get_node(win.window_bar.print_ui_settings.l_id).page_info();
			int pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, l_id);
			if(pn >= 0) {
				win.window_bar.print_ui_settings.go_to_page(win, uint32_t(pn), *containing_pi);
				win.redraw_ui();
			}
		}
		auto result_to_str = win.text_data.format_double(extracted_value, 3);
		quiet_set_text(win, result_to_str.text_content.text);
	}

	void font_stretch_edit::on_edit_finished(window_data& win, std::wstring const& txt) {
		auto extracted_value = win.text_data.text_to_double(txt.data(), uint32_t(txt.length()));
		if(extracted_value > 0) {
			extracted_value = std::clamp(extracted_value, 50.0, 200.0);

			if(text_sz == text_size::standard)
				win.dynamic_settings.primary_font.span = float(extracted_value);
			else if(text_sz == text_size::note)
				win.dynamic_settings.small_font.span = float(extracted_value);
			else
				win.dynamic_settings.header_font.span = float(extracted_value);

			win.dynamic_settings.settings_changed = true;
			win.text_data.update_fonts(win);

			win.get_layout();
			auto containing_pi = win.get_node(win.window_bar.print_ui_settings.l_id).page_info();
			int pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, l_id);
			if(pn >= 0) {
				win.window_bar.print_ui_settings.go_to_page(win, uint32_t(pn), *containing_pi);
				win.redraw_ui();
			}
		}
		auto result_to_str = win.text_data.format_double(extracted_value, 0);
		quiet_set_text(win, result_to_str.text_content.text);
	}

	void font_button::button_action(window_data& win) {
		if(text_sz == text_size::standard)
			win.dynamic_settings.primary_font.name = name;
		else if(text_sz == text_size::note)
			win.dynamic_settings.small_font.name = name;
		else
			win.dynamic_settings.header_font.name = name;

		win.dynamic_settings.settings_changed = true;
		win.text_data.update_fonts(win);
		
		win.get_layout();

		auto containing_pi = win.get_node(win.window_bar.print_ui_settings.l_id).page_info();
		int pn = -1;

		if(text_sz == text_size::standard)
			pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, win.window_bar.print_ui_settings.primary_font_menu.l_id);
		else if(text_sz == text_size::note)
			pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, win.window_bar.print_ui_settings.small_font_menu.l_id);
		else
			pn = win.get_containing_page_number(win.window_bar.print_ui_settings.l_id, *containing_pi, win.window_bar.print_ui_settings.header_font_menu.l_id);

		
		if(pn >= 0) {
			win.window_bar.print_ui_settings.go_to_page(win, uint32_t(pn), *containing_pi);
			win.redraw_ui();
		}
		

		if(text_sz == text_size::standard)
			win.window_bar.print_ui_settings.primary_font_menu.close(win, true);
		else if(text_sz == text_size::note)
			win.window_bar.print_ui_settings.small_font_menu.close(win, true);
		else
			win.window_bar.print_ui_settings.header_font_menu.close(win, true);
	}

	std::vector<page_content> font_menu::get_options(window_data& win) {
		std::vector<page_content> result;

		if(lbuttons.size() == 0) {
			auto font_names = win.text_interface.ennumerate_fonts(win.text_data.locale_name());
			for(auto& l : font_names) {
				auto ptr = std::make_unique<font_button>(text_sz);
				ptr->name = l;
				ptr->set_text(win, l);
				ptr->set_text_alignment(content_alignment::trailing);
				bool is_current = text_sz == text_size::standard ? (l == win.dynamic_settings.primary_font.name) : 
					(text_sz == text_size::note ? (l == win.dynamic_settings.small_font.name) : (l == win.dynamic_settings.header_font.name));
				ptr->set_selected(win, is_current);
				lbuttons.push_back(std::move(ptr));
			}
		}

		for(auto& ptr : lbuttons) {
			result.push_back(page_content{ ptr.get(), column_break_behavior::normal, item_type::single_item });
		}
		return result;
	}
	void font_menu::on_open(window_data& win) {
		for(auto& ptr : lbuttons) {
			font_button* b = static_cast<font_button*>(ptr.get());
			bool is_current = text_sz == text_size::standard ? (b->name == win.dynamic_settings.primary_font.name) :
				(text_sz == text_size::note ? (b->name == win.dynamic_settings.small_font.name) : (b->name == win.dynamic_settings.header_font.name));
			b->set_selected(win, is_current);
		}
	}
	void font_menu::on_close(window_data&) {

	}
	accessibility_object* font_menu::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_expandable_selection_list(win, this, this, name, alt_text);
		}
		return acc_obj;
	}
	layout_interface* font_menu::selected_item() const {
		layout_interface* sel = nullptr;
		for(auto& ptr : lbuttons) {
			font_button* b = static_cast<font_button*>(ptr.get());
			if(b->is_selected()) {
				sel = ptr.get();
				break;
			}
		}
		return sel;
	}


	void language_button::button_action(window_data& win) {
		win.dynamic_settings.locale_lang = lang;
		win.dynamic_settings.locale_region = region;

		win.text_data.change_locale(lang, region, win, true);
		win.dynamic_settings.settings_changed = true;

		win.window_bar.print_ui_settings.lang_menu.close(win, true);
	}

	std::vector<page_content> language_menu::get_options(window_data& win) {
		std::vector<page_content> result;

		if(lbuttons.size() == 0) {
			auto langs = text::ennumerate_languages(win);
			for(auto& l : langs) {
				auto ptr = std::make_unique<language_button>();
				ptr->lang = l.language;
				ptr->region = l.region;
				ptr->set_text(win, l.display_name);
				ptr->set_text_alignment(content_alignment::trailing);
				ptr->set_selected(win, win.text_data.is_current_locale(l.language, l.region));
				lbuttons.push_back(std::move(ptr));
			}
		}

		for(auto& ptr : lbuttons) {
			result.push_back(page_content{ ptr.get(), column_break_behavior::normal, item_type::single_item });
		}
		return result;
	}
	void language_menu::on_open(window_data& win) {
		for(auto& ptr : lbuttons) {
			language_button* b = static_cast<language_button*>(ptr.get());
			b->set_selected(win, win.text_data.is_current_locale(b->lang, b->region));
		}
	}
	void language_menu::on_close(window_data&) {

	}
	accessibility_object* language_menu::get_accessibility_interface(window_data& win) {
		if(!acc_obj) {
			acc_obj = win.accessibility_interface.make_expandable_selection_list(win, this, this, name, alt_text);
		}
		return acc_obj;
	}
	layout_interface* language_menu::selected_item() const {
		layout_interface* sel = nullptr;
		for(auto& ptr : lbuttons) {
			language_button* b = static_cast<language_button*>(ptr.get());
			if(b->is_selected()) {
				sel = ptr.get();
				break;
			}
		}
		return sel;
	}

	void ui_animation_toggle_button::toggle_action(window_data& win, bool toggle_state) {
		if(toggle_state != win.dynamic_settings.uianimations) {
			win.dynamic_settings.uianimations = toggle_state;
			win.dynamic_settings.settings_changed = true;
		}
	}

	void ui_scale_edit::on_edit_finished(window_data& win, std::wstring const& txt) {
		auto extracted_value = win.text_data.text_to_double(txt.data(), uint32_t(txt.length()));
		if(extracted_value > 0.0) {
			extracted_value = std::clamp(extracted_value, 0.5, 3.0);
			win.change_size_multiplier(float(extracted_value));
			win.dynamic_settings.settings_changed = true;
		}
		auto result_to_str = win.text_data.format_double(win.dynamic_settings.global_size_multiplier, 2);
		quiet_set_text(win, result_to_str.text_content.text);
	}

	void common_printui_settings::update_with_keyboard_settings(window_data& win) {
		kb_arrangement_list.quiet_select_option_by_value(win, size_t(win.dynamic_settings.keys.type));

		key1_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[0].scancode));
		key1_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key1_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[0].display_name);

		key2_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[1].scancode));
		key2_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key2_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[1].display_name);

		key3_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[2].scancode));
		key3_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key3_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[2].display_name);

		key4_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[3].scancode));
		key4_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key4_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[3].display_name);

		key5_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[4].scancode));
		key5_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key5_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[4].display_name);

		key6_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[5].scancode));
		key6_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key6_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[5].display_name);

		key7_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[6].scancode));
		key7_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key7_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[6].display_name);

		key8_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[7].scancode));
		key8_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key8_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[7].display_name);

		key9_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[8].scancode));
		key9_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key9_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[8].display_name);

		key10_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[9].scancode));
		key10_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key10_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[9].display_name);

		key11_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[10].scancode));
		key11_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key11_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[10].display_name);

		key12_code_button.set_text(win, std::to_wstring(win.dynamic_settings.keys.main_keys[11].scancode));
		key12_code_button.set_disabled(win, win.dynamic_settings.keys.type != keyboard_type::custom);
		key12_edit.quiet_set_text(win, win.dynamic_settings.keys.main_keys[11].display_name);
	}
}

