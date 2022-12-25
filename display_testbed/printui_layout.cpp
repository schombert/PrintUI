#include "printui_main_header.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <algorithm>
#include <charconv>
#include <array>
#include <Windows.h>

namespace printui {

	void window_data::immediate_resize(layout_node& node, int32_t new_width, int32_t new_height) {
		if(node.width != uint16_t(new_width) || node.height != uint16_t(new_height)) {
			node.width = uint16_t(new_width);
			node.height = uint16_t(new_height);
			if(node.l_interface && node.is_deferred() == false) {
				recreate_contents(node.l_interface, &node);
			}
		}
	}
	void window_data::recreate_contents(layout_interface* l_interface, layout_node* retvalue) {

		layout_node_type ntype = l_interface->get_node_type();
		retvalue->set_deferred(false);

		switch(ntype) {
			case layout_node_type::visible:
			case layout_node_type::control:
			{
				retvalue->contents = std::monostate{};
				return;
			}
			case layout_node_type::container:
			{
				recreate_container_contents(l_interface, retvalue);
				return;
			}
			case layout_node_type::page:
			{
				recreate_page_contents(l_interface, retvalue);
				return;
			}
		}
	}

	layout_reference window_data::create_node(layout_interface* l_interface, int32_t max_width, int32_t max_height, bool force_create_children, bool force_create) {

		bool already_existed = l_interface->l_id != layout_reference_none;

		layout_reference retvalue_id = layout_nodes.allocate_node(l_interface);
		layout_node* retvalue = &layout_nodes.get_node(retvalue_id);

		auto const spec = l_interface->get_specification(*this);
		
		if(already_existed && !force_create) {
			auto new_height = std::min(retvalue->height, uint16_t(max_height));
			if(spec.page_flags == size_flags::fill_to_max || spec.page_flags == size_flags::fill_to_max_single_col) {
				new_height = uint16_t(max_height);
			}
			auto new_width = std::min(retvalue->width, uint16_t(max_width));
			if(spec.line_flags == size_flags::fill_to_max || spec.line_flags == size_flags::fill_to_max_single_col) {
				new_width = uint16_t(max_width);
			}
			if(force_create_children) {
				retvalue->height = new_height;
				retvalue->width = new_width;
				recreate_contents(l_interface, retvalue);
			} else {
				immediate_resize(*retvalue, new_width, new_height);
			}
		} else {
			auto new_height = (spec.page_flags != size_flags::none)
				? std::max(uint16_t(max_height), spec.minimum_page_size)
				: spec.minimum_page_size;
			auto new_width = (spec.line_flags != size_flags::none)
				? std::max(uint16_t(max_width), spec.minimum_line_size)
				: spec.minimum_line_size;
			retvalue->height = new_height;
			retvalue->width = new_width;

			if(!force_create_children &&
				(spec.page_flags == size_flags::none || spec.page_flags == size_flags::fill_to_max) &&
				(spec.line_flags == size_flags::none || spec.line_flags == size_flags::fill_to_max)) {

				retvalue->set_deferred(true);
			} else {

				recreate_contents(l_interface, retvalue);
			}
		}

		return retvalue_id;
	}

	void window_data::recreate_container_contents(layout_interface* l_interface, layout_node* node) {
		if(!node->container_info()) {
			node->contents = std::make_unique<container_information>();
		} else {
			node->container_info()->clear_children();
		}
		l_interface->recreate_contents(*this, *node);
	}

	int32_t window_data::get_containing_page_number(layout_reference page_id, page_information& pi, layout_reference c) {
		if(c != layout_reference_none && is_child_of(page_id, c)) {
			uint32_t col_offset = 0;
			for(uint32_t i = 0; i < pi.subpage_divisions.size(); ++i) {
				for(; col_offset < pi.subpage_divisions[i]; ++col_offset) {
					if(is_child_of(pi.view_columns()[col_offset], c)) {
						return int32_t(i);
					}
				}
			}

			for(; col_offset < pi.view_columns().size(); ++col_offset) {
				if(is_child_of(pi.view_columns()[col_offset], c)) {
					return int32_t(pi.subpage_divisions.size());
				}
			}
		}
		return -1;
		
	}

	layout_reference get_current_page_visibility_marker(window_data const& win, layout_reference, page_information const& pi) {
		auto subpage = 0;
		if(pi.subpage_offset > 1 && pi.subpage_offset <= pi.subpage_divisions.size()) {
			subpage = pi.subpage_divisions[pi.subpage_offset - 1];
		}
		if(pi.view_columns().size() == 0) {
			return layout_reference_none;
		} else if(auto ri = win.get_interactable_render_holder(pi.view_columns()[subpage]); ri) {
			return ri->l_id;
		} else {
			return layout_reference_none;
		}
	}

	void window_data::recreate_page_contents(layout_interface* l_interface, layout_node* node) {
		auto pi = node->page_info();
		if(pi) {

			layout_reference old_focus_column = get_current_page_visibility_marker(*this, l_interface->l_id, *pi);

			pi->clear_columns();
			pi->subpage_divisions.clear();
			l_interface->recreate_contents(*this, *node);

			int pn = get_containing_page_number(l_interface->l_id, *pi, old_focus_column);
			if(pn == -1)
				pn = std::min(int32_t(pi->subpage_offset), int32_t(pi->subpage_divisions.size()));
			l_interface->go_to_page(*this, uint32_t(pn), *pi);
			if(pi->footer != layout_reference_none) {
				if(auto vrect = get_node(pi->footer).visible_rect; vrect < get_ui_rects().size()) {
					get_ui_rects()[vrect].display_flags |= ui_rectangle::flag_needs_update;
				}
			}
		} else {
			node->contents = std::make_unique<page_information>();
			l_interface->recreate_contents(*this, *node);
		}
	}
	enum class highlight_state {
		highlighting_item,
		not_highlighting_item,
		just_finished_highlighted_item,
		just_finished_not_highlighted_item,
		painted_normal
	};


