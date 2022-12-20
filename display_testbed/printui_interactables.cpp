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

	void window_data::change_focus(layout_reference old_focus, layout_reference new_focus) {
		if(new_focus == old_focus)
			return;

		auto common_root = find_common_root(new_focus, old_focus);

		for(layout_reference losing = old_focus; losing != layout_reference_none && losing != common_root; losing = layout_nodes.get_node(losing).parent) {
			if(auto i = layout_nodes.get_node(losing).l_interface; i) {
				i->on_lose_focus(*this);
			}
		}

		for(layout_reference gaining = new_focus; gaining != layout_reference_none && gaining != common_root; gaining = layout_nodes.get_node(gaining).parent) {
			if(auto i = layout_nodes.get_node(gaining).l_interface; i) {
				i->on_focus(*this);
			}
		}
	}
	
	layout_reference window_data::get_minimimal_visible(layout_reference leaf) const {
		if(leaf == layout_reference_none)
			return layout_reference_none;

		auto min_parent = get_minimimal_visible(layout_nodes.get_node(leaf).parent);

		if(layout_nodes.get_node(leaf).parent == min_parent && is_rendered(leaf))
			return leaf;

		return min_parent;
	}


	struct visible_children_it {
		std::vector<layout_reference> const& n;
		uint32_t content_pos = 0;

		visible_children_it(std::vector<layout_reference> const& m, uint32_t p) : n(m), content_pos(p) {
		}
		layout_reference operator*() const {
			return n[content_pos];
		}
		bool operator==(visible_children_it const& o) const {
			return content_pos == o.content_pos;
		}
		bool operator!=(visible_children_it const& o) const {
			return content_pos != o.content_pos;
		}
		visible_children_it& operator++() {
			++content_pos;
			return *this;
		}
		visible_children_it& operator--() {
			--content_pos;
			return *this;
		}
	};

	struct visible_children_it_generator {
		layout_node const& n;
		std::vector<layout_reference> const* contents = nullptr;
		visible_children_it_generator(layout_node const& m) : n(m) {
			if(auto p = n.page_info(); p) {
				contents = &p->view_columns();
			} else if(auto c = n.container_info(); c) {
				contents = &c->view_children();
			} else {
				std::abort(); // error
			}
		}
		visible_children_it begin() {
			uint32_t content_start = 0;
			if(auto p = n.page_info(); p) {
				if(p->subpage_offset > 0) {
					content_start = p->subpage_divisions[p->subpage_offset - 1];
				}
			}
			return visible_children_it(*contents, content_start);
		}
		visible_children_it end() {
			if(auto p = n.page_info(); p) {
				uint32_t content_end = uint32_t(p->view_columns().size());
				if(p->subpage_offset < p->subpage_divisions.size()) {
					content_end = p->subpage_divisions[p->subpage_offset];
				}
				return visible_children_it(*contents, content_end);
			} else {
				return visible_children_it(*contents, uint32_t(contents->size()));
			}
		}
	};
	visible_children_it_generator visible_children(layout_node const& n) {
		return visible_children_it_generator(n);
	}

	constexpr int32_t interactable_threshold_count = 3;

	std::optional<int32_t> window_data::interactables_at_node_over_threshold(layout_node const& n, int32_t threshold_count) const {
		if(n.page_info() || n.container_info()) {
			int32_t total = 0;

			for(auto i : visible_children(n)) {
				auto res = interactables_at_node_over_threshold(get_node(i), threshold_count);
				if(!res.has_value())
					return std::optional<int32_t>();
				total += *res;
				if(total > threshold_count)
					return std::optional<int32_t>();
			}
			return total;
		} else if(n.visible_rect < get_ui_rects().size()) {
			if(auto i = get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i) {
				auto icount = i->interactable_count(*this);
				if(icount > threshold_count)
					return std::optional<int32_t>();
				else
					return icount;
			}
		}
		return 0;
	}

	bool has_any_interactables(window_data const& lm, layout_node const& n) {
		if(n.container_info() || n.page_info()) {
			for(auto i : visible_children(n)) {
				if(has_any_interactables(lm, lm.get_node(i)))
					return true;
			}
			return false;
		} else {
			if(auto i = lm.get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i) {
				if(i->interactable_count(lm) > 0)
					return true;
				else
					return false;
			}
		}
		return false;
	}

	int32_t calaculate_top_division_in_page(window_data const& lm, layout_node const& n) {
		int32_t total = 0;
		auto p = n.page_info();

		if(p && p->header != layout_reference_none && !lm.interactables_at_node_over_threshold(lm.get_node(p->header), 0).has_value())
			total += 1;
		if(p && p->footer != layout_reference_none && !lm.interactables_at_node_over_threshold(lm.get_node(p->footer), 0).has_value())
			total += 1;

		// add 1 for footer?
		return 12 - total;
	}

	enum class node_description {
		sub_page, // = a child node that is a page with > interactable_threshold_count contents
		simple_child, // = visible layout node with one interactable
		complex_child // = visible layout node with interactables > 1
	};
	struct sub_item_node {
		node_description desc;
		layout_reference ref;
	};

	enum class special_page_iteration {
		header, body, finished, footer
	};

	struct sub_items_iterator {
		window_data const& lm;
		page_information* pi = nullptr;
		visible_children_it child_it;
		visible_children_it child_it_end;
		special_page_iteration page_it_status = special_page_iteration::header;

		std::vector<std::pair<sub_items_iterator, sub_items_iterator>> sub_iterators;

		void advance_iterator_status() {
			if(page_it_status == special_page_iteration::header) {
				if(child_it.content_pos != child_it_end.content_pos)
					page_it_status = special_page_iteration::body;
				else
					page_it_status =  special_page_iteration::footer;
			} else if(page_it_status == special_page_iteration::body) {
				++child_it;
				if(child_it == child_it_end)
					page_it_status = pi ? special_page_iteration::footer : special_page_iteration::finished;
			} else {
				page_it_status = special_page_iteration::finished;
			}
		}

		bool try_node(layout_reference r) {
			auto& cn = lm.get_node(r);
			if(cn.ignore()) {
				return false;
			} else if(cn.page_info()) {
				std::optional<int32_t> count = lm.interactables_at_node_over_threshold(cn, interactable_threshold_count);
				if(!count.has_value()) {
					return true; // case: subpage
				} else if(*count == 0) {
					return false;
				} else {
					auto vis_range = visible_children(cn);
					sub_items_iterator sub(lm, cn.page_info(), vis_range.begin(), vis_range.end());
					if(sub.child_it != sub.child_it_end) {
						sub_iterators.push_back(std::pair<sub_items_iterator, sub_items_iterator>(sub, sub_items_iterator(lm, sub.child_it_end)));
						return true;
					}
				}
			} else if(cn.container_info()) { // case: subpage
				auto vis_range = visible_children(cn);
				sub_items_iterator sub(lm, nullptr, vis_range.begin(), vis_range.end());
				if(sub.child_it != sub.child_it_end) {
					sub_iterators.push_back(std::pair<sub_items_iterator, sub_items_iterator>(sub, sub_items_iterator(lm, sub.child_it_end)));
					return true;
				}
			} else { // not a page
				if(auto ptr = lm.get_ui_rects()[cn.visible_rect].parent_object.get_render_interface(); ptr) {
					if(ptr->interactable_count(lm) > 0) {
						return true;
					}
				}
			}
			return false;
		}

		sub_items_iterator(window_data const& l, page_information* p, visible_children_it i, visible_children_it e) : lm(l), pi(p), child_it(i), child_it_end(e) {
			// advance to first

			if(!pi) {
				if(child_it.content_pos != child_it_end.content_pos)
					page_it_status = special_page_iteration::body;
				else
					page_it_status = special_page_iteration::finished;
			}

			if(page_it_status == special_page_iteration::header) {
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
			if(page_it_status == special_page_iteration::footer) {
				if(pi->footer != layout_reference_none && try_node(pi->footer)) {
					return;
				}
				advance_iterator_status();
			}
		}
		sub_items_iterator(window_data const& l, visible_children_it e) : lm(l), child_it(e), child_it_end(e), page_it_status(special_page_iteration::finished) {
		}
		bool operator==(sub_items_iterator const& o) const {
			return child_it == o.child_it && page_it_status == o.page_it_status;
		}
		bool operator!=(sub_items_iterator const& o) const {
			return !(*this == o);
		}
		sub_item_node operator*() const {
			if(sub_iterators.empty()) {
				if(page_it_status == special_page_iteration::header) {
					auto& cn = lm.get_node(pi->header);
					if(auto i = lm.get_ui_rects()[cn.visible_rect].parent_object.get_render_interface(); i) {
						auto icount = i->interactable_count(lm);
						if(icount == 0) {
							std::abort(); // error
						} else if(icount == 1) {
							return sub_item_node{ node_description::simple_child, pi->header };
						} else {
							return sub_item_node{ node_description::complex_child, pi->header };
						}
					} else {
						std::abort(); // error
					}
				} else if(page_it_status == special_page_iteration::footer) {
					auto& cn = lm.get_node(pi->footer);
					if(auto i = lm.get_ui_rects()[cn.visible_rect].parent_object.get_render_interface(); i) {
						auto icount = i->interactable_count(lm);
						if(icount == 0) {
							std::abort(); // error
						} else if(icount == 1) {
							return sub_item_node{ node_description::simple_child, pi->footer };
						} else {
							return sub_item_node{ node_description::complex_child, pi->footer };
						}
					} else {
						std::abort(); // error
					}
				}

				auto& cn = lm.get_node(*child_it);
				if(cn.page_info()) {
					return sub_item_node{ node_description::sub_page, *child_it };
				} else {
					if(auto i = lm.get_ui_rects()[cn.visible_rect].parent_object.get_render_interface(); i) {
						auto icount = i->interactable_count(lm);
						if(icount == 0) {
							std::abort(); // error
						} else if(icount == 1) {
							return sub_item_node{ node_description::simple_child, *child_it };
						} else {
							return sub_item_node{ node_description::complex_child, *child_it };
						}
					} else {
						std::abort(); // error
					}
				}
			} else {
				return *(sub_iterators.back().first);
			}
		}
		sub_items_iterator& operator++() {
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
				if(page_it_status == special_page_iteration::footer) {
					if(pi->footer != layout_reference_none && try_node(pi->footer)) {
						return *this;
					}
					advance_iterator_status();
				}
			}
			return *this;
		}
		sub_items_iterator& operator+=(int32_t n) {
			while(n > 0) {
				++(*this);
				--n;
			}
			return *this;
		}
	};

	struct sub_items_iterator_generator {
		window_data const& lm;
		page_information* pi;
		visible_children_it b;
		visible_children_it e;

		sub_items_iterator_generator(window_data const& l, layout_node const& n) : lm(l), pi(n.page_info()), b(visible_children(n).begin()), e(visible_children(n).end()) {
		}
		sub_items_iterator begin() {
			return sub_items_iterator(lm, pi, b, e);
		}
		sub_items_iterator end() {
			return sub_items_iterator(lm, e);
		}
	};

	sub_items_iterator_generator interactables(window_data const& lm, layout_node const& n) {
		return sub_items_iterator_generator(lm, n);
	}

	int32_t calaculate_interactables_at_node(window_data const& lm, layout_node const& n) {
		int32_t total = 0;
		if(n.page_info() || n.container_info()) {
			for(auto i : interactables(lm, n)) {
				++total;
			}
		} else if(n.visible_rect < lm.get_ui_rects().size()) {
			if(auto i = lm.get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i) {
				return i->interactable_count(lm);
			}
		}
		return total;
	}

	// assumes has page_info
	std::vector<grouping_range> group_columns(window_data const& lm, layout_node const& n) {
		std::vector<grouping_range> result;
		result.reserve(12);

		int32_t last_range = 0;

		if(n.page_info() && n.page_info()->header != layout_reference_none) {
			last_range = calaculate_interactables_at_node(lm, lm.get_node(n.page_info()->header));
		}

		for(auto r : visible_children(n)) {
			if(lm.is_rendered(r)) {
				auto& cn = lm.get_node(r);
				
				auto const interactable_in_obj = calaculate_interactables_at_node(lm, cn);
				if(interactable_in_obj > 0)
					result.push_back(grouping_range{ last_range, last_range + interactable_in_obj });
				last_range += interactable_in_obj;
			}
		}

		return result;
	}

	void reduce_groups(std::vector<grouping_range>& groups, size_t target) {
		auto last = groups.size() > 0 ? groups.back().end : 0;

		if(last <= target) {
			groups.resize(1);
			groups[0].start = 0;
			groups[0].end = last;
		} else if(groups.size() > target) {
			groups.resize(target);
			groups.back().end = last;
		}
	}

	std::array<grouping_range, 12> divide_group(grouping_range i, int32_t into) {
		std::array<grouping_range, 12> result = {};

		auto i_size = i.end - i.start;
		auto each = (i_size + into - 1) / into;
		for(int32_t j = 0; j < into; ++j) {
			result[j].start = std::min(i.start + each * j, i.end);
			result[j].end = std::min(i.start + each * (j + 1), i.end);
		}

		return result;
	}

	int32_t make_top_groups(window_data const& lm, layout_node const& n, grouping_range* out) {
		if(n.page_info() || n.container_info()) {
			auto top_col_groups = group_columns(lm, n);
			auto top_divisions_count = calaculate_top_division_in_page(lm, n);
			reduce_groups(top_col_groups, size_t(top_divisions_count));

			if(top_col_groups.size() == 0) {
				return 0;
			}

			auto pi = n.page_info();
			auto header_count = (pi && pi->header != layout_reference_none) ? calaculate_interactables_at_node(lm, lm.get_node(pi->header)) : 0;
			auto footer_count = (pi && pi->footer != layout_reference_none) ? calaculate_interactables_at_node(lm, lm.get_node(pi->footer)) : 0;

			if(top_col_groups.size() == 1) {
				auto groups = divide_group(top_col_groups.back(), top_divisions_count);
				top_col_groups.clear();

				int32_t i = 0;

				if(header_count > 0) {
					out[0] = grouping_range{ 0, header_count };
					++i;
				}

				for(int32_t j = 0; j < top_divisions_count; ++j) {
					if(groups[j].start != groups[j].end) {
						out[i] = grouping_range{ groups[j].start + header_count, groups[j].end + header_count };
						++i;
					}
				}

				if(footer_count > 0) {
					auto old_last = i != 0 ? out[i - 1].end : 0;
					out[i] = grouping_range{ old_last, old_last + footer_count };
					++i;
				}

				return i;
			} else {

				int32_t i = 0;

				if(header_count > 0) {
					out[0] = grouping_range{ 0, header_count };
					++i;
				}

				for(uint32_t j = 0; j < top_col_groups.size(); ++j) {
					if(top_col_groups[j].start != top_col_groups[j].end) {
						out[i] = grouping_range{ top_col_groups[j].start + header_count, top_col_groups[j].end + header_count };
						++i;
					}
				}

				if(footer_count > 0) {
					auto old_last = i != 0 ? out[i - 1].end : 0;
					out[i] = grouping_range{ old_last, old_last + footer_count };
					++i;
				}

				return i;
			}
		} else {
			return 0;
		}
	}

	layout_reference get_focus_stack_top_ref(std::vector<stored_focus> const& vec) {
		if(!vec.empty()) {
			return vec.back().l_interface->l_id;
		}
		return layout_reference_none;
	}
	layout_interface* get_containing_interface(window_data const& lm, layout_reference r) {
		while(r != layout_reference_none) {
			auto& n = lm.get_node(r);
			if(n.l_interface)
				return n.l_interface;
			r = n.parent;
		}
		return nullptr;
	}

	void window_data::update_window_focus() {
		if(window_interface.is_mouse_cursor_visible()) {
			auto new_lref = layout_reference_under_point(*this, get_ui_rects(), last_cursor_x_position, last_cursor_y_position);
			set_window_focus_from_mouse(new_lref);
		} else {
			auto focus_id = get_focus_stack_top_ref(focus_stack);
			if(focus_stack.size() != 0 && (focus_id == layout_reference_none || focus_id != get_minimimal_visible(focus_id))) {
				do {
					focus_stack.pop_back();
					focus_id = get_focus_stack_top_ref(focus_stack);
				} while(focus_stack.size() != 0 && (focus_id == layout_reference_none || focus_id != get_minimimal_visible(focus_id)));

				if(focus_stack.size() == 0) {
					set_window_focus(nullptr);
				} else {
					if(focus_stack.back().child_offset != -1) {
						current_focus_groupings = divide_group(grouping_range{ focus_stack.back().child_offset, focus_stack.back().child_offset_end }, 12);
						current_groupings_size = 12;
					} else {
						current_groupings_size = make_top_groups(*this, get_node(focus_id), current_focus_groupings.data());
					}

					repopulate_key_actions();
					if(prompts != prompt_mode::hidden)
						window_interface.invalidate_window();
				}
			}
		}
	}

	void window_data::set_window_focus(layout_interface* r) {
		auto focus_id = get_focus_stack_top_ref(focus_stack);

		if(!r || r->l_id == layout_reference_none) {
			change_focus(focus_id, layout_reference_none);
			

			if(!focus_stack.empty() && prompts != prompt_mode::hidden)
				window_interface.invalidate_window();

			focus_stack.clear();
			repopulate_key_actions();
			return;
		}
		
		if(r->l_id != focus_id) {
			change_focus(focus_id, r->l_id);
			if(window_interface.is_mouse_cursor_visible()) {
				focus_stack.clear();
				focus_stack.push_back(stored_focus{r, -1, -1});
			} else {
				focus_stack.push_back(stored_focus{ r, -1, -1 });
			}

			current_groupings_size = make_top_groups(*this, get_node(r->l_id), current_focus_groupings.data());
			repopulate_key_actions();

			if(prompts != prompt_mode::hidden)
				window_interface.invalidate_window();
		}
	}

	void window_data::set_window_focus(focus_tracker r) {
		auto focus_id = get_focus_stack_top_ref(focus_stack);

		if(r.node == layout_reference_none) {
			change_focus(focus_id, layout_reference_none);

			if(!focus_stack.empty() && prompts != prompt_mode::hidden)
				window_interface.invalidate_window();

			focus_stack.clear();
			repopulate_key_actions();
			return;
		}
		auto r_layout = get_node(r.node).l_interface;
		if(!r_layout) {
			change_focus(focus_id, layout_reference_none);

			if(!focus_stack.empty() && prompts != prompt_mode::hidden)
				window_interface.invalidate_window();

			focus_stack.clear();
			repopulate_key_actions();
			return;
		}

		if(r.node != focus_id || focus_stack.empty() || r.child_offset != focus_stack.back().child_offset || r.child_offset_end != focus_stack.back().child_offset_end) {

			change_focus(focus_id, r.node);

			if(window_interface.is_mouse_cursor_visible()) {
				focus_stack.clear();
				focus_stack.push_back(stored_focus{ r_layout, r.child_offset, r.child_offset_end });
			} else {
				focus_stack.push_back(stored_focus{ r_layout, r.child_offset, r.child_offset_end });
			}

			if(r.child_offset != -1) {
				current_focus_groupings = divide_group(grouping_range{ r.child_offset, r.child_offset_end }, 12);
				current_groupings_size = 12;
			} else {
				current_groupings_size = make_top_groups(*this, get_node(r.node), current_focus_groupings.data());
			}

			repopulate_key_actions();

			if(prompts != prompt_mode::hidden)
				window_interface.invalidate_window();
		}
	}

	void window_data::back_focus_out_of(layout_interface* l) {
		auto old_focus_id = get_focus_stack_top_ref(focus_stack);
		auto new_focus_id = get_focus_stack_top_ref(focus_stack);

		while(focus_stack.size() > 0) {
			if(new_focus_id != layout_reference_none && focus_stack.back().l_interface != l)
				break;
			focus_stack.pop_back();
			new_focus_id = get_focus_stack_top_ref(focus_stack);
		}

		change_focus(old_focus_id, new_focus_id);

		if(focus_stack.size() != 0) {
			if(focus_stack.back().child_offset != -1) {
				current_focus_groupings = divide_group(grouping_range{ focus_stack.back().child_offset, focus_stack.back().child_offset_end }, 12);
				current_groupings_size = 12;
			} else {
				current_groupings_size = make_top_groups(*this, get_node(new_focus_id), current_focus_groupings.data());
			}

			repopulate_key_actions();

			if(prompts != prompt_mode::hidden)
				window_interface.invalidate_window();
		} else {
			set_window_focus(nullptr);
		}
	}

	void window_data::execute_focus_action(key_action a) {
		if(std::holds_alternative<focus_tracker>(a)) {
			auto i = std::get<focus_tracker>(a);
			set_window_focus(i);
		} else if(std::holds_alternative<interactable>(a)) {
			auto i = std::get<interactable>(a);

			if(pending_right_click) {

				if((window_interface.get_key_state(primary_right_click_modifier_sc) == 0 && window_interface.get_key_state(secondary_right_click_modifier_sc) == 0) || dynamic_settings.imode == input_mode::mouse_only) {
					pending_right_click = false; // right click modifier not held down
					window_bar.info_i.mark_for_update(*this);
				}
				i.ptr->on_right_click(*this, uint32_t(i.child_offset));
			} else {
				i.ptr->on_click(*this, uint32_t(i.child_offset));
			}
		} else if(std::holds_alternative<move_focus_to_node>(a)) {
			auto to_node = std::get<move_focus_to_node>(a).to_node;
			set_window_focus(to_node != layout_reference_none ? get_node(to_node).l_interface : nullptr);
		} else if(std::holds_alternative<go_up>(a)) {
			if(focus_stack.size() > 1) {

				auto old_focus_id = get_focus_stack_top_ref(focus_stack);
				focus_stack.pop_back();
				auto new_focus_id = get_focus_stack_top_ref(focus_stack);

				change_focus(old_focus_id, new_focus_id);

				if(new_focus_id == layout_reference_none) {
					set_window_focus(nullptr);
				} else {
					if(focus_stack.back().child_offset != -1) {
						current_focus_groupings = divide_group(grouping_range{ focus_stack.back().child_offset, focus_stack.back().child_offset_end }, 12);
						current_groupings_size = 12;
					} else {
						current_groupings_size = make_top_groups(*this, get_node(new_focus_id), current_focus_groupings.data());
					}

					repopulate_key_actions();

					if(prompts != prompt_mode::hidden)
						window_interface.invalidate_window();
				}
			} else {
				set_window_focus(nullptr);
			}
		}
	}


	render_interface* window_data::get_interactable_render_holder(layout_reference r) const {
		auto& n = get_node(r);
		if(n.visible_rect < get_ui_rects().size()) {
			if(auto i = get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i && i->interactable_count(*this) != 0) {
				return i;
			}
		} else {
			return nullptr; // there is nothing visible to attach the interactable tag to
		}
		if(auto p = n.page_info(); p) {
			if(p->header != layout_reference_none) {
				auto& hn = get_node(p->header);
				if(hn.visible_rect < get_ui_rects().size()) {
					if(auto i = get_ui_rects()[hn.visible_rect].parent_object.get_render_interface(); i) {
						return i;
					}
				}
			}
			
			if(p->view_columns().size() != 0) {
				return get_interactable_render_holder(p->view_columns().front());
			}
		} else if(auto c = n.container_info(); c) {
			if(c->view_children().size() != 0) {
				return get_interactable_render_holder(c->view_children().front());
			}
		}
		return nullptr;
	}

	std::optional<grouping_range> find_group_containing(window_data const& lm, layout_node const& n, grouping_range i) {
		if(i.start < 0)
			return std::optional<grouping_range>();

		auto total_interactables = calaculate_interactables_at_node(lm, n);
		if(total_interactables <= i.end - i.start)
			return std::optional<grouping_range>();

		std::array<grouping_range, 12> top_col_groups;
		auto top_used = uint32_t(make_top_groups(lm, n, top_col_groups.data()));

		uint32_t match = 0;
		bool is_everything = true;

		if(top_used == 0) {
			return std::optional<grouping_range>();
		} else if(top_used > 1) {
			for(; match < top_used; ++match) {
				if(top_col_groups[match].start <= i.start && top_col_groups[match].end >= i.end &&
					(top_col_groups[match].end - top_col_groups[match].start) > (i.end - i.start)) {
					is_everything = false;
					break;
				}
			}
			if(match == top_used)
				return grouping_range{-1, -1};
		} else {
			//fallthrough: all in one group
		}

		grouping_range test_group = top_col_groups[match];
		while(test_group.end - test_group.start > 0) {
			auto divided = divide_group(test_group, 12);

			uint32_t submatch = 0;
			for(; submatch < 12ui32; ++submatch) {
				if(divided[submatch].start <= i.start && divided[submatch].end >= i.end &&
					(divided[submatch].end - divided[submatch].start) > (i.end - i.start)) {
					is_everything = false;
					break;
				}
			}

			if(submatch == 12ui32) {
				if(is_everything) {
					return grouping_range{ -1, -1 };
				} else {
					return test_group;
				}
			}

			test_group = divided[submatch];
		}
		return std::optional<grouping_range>();
	}

	int32_t populate_key_actions(window_data const& win, layout_node const& n, layout_reference nref, key_action* dest, int32_t start_offset, grouping_range* groups, int32_t max_count) {

		int32_t added = 0;

		if(!n.page_info() && !n.container_info())
			return 0;

		auto rng = interactables(win, n);

		for(auto i = rng.begin(); i != rng.end(); ++i) {
			if(start_offset > 0) {
				--start_offset;
			} else {
				while(groups[added].end == groups[added].start && added < max_count) {
					dest[added] = std::monostate{};
					++added;
				}
				if(added == max_count)
					return max_count;

				if((*i).desc == node_description::sub_page) {
					if(groups[added].end == groups[added].start + 1) {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							dest[added] = move_focus_to_node{ (*i).ref };
							++added;
						}
					} else {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							dest[added] = focus_tracker{ nref, groups[added].start, groups[added].end };

							int32_t count = (groups[added].end - groups[added].start) - 1;
							for(int32_t j = 0; j < count; ++j) {
								++i;
							}
							++added;
						}
					}
				} else if((*i).desc == node_description::complex_child) {
					if(groups[added].end == groups[added].start + 1) {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							dest[added] = move_focus_to_node{ (*i).ref };
							++added;
						}
					} else {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							dest[added] = focus_tracker{ nref, groups[added].start, groups[added].end };

							int32_t count = (groups[added].end - groups[added].start) - 1;
							for(int32_t j = 0; j < count; ++j) {
								++i;
							}
							++added;
						}
					}

				} else if((*i).desc == node_description::simple_child) {
					if(groups[added].end == groups[added].start + 1) {
						if(auto ptr = win.get_interactable_render_holder((*i).ref); ptr) {
							dest[added] = interactable{ ptr, 0 };
							++added;
						}
					} else {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							dest[added] = focus_tracker{ nref, groups[added].start, groups[added].end };

							int32_t count = (groups[added].end - groups[added].start) - 1;
							for(int32_t j = 0; j < count; ++j) {
								++i;
							}
							++added;
						}
					}
				}

				if(added == max_count)
					return max_count;
			}
		}
		return added;

	}

	int32_t populate_interactables(window_data const& win, layout_node const& n, int32_t start_offset, grouping_range* groups, int32_t max_count) {

		int32_t added = 0;

		auto rng = interactables(win, n);
		for(auto i = rng.begin(); i != rng.end(); ++i) {
			if(start_offset > 0) {
				--start_offset;
			} else {
				while(groups[added].end == groups[added].start && added < max_count) {
					++added;
				}
				if(added == max_count)
					return max_count;

				if((*i).desc == node_description::sub_page) {
					if(groups[added].end == groups[added].start + 1) {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state(interactable_state::key, uint8_t(added)));
							++added;
						}
					} else {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state(interactable_state::key, uint8_t(added)));

							int32_t count = (groups[added].end - groups[added].start) - 1;
							for(int32_t j = 0; j < count; ++j) {
								++i;
								if(auto pi2 = win.get_interactable_render_holder((*i).ref); pi2) {
									pi2->set_interactable(-1, interactable_state(interactable_state::group, uint8_t(added)));
								}
							}
							++added;
						}
					}
				} else if((*i).desc == node_description::complex_child) {
					if(groups[added].end == groups[added].start + 1) {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state(interactable_state::key, uint8_t(added)));
							++added;
						}
					} else {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state(interactable_state::key, uint8_t(added)));

							int32_t count = (groups[added].end - groups[added].start) - 1;
							for(int32_t j = 0; j < count; ++j) {
								++i;
								if(auto pi2 = win.get_interactable_render_holder((*i).ref); pi2) {
									pi2->set_interactable(-1, interactable_state(interactable_state::group, uint8_t(added)));
								}
							}
							++added;
						}
					}

				} else if((*i).desc == node_description::simple_child) {
					if(groups[added].end == groups[added].start + 1) {
						if(auto ptr = win.get_interactable_render_holder((*i).ref); ptr) {
							ptr->set_interactable(0, interactable_state(interactable_state::key, uint8_t(added)));
							++added;
						}
					} else {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state(interactable_state::key, uint8_t(added)));

							int32_t count = (groups[added].end - groups[added].start) - 1;
							for(int32_t j = 0; j < count; ++j) {
								++i;
								if(auto pi2 = win.get_interactable_render_holder((*i).ref); pi2) {
									pi2->set_interactable(-1, interactable_state(interactable_state::group, uint8_t(added)));
								}
							}
							++added;
						}
					}
				}

				if(added == max_count)
					return max_count;
			}
		}
		return added;
	}

	void unpopulate_interactables(window_data const& win, layout_node const& n, int32_t start_offset, grouping_range* groups, int32_t max_count) {
		int32_t added = 0;

		auto rng = interactables(win, n);
		for(auto i = rng.begin(); i != rng.end(); ++i) {
			if(start_offset > 0) {
				--start_offset;
			} else {
				while(groups[added].end == groups[added].start && added < max_count) {
					++added;
				}
				if(added == max_count)
					return;

				if((*i).desc == node_description::sub_page) {
					if(groups[added].end == groups[added].start + 1) {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state());
							++added;
						}
					} else {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state());

							int32_t count = (groups[added].end - groups[added].start) - 1;
							for(int32_t j = 0; j < count; ++j) {
								++i;
								if(auto pi2 = win.get_interactable_render_holder((*i).ref); pi2) {
									pi2->set_interactable(-1, interactable_state());
								}
							}
							++added;
						}
					}
				} else if((*i).desc == node_description::complex_child) {
					if(groups[added].end == groups[added].start + 1) {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state());
							++added;
						}
					} else {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state());

							int32_t count = (groups[added].end - groups[added].start) - 1;
							for(int32_t j = 0; j < count; ++j) {
								++i;
								if(auto pi2 = win.get_interactable_render_holder((*i).ref); pi2) {
									pi2->set_interactable(-1, interactable_state());
								}
							}
							++added;
						}
					}

				} else if((*i).desc == node_description::simple_child) {
					if(groups[added].end == groups[added].start + 1) {
						if(auto ptr = win.get_interactable_render_holder((*i).ref); ptr) {
							ptr->set_interactable(0, interactable_state());
							++added;
						}
					} else {
						if(auto pi = win.get_interactable_render_holder((*i).ref); pi) {
							pi->set_interactable(-1, interactable_state());

							int32_t count = (groups[added].end - groups[added].start) - 1;
							for(int32_t j = 0; j < count; ++j) {
								++i;
								if(auto pi2 = win.get_interactable_render_holder((*i).ref); pi2) {
									pi2->set_interactable(-1, interactable_state());
								}
							}
							++added;
						}
					}
				}

				if(added == max_count)
					return;
			}
		}
	}

	int32_t populate_key_actions_from_multi_element(window_data const& lm, layout_node const& n, key_action* dest) {
		if(n.visible_rect < lm.get_ui_rects().size()) {
			if(auto i = lm.get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i) {
				auto icount = i->interactable_count(lm);
				int32_t j = 0;
				for(j = 0; j < icount; ++j) {
					dest[j] = interactable{ i, j };
				}
				return icount;
			}
		}
		return 0;
	}

	int32_t populate_interactables_from_multi_element(window_data const& lm, layout_node const& n) {
		if(n.visible_rect < lm.get_ui_rects().size()) {
			if(auto i = lm.get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i) {
				auto icount = i->interactable_count(lm);
				int32_t j = 0;
				for(j = 0; j < icount; ++j) {
					i->set_interactable(j, interactable_state(interactable_state::key, uint8_t(j)));
				}
				return icount;
			}
		}
		return 0;
	}

	void unpopulate_interactables_from_multi_element(window_data const& lm, layout_node const& n) {
		if(n.visible_rect < lm.get_ui_rects().size()) {
			if(auto i = lm.get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i) {
				auto icount = i->interactable_count(lm);
				int32_t j = 0;
				for(j = 0; j < icount; ++j) {
					i->set_interactable(j, interactable_state());
				}
				return;
			}
		}
	}

	

	focus_tracker window_data::get_parent_group_or_node(focus_tracker r) const {
		if(r.node != layout_reference_none) {
			auto& n = get_node(r.node);
			auto opt_range = find_group_containing(*this, n, grouping_range{ r.child_offset, r.child_offset_end });

			if(!opt_range.has_value()) {
				auto parent = n.parent;
				while(parent != layout_reference_none) {
					auto& p = get_node(parent);
					if(p.page_info()) {
						return focus_tracker{ parent, -1, -1 };
					}
					parent = p.parent;
				}
				return focus_tracker{ layout_reference_none, -1, -1 };
			} else {
				return focus_tracker{ r.node, opt_range->start, opt_range->end };
			}
		} else {
			return r;
		}
	}

	int32_t window_data::get_item_offset(layout_node& n, layout_reference target) const {
		int32_t count = 0;
		for(auto i : interactables(*this, n)) {
			if(i.ref == target) {
				return count;
			}
			++count;
		}
		return -1;
	}

	void window_data::set_window_focus_from_mouse(layout_reference new_lr) {
		auto new_interface = get_containing_interface(*this, new_lr);
		if(new_interface == last_layout_under_cursor)
			return;

		if(new_interface) {
			change_focus(last_layout_under_cursor ? last_layout_under_cursor->l_id : layout_reference_none, new_interface->l_id);
				
			last_layout_under_cursor = new_interface;

			auto containing_page = get_enclosing_node(new_lr);

			if(containing_page == layout_reference_none) {
				if(!focus_stack.empty() && prompts != prompt_mode::hidden)
					window_interface.invalidate_window();

				focus_stack.clear();
				repopulate_key_actions();
				return;
			}

			layout_node& pn = get_node(containing_page);
			int32_t index = get_item_offset(pn, new_lr);

			focus_tracker new_focus;
			if(index == -1) {
				new_focus = focus_tracker{ containing_page, -1, -1 };
			} else {
				new_focus = get_parent_group_or_node(focus_tracker{ containing_page, index, index + 1 });
			}

			if(focus_stack.empty() || focus_stack.back().l_interface != pn.l_interface || focus_stack.back().child_offset != new_focus.child_offset || focus_stack.back().child_offset_end != new_focus.child_offset_end) {

				focus_stack.clear();
				focus_stack.push_back(stored_focus{ pn.l_interface, new_focus.child_offset, new_focus.child_offset_end });

				if(new_focus.child_offset != -1) {
					current_focus_groupings = divide_group(grouping_range{ new_focus.child_offset, new_focus.child_offset_end }, 12);
					current_groupings_size = 12;
				} else {
					current_groupings_size = make_top_groups(*this, get_node(containing_page), current_focus_groupings.data());
				}

				repopulate_key_actions();

				if(prompts != prompt_mode::hidden)
					window_interface.invalidate_window();
			}
		} else {
			change_focus(last_layout_under_cursor ? last_layout_under_cursor->l_id : layout_reference_none, layout_reference_none);

			focus_stack.clear();
			repopulate_key_actions();

			if(prompts != prompt_mode::hidden)
				window_interface.invalidate_window();
		}
	}

	void window_data::remove_interactable_statuses() {
		if(focus_stack.size() != 0 && focus_stack.back().l_interface->l_id != layout_reference_none) {
			auto& n = get_node(focus_stack.back().l_interface->l_id);
			if(n.visible_rect < get_ui_rects().size()) {
				if(auto i = get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i && i->interactable_count(*this) > 0) {
					unpopulate_interactables_from_multi_element(*this, n);
					return;
				}
			}
			unpopulate_interactables(*this, n, focus_stack.back().child_offset, current_focus_groupings.data(), current_groupings_size);
		} else {
			//top level
			if(window_bar.l_id != layout_reference_none) {
				auto& n = get_node(window_bar.l_id);
				unpopulate_interactables(*this, n, 0, current_focus_groupings.data(), current_groupings_size);
			}
			if(get_left_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder( get_left_node()); pi2) {
					pi2->set_interactable(-1, interactable_state());
				}
			}
			if(get_top_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder( get_top_node()); pi2) {
					pi2->set_interactable(-1, interactable_state());
				}
			}
			if(get_bottom_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder( get_bottom_node()); pi2) {
					pi2->set_interactable(-1, interactable_state());
				}
			}
			if(get_right_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder( get_right_node()); pi2) {
					pi2->set_interactable(-1, interactable_state());
				}
			}
		}
	}

	void window_data::repopulate_interactable_statuses() {
		if(focus_stack.size() != 0 && focus_stack.back().l_interface->l_id != layout_reference_none) {
			auto& n = get_node(focus_stack.back().l_interface->l_id);
			if(n.visible_rect < get_ui_rects().size()) {
				if(auto i = get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i && i->interactable_count(*this) > 0) {
					populate_interactables_from_multi_element(*this, n);
					return;
				}
			}

			populate_interactables(*this, n, focus_stack.back().child_offset, current_focus_groupings.data(), current_groupings_size);
		} else {
			//top level
			if(window_bar.l_id != layout_reference_none) {
				auto& n = get_node(window_bar.l_id);

				current_groupings_size = make_top_groups(*this, n, current_focus_groupings.data());

				populate_interactables(*this, n, 0, current_focus_groupings.data(), current_groupings_size);
			}

			if(get_left_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder(get_left_node()); pi2) {
					pi2->set_interactable(-1, interactable_state(interactable_state::key, 8));
				}
			}
			if(get_top_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder(get_top_node()); pi2) {
					pi2->set_interactable(-1, interactable_state(interactable_state::key, 9));
				}
			}
			if(get_bottom_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder(get_bottom_node()); pi2) {
					pi2->set_interactable(-1, interactable_state(interactable_state::key, 10));
				}
			}
			if(get_right_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder(get_right_node()); pi2) {
					pi2->set_interactable(-1, interactable_state(interactable_state::key, 11));
				}
			}

		}
	}

	void window_data::repopulate_key_actions() {
		// populate parent action

		if(focus_stack.size() == 0) {
			focus_actions.escape = move_focus_to_node{ layout_reference_none };
		} else if(focus_stack.size() == 1) {
			auto explicit_parent = get_parent_group_or_node(
				focus_tracker{ focus_stack.back().l_interface->l_id, focus_stack.back().child_offset, focus_stack.back().child_offset_end });

			if(explicit_parent.child_offset == -1) {
				focus_actions.escape = move_focus_to_node{ explicit_parent.node };
			} else {
				focus_actions.escape = explicit_parent;
			}
		} else {
			focus_actions.escape = go_up{};
		}
		

		if(focus_stack.size() != 0 && focus_stack.back().l_interface->l_id != layout_reference_none) {
			auto& n = get_node(focus_stack.back().l_interface->l_id);
			if(n.visible_rect < get_ui_rects().size()) {
				if(auto i = get_ui_rects()[n.visible_rect].parent_object.get_render_interface(); i && i->interactable_count(*this) != 0) {
					focus_actions.valid_key_action_count =
						populate_key_actions_from_multi_element(*this, n, focus_actions.button_actions.data());
					for(int32_t k = focus_actions.valid_key_action_count; k < 12; ++k) {
						focus_actions.button_actions[k] = std::monostate{};
					}
					return;
				}
			}

			focus_actions.valid_key_action_count = populate_key_actions(*this, n, focus_stack.back().l_interface->l_id, focus_actions.button_actions.data(), focus_stack.back().child_offset, current_focus_groupings.data(), current_groupings_size);
			for(int32_t k = focus_actions.valid_key_action_count; k < 12; ++k) {
				focus_actions.button_actions[k] = std::monostate{};
			}
		} else {
			//top level
			if(window_bar.l_id != layout_reference_none) {
				auto& n = get_node(window_bar.l_id);

				current_groupings_size = make_top_groups(*this, n, current_focus_groupings.data());

				focus_actions.valid_key_action_count = populate_key_actions(*this, n, window_bar.l_id, focus_actions.button_actions.data(), 0, current_focus_groupings.data(), current_groupings_size);
			}
			for(int32_t k = focus_actions.valid_key_action_count; k < 12; ++k) {
				focus_actions.button_actions[k] = std::monostate{};
			}
			focus_actions.valid_key_action_count = 12;


			if(get_left_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder(get_left_node()); pi2) {
					focus_actions.button_actions[8] = move_focus_to_node{ get_left_node() };
				}
			}
			if(get_top_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder(get_top_node()); pi2) {
					focus_actions.button_actions[9] = move_focus_to_node{ get_left_node() };
				}
			}
			if(get_bottom_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder(get_bottom_node()); pi2) {
					focus_actions.button_actions[10] = move_focus_to_node{ get_left_node() };
				}
			}
			if(get_right_node() != layout_reference_none) {
				if(auto pi2 = get_interactable_render_holder(get_right_node()); pi2) {
					focus_actions.button_actions[11] = move_focus_to_node{ get_left_node() };
				}
			}
		}
	}
}