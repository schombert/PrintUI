#include "printui_utility.hpp"

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

	template<typename T>
	T& o_select(content_orientation o, T& horz, T& vert) {
		return o == content_orientation::page ? horz : vert;
	}

	

	void format_columns(page_information& containing_page_info, window_data& layout_nodes,
		int32_t max_v_size, int32_t max_h_size, formatting_values const& values) {
		auto o = values.o;
		auto header_size = values.header_size;
		auto footer_size = values.footer_size;
		auto align = values.column_in_page;

		for(int32_t i = -1; i < int32_t(containing_page_info.subpage_divisions.size()); ++i) {
			uint32_t start = 0;
			uint32_t end = uint32_t(containing_page_info.columns.size());
			if(i >= 0)
				start = containing_page_info.subpage_divisions[i];
			if(i + 1 < int32_t(containing_page_info.subpage_divisions.size()))
				end = containing_page_info.subpage_divisions[i + 1];

			if(start < end) {
				int32_t max_col_height = 0;
				int32_t total_col_width = 0;
				for(uint32_t j = start; j < end; ++j) {
					auto& child_ref = layout_nodes.get_node(containing_page_info.columns[j]);
					
					max_col_height = std::max(max_col_height, int32_t(o_select(o, child_ref.height, child_ref.width)));
					total_col_width += o_select(o, child_ref.width, child_ref.height);
				}

				auto const with_orientation_margin_space = (o_select(o, max_v_size, max_h_size) - max_col_height) - (o == content_orientation::page ? (header_size + footer_size) : 0);
				auto const leading_with_o_margin = std::clamp(with_orientation_margin_space / 2,
					values.with_orientation_lmn,
					values.with_orientation_lmx) + (o == content_orientation::page ? header_size : 0);
				auto const trailing_with_o_margin = std::clamp(with_orientation_margin_space - leading_with_o_margin,
					values.with_orientation_tmn,
					values.with_orientation_tmx) + (o == content_orientation::page ? footer_size : 0);
				auto const avail_space = o_select(o, max_v_size, max_h_size) - (leading_with_o_margin + trailing_with_o_margin);

				auto const num_cols = start - end;
				auto const page_h_margin_space = max_h_size - (total_col_width);
				auto const margin_per = int32_t(page_h_margin_space / num_cols);
				auto const remainder = page_h_margin_space % num_cols;
				auto const extra_lead = values.column_size != size_flags::fill_to_max ? remainder / 2 : 0;

				auto const leading_line_margin = values.column_size != size_flags::fill_to_max ? std::clamp(margin_per / 2, values.across_orientation_lmn, values.across_orientation_lmx) : 0;
				
				int32_t htrack = leading_line_margin + extra_lead;
				for(uint32_t j = start; j < end; ++j) {
					auto& child_ref = layout_nodes.get_node(containing_page_info.columns[j]);
					o_select(o, child_ref.x, child_ref.y) = uint16_t(htrack);

					if(align == content_alignment::leading) {
						o_select(o, child_ref.y, child_ref.x) = uint16_t(leading_with_o_margin);
					} else if(align == content_alignment::centered) {
						o_select(o, child_ref.y, child_ref.x) = uint16_t(leading_with_o_margin + (avail_space - o_select(o, child_ref.height, child_ref.width)) / 2 );
					} else if(align == content_alignment::trailing) {
						o_select(o, child_ref.y, child_ref.x) = uint16_t(leading_with_o_margin + avail_space - o_select(o, child_ref.height, child_ref.width));
					}

					
					htrack += margin_per + o_select(o, child_ref.width, child_ref.height);
					if(values.column_size == size_flags::fill_to_max) {
						o_select(o, child_ref.width, child_ref.height) += uint16_t(margin_per);
						if(j == end - 1) {
							o_select(o, child_ref.width, child_ref.height) += uint16_t(remainder);
						}
					}

					auto col_content = child_ref.container_info();
					for(auto child_result : col_content->children) {
						auto& child = layout_nodes.get_node(child_result);

						switch(values.content_in_column) {
							case content_alignment::leading: //no change
								break;
							case content_alignment::trailing:
								o_select(o, child.x, child.y) = uint16_t(o_select(o, child_ref.width, child_ref.height) - o_select(o, child.width, child.height));
								break;
							case content_alignment::centered:
								o_select(o, child.x, child.y) = uint16_t((o_select(o, child_ref.width, child_ref.height) - o_select(o, child.width, child.height)) / 2);
								break;
							case content_alignment::justified:
								o_select(o, child.x, child.y) = 0ui16;
								layout_nodes.immediate_resize(child,
									o == content_orientation::page ? o_select(o, child_ref.width, child_ref.height) : child.width,
									o == content_orientation::page ? child.height : o_select(o, child_ref.width, child_ref.height));
								break;
						}
					}
				}


			}
		}
	}

	

	column_content make_column(std::vector<layout_interface*> const& source, window_data& win, int32_t& index_into_source, content_orientation o, int32_t max_space, int32_t across_column_min, int32_t across_column_max, int32_t& running_list_position, bool single_col) {

		column_content retvalue;

		for(; index_into_source < int32_t(source.size()); ++index_into_source) {
			auto node_basic_spec = source[index_into_source]->get_specification(win);

			if(!single_col && retvalue.along_column_size != 0 && (node_basic_spec.paragraph_breaking == pargraph_break_behavior::break_before || node_basic_spec.paragraph_breaking == pargraph_break_behavior::break_both)) {
				return retvalue;
			}

			layout_reference child_ref;
			auto node_type = source[index_into_source]->get_node_type();

			if(node_type != layout_node_type::list_stub) {
				int32_t temp_unity = 1;
				child_ref = win.create_node(source[index_into_source],
					o_select(o, across_column_min, temp_unity),
					o_select(o, temp_unity, across_column_min), false, true);
				auto& cn = win.get_node(child_ref);
				if(across_column_max > 0 && o_select(o, cn.width, cn.height) > across_column_max) {
					int32_t temp = o_select(o, cn.height, cn.width);
					win.immediate_resize(cn, o_select(o, across_column_max, temp), o_select(o, temp, across_column_max));
				}
			} else {
				child_ref = win.allocate_node();
			}

			layout_node& cnode = win.get_node(child_ref);
			cnode.y = 0;
			cnode.x = 0;

			if(!single_col && retvalue.along_column_size != 0
				&& ((o_select(o, cnode.height, cnode.width) + retvalue.along_column_size) > max_space)) {

				if(node_type == layout_node_type::list_stub) {
					win.release_node(child_ref);
				}
				return retvalue;
			}

			retvalue.accross_column_max = std::max(retvalue.accross_column_max, int32_t(o_select(o, cnode.width, cnode.height)));

			retvalue.contents.push_back(child_ref);

			if(node_type == layout_node_type::list_stub) {
				auto list_layout = source[index_into_source]->get_auto_layout_specification(win);
				auto const total_space_remaining = max_space - retvalue.along_column_size;
				auto const children_remainder = int32_t(list_layout.list_size) - running_list_position;
				auto const child_space = o_select(o, node_basic_spec.minimum_page_size, node_basic_spec.minimum_line_size);

				cnode.contents = std::make_unique<container_information>();
				cnode.container_info()->list_offset = running_list_position;
				cnode.l_interface = source[index_into_source];
				cnode.layout_deferred = true;

				if(single_col || total_space_remaining / child_space >= children_remainder) {
					o_select(o, cnode.y, cnode.x) = uint16_t(retvalue.along_column_size);
					o_select(o, cnode.width, cnode.height) = o_select(o, node_basic_spec.minimum_line_size, node_basic_spec.minimum_page_size);
					o_select(o, cnode.height, cnode.width) = uint16_t(children_remainder * child_space);

					retvalue.along_column_size += children_remainder * child_space;
					running_list_position = 0;
				} else {
					auto chldren_in_batch = total_space_remaining / child_space;

					o_select(o, cnode.y, cnode.x) = uint16_t(retvalue.along_column_size);
					o_select(o, cnode.width, cnode.height) = o_select(o, node_basic_spec.minimum_line_size, node_basic_spec.minimum_page_size);
					o_select(o, cnode.height, cnode.width) = uint16_t(chldren_in_batch * child_space);

					running_list_position += chldren_in_batch;
					retvalue.along_column_size += total_space_remaining;
					--index_into_source; // repeat
				}
			} else if( o_select(o, cnode.height, cnode.width) != 0) {
				o_select(o, cnode.y, cnode.x) = uint16_t(retvalue.along_column_size);
				retvalue.along_column_size += o_select(o, cnode.height, cnode.width);
			}

			if(!single_col && (node_basic_spec.paragraph_breaking == pargraph_break_behavior::break_after || node_basic_spec.paragraph_breaking == pargraph_break_behavior::break_both)) {

				++index_into_source;
				return retvalue;
			}
		}

		return retvalue;
	}

	int32_t format_pages(layout_node& n, window_data& layout_nodes, formatting_values const& values, int32_t min_margin, int32_t max_space, uint8_t max_cols) {
		auto o = values.o;

		auto node_page_info = n.page_info();
		for(auto col_id : node_page_info->columns) {
			auto& col = layout_nodes.get_node(col_id);
			if(col.container_info()) {
				int32_t max_col_width = 0;
				for(auto cc : col.container_info()->children) {
					auto& cc_ref = layout_nodes.get_node(cc);
					max_col_width = std::max(max_col_width, int32_t(o_select(o, cc_ref.width, cc_ref.height)));
				}
				col.width = uint16_t(max_col_width);
				for(auto cc : col.container_info()->children) {
					auto& cc_ref = layout_nodes.get_node(cc);
					o_select(o, cc_ref.width, cc_ref.height) = uint16_t(max_col_width);
				}
				o_select(o, n.width, n.height) = std::max(o_select(o, n.width, n.height), uint16_t(max_col_width + min_margin));
			}
		}
		int32_t max_page_width = 0;
		int32_t used_space = 0;
		int32_t count = 0;
		for(int32_t i = 0; i < int32_t(node_page_info->columns.size()); ++i) {
			auto& ch_ref = layout_nodes.get_node(node_page_info->columns[i]);
			if(used_space + min_margin + o_select(o, ch_ref.width, ch_ref.height) > max_space || (max_cols != 0 && count >= max_cols)) {
				node_page_info->subpage_divisions.push_back(uint16_t(i));
				max_page_width = std::max(used_space, max_page_width);
				used_space = 0;
				count = 0;
			} 

			used_space += (min_margin + o_select(o, ch_ref.width, ch_ref.height));
			++count;
		}
		max_page_width = std::max(used_space, max_page_width);
		return max_page_width;
	}
	void window_data::immediate_resize(layout_node& node, int32_t new_width, int32_t new_height) {
		if(node.width != uint16_t(new_width) || node.height != uint16_t(new_height)) {
			node.width = uint16_t(new_width);
			node.height = uint16_t(new_height);
			if(node.l_interface && node.layout_deferred == false) {
				recreate_contents(node.l_interface, &node);
			}
		}
	}
	void window_data::recreate_contents(layout_interface* l_interface, layout_node* retvalue) {

		layout_node_type ntype = l_interface->get_node_type();
		retvalue->layout_deferred = false;

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
			case layout_node_type::list_stub:
			{
				recreate_list_contents(l_interface, retvalue);
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

				retvalue->layout_deferred = true;
			} else {

				recreate_contents(l_interface, retvalue);
			}
		}

		return retvalue_id;
	}

	void window_data::recreate_container_contents(layout_interface* l_interface, layout_node* node) {
		if(!node->container_info()) {
			node->contents = std::make_unique<container_information>();
			node->container_info()->list_offset = -1;
		} else {
			node->container_info()->children.clear();
		}
		l_interface->recreate_contents(*this, *node);
	}
	void window_data::recreate_list_contents(layout_interface* l_interface, layout_node* node) {
		
		auto list_layout = l_interface->get_auto_layout_specification(*this);
		auto o = list_layout.orientation;

		if(!node->container_info()) {
			node->contents = std::make_unique<container_information>();
			node->container_info()->list_offset = 0;
		}
		auto ci = node->container_info();
		ci->children.clear();

		int32_t space_used = 0;

		for(int32_t i = ci->list_offset;
			i < list_layout.list_size && space_used < int32_t(o_select(o, node->height, node->width));
			++i) {
			
			if(auto node_interface = l_interface->get_list_item(i); node_interface) {

				auto node_basic_spec = node_interface->get_specification(*this);
				uint16_t temp_unity = 1;
				layout_reference child_ref = create_node(node_interface,
					o_select(o, node->width, temp_unity), o_select(o, temp_unity, node->height), false, true);
				node = &get_node(l_interface->l_id);

				layout_node& cnode = get_node(child_ref);
				cnode.y = 0;
				cnode.x = 0;

				if(o_select(o, cnode.height, cnode.width) + space_used > o_select(o, node->height, node->width)) {
					break;
				}

				immediate_resize(cnode, o_select(o, node->width, cnode.width), o_select(o, cnode.height, node->height));
				
				ci->children.push_back(child_ref);
				o_select(o, cnode.y, cnode.x) = uint16_t(space_used);
				space_used += o_select(o, cnode.height, cnode.width);
			}
		}
	}


	int32_t window_data::get_containing_page_number(layout_reference page_id, page_information& pi, layout_reference c) {
		if(c != layout_reference_none && is_child_of(page_id, c)) {
			uint32_t col_offset = 0;
			for(uint32_t i = 0; i < pi.subpage_divisions.size(); ++i) {
				for(; col_offset < pi.subpage_divisions[i]; ++col_offset) {
					if(is_child_of(pi.columns[col_offset], c)) {
						return int32_t(i);
					}
				}
			}

			for(; col_offset < pi.columns.size(); ++col_offset) {
				if(is_child_of(pi.columns[col_offset], c)) {
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
		if(pi.columns.size() == 0) {
			return layout_reference_none;
		} else if(auto ri = win.get_interactable_render_holder(pi.columns[subpage]); ri) {
			return ri->l_id;
		} else {
			return layout_reference_none;
		}
	}

	void window_data::recreate_page_contents(layout_interface* l_interface, layout_node* node) {
		auto pi = node->page_info();
		if(pi) {

			layout_reference old_focus_column = get_current_page_visibility_marker(*this, l_interface->l_id, *pi);

			pi->columns.clear();
			pi->subpage_divisions.clear();
			l_interface->recreate_contents(*this, *node);

			int pn = get_containing_page_number(l_interface->l_id, *pi, old_focus_column);
			if(pn == -1)
				pn = std::min(int32_t(pi->subpage_offset), int32_t(pi->subpage_divisions.size()));
			l_interface->go_to_page(uint32_t(pn), *pi);
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

	void default_recreate_container(window_data& win, layout_interface* l_interface, layout_node* retvalue, std::vector<layout_interface*> const& children) {

		auto const content_spec = retvalue->l_interface->get_auto_layout_specification(win);
		auto* ci = retvalue->container_info();

		if(children.size() == 0)
			return;

		auto const spec = retvalue->l_interface->get_specification(win);
		auto const lrect = retvalue->l_interface->get_content_rectangle(win);

		auto const o = content_spec.orientation;

		auto const available_horz_space = o_select(o, retvalue->width, retvalue->height) + o_select(o, lrect.width, lrect.height);
		auto const available_vert_space = o_select(o, retvalue->height, retvalue->width) + o_select(o, lrect.height, lrect.width);

		int32_t index_into_children = 0;
		int32_t running_list_position = 0;

		auto cres = make_column(children, win, index_into_children, o, available_vert_space, available_horz_space, 0, running_list_position, true);

		retvalue = &win.get_node(l_interface->l_id);

		if(cres.contents.size() != 0) {
			ci->children.reserve(cres.contents.size());

			retvalue->height = std::max(retvalue->height, uint16_t(o_select(o, cres.along_column_size, cres.accross_column_max)));
			retvalue->width = std::max(retvalue->width, uint16_t(o_select(o, cres.accross_column_max, cres.along_column_size)));
				
			uint16_t group_offset = 0;
			if(content_spec.group_alignment == content_alignment::centered) {
				group_offset = uint16_t((o_select(o, retvalue->height, retvalue->width) - cres.along_column_size) / 2);
			} else if(content_spec.group_alignment == content_alignment::trailing) {
				group_offset = uint16_t(o_select(o, retvalue->height, retvalue->width) - cres.along_column_size);
			}

			for(auto child_result : cres.contents) {
				auto& child = win.get_node(child_result);
				child.parent = l_interface->l_id;
				ci->children.push_back(child_result);

				o_select(o, child.y, child.x) += group_offset;

				switch(content_spec.internal_alignment) {
					case content_alignment::leading: //no change
						break;
					case content_alignment::trailing:
						o_select(o, child.x, child.y) = uint16_t(cres.accross_column_max - o_select(o, child.width, child.height));
						break;
					case content_alignment::centered:
						o_select(o, child.x, child.y) = uint16_t((cres.accross_column_max - o_select(o, child.width, child.height)) / 2);
						break;
					case content_alignment::justified:
						o_select(o, child.x, child.y) = 0ui16;
						win.immediate_resize(child,
							o == content_orientation::page ? cres.accross_column_max : child.width,
							o == content_orientation::page ? child.height : cres.accross_column_max);
						break;
				}
			}

			// fit size to content
			if(spec.line_flags == size_flags::match_content) {
				retvalue->width = uint16_t(o_select(o, cres.accross_column_max, cres.along_column_size));
			}
			if(spec.page_flags == size_flags::match_content) {
				retvalue->height = uint16_t(o_select(o, cres.along_column_size, cres.accross_column_max));
			}
		}
		
	}

	void default_recreate_page(window_data& win, layout_interface* l_interface, layout_node* retvalue, std::vector<layout_interface*> const& children) {

		auto const page_spec = l_interface->get_page_layout_specification(win);
		auto* pi = retvalue->page_info();

		int32_t header_size = 0;
		if(page_spec.header) {
			pi->header = win.create_node(page_spec.header, retvalue->width, retvalue->height, false);
			auto& n = win.get_node(pi->header);
			n.parent = l_interface->l_id;
			header_size = n.height;
			retvalue = &win.get_node(l_interface->l_id);
			retvalue->width = std::max(retvalue->width, n.width);
		}
		int32_t footer_size = 0;
		if(page_spec.footer) {
			pi->footer = win.create_node(page_spec.footer, retvalue->width, retvalue->height, false);
			auto& n = win.get_node(pi->footer);
			n.parent = l_interface->l_id;
			footer_size = n.height;
			retvalue = &win.get_node(l_interface->l_id);
			retvalue->width = std::max(retvalue->width, n.width);
		}

		retvalue->height = std::max(retvalue->height, uint16_t(header_size + footer_size + page_spec.page_leading_margin_min + page_spec.page_trailing_margin_min));

		if(children.size() != 0) {

			auto const spec = l_interface->get_specification(win);

			auto const o = page_spec.orientation;

			formatting_values fdata;

			fdata.across_orientation_lmn = o_select(o, page_spec.line_leading_margin_min, page_spec.page_leading_margin_min);
			fdata.across_orientation_lmx = o_select(o, page_spec.line_leading_margin_max, page_spec.page_leading_margin_max);
			fdata.across_orientation_tmn = o_select(o, page_spec.line_trailing_margin_min, page_spec.page_trailing_margin_min);
			fdata.across_orientation_tmx = o_select(o, page_spec.line_trailing_margin_max, page_spec.page_trailing_margin_max);

			fdata.with_orientation_lmn = o_select(o, page_spec.page_leading_margin_min, page_spec.line_leading_margin_min);
			fdata.with_orientation_lmx = o_select(o, page_spec.page_leading_margin_max, page_spec.line_leading_margin_max);
			fdata.with_orientation_tmn = o_select(o, page_spec.page_trailing_margin_min, page_spec.line_trailing_margin_min);
			fdata.with_orientation_tmx = o_select(o, page_spec.page_trailing_margin_max, page_spec.line_trailing_margin_max);
			fdata.o = o;
			fdata.footer_size = footer_size;
			fdata.header_size = header_size;
			fdata.column_in_page = page_spec.column_in_page_alignment;
			fdata.content_in_column = page_spec.column_content_alignment;
			fdata.column_size = page_spec.column_size_flags;

			o_select(o, retvalue->height, retvalue->width) = uint16_t(std::max(int32_t(o_select(o, retvalue->height, retvalue->width)), fdata.with_orientation_lmn + fdata.with_orientation_tmn));

			//auto const available_horz_space = o_select(o, retvalue->width, retvalue->height) + o_select(o, lrect.width, lrect.height) - (o == content_orientation::page ? 0 : (header_size + footer_size));
			auto const available_vert_space = o_select(o, retvalue->height, retvalue->width) - (o == content_orientation::line ? 0 : (header_size + footer_size));

			int32_t index_into_children = 0;
			int32_t running_list_position = 0;

			bool in_one_column = (o == content_orientation::page && spec.page_flags == size_flags::fill_to_max_single_col)
				|| (o == content_orientation::line && spec.line_flags == size_flags::fill_to_max_single_col);
			while(index_into_children < int32_t(children.size())) {
				auto cres = make_column(children, win, index_into_children, o,
					available_vert_space - (fdata.with_orientation_lmn + fdata.with_orientation_tmn),
					page_spec.min_column_line_size,
					page_spec.max_column_line_size,
					running_list_position, in_one_column);

				retvalue = &win.get_node(l_interface->l_id);

				if(cres.contents.size() != 0) {
					auto column_container_id = win.allocate_node();
					auto& column_container = win.get_node(column_container_id);

					column_container.contents = std::make_unique<container_information>();
					column_container.layout_deferred = false;
					column_container.parent = l_interface->l_id;
					auto column_container_info = column_container.container_info();
					o_select(o, column_container.height, column_container.width) = uint16_t(cres.along_column_size);
					o_select(o, column_container.width, column_container.height) = uint16_t(cres.accross_column_max);

					retvalue = &win.get_node(l_interface->l_id);

					if(in_one_column) {
						o_select(o, retvalue->height, retvalue->width) =
							uint16_t(std::max(int32_t(o_select(o, retvalue->height, retvalue->width)), fdata.with_orientation_lmn + fdata.with_orientation_tmn + o_select(o, column_container.height, column_container.width) ));
					}

					for(auto child_result : cres.contents) {
						auto& child = win.get_node(child_result);
						child.parent = column_container_id;
						column_container_info->children.push_back(child_result);
					}

					retvalue = &win.get_node(l_interface->l_id);

					retvalue->height = std::max(retvalue->height, uint16_t(page_spec.page_leading_margin_min + page_spec.page_trailing_margin_min + column_container.height + (header_size + footer_size)));
					retvalue->width = std::max(retvalue->width, uint16_t(page_spec.line_leading_margin_min + page_spec.line_trailing_margin_max + column_container.width));

					pi->columns.push_back(column_container_id);
				}
			}

			auto page_width_used = format_pages(*retvalue, win, fdata,
				fdata.across_orientation_lmn + fdata.across_orientation_tmn,
				o_select(o, retvalue->width, retvalue->height) - (o == content_orientation::page ? 0 : (header_size + footer_size)), page_spec.max_columns);

			// fit size to content
			if(o == content_orientation::page && spec.line_flags == size_flags::match_content) {
				retvalue->width = uint16_t(std::max(0, page_width_used));
			}
			if(o == content_orientation::line && spec.page_flags == size_flags::match_content) {
				retvalue->height = uint16_t(std::max(0, page_width_used));
			}

			format_columns(*pi, win, retvalue->height, retvalue->width, fdata);

		}

		if(page_spec.header) {
			auto& n = win.get_node(pi->header);
			n.x = 0;
			n.y = 0;
			win.immediate_resize(n, retvalue->width, n.height);
		}
		if(page_spec.footer) {
			auto& n = win.get_node(pi->footer);
			n.x = 0;
			n.y = retvalue->height - n.height;
			win.immediate_resize(n, retvalue->width, n.height);
		}

	}

	void default_relayout_page_columns(window_data& win, layout_interface* l_interface, layout_node* retvalue) {

		auto const page_spec = l_interface->get_page_layout_specification(win);
		auto* pi = retvalue->page_info();
		auto const o = page_spec.orientation;
		auto const lrect = l_interface->get_content_rectangle(win);
		auto const spec = l_interface->get_specification(win);

		layout_reference old_focus_column = get_current_page_visibility_marker(win, l_interface->l_id, *pi);

		pi->subpage_divisions.clear();

		int32_t header_size = 0;
		if(pi->header != layout_reference_none) {
			auto& n = win.get_node(pi->header);
			n.parent = l_interface->l_id;
			header_size = n.height;
		}
		int32_t footer_size = 0;
		if(pi->footer != layout_reference_none) {
			auto& n = win.get_node(pi->footer);
			n.parent = l_interface->l_id;
			footer_size = n.height;
		}

		formatting_values fdata;
		fdata.across_orientation_lmn = o_select(o, page_spec.line_leading_margin_min, page_spec.page_leading_margin_min);
		fdata.across_orientation_lmx = o_select(o, page_spec.line_leading_margin_max, page_spec.page_leading_margin_max);
		fdata.across_orientation_tmn = o_select(o, page_spec.line_trailing_margin_min, page_spec.page_trailing_margin_min);
		fdata.across_orientation_tmx = o_select(o, page_spec.line_trailing_margin_max, page_spec.page_trailing_margin_max);
		fdata.with_orientation_lmn = o_select(o, page_spec.page_leading_margin_min, page_spec.line_leading_margin_min);
		fdata.with_orientation_lmx = o_select(o, page_spec.page_leading_margin_max, page_spec.line_leading_margin_max);
		fdata.with_orientation_tmn = o_select(o, page_spec.page_trailing_margin_min, page_spec.line_trailing_margin_min);
		fdata.with_orientation_tmx = o_select(o, page_spec.page_trailing_margin_max, page_spec.line_trailing_margin_max);
		fdata.o = o;
		fdata.footer_size = footer_size;
		fdata.header_size = header_size;
		fdata.column_in_page = page_spec.column_in_page_alignment;
		fdata.content_in_column = page_spec.column_content_alignment;
		fdata.column_size = page_spec.column_size_flags;

		auto page_width_used = format_pages(*retvalue, win, fdata,
			fdata.across_orientation_lmn + fdata.across_orientation_tmn,
			o_select(o, retvalue->width, retvalue->height) + o_select(o, lrect.width, lrect.height) - (o == content_orientation::page ? 0 : (header_size + footer_size)), page_spec.max_columns);

		// fit size to content
		if(o == content_orientation::page && spec.line_flags == size_flags::match_content) {
			retvalue->width = uint16_t(std::max(0, page_width_used - lrect.width));
		}
		if(o == content_orientation::line && spec.page_flags == size_flags::match_content) {
			retvalue->height = uint16_t(std::max(0, page_width_used - lrect.height));
		}

		format_columns(*pi, win, retvalue->height + lrect.height, retvalue->width + lrect.width, fdata);

		int pn = win.get_containing_page_number(l_interface->l_id, *pi, old_focus_column);
		if(pn == -1)
			pn = std::min(int32_t(pi->subpage_offset), int32_t(pi->subpage_divisions.size()));
		l_interface->go_to_page(uint32_t(pn), *pi);
		if(pi->footer != layout_reference_none) {
			if(auto vrect = win.get_node(pi->footer).visible_rect; vrect < win.get_ui_rects().size()) {
				win.get_ui_rects()[vrect].display_flags |= ui_rectangle::flag_needs_update;
			}
		}
	}

	void window_data::repopulate_ui_rects(layout_reference id, layout_position base, uint8_t parent_foreground, uint8_t parent_background, bool highlight_line, bool skip_bg) {

		layout_node* n = &get_node(id);

		if(n->ignore)
			return;

		if(n->layout_deferred && n->l_interface)
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
			content_rect.x = -base.y;
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
			if(highlight_line)
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

			if(highlight_line)
				candidate.display_flags |= ui_rectangle::flag_line_highlight;
			if(skip_bg && candidate.background_index == parent_background && content_rect.width == 0 && content_rect.height == 0)
				candidate.display_flags |= ui_rectangle::flag_skip_bg;

			uint32_t content_start = 0;
			uint32_t content_end = uint32_t(pi->columns.size());
			if(pi->subpage_offset > 0) {
				content_start = pi->subpage_divisions[pi->subpage_offset - 1];
			}
			if(pi->subpage_offset < pi->subpage_divisions.size()) {
				content_end = pi->subpage_divisions[pi->subpage_offset];
			}
			pi->subpage_offset = std::min(pi->subpage_offset,
				uint16_t(pi->subpage_divisions.size()));

			layout_nodes.get_node(id).visible_rect = ui_reference(prepared_layout.size());
			prepared_layout.push_back(candidate);

			for(uint32_t i = content_start; i < content_end; ++i) {
				auto& cn = layout_nodes.get_node(pi->columns[i]);
				
				repopulate_ui_rects(pi->columns[i],
					layout_position{ int16_t(base.x + content_rect.x + cn.x), int16_t(base.y + content_rect.y + cn.y) },
					candidate.foreground_index, candidate.background_index,
					highlight_line, true);
				n = &layout_nodes.get_node(id);
				
			}

			if(pi->header != layout_reference_none) {
				auto& cn = layout_nodes.get_node(pi->header);
				repopulate_ui_rects(pi->header,
					layout_position{ int16_t(base.x + content_rect.x + cn.x), int16_t(base.y + content_rect.y + cn.y) },
					candidate.foreground_index, candidate.background_index, highlight_line);
				n = &layout_nodes.get_node(id);
			}
			if(pi->footer != layout_reference_none) {
				auto& cn = layout_nodes.get_node(pi->footer);
				repopulate_ui_rects(pi->footer,
					layout_position{ int16_t(base.x + content_rect.x + cn.x), int16_t(base.y + content_rect.y + cn.y) },
					candidate.foreground_index, candidate.background_index, highlight_line);
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

			if(highlight_line)
				candidate.display_flags |= ui_rectangle::flag_line_highlight;
			if(skip_bg && candidate.background_index == parent_background && content_rect.width == 0 && content_rect.height == 0)
				candidate.display_flags |= ui_rectangle::flag_skip_bg;


			candidate.x_position = uint16_t(dest_rect.x);
			candidate.y_position = uint16_t(dest_rect.y);
			candidate.width = uint16_t(dest_rect.width);
			candidate.height = uint16_t(dest_rect.height);

			uint32_t content_start = 0;
			uint32_t content_end = uint32_t(cinfo->children.size());
			
			layout_nodes.get_node(id).visible_rect = ui_reference(prepared_layout.size());
			prepared_layout.push_back(candidate);
			
			bool is_list = cinfo->list_offset != -1;
			content_orientation ldirection = content_orientation::page;
			int32_t litem_height = 1;

			if(is_list && n->l_interface) {
				auto const content_spec = n->l_interface->get_auto_layout_specification(*this);
				const auto spec = n->l_interface->get_specification(*this);
				ldirection = content_spec.orientation;
				litem_height = (content_spec.orientation == content_orientation::page) ? spec.minimum_page_size : spec.minimum_page_size;
			}

			for(uint32_t i = content_start; i < content_end; ++i) {
				auto& cn = layout_nodes.get_node(cinfo->children[i]);
				bool needs_line_highlight = is_list && ((ldirection == content_orientation::page ? base.y + content_rect.y + cn.y : base.x + content_rect.x + cn.x) / litem_height) % 2 == 0;

				repopulate_ui_rects(cinfo->children[i],
					layout_position{ int16_t(base.x + content_rect.x + cn.x), int16_t(base.y + content_rect.y + cn.y) },
					candidate.foreground_index, candidate.background_index,
					needs_line_highlight,
					!needs_line_highlight);
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

			candidate.x_position = uint16_t(dest_rect.x);
			candidate.y_position = uint16_t(dest_rect.y);
			candidate.width = uint16_t(dest_rect.width);
			candidate.height = uint16_t(dest_rect.height);
			if(highlight_line)
				candidate.display_flags |= ui_rectangle::flag_line_highlight;
			if(skip_bg && candidate.background_index == parent_background)
				candidate.display_flags |= ui_rectangle::flag_skip_bg;

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
		redraw_completely_pending = true;
	}

	void window_data::update_generation(layout_reference id) {
		layout_node& node = get_node(id);
		layout_nodes.update_generation(node);
		if(auto pi = node.page_info(); pi) {
			for(auto c : pi->columns)
				update_generation(c);
			if(pi->header != layout_reference_none)
				update_generation(pi->header);
			if(pi->footer != layout_reference_none)
				update_generation(pi->footer);
		} else if(auto ci = node.container_info(); ci) {
			for(auto c : ci->children)
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

			auto work_area = window_interface->get_available_workspace();

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

	void window_data::change_orientation(layout_orientation o) {
		orientation = o;
		layout_out_of_date = true;
		stop_ui_animations();

		++text_data.text_generation;

		init_layout_graphics();
		common_icons.redraw_icons(*this);
		create_interactiable_tags();

		reset_layout();
		get_layout();

		update_window_focus();

		window_bar.print_ui_settings.orientation_list.quiet_select_option_by_value(size_t(o));

		if(minimum_ui_width > ui_width || minimum_ui_height > ui_height) {
			expand_to_fit_content();
		} else {
			window_interface->invalidate_window();
		}
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
		ignore = false;
		layout_deferred = true;
	}
	

	//layout_node_storage::
	void layout_node_storage::reset() {
		last_free = std::numeric_limits<layout_reference>::max();
		node_storage.clear();
	}
	void layout_node_storage::begin_new_generation() {
		current_generation = uint8_t(0x0F & (current_generation + 1));
	}
	void layout_node_storage::release_node(layout_reference id) {
		if((node_storage[id].generation & 0xF0) == 0) { // prevent double frees
			node_storage[id].reset_node();
			node_storage[id].generation = 255ui8;
			node_storage[id].parent = last_free;
			last_free = id;
		}
	}
	void layout_node_storage::update_generation(layout_node& n) {
		n.generation = current_generation;
	}
	void layout_node_storage::garbage_collect() {
		const uint32_t sz = uint32_t(node_storage.size());
		for(uint32_t i = 0; i < sz; ++i) {
			if(node_storage[i].generation < 0x10 && node_storage[i].generation != current_generation) {
				node_storage[i].reset_node();
				node_storage[i].generation = 255ui8;
				node_storage[i].parent = last_free;
				last_free = layout_reference(i);
			}
		}
	}
	layout_reference layout_node_storage::allocate_node() {
		if(last_free == std::numeric_limits<layout_reference>::max()) {
			auto new_id = layout_reference(node_storage.size());
			node_storage.emplace_back();
			node_storage.back().generation = current_generation;
			if(new_id == std::numeric_limits<layout_reference>::max())
				std::abort(); // ERROR used too many layout ids
			return new_id;
		} else {
			auto new_id = last_free;
			last_free = node_storage[last_free].parent;
			node_storage[new_id].parent = std::numeric_limits<layout_reference>::max();
			node_storage[new_id].generation = current_generation;
			return new_id;
		}
	}
	layout_reference layout_node_storage::allocate_node(layout_interface* li) {
		auto li_existing = li->l_id;
		if(li_existing != std::numeric_limits<layout_reference>::max()) {
			node_storage[li_existing].generation = current_generation;
			return li_existing;
		}
		layout_reference new_id;
		if(last_free == std::numeric_limits<layout_reference>::max()) {
			new_id = layout_reference(node_storage.size());
			node_storage.emplace_back();
			node_storage.back().generation = current_generation;
			if(new_id == std::numeric_limits<layout_reference>::max())
				std::abort(); // ERROR used too many layout ids
		} else {
			new_id = last_free;
			last_free = node_storage[last_free].parent;
			node_storage[new_id].parent = std::numeric_limits<layout_reference>::max();
			node_storage[new_id].generation = current_generation;
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
		window_interface->invalidate_window();
	}

	void window_data::set_window_title(std::wstring const& title) {
		if(has_window_title) {
			if(title.length() == 0) {
				window_title.clear();
				has_window_title = false;
				layout_out_of_date = true;
				window_interface->set_window_title(window_title.c_str());
			} else {
				window_title = title;
				window_interface->set_window_title(window_title.c_str());
			}
		} else {
			if(title.length() == 0) {

			} else {
				window_title = title;
				has_window_title = true;
				layout_out_of_date = true;
				window_interface->set_window_title(window_title.c_str());
			}
		}
	}

	bool window_data::is_rendered(layout_reference r) const {
		return get_node(r).visible_rect < prepared_layout.size() && !get_node(r).ignore;
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

}