	void default_recreate_page(window_data& win, layout_interface* l_interface, page_layout_specification const& spec) { 

		auto const l_id = l_interface->l_id;
		auto* pi = win.get_node(l_id).page_info();

		int32_t header_size = 0;
		if(spec.header) {
			pi->header = win.create_node(spec.header, win.get_node(l_id).width, win.get_node(l_id).height, false);
			auto& n = win.get_node(pi->header);
			n.parent = l_interface->l_id;
			header_size = n.height;
			win.get_node(l_id).width = std::max(win.get_node(l_id).width, n.width);
		}
		int32_t footer_size = 0;
		if(spec.footer) {
			pi->footer = win.create_node(spec.footer, win.get_node(l_id).width, win.get_node(l_id).height, false);
			auto& n = win.get_node(pi->footer);
			n.parent = l_interface->l_id;
			footer_size = n.height;
			win.get_node(l_id).width = std::max(win.get_node(l_id).width, n.width);
		}

		win.get_node(l_id).height = std::max(win.get_node(l_id).height, uint16_t(header_size + footer_size + spec.ex_page_bottom_margin + spec.ex_page_top_margin + 2));
		win.get_node(l_id).width = std::max(win.get_node(l_id).width, uint16_t(spec.min_column_horizontal_size * spec.min_columns + spec.ex_inter_column_margin * (spec.min_columns - 1) + spec.ex_page_left_margin + spec.ex_page_left_margin));

		if(spec.begin != spec.end) {

			auto const available_vert_space = win.get_node(l_id).height - (header_size + footer_size + spec.ex_page_bottom_margin + spec.ex_page_top_margin);
			auto const available_horz_space = win.get_node(l_id).width - (spec.ex_page_left_margin + spec.ex_page_right_margin);

			int32_t max_width = 0;

			//
			// SIZE CONTENTS, MAKE HIGHLIGHTS
			//

			highlight_state hs = highlight_state::painted_normal;
			int32_t max_item_width = 0;
			for(auto i = spec.begin; i != spec.end; ++i) {
				if(i->item) {
					auto item_spec = i->item->get_specification(win);
					auto cn = win.create_node(i->item, item_spec.minimum_line_size, item_spec.minimum_page_size, false, true);
					max_item_width = std::max(max_item_width, int32_t(item_spec.minimum_line_size));
					win.get_node(cn).set_margins(spec.column_left_margin, spec.column_right_margin);

					layout_reference label_control = layout_reference_none;
					if(i->label) {
						auto label_space = i->label->get_specification(win);
						label_control = win.create_node(i->label, label_space.minimum_line_size, label_space.minimum_page_size, false, true);

						if((label_space.minimum_line_size + item_spec.minimum_line_size + 1 + spec.column_left_margin + spec.column_right_margin) < spec.max_column_horizontal_size) {
							max_item_width = std::max(max_item_width, int32_t(label_space.minimum_line_size + item_spec.minimum_line_size + 1));
							win.get_node(label_control).set_margins(spec.column_left_margin, spec.column_right_margin + item_spec.minimum_line_size + 1);
							win.get_node(cn).set_margins(spec.column_left_margin + label_space.minimum_line_size + 1, spec.column_right_margin);
							win.get_node(label_control).flags |= layout_node::flag_overlay;
						} else {
							max_item_width = std::max(max_item_width, int32_t(label_space.minimum_line_size));
							win.get_node(label_control).set_margins(spec.column_left_margin, spec.column_right_margin);
							win.get_node(label_control).flags &= ~layout_node::flag_overlay;
						}
					}

					if(i->type == item_type::item_start) {
						if(hs == highlight_state::just_finished_not_highlighted_item) {
							hs = highlight_state::highlighting_item;
							win.get_node(cn).set_highlight(true);
							if(label_control != layout_reference_none)
								win.get_node(label_control).set_highlight(true);
						} else {
							hs = highlight_state::not_highlighting_item;
							win.get_node(cn).set_highlight(false);
							if(label_control != layout_reference_none)
								win.get_node(label_control).set_highlight(false);
						}
					} else if(i->type == item_type::item_end) {
						if(hs == highlight_state::highlighting_item) {
							win.get_node(cn).set_highlight(true);
							if(label_control != layout_reference_none)
								win.get_node(label_control).set_highlight(true);
							hs = highlight_state::just_finished_highlighted_item;
						} else {
							win.get_node(cn).set_highlight(false);
							if(label_control != layout_reference_none)
								win.get_node(label_control).set_highlight(false);
							hs = highlight_state::just_finished_not_highlighted_item;
						}
					} else if(i->type == item_type::single_item) {
						if(hs == highlight_state::just_finished_not_highlighted_item) {
							win.get_node(cn).set_highlight(true);
							if(label_control != layout_reference_none)
								win.get_node(label_control).set_highlight(true);
							hs = highlight_state::just_finished_highlighted_item;
						} else {
							win.get_node(cn).set_highlight(false);
							if(label_control != layout_reference_none)
								win.get_node(label_control).set_highlight(false);
							hs = highlight_state::just_finished_not_highlighted_item;
						}
					} else {
						if(hs == highlight_state::highlighting_item) {
							win.get_node(cn).set_highlight(true);
							if(label_control != layout_reference_none)
								win.get_node(label_control).set_highlight(true);
						} else if(hs == highlight_state::not_highlighting_item) {
							win.get_node(cn).set_highlight(false);
							if(label_control != layout_reference_none)
								win.get_node(label_control).set_highlight(false);
						} else {
							win.get_node(cn).set_highlight(false);
							if(label_control != layout_reference_none)
								win.get_node(label_control).set_highlight(false);
							hs = highlight_state::painted_normal;
						}
					}
				}
			}

			max_width = std::min(max_item_width + (spec.column_left_margin + spec.column_right_margin), int32_t(spec.max_column_horizontal_size));
			max_width = std::max(max_width, int32_t(spec.min_column_horizontal_size));

			//
			// MAKE COLUMNS
			//

			std::vector<layout_reference> columns;
			int32_t vertical_offset = 0;
			bool inside_glued_chunk = false;
			bool chunk_started_at_zero = false;
			page_content const* chunk_start = nullptr;

			layout_reference current_column = win.allocate_node();
			columns.push_back(current_column);
			{
				auto& column_container = win.get_node(current_column);
				column_container.contents = std::make_unique<container_information>();
				column_container.set_deferred(false);
				win.immediate_add_child(l_id, current_column);
			}

			for(auto i = spec.begin; i != spec.end; ++i) {
				if(i->item) {
					if(i->brk == column_break_behavior::column_header) {
						vertical_offset = available_vert_space;
					} else if(i->brk == column_break_behavior::section_header) {
						inside_glued_chunk = true;
						chunk_started_at_zero = chunk_started_at_zero || vertical_offset == 0;
						chunk_start = i;
					} else if(i->brk == column_break_behavior::dont_break_after) {
						inside_glued_chunk = true;
						chunk_started_at_zero = chunk_started_at_zero || vertical_offset == 0;
						if(!chunk_start)
							chunk_start = i;
					}
					auto item_space = win.get_node(i->item->l_id).height;
					if(i->label) {
						item_space = std::max(item_space, win.get_node(i->label->l_id).height);
					}
					if(item_space + vertical_offset > available_vert_space) {
						// opt, roll back, make new column
						if(inside_glued_chunk && !chunk_started_at_zero) {
							auto ci = win.get_node(current_column).container_info();

							while(i != chunk_start) {
								--i;
								if(ci->view_children().size() != 0) {
									if(i->label && i->item && ci->view_children().back() == i->label->l_id) {
										ci->modify_children().pop_back();
										ci->modify_children().pop_back();
									}
									if(i->item && ci->view_children().back() == i->item->l_id) {
										ci->modify_children().pop_back();
									}
									if(i->type == item_type::decoration_footer && win.get_node(ci->view_children().back()).l_interface == nullptr) {
										ci->modify_children().pop_back();
									}
								}
							}
							chunk_started_at_zero = true;
						}

						current_column = win.allocate_node();
						columns.push_back(current_column);
						{
							auto& column_container = win.get_node(current_column);
							column_container.contents = std::make_unique<container_information>();
							column_container.set_deferred(false);
							win.immediate_add_child(l_id, current_column);
						}
						vertical_offset = 0;
					}

					win.immediate_add_child(current_column, i->item->l_id);
					win.get_node(i->item->l_id).x = 0;
					win.get_node(i->item->l_id).y = uint16_t(vertical_offset);

					if(i->label) {
						win.immediate_add_child(current_column, i->label->l_id);
						win.get_node(i->label->l_id).x = 0;
						win.get_node(i->label->l_id).y = uint16_t(vertical_offset);
						if(win.get_node(i->label->l_id).height > win.get_node(i->item->l_id).height) {
							auto hoff = (1 + win.get_node(i->label->l_id).height - win.get_node(i->item->l_id).height) / 2;
							win.get_node(i->item->l_id).y += uint16_t(hoff);

						} else if(win.get_node(i->label->l_id).height < win.get_node(i->item->l_id).height) {
							auto hoff = (1 + win.get_node(i->item->l_id).height - win.get_node(i->label->l_id).height) / 2;
							win.get_node(i->label->l_id).y += uint16_t(hoff);
						}
					}
					
					vertical_offset += item_space;
					win.get_node(current_column).height = uint16_t(vertical_offset);

					if(i->brk != column_break_behavior::section_header && i->brk != column_break_behavior::dont_break_after) {
						inside_glued_chunk = false;
						chunk_started_at_zero = false;
						chunk_start = nullptr;
					}
				} else if(i->type == item_type::single_space) {
					vertical_offset += 1;
				} else if(i->type == item_type::double_space) {
					vertical_offset += 2;
				} else if(i->type == item_type::decoration_footer) {
					if(vertical_offset > 0 && spec.section_footer_decoration != uint8_t(-1)) {
						auto temp_offset = vertical_offset;
						vertical_offset += win.rendering_interface.get_icon_size(spec.section_footer_decoration).y;
						if(vertical_offset <= available_vert_space) {
							// add decoration
							auto decoration_node = win.allocate_node();
							auto& deco = win.get_node(decoration_node);
							deco.y = uint16_t(temp_offset);
							deco.x = 0;
							deco.width = win.rendering_interface.get_icon_size(spec.section_footer_decoration).x;
							deco.height = win.rendering_interface.get_icon_size(spec.section_footer_decoration).y;
							deco.contents = decoration_id{ spec.section_footer_decoration , spec.decoration_brush };
							win.get_node(current_column).height = uint16_t(vertical_offset);
							win.immediate_add_child(current_column, decoration_node);
						}
					}
				}
			}

			//
			// SIZE CHILDREN TO COLUMN
			//

			for(auto col : pi->view_columns()) {
				auto& col_children = win.get_node(col).container_info()->view_children();
				//calculate column size
				if(spec.uniform_column_width) {
					win.get_node(col).width = uint16_t(max_width);
				} else {
					int32_t column_max = spec.min_column_horizontal_size;
					for(auto cc : col_children) {
						column_max = std::max(column_max, win.get_node(cc).width + spec.column_left_margin + spec.column_right_margin);
					}
					win.get_node(col).width = uint16_t(std::min(column_max, int32_t(spec.max_column_horizontal_size)));
				}
				auto col_width = win.get_node(col).width;
				// resize children
				for(auto cc : col_children) {
					win.immediate_resize(win.get_node(cc), col_width, win.get_node(cc).height);
				}
			}

			//
			// DIVIDE COLUMNS INTO PAGES
			//

			int32_t max_page_width = 0;
			int32_t used_space = 0;
			int32_t count = 0;
			for(int32_t i = 0; i < int32_t(pi->view_columns().size()); ++i) {
				auto& ch_ref = win.get_node(pi->view_columns()[i]);
				if((used_space + spec.ex_inter_column_margin + ch_ref.width > available_horz_space && count >= spec.min_columns) || count >= spec.max_columns) {
					pi->subpage_divisions.push_back(uint16_t(i));
					max_page_width = std::max(used_space, max_page_width);
					used_space = 0;
					count = 0;
				}

				used_space += (spec.ex_inter_column_margin + ch_ref.width);
				++count;
			}
			max_page_width = std::max(used_space, max_page_width);
			max_page_width += spec.ex_page_left_margin + spec.ex_page_right_margin;

			win.get_node(l_id).width = std::max(win.get_node(l_id).width, uint16_t(max_page_width));
			if(spec.horz_shrink_page_to_content) {
				win.get_node(l_id).width = std::min(win.get_node(l_id).width, uint16_t(max_page_width));
			}

			//
			// POSITION COLUMNS IN PAGES
			//

			for(int32_t i = -1; i < int32_t(pi->subpage_divisions.size()); ++i) {
				uint32_t start = 0;
				uint32_t end = uint32_t(pi->view_columns().size());
				if(i >= 0)
					start = pi->subpage_divisions[i];
				if(i + 1 < int32_t(pi->subpage_divisions.size()))
					end = pi->subpage_divisions[i + 1];

				if(start < end) { // for each page
					int32_t max_col_height = 0;
					int32_t total_col_width = 0;
					for(uint32_t j = start; j < end; ++j) {
						auto& child_ref = win.get_node(pi->view_columns()[j]);

						max_col_height = std::max(max_col_height, int32_t(child_ref.height));
						total_col_width += child_ref.width;
					}

					int32_t space_used = spec.horizontal_columns_alignment != content_alignment::trailing ? spec.ex_page_left_margin : spec.ex_page_right_margin;
					int32_t space_between = spec.ex_inter_column_margin;
					int32_t extra_space = win.get_node(l_id).width - (total_col_width + (end - start - 1) * space_between + spec.ex_page_left_margin + spec.ex_page_right_margin);

					if(spec.additional_space_to_outer_margins || end == start + 1) {
						space_used += extra_space / 2;
					} else {
						auto divided_space = extra_space / (end - start - 1);
						space_between += divided_space;
						space_used += (extra_space - divided_space * (end - start - 1)) / 2;
					}

					if(spec.horizontal_columns_alignment == content_alignment::centered) {
						space_used += (win.get_node(l_id).width - (total_col_width + (end - start - 1) * space_between)) / 2;
					}
					for(uint32_t j = start; j < end; ++j) {
						auto& child_ref = win.get_node(pi->view_columns()[j]);
						if(spec.horizontal_columns_alignment != content_alignment::trailing) {
							child_ref.x = uint16_t(space_used);
						} else {
							child_ref.x = uint16_t(win.get_node(l_id).width - (space_used + child_ref.width));
						}
						space_used += child_ref.width + space_between;

						if(spec.vertical_column_alignment == content_alignment::leading) {
							child_ref.y = uint16_t(header_size + spec.ex_page_top_margin);
						} else if(spec.vertical_column_alignment == content_alignment::trailing) {
							child_ref.y = uint16_t(win.get_node(l_id).height - (footer_size + spec.ex_page_bottom_margin + child_ref.height));
						} else {
							child_ref.y = uint16_t(header_size + spec.ex_page_top_margin + (available_vert_space - max_col_height) / 2);
						}
					}
				} // end: for each page
			}

			//
			// END, POSITIONING COLUMNS
			// END, LAYOUT
			//
		}

		if(spec.header) {
			auto& n = win.get_node(pi->header);
			n.x = 0;
			n.y = 0;
			win.immediate_resize(n, win.get_node(l_id).width, n.height);
			n.set_margins(2, 2);
		}
		if(spec.footer) {
			auto& n = win.get_node(pi->footer);
			n.x = 0;
			n.y = win.get_node(l_id).height - n.height;
			win.immediate_resize(n, win.get_node(l_id).width, n.height);
			n.set_margins(1, 1);
		}

	}


	void window_data::repopulate_ui_rects(layout_reference id, layout_position base, uint8_t parent_foreground, uint8_t parent_background, bool highlight_line, bool skip_bg) {

		layout_node* n = &get_node(id);

		if(n->ignore())
			return;

		if(n->is_deferred() && n->l_interface)
			recreate_contents(n->l_interface, n);

		n = &layout_nodes.get_node(id);

		auto content_rect = n->l_interface ? n->l_interface->get_content_rectangle(*this) : layout_rect{ 0i16, 0i16, 0i16, 0i16 };
		if(n->width + content_rect.width > int32_t(layout_width)) {
			content_rect.width = int16_t(layout_width - n->width);
		}
		if(n->height + content_rect.height > int32_t(layout_height)) {
			content_rect.height = int16_t(layout_height - n->height);
		}
		if(base.x + content_rect.x < 0) {
			content_rect.x = -base.x;
		}
		if(base.y + content_rect.y < 0) {
			content_rect.y = -base.y;
		}
		if(base.x + content_rect.x + n->width + content_rect.width > int32_t(layout_width)) {
			content_rect.x -= int16_t((base.x + content_rect.x + n->width + content_rect.width) - int32_t(layout_width));
		}
		if(base.y + content_rect.y + n->height + content_rect.height > int32_t(layout_height)) {
			content_rect.y -= int16_t((base.y + content_rect.y + n->height + content_rect.height) - int32_t(layout_height));
		}

		auto const dest_rect = screen_rectangle_from_layout(*this,
			base.x + content_rect.x,
			base.y + content_rect.y,
			n->width + content_rect.width,
			n->height + content_rect.height);

		if(content_rect.width != 0 || content_rect.height != 0) {
			ui_rectangle p_candidate;
			p_candidate.parent_object = id;
			p_candidate.x_position = uint16_t(dest_rect.x);
			p_candidate.y_position = uint16_t(dest_rect.y);
			p_candidate.width = uint16_t(dest_rect.width);
			p_candidate.height = uint16_t(dest_rect.height);

			p_candidate.display_flags = ui_rectangle::flag_clear_rect;
			prepared_layout.push_back(std::move(p_candidate));
		}

		if(!n->l_interface && n->container_info() == nullptr) {
			ui_rectangle candidate;
			candidate.parent_object = id;
			candidate.background_index = parent_background;
			candidate.foreground_index = parent_foreground;

			candidate.x_position = uint16_t(dest_rect.x);
			candidate.y_position = uint16_t(dest_rect.y);
			candidate.width = uint16_t(dest_rect.width);
			candidate.height = uint16_t(dest_rect.height);

			if((highlight_line && !skip_bg) || (!highlight_line && n->highlight()))
				candidate.display_flags = ui_rectangle::flag_line_highlight;
			if(skip_bg)
				candidate.display_flags |= ui_rectangle::flag_skip_bg;

			candidate.rotate_borders(orientation);

			layout_nodes.get_node(id).visible_rect = ui_reference(prepared_layout.size());
			prepared_layout.push_back(std::move(candidate));
		} else if(auto pi = n->page_info(); pi) {
			ui_rectangle candidate = n->l_interface->prototype_ui_rectangle(*this, parent_foreground, parent_background);

			if(candidate.parent_object.get_render_interface() == nullptr) {
				candidate.parent_object = id;
			}
			
			candidate.x_position = uint16_t(dest_rect.x);
			candidate.y_position = uint16_t(dest_rect.y);
			candidate.width = uint16_t(dest_rect.width);
			candidate.height = uint16_t(dest_rect.height);

			if(skip_bg && candidate.background_index == parent_background && content_rect.width == 0 && content_rect.height == 0) {
				candidate.display_flags |= ui_rectangle::flag_skip_bg;
				if(!highlight_line && n->highlight())
					candidate.display_flags |= ui_rectangle::flag_line_highlight;
			} else {
				if((highlight_line || n->highlight()) && content_rect.width == 0 && content_rect.height == 0)
					candidate.display_flags |= ui_rectangle::flag_line_highlight;
			}

			if((n->flags & layout_node::flag_overlay) != 0) {
				candidate.display_flags |= ui_rectangle::flag_overlay;
			}

			uint32_t content_start = 0;
			uint32_t content_end = uint32_t(pi->view_columns().size());
			if(pi->subpage_offset > 0) {
				content_start = pi->subpage_divisions[pi->subpage_offset - 1];
			}
			if(pi->subpage_offset < pi->subpage_divisions.size()) {
				content_end = pi->subpage_divisions[pi->subpage_offset];
			}
			pi->subpage_offset = std::min(pi->subpage_offset, uint16_t(pi->subpage_divisions.size()));

			layout_nodes.get_node(id).visible_rect = ui_reference(prepared_layout.size());
			prepared_layout.push_back(candidate);

			for(uint32_t i = content_start; i < content_end; ++i) {
				auto& cn = layout_nodes.get_node(pi->view_columns()[i]);
				
				repopulate_ui_rects(pi->view_columns()[i],
					layout_position{ int16_t(base.x + content_rect.x + cn.x), int16_t(base.y + content_rect.y + cn.y) },
					candidate.foreground_index, candidate.background_index,
					(candidate.display_flags & ui_rectangle::flag_line_highlight) != 0, true);
				n = &layout_nodes.get_node(id);
				
			}

			if(pi->header != layout_reference_none) {
				auto& cn = layout_nodes.get_node(pi->header);
				repopulate_ui_rects(pi->header,
					layout_position{ int16_t(base.x + content_rect.x + cn.x), int16_t(base.y + content_rect.y + cn.y) },
					candidate.foreground_index, candidate.background_index, highlight_line || n->highlight(), true);
				n = &layout_nodes.get_node(id);
			}
			if(pi->footer != layout_reference_none) {
				auto& cn = layout_nodes.get_node(pi->footer);
				repopulate_ui_rects(pi->footer,
					layout_position{ int16_t(base.x + content_rect.x + cn.x), int16_t(base.y + content_rect.y + cn.y) },
					candidate.foreground_index, candidate.background_index, highlight_line || n->highlight(), true);
				n = &layout_nodes.get_node(id);
			}

			if(candidate.top_border != 0 || candidate.bottom_border != 0 || candidate.left_border != 0 ||candidate.right_border != 0) {
				candidate.rotate_borders(orientation);
				candidate.display_flags = ui_rectangle::flag_frame | ui_rectangle::flag_skip_bg;
				candidate.parent_object = nullptr;
				prepared_layout.push_back(candidate);
			}
		} else if(auto cinfo = n->container_info(); cinfo) {
			ui_rectangle candidate = n->l_interface ? n->l_interface->prototype_ui_rectangle(*this, parent_foreground, parent_background) : ui_rectangle(parent_foreground, parent_background);

			if(candidate.parent_object.get_render_interface() == nullptr) {
				candidate.parent_object = id;
			}

			if(skip_bg && candidate.background_index == parent_background && content_rect.width == 0 && content_rect.height == 0) {
				candidate.display_flags |= ui_rectangle::flag_skip_bg;
				if(!highlight_line && n->highlight())
					candidate.display_flags |= ui_rectangle::flag_line_highlight;
			} else {
				if((highlight_line || n->highlight()) && content_rect.width == 0 && content_rect.height == 0)
					candidate.display_flags |= ui_rectangle::flag_line_highlight;
			}
			if((n->flags & layout_node::flag_overlay) != 0) {
				candidate.display_flags |= ui_rectangle::flag_overlay;
			}

			candidate.x_position = uint16_t(dest_rect.x);
			candidate.y_position = uint16_t(dest_rect.y);
			candidate.width = uint16_t(dest_rect.width);
			candidate.height = uint16_t(dest_rect.height);

			uint32_t content_start = 0;
			uint32_t content_end = uint32_t(cinfo->view_children().size());
			
			layout_nodes.get_node(id).visible_rect = ui_reference(prepared_layout.size());
			prepared_layout.push_back(candidate);
			
			for(uint32_t i = content_start; i < content_end; ++i) {
				auto& cn = layout_nodes.get_node(cinfo->view_children()[i]);
				
				repopulate_ui_rects(cinfo->view_children()[i],
					layout_position{ int16_t(base.x + content_rect.x + cn.x), int16_t(base.y + content_rect.y + cn.y) },
					candidate.foreground_index, candidate.background_index,
					(candidate.display_flags& ui_rectangle::flag_line_highlight) != 0, true);
				n = &layout_nodes.get_node(id);
				
			}

			if(candidate.top_border != 0 || candidate.bottom_border != 0 || candidate.left_border != 0 || candidate.right_border != 0) {
				candidate.rotate_borders(orientation);
				candidate.display_flags = ui_rectangle::flag_frame | ui_rectangle::flag_skip_bg;
				candidate.parent_object = nullptr;
				prepared_layout.push_back(candidate);
			}
		} else {
			ui_rectangle candidate = n->l_interface->prototype_ui_rectangle(*this, parent_foreground, parent_background);

			if(candidate.parent_object.get_render_interface() == nullptr) {
				candidate.parent_object = id;
			}

			if(candidate.top_border != 0 || candidate.bottom_border != 0 || candidate.left_border != 0 || candidate.right_border != 0) {
				candidate.display_flags |= ui_rectangle::flag_frame;
			}

			if((n->flags & layout_node::flag_overlay) != 0) {
				candidate.display_flags |= ui_rectangle::flag_overlay;
			}

			candidate.x_position = uint16_t(dest_rect.x);
			candidate.y_position = uint16_t(dest_rect.y);
			candidate.width = uint16_t(dest_rect.width);
			candidate.height = uint16_t(dest_rect.height);

			if(skip_bg && candidate.background_index == parent_background) {
				candidate.display_flags |= ui_rectangle::flag_skip_bg;
				if(!highlight_line && n->highlight())
					candidate.display_flags |= ui_rectangle::flag_line_highlight;
			} else {
				if(highlight_line || n->highlight())
					candidate.display_flags |= ui_rectangle::flag_line_highlight;
			}

			candidate.rotate_borders(orientation);

			layout_nodes.get_node(id).visible_rect = ui_reference(prepared_layout.size());

			prepared_layout.push_back(candidate);
		}

		if(content_rect.width != 0 || content_rect.height != 0) {
			ui_rectangle p_candidate;
			p_candidate.parent_object = id;
			p_candidate.x_position = uint16_t(dest_rect.x);
			p_candidate.y_position = uint16_t(dest_rect.y);
			p_candidate.width = uint16_t(dest_rect.width);
			p_candidate.height = uint16_t(dest_rect.height);

			p_candidate.display_flags = ui_rectangle::flag_preserve_rect;
			prepared_layout.push_back(std::move(p_candidate));
		}
	}
	void window_data::repopulate_ui_rects() {
		clear_prepared_layout();
		if(info_popup.currently_visible && info_popup.l_id != layout_reference_none) {
			auto& n = get_node(info_popup.l_id);
			repopulate_ui_rects(info_popup.l_id, layout_position{ int16_t(n.x), int16_t(n.y) }, 1, 0);
			ui_rectangle vr = get_ui_rects()[n.visible_rect];
			vr.display_flags = ui_rectangle::flag_preserve_rect;
			vr.parent_object = nullptr;
			prepared_layout.push_back(vr);
		}
		if(top_node_id != layout_reference_none) {
			repopulate_ui_rects(top_node_id,
				layout_position{ int16_t(layout_nodes.get_node(top_node_id).x), int16_t(layout_nodes.get_node(top_node_id).y) },
				1, 0);
		}
		if(bottom_node_id != layout_reference_none) {
			repopulate_ui_rects(bottom_node_id,
				layout_position{ int16_t(layout_nodes.get_node(bottom_node_id).x), int16_t(layout_nodes.get_node(bottom_node_id).y) },
				1, 0);
		}
		if(left_node_id != layout_reference_none) {
			repopulate_ui_rects(left_node_id,
				layout_position{ int16_t(layout_nodes.get_node(left_node_id).x), int16_t(layout_nodes.get_node(left_node_id).y) },
				1, 0);
		}
		if(right_node_id != layout_reference_none) {
			repopulate_ui_rects(right_node_id,
				layout_position{ int16_t(layout_nodes.get_node(right_node_id).x), int16_t(layout_nodes.get_node(right_node_id).y) },
				1, 0);
		}
		if(has_window_title && title_bar.l_id != layout_reference_none) {
			repopulate_ui_rects(title_bar.l_id, layout_position{ 0i16,0i16 }, 1, 0);
		}
		if(window_bar.l_id != layout_reference_none) {
			repopulate_ui_rects(window_bar.l_id, layout_position{ 0i16, has_window_title ? 1i16 : 0i16 }, 1, 0);
		}
		repopulate_key_actions();
		ui_rects_out_of_date = false;
		rendering_interface.mark_for_complete_redraw();
	}

	void window_data::update_generation(layout_reference id) {
		layout_node& node = get_node(id);
		layout_nodes.update_generation(node);
		if(auto pi = node.page_info(); pi) {
			for(auto c : pi->view_columns())
				update_generation(c);
			if(pi->header != layout_reference_none)
				update_generation(pi->header);
			if(pi->footer != layout_reference_none)
				update_generation(pi->footer);
		} else if(auto ci = node.container_info(); ci) {
			for(auto c : ci->view_children())
				update_generation(c);
		}
	}
	void window_data::run_garbage_collector() {
		layout_nodes.begin_new_generation();
		if(top_node_id != layout_reference_none)
			update_generation(top_node_id);
		if(bottom_node_id != layout_reference_none)
			update_generation(bottom_node_id);
		if(left_node_id != layout_reference_none)
			update_generation(left_node_id);
		if(right_node_id != layout_reference_none)
			update_generation(right_node_id);
		if(title_bar.l_id != layout_reference_none)
			update_generation(title_bar.l_id);
		if(window_bar.l_id != layout_reference_none)
			update_generation(window_bar.l_id);
		if(info_popup.l_id != layout_reference_none)
			update_generation(info_popup.l_id);

		layout_nodes.garbage_collect();
	}

	void window_data::recreate_layout() {
		layout_out_of_date = true;
	}

	void window_data::internal_recreate_layout() {

		int32_t layout_x = int32_t(ui_width - window_border * 2) / layout_size;
		int32_t layout_y = int32_t(ui_height - window_border * 2) / layout_size;

		if(orientation == layout_orientation::vertical_left_to_right || orientation == layout_orientation::vertical_right_to_left) {
			std::swap(layout_x, layout_y);
		}

		layout_width = layout_x;
		layout_height = layout_y;

		layout_x -= 2;
		layout_y -= (has_window_title ? 1 : 0);

		if(top_node)
			top_node_id = create_node(top_node, layout_x, layout_y, false);
		if(bottom_node)
			bottom_node_id = create_node(bottom_node, layout_x, layout_y, false);

		auto rem_vert_space = layout_y -
			(((top_node_id != layout_reference_none) ? layout_nodes.get_node(top_node_id).height : 0)
				+ ((bottom_node_id != layout_reference_none) ? layout_nodes.get_node(bottom_node_id).height : 0));

		if(left_node)
			left_node_id = create_node(left_node, layout_x, rem_vert_space, false);
		if(right_node)
			right_node_id = create_node(right_node, layout_x, rem_vert_space, false);

		if(top_node_id != layout_reference_none) {
			layout_nodes.get_node(top_node_id).x = 0ui16;
			layout_nodes.get_node(top_node_id).y = 0ui16;
		}

		auto temp_middle_y_size = content_window_y;
		if(left_node_id != layout_reference_none)
			temp_middle_y_size = std::max(temp_middle_y_size, uint32_t(layout_nodes.get_node(left_node_id).height));
		if(right_node_id != layout_reference_none)
			temp_middle_y_size = std::max(temp_middle_y_size, uint32_t(layout_nodes.get_node(right_node_id).height));

		auto temp_window_min_size_x = content_window_x
			+ (left_node_id != layout_reference_none ? layout_nodes.get_node(left_node_id).width : 0)
			+ (right_node_id != layout_reference_none ? layout_nodes.get_node(right_node_id).width : 0);
		auto temp_window_min_size_y = temp_middle_y_size
			+ (top_node_id != layout_reference_none ? layout_nodes.get_node(top_node_id).height : 0)
			+ (bottom_node_id != layout_reference_none ? layout_nodes.get_node(bottom_node_id).height : 0);

		if(top_node_id != layout_reference_none)
			temp_window_min_size_x = std::max(temp_window_min_size_x, uint32_t(layout_nodes.get_node(top_node_id).width));
		if(bottom_node_id != layout_reference_none)
			temp_window_min_size_x = std::max(temp_window_min_size_x, uint32_t(layout_nodes.get_node(bottom_node_id).width));

		if(info_popup.currently_visible)
			create_node(&info_popup, layout_x, layout_y, false);

		auto wbar = create_node(&window_bar, 2, layout_y, true);

		temp_window_min_size_x = std::max(temp_window_min_size_x, uint32_t(layout_nodes.get_node(window_bar.settings_pages.l_id).width));

		if(has_window_title) {
			create_node(&title_bar, layout_x + 2, 1, false, true);
			layout_nodes.get_node(wbar).y = 1ui16;
		} else {
			layout_nodes.get_node(wbar).y = 0ui16;
		}
		
		auto calculated_bar_size = uint32_t(6 + (window_bar.min_i.has_value() ? 3 : 0) + (window_bar.max_i.has_value() ? 3 : 0) + (window_bar.setting_i.has_value() ? 3 : 0));
		temp_window_min_size_y = std::max(calculated_bar_size, temp_window_min_size_y);

		temp_window_min_size_x += 2;
		temp_window_min_size_y += (has_window_title ? 1 : 0);

		

		if(top_node_id != layout_reference_none) {
			layout_nodes.get_node(top_node_id).x = 2ui16;
			layout_nodes.get_node(top_node_id).width = uint16_t(layout_x);
			layout_nodes.get_node(top_node_id).y = (has_window_title ? 1ui16 : 0ui16);
		}
		if(left_node_id != layout_reference_none) {
			layout_nodes.get_node(left_node_id).x = 2ui16;
			layout_nodes.get_node(left_node_id).y = uint16_t((top_node_id != layout_reference_none ? layout_nodes.get_node(top_node_id).height : 0) + (has_window_title ? 1 : 0));
		}
		if(right_node_id != layout_reference_none) {
			layout_nodes.get_node(right_node_id).x = uint16_t(layout_x - layout_nodes.get_node(right_node_id).width + 2);
			layout_nodes.get_node(right_node_id).y = uint16_t((top_node_id != layout_reference_none ? layout_nodes.get_node(top_node_id).height : 0) + (has_window_title ? 1 : 0));
		}
		if(bottom_node_id != layout_reference_none) {
			layout_nodes.get_node(bottom_node_id).x = 2ui16;
			layout_nodes.get_node(bottom_node_id).width = uint16_t(layout_x);
			layout_nodes.get_node(bottom_node_id).y = uint16_t(layout_y - layout_nodes.get_node(bottom_node_id).height + (has_window_title ? 1ui16 : 0ui16));
		}


		
		if(orientation == layout_orientation::vertical_left_to_right || orientation == layout_orientation::vertical_right_to_left) {
			std::swap(temp_window_min_size_x, temp_window_min_size_y);
		}


		if(temp_window_min_size_x * layout_size + window_border * 2 > ui_width || temp_window_min_size_y * layout_size + window_border * 2 > ui_height) {
			// resize, exit

			auto new_x_size = std::max(temp_window_min_size_x * layout_size + window_border * 2, ui_width);
			auto new_y_size = std::max(temp_window_min_size_y * layout_size + window_border * 2, ui_height);

			auto work_area = window_interface.get_available_workspace();

			new_x_size = std::min(new_x_size, uint32_t(work_area.width));
			new_y_size = std::min(new_y_size, uint32_t(work_area.height));

			// check whether the window is really getting larger
			// this can fail if the window has already reached the maximum size, but the layout still wants more space
			if(new_x_size > ui_width)
				minimum_ui_width = new_x_size;
			if(new_y_size > ui_height)
				minimum_ui_height = new_y_size;

		}

		repopulate_ui_rects();

		layout_out_of_date = false;

		run_garbage_collector();
	}

	void window_data::change_size_multiplier(float v) {
		if(dynamic_settings.global_size_multiplier != v) {
			dynamic_settings.global_size_multiplier = v;
			layout_out_of_date = true;
			rendering_interface.stop_ui_animations(*this);

			++text_data.text_generation;
			info_popup.currently_visible = false;

			layout_size = int32_t(std::round(dynamic_settings.global_size_multiplier * float(dynamic_settings.layout_base_size) * dpi / 96.0f));

			text_interface.initialize_fonts(*this);
			rendering_interface.recreate_dpi_dependent_resource(*this);

			reset_layout();
			get_layout();

			update_window_focus();

			if(minimum_ui_width > ui_width || minimum_ui_height > ui_height) {
				expand_to_fit_content();
			} else {
				window_interface.invalidate_window();
			}
			accessibility_interface.on_window_layout_changed();
		}
	}
	void window_data::change_orientation(layout_orientation o) {
		orientation = o;
		dynamic_settings.preferred_orientation = o;
		layout_out_of_date = true;
		rendering_interface.stop_ui_animations(*this);

		++text_data.text_generation;
		info_popup.currently_visible = false;

		rendering_interface.recreate_dpi_dependent_resource(*this);

		reset_layout();
		get_layout();

		update_window_focus();

		window_bar.print_ui_settings.orientation_list.quiet_select_option_by_value(*this, size_t(o));

		if(minimum_ui_width > ui_width || minimum_ui_height > ui_height) {
			expand_to_fit_content();
		} else {
			window_interface.invalidate_window();
		}
		accessibility_interface.on_window_layout_changed();
	}


	void window_data::propogate_layout_change_upwards(layout_reference id) {
		auto& lref = get_node(id);
		if(lref.l_interface) {
			auto const old_width = lref.width;
			auto const old_height = lref.height;
			create_node(lref.l_interface, old_width, old_height, false, true);
			auto& lrefb = layout_nodes.get_node(id);

			if(lrefb.parent != std::numeric_limits<layout_reference>::max()
				&& (old_width != lrefb.width || old_height != lrefb.height)) {

				propogate_layout_change_upwards(lrefb.parent);
			} else if(lrefb.parent == std::numeric_limits<layout_reference>::max()
				&& (old_width != lrefb.width || old_height != lrefb.height)) {
				layout_out_of_date = true;
			}
		} else if(lref.parent != std::numeric_limits<layout_reference>::max()) {
			propogate_layout_change_upwards(lref.parent);
		} else {
			layout_out_of_date = true;
		}
	}

	void window_data::resize_item(layout_reference id, int32_t new_width, int32_t new_height) {

		propogate_layout_change_upwards(id);
		auto& lref = get_node(id);
		if(lref.l_interface) {
			auto const old_width = lref.width;
			auto const old_height = lref.height;
			create_node(lref.l_interface, new_width, new_height, false, true);
			auto& lrefb = layout_nodes.get_node(id);

			if(lrefb.parent != std::numeric_limits<layout_reference>::max()
				&& (old_width != lrefb.width || old_height != lrefb.height)) {

				propogate_layout_change_upwards(lrefb.parent);
			} else if(lrefb.parent == std::numeric_limits<layout_reference>::max()
				&& (old_width != lrefb.width || old_height != lrefb.height)) {
				layout_out_of_date = true;
			}
		} else {
			lref.width = uint16_t(new_width);
			lref.height = uint16_t(new_height);
			propogate_layout_change_upwards(id);
		}
		ui_rects_out_of_date = true;
	}


	void window_data::clear_prepared_layout() {
		for(auto& r : prepared_layout) {
			auto ln = r.parent_object.get_layout_reference();
			if(ln != layout_reference_none) {
				layout_nodes.get_node(ln).visible_rect = ui_reference_none;
			}
		}
		prepared_layout.clear();
	}

	void window_data::reset_layout() {
		last_under_cursor = ui_reference_none;

		layout_nodes.begin_new_generation();
		layout_nodes.garbage_collect();
		layout_nodes.reset();
		prepared_layout.clear();
		layout_out_of_date = true;
	}

	layout_reference interface_or_layout_ref::get_layout_reference() const {
		if(ptr == nullptr)
			return layout_reference_none;
		else if((reinterpret_cast<size_t>(ptr) & 0x01) == 0)
			return ptr->l_id;
		else
			return layout_reference(reinterpret_cast<size_t>(ptr) >> 1);
	}
	render_interface* interface_or_layout_ref::get_render_interface() const {
		if(ptr == nullptr)
			return nullptr;
		else if((reinterpret_cast<size_t>(ptr) & 0x01) == 0)
			return ptr;
		else
			return nullptr;
	}
	void layout_node::reset_node() {
		contents = std::monostate{};

		if(l_interface)
			l_interface->set_layout_id(layout_reference_none);
		l_interface = nullptr;
		parent = layout_reference_none;

		x = 0;
		y = 0;
		width = 0;
		height = 0;
		flags = 0;
		margins = 0;
	}
	

	//layout_node_storage::
	void layout_node_storage::reset() {
		const uint32_t sz = uint32_t(node_storage.size());
		for(uint32_t i = 0; i < sz; ++i) {
			node_storage[i].reset_node();
		}

		last_free = layout_reference_none;
		node_storage.clear();
	}
	void layout_node_storage::begin_new_generation() {
		current_generation = uint8_t(layout_node::flag_generation_mask & (current_generation + 1));
	}
	void layout_node_storage::release_node(layout_reference id) {
		if((node_storage[id].flags & layout_node::flag_freed) == 0) { // prevent double frees
			node_storage[id].reset_node();
			node_storage[id].flags = layout_node::flag_freed;
			node_storage[id].parent = last_free;
			last_free = id;
		}
	}
	void layout_node_storage::update_generation(layout_node& n) {
		n.set_generation(current_generation);
	}
	void layout_node_storage::garbage_collect() {
		const uint32_t sz = uint32_t(node_storage.size());
		for(uint32_t i = 0; i < sz; ++i) {
			if((node_storage[i].flags & layout_node::flag_freed) == 0 && node_storage[i].generation() != current_generation) {
				node_storage[i].reset_node();
				node_storage[i].flags = layout_node::flag_freed;
				node_storage[i].parent = last_free;
				last_free = layout_reference(i);
			}
		}
	}
	layout_reference layout_node_storage::allocate_node() {
		if(last_free == layout_reference_none) {
			auto new_id = layout_reference(node_storage.size());
			node_storage.emplace_back();
			node_storage.back().set_generation(current_generation);
			if(new_id == layout_reference_none)
				std::abort(); // ERROR used too many layout ids
			return new_id;
		} else {
			auto new_id = last_free;
			last_free = node_storage[last_free].parent;
			node_storage[new_id].parent = layout_reference_none;
			node_storage[new_id].flags = 0;
			node_storage[new_id].set_generation(current_generation);
			return new_id;
		}
	}
	layout_reference layout_node_storage::allocate_node(layout_interface* li) {
		auto li_existing = li->l_id;
		if(li_existing != layout_reference_none) {
			node_storage[li_existing].set_generation(current_generation);
			return li_existing;
		}
		layout_reference new_id;
		if(last_free == layout_reference_none) {
			new_id = layout_reference(node_storage.size());
			node_storage.emplace_back();
			node_storage.back().set_generation(current_generation);
			if(new_id == layout_reference_none)
				std::abort(); // ERROR used too many layout ids
		} else {
			new_id = last_free;
			last_free = node_storage[last_free].parent;
			node_storage[new_id].parent = layout_reference_none;
			node_storage[new_id].flags = 0;
			node_storage[new_id].set_generation(current_generation);
		}
		node_storage[new_id].l_interface = li;
		li->set_layout_id(new_id);
		return new_id;
	}
	layout_node& layout_node_storage::get_node(layout_reference id) {
		return node_storage[id];
	}
	layout_node const& layout_node_storage::get_node(layout_reference id) const {
		return node_storage[id];
	}

	std::vector<ui_rectangle>& window_data::get_layout() {
		if(layout_out_of_date) {
			internal_recreate_layout();
		} else if(ui_rects_out_of_date) {
			repopulate_ui_rects();
		}
		return prepared_layout;
	};

	void window_data::redraw_ui() {
		ui_rects_out_of_date = true;
		window_interface.invalidate_window();
	}

	void window_data::set_window_title(std::wstring const& title) {
		if(has_window_title) {
			if(title.length() == 0) {
				window_title.clear();
				has_window_title = false;
				layout_out_of_date = true;
				window_interface.set_window_title(window_title.c_str());
			} else {
				window_title = title;
				window_interface.set_window_title(window_title.c_str());
			}
		} else {
			if(title.length() == 0) {

			} else {
				window_title = title;
				has_window_title = true;
				layout_out_of_date = true;
				window_interface.set_window_title(window_title.c_str());
			}
		}
	}

	wchar_t const* window_data::get_window_title() const {
		return window_title.c_str();

	}

	bool window_data::is_rendered(layout_reference r) const {
		return get_node(r).visible_rect < prepared_layout.size() && !get_node(r).ignore();
	}


	layout_reference window_data::find_common_root(layout_reference a, layout_reference b) const {
		if(b == layout_reference_none)
			return layout_reference_none;

		for(layout_reference outer = a; outer != layout_reference_none; outer = layout_nodes.get_node(outer).parent) {
			for(layout_reference inner = b; inner != layout_reference_none; inner = layout_nodes.get_node(inner).parent) {

				if(inner == outer)
					return inner;

			}
		}

		return std::numeric_limits<layout_reference>::max();
	}

	bool window_data::is_child_of(layout_reference parent, layout_reference child) const {
		for(layout_reference inner = child; inner != layout_reference_none; inner = get_node(inner).parent) {

			if(inner == parent)
				return true;

		}
		return false;
	}

	layout_reference window_data::get_containing_page(layout_reference r) const {
		auto& n = get_node(r);
		r = n.parent;

		while(r != layout_reference_none) {
			auto& p = get_node(r);
			if(p.page_info()) {
				return r;
			}
			r = p.parent;
		}
		return layout_reference_none;
	}

	layout_reference window_data::get_containing_proper_page(layout_reference r) const {
		auto& n = get_node(r);
		r = n.parent;

		while(r != layout_reference_none) {
			auto& p = get_node(r);
			if(auto pi = p.page_info(); pi && pi->header != layout_reference_none) {
				return r;
			}
			r = p.parent;
		}
		return layout_reference_none;
	}

	layout_reference window_data::get_containing_page_or_container(layout_reference r) const {
		auto& n = get_node(r);
		r = n.parent;

		while(r != layout_reference_none) {
			auto& p = get_node(r);
			if(p.page_info()) {
				return r;
			}
			if(p.container_info()) {
				return r;
			}
			r = p.parent;
		}
		return layout_reference_none;
	}

	layout_reference window_data::get_enclosing_node(layout_reference r)  {
		while(r != layout_reference_none) {
			auto& p = get_node(r);
			if(p.page_info() && p.page_info()->header != layout_reference_none) {
				return r;
			}
			if(p.container_info() && p.l_interface) {
				auto expandrect = p.l_interface->get_content_rectangle(*this);
				if(expandrect.width != 0 || expandrect.height != 0)
					return r;
			}
			r = p.parent;
		}
		return layout_reference_none;
	}


	bool window_data::is_visible(layout_reference r) const {
		if(r == layout_reference_none)
			return false;
		return !get_node(r).ignore() && get_node(r).visible_rect < get_ui_rects().size();
	}

	struct acc_visible_children_it {
		std::vector<layout_reference> const& n;
		uint32_t content_pos = 0;

		acc_visible_children_it(std::vector<layout_reference> const& m, uint32_t p) : n(m), content_pos(p) {
		}
		layout_reference operator*() const {
			return n[content_pos];
		}
		bool operator==(acc_visible_children_it const& o) const {
			return content_pos == o.content_pos;
		}
		bool operator!=(acc_visible_children_it const& o) const {
			return content_pos != o.content_pos;
		}
		acc_visible_children_it& operator++() {
			++content_pos;
			return *this;
		}
		acc_visible_children_it& operator--() {
			--content_pos;
			return *this;
		}
	};

	std::vector<layout_reference> dummy_vector;

	struct acc_visible_children_it_generator {
		layout_node const& n;
		std::vector<layout_reference> const* contents = nullptr;
		acc_visible_children_it_generator(layout_node const& m) : n(m) {
			if(auto p = n.page_info(); p) {
				contents = &p->view_columns();
			} else if(auto c = n.container_info(); c) {
				contents = &c->view_children();
			} else {
				contents = &dummy_vector;
			}
		}
		acc_visible_children_it begin() {
			uint32_t content_start = 0;
			if(auto p = n.page_info(); p) {
				if(p->subpage_offset > 0) {
					content_start = p->subpage_divisions[p->subpage_offset - 1];
				}
			}
			return acc_visible_children_it(*contents, content_start);
		}
		acc_visible_children_it end() {
			if(auto p = n.page_info(); p) {
				uint32_t content_end = uint32_t(p->view_columns().size());
				if(p->subpage_offset < p->subpage_divisions.size()) {
					content_end = p->subpage_divisions[p->subpage_offset];
				}
				return acc_visible_children_it(*contents, content_end);
			} else {
				return acc_visible_children_it(*contents, uint32_t(contents->size()));
			}
		}
	};
	acc_visible_children_it_generator acc_visible_children(layout_node const& n) {
		return acc_visible_children_it_generator(n);
	}

	enum class acc_special_page_iteration {
		header, body, finished, footer
	};

	struct acc_sub_items_iterator {
		window_data& lm;
		page_information* pi = nullptr;
		acc_visible_children_it child_it;
		acc_visible_children_it child_it_end;
		acc_special_page_iteration page_it_status = acc_special_page_iteration::header;

		std::vector<std::pair<acc_sub_items_iterator, acc_sub_items_iterator>> sub_iterators;

		void advance_iterator_status() {
			if(page_it_status == acc_special_page_iteration::header) {
				if(child_it.content_pos != child_it_end.content_pos)
					page_it_status = acc_special_page_iteration::body;
				else
					page_it_status = acc_special_page_iteration::footer;
			} else if(page_it_status == acc_special_page_iteration::body) {
				++child_it;
				if(child_it == child_it_end)
					page_it_status = pi ? acc_special_page_iteration::footer : acc_special_page_iteration::finished;
			} else {
				page_it_status = acc_special_page_iteration::finished;
			}
		}

		bool try_node(layout_reference r) {
			auto& cn = lm.get_node(r);
			if(cn.ignore()) {
				return false;
			} else if(auto i = cn.l_interface; i) {
				if(auto a = i->get_accessibility_interface(lm); a)
					return true;
			}
			
			if(cn.page_info()) { // case: subpage
				auto vis_range = acc_visible_children(cn);
				acc_sub_items_iterator sub(lm, cn.page_info(), vis_range.begin(), vis_range.end());
				if(sub.child_it != sub.child_it_end) {
					sub_iterators.push_back(std::pair<acc_sub_items_iterator, acc_sub_items_iterator>(sub, acc_sub_items_iterator(lm, sub.child_it_end)));
						return true;
				}
			} else if(cn.container_info()) { // case: subpage
				auto vis_range = acc_visible_children(cn);
				acc_sub_items_iterator sub(lm, nullptr, vis_range.begin(), vis_range.end());
				if(sub.child_it != sub.child_it_end) {
					sub_iterators.push_back(std::pair<acc_sub_items_iterator, acc_sub_items_iterator>(sub, acc_sub_items_iterator(lm, sub.child_it_end)));
					return true;
				}
			}

			return false;
		}

		acc_sub_items_iterator(window_data& l, page_information* p, acc_visible_children_it i, acc_visible_children_it e) : lm(l), pi(p), child_it(i), child_it_end(e) {
			// advance to first

			if(!pi) {
				if(child_it.content_pos != child_it_end.content_pos)
					page_it_status = acc_special_page_iteration::body;
				else
					page_it_status = acc_special_page_iteration::finished;
			}

			if(page_it_status == acc_special_page_iteration::header) {
				if(pi->header != layout_reference_none && try_node(pi->header)) {
					return;
				}
				advance_iterator_status();
			}
			while(child_it.content_pos != child_it_end.content_pos) {
				if(lm.is_rendered(*child_it)) {
					if(try_node(*child_it))
						return;
				}
				advance_iterator_status();
			}
			if(page_it_status == acc_special_page_iteration::footer) {
				if(pi->footer != layout_reference_none && try_node(pi->footer)) {
					return;
				}
				advance_iterator_status();
			}
		}
		acc_sub_items_iterator(window_data& l, acc_visible_children_it e) : lm(l), child_it(e), child_it_end(e), page_it_status(acc_special_page_iteration::finished) {
		}
		bool operator==(acc_sub_items_iterator const& o) const {
			return child_it == o.child_it && page_it_status == o.page_it_status;
		}
		bool operator!=(acc_sub_items_iterator const& o) const {
			return !(*this == o);
		}
		accessibility_object* operator*() const {
			if(sub_iterators.empty()) {
				if(page_it_status == acc_special_page_iteration::header) {
					auto& cn = lm.get_node(pi->header);
					return cn.l_interface->get_accessibility_interface(lm);
				} else if(page_it_status == acc_special_page_iteration::footer) {
					auto& cn = lm.get_node(pi->footer);
					return cn.l_interface->get_accessibility_interface(lm);
				}

				auto& cn = lm.get_node(*child_it);
				return cn.l_interface->get_accessibility_interface(lm);
			} else {
				return *(sub_iterators.back().first);
			}
		}
		acc_sub_items_iterator& operator++() {
			while(!sub_iterators.empty()) {
				++(sub_iterators.back().first);
				if(sub_iterators.back().first != sub_iterators.back().second)
					break;
				sub_iterators.pop_back();
			}
			if(sub_iterators.empty()) {
				advance_iterator_status();

				while(child_it.content_pos != child_it_end.content_pos) {
					if(lm.is_rendered(*child_it)) {
						if(try_node(*child_it))
							return *this;
					}
					advance_iterator_status();
				}
				if(page_it_status == acc_special_page_iteration::footer) {
					if(pi->footer != layout_reference_none && try_node(pi->footer)) {
						return *this;
					}
					advance_iterator_status();
				}
			}
			return *this;
		}
		acc_sub_items_iterator& operator+=(int32_t n) {
			while(n > 0) {
				++(*this);
				--n;
			}
			return *this;
		}
	};

	struct acc_sub_items_iterator_generator {
		window_data& lm;
		page_information* pi;
		acc_visible_children_it b;
		acc_visible_children_it e;

		acc_sub_items_iterator_generator(window_data& l, layout_node const& n) : lm(l), pi(n.page_info()), b(acc_visible_children(n).begin()), e(acc_visible_children(n).end()) {
		}
		acc_sub_items_iterator begin() {
			return acc_sub_items_iterator(lm, pi, b, e);
		}
		acc_sub_items_iterator end() {
			return acc_sub_items_iterator(lm, e);
		}
	};

	acc_sub_items_iterator_generator accessible_children(window_data& lm, layout_node const& n) {
		return acc_sub_items_iterator_generator(lm, n);
	}

	accessibility_object* window_data::get_parent_accessibility_object(layout_reference r) {
		if(r == layout_reference_none)
			return nullptr;
		r = get_node(r).parent;

		while(r != layout_reference_none) {
			auto& p = get_node(r);
			if(p.l_interface) {
				if(auto a = p.l_interface->get_accessibility_interface(*this); a)
					return a;
			}
			r = p.parent;
		}

		return nullptr;
	}

	layout_reference get_parent_accessibility_node(window_data& win, layout_reference r) {
		if(r == layout_reference_none)
			return layout_reference_none;

		r = win.get_node(r).parent;

		while(r != layout_reference_none) {
			auto& p = win.get_node(r);
			if(p.l_interface) {
				if(auto a = p.l_interface->get_accessibility_interface(win); a)
					return r;
			}
			r = p.parent;
		}

		return layout_reference_none;
	}
	accessibility_object* get_ao_from_id(window_data& win, layout_reference r) {
		return r != layout_reference_none ? (win.get_node(r).l_interface ? win.get_node(r).l_interface->get_accessibility_interface(win) : nullptr) : nullptr;
	}
	accessibility_object* window_data::get_previous_sibling_accessibility_object(layout_reference r) {
		auto parent_id = get_parent_accessibility_node(*this, r);
		auto self_acc_object = get_ao_from_id(*this, r);

		if(!self_acc_object)
			return nullptr;

		if(parent_id == layout_reference_none) { // parent = top window

			accessibility_object* previous = get_ao_from_id(*this, title_bar.l_id);
			accessibility_object* next = get_ao_from_id(*this, window_bar.l_id);
			if(next == self_acc_object)
				return previous;
			previous = next;

			next = get_ao_from_id(*this, top_node_id);
			if(next == self_acc_object)
				return previous;
			previous = next;

			next = get_ao_from_id(*this, left_node_id);
			if(next == self_acc_object)
				return previous;
			previous = next;

			next = get_ao_from_id(*this, right_node_id);
			if(next == self_acc_object)
				return previous;
			previous = next;

			next = get_ao_from_id(*this, bottom_node_id);
			if(next == self_acc_object)
				return previous;
			previous = next;
			
			return nullptr;
		} else {
			accessibility_object* previous = nullptr;
			for(auto i : accessible_children(*this, get_node(parent_id))) {
				if(i == self_acc_object)
					return previous;
				previous = i;
			}
			return nullptr;
		}
	}
	accessibility_object* window_data::get_next_sibling_accessibility_object(layout_reference r) {
		auto parent_id = get_parent_accessibility_node(*this, r);
		auto self_acc_object = get_ao_from_id(*this, r);

		if(!self_acc_object)
			return nullptr;

		if(parent_id == layout_reference_none) { // parent = top window

			accessibility_object* previous = get_ao_from_id(*this, title_bar.l_id);
			accessibility_object* next = get_ao_from_id(*this, window_bar.l_id);
			if(previous == self_acc_object)
				return next;
			previous = next;

			next = get_ao_from_id(*this, top_node_id);
			if(previous == self_acc_object)
				return next;
			previous = next;

			next = get_ao_from_id(*this, left_node_id);
			if(previous == self_acc_object)
				return next;
			previous = next;

			next = get_ao_from_id(*this, right_node_id);
			if(previous == self_acc_object)
				return next;
			previous = next;

			next = get_ao_from_id(*this, bottom_node_id);
			if(previous == self_acc_object)
				return next;
			previous = next;

			return nullptr;
		} else {
			accessibility_object* previous = nullptr;
			for(auto i : accessible_children(*this, get_node(parent_id))) {
				if(previous == self_acc_object)
					return i;
				previous = i;
			}
			return nullptr;
		}
	}
	accessibility_object* window_data::get_first_child_accessibility_object(layout_reference r) {
		auto self_acc_object = get_ao_from_id(*this, r);

		if(!self_acc_object)
			return nullptr;

		auto rng = accessible_children(*this, get_node(r));
		if(rng.begin() == rng.end())
			return nullptr;
		else
			return *(rng.begin());
	}
	accessibility_object* window_data::get_last_child_accessibility_object(layout_reference r) {
		auto self_acc_object = get_ao_from_id(*this, r);

		if(!self_acc_object)
			return nullptr;

		accessibility_object* previous = nullptr;
		for(auto i : accessible_children(*this, get_node(r))) {
			previous = i;
		}
		return previous;
	}

	void window_data::immediate_add_child(layout_reference parent, layout_reference child) {
		auto& p = get_node(parent);
		auto& c = get_node(child);
		if(auto pi = p.page_info(); pi) {
			pi->add(child);
		} else if(auto ci = p.container_info(); ci) {
			ci->add(child);
		}
		c.parent = parent;
	}
}
