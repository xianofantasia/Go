

/**************************************************************************/
/*  shared_controls.h                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef SHARED_CONTROLS_H
#define SHARED_CONTROLS_H

#include "scene/gui/tree.h"
#include "scene/gui/center_container.h"
#include "scene/gui/margin_container.h"
#include "../snapshot_data.h"
#include "scene/gui/panel_container.h"
#include "editor/editor_node.h"
#include "scene/resources/style_box_flat.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "snapshot_view.h"


class SpanningHeader : public PanelContainer {
    GDCLASS(SpanningHeader, PanelContainer);

public:
    SpanningHeader(const String& text) {
        StyleBoxFlat* title_sbf = memnew(StyleBoxFlat);
        title_sbf->set_bg_color(EditorNode::get_singleton()->get_editor_theme()->get_color("dark_color_3", "Editor"));
        add_theme_style_override("panel", title_sbf);
        set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
        Label* title = memnew(Label(text));
        add_child(title);
        title->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
        title->set_vertical_alignment(VerticalAlignment::VERTICAL_ALIGNMENT_CENTER);
    }
};

class DarkPanelContainer : public PanelContainer {
    GDCLASS(DarkPanelContainer, PanelContainer);

public:
    DarkPanelContainer() {
        set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
        set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
        StyleBoxFlat* content_wrapper_sbf = memnew(StyleBoxFlat);
        content_wrapper_sbf->set_bg_color(EditorNode::get_singleton()->get_editor_theme()->get_color("dark_color_2", "Editor"));
        add_theme_style_override("panel", content_wrapper_sbf);
    }
};



class TreeSortAndFilterBar : public HBoxContainer {
    GDCLASS(TreeSortAndFilterBar, HBoxContainer);

    LineEdit* filter_edit;
    MenuButton* sort_button;
    Tree* managed_tree;

public:
    enum SortType {
        NUMERIC_SORT = 0,
        ALPHA_SORT,
        SORT_TYPE_MAX
    };

    struct SortOptionIndexes {
        int ascending;
        int descending;
    };

private:
    struct SortItem {
        SortItem(){}
        SortItem(int p_id, String p_label, SortType p_type, bool p_ascending, int p_column):
            id(p_id), label(p_label), type(p_type), ascending(p_ascending), column(p_column) {}
        int id;
        String label;
        SortType type;
        bool ascending;
        int column;
    };

    struct TreeItemColumn {
        TreeItemColumn() {}
        TreeItemColumn(TreeItem* p_item, int p_column) : item(p_item), column(p_column) {}
        TreeItem* item;
        int column;
    };

    HashMap<int, SortItem> sort_items;
    int current_sort = 0;
    
	struct TreeItemAlphaComparator {
		bool operator()(const TreeItemColumn p_a, const TreeItemColumn p_b) const {
			return NoCaseComparator()(p_a.item->get_text(p_a.column), p_b.item->get_text(p_b.column));
		}
	};
	
	struct TreeItemNumericComparator {
		bool operator()(const TreeItemColumn p_a, const TreeItemColumn p_b) const {
			return p_a.item->get_text(p_a.column).to_int() < p_b.item->get_text(p_b.column).to_int();
		}
	};
    
    void _apply_filter(TreeItem* current_node = nullptr) {
        if (!current_node) {
            current_node = managed_tree->get_root();
        }

        if (!current_node) {
            return;
        }

        // reset ourself to default state
        current_node->set_visible(true);
        current_node->clear_custom_color(0);

        // go through each child and filter them
        bool any_child_visible = false;
        for (TreeItem *child = current_node->get_first_child(); child; child = child->get_next()) {
            _apply_filter(child);
            if (child->is_visible()) {
                any_child_visible = true;
            }
        }

        // check if we match the filter
        String filter_str = filter_edit->get_text().strip_edges(true, true).to_lower();

        // we are visible
        bool matches_filter = false;
        for (int i = 0; i < managed_tree->get_columns(); i++) {
            if (current_node->get_text(i).to_lower().contains(filter_str)) {
                matches_filter = true;
                break;
            }
        }
        if (matches_filter || filter_str.is_empty()) {
            current_node->set_visible(true);
        // we have a visible child
        } else if (any_child_visible) {
            current_node->set_custom_color(0, get_theme_color(SNAME("font_disabled_color"), EditorStringName(Editor)));

        // we and out children aren't visible
        } else {
            current_node->set_visible(false);
        }
    }

    void _apply_sort() {
        if (!sort_button->is_visible()) {
            return;
        }
        for (int i = 0; i != sort_button->get_popup()->get_item_count(); i++) {
            // update the popup buttons to be checked/unchecked
            sort_button->get_popup()->set_item_checked(i, (i == (int)current_sort));
        }

        SortItem sort = sort_items[current_sort];

        List<TreeItem*> items_to_sort;
        items_to_sort.push_back(managed_tree->get_root());


        while (items_to_sort.size() > 0) {
            TreeItem* to_sort = items_to_sort.get(0);
            items_to_sort.pop_front();
            
            List<TreeItemColumn> items;
            for (int i = 0; i < to_sort->get_child_count(); i++) {
                items.push_back(TreeItemColumn(to_sort->get_child(i), sort.column));
            }

            if (sort.type == ALPHA_SORT && sort.ascending == true) {
                items.sort_custom<TreeItemAlphaComparator>();
            }
            if (sort.type == ALPHA_SORT && sort.ascending == false) {
                items.sort_custom<TreeItemAlphaComparator>();
                items.reverse();
            }
            if (sort.type == NUMERIC_SORT && sort.ascending == true) {
                items.sort_custom<TreeItemNumericComparator>();
            }
            if (sort.type == NUMERIC_SORT && sort.ascending == false) {
                items.sort_custom<TreeItemNumericComparator>();
                items.reverse();
            }

            for (int i = 0; i < items.size(); i++) {
                items.get(i).item->move_before(to_sort->get_child(i));
                items_to_sort.push_back(items.get(i).item);
            }
        }
    }

    void _sort_changed(int id) {
        current_sort = id;
        _apply_sort();
    }

    void _filter_changed(const String& filter) {
        _apply_filter();
    }

public:
    TreeSortAndFilterBar(Tree* p_managed_tree, const String& filter_placeholder_text = "Filter"): managed_tree(p_managed_tree) {
        set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
        add_theme_constant_override("h_separation", 10 * EDSCALE);
        filter_edit = memnew(LineEdit);
        filter_edit->set_clear_button_enabled(true);
        filter_edit->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
        filter_edit->set_placeholder("Filter Classes");
        add_child(filter_edit);
        filter_edit->connect(SceneStringName(text_changed), callable_mp(this, &TreeSortAndFilterBar::_filter_changed));

        sort_button = memnew(MenuButton);
        sort_button->set_visible(false);
        sort_button->set_flat(false);
        sort_button->set_theme_type_variation("FlatMenuButton");
        PopupMenu *p = sort_button->get_popup();
        p->connect(SceneStringName(id_pressed), callable_mp(this, &TreeSortAndFilterBar::_sort_changed));
        
        add_child(sort_button);
        
    }

    void _notification(int p_what) {
        switch (p_what) {
            case NOTIFICATION_POSTINITIALIZE:
            case NOTIFICATION_ENTER_TREE:
            case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
            case NOTIFICATION_THEME_CHANGED:
            case NOTIFICATION_TRANSLATION_CHANGED: {
                filter_edit->set_right_icon(get_editor_theme_icon(SNAME("Search")));
                sort_button->set_icon(get_editor_theme_icon(SNAME("Sort")));
                    
                apply();

            } break;
        }
	}

    SortOptionIndexes add_sort_option(const String& new_option, SortType sort_type, int sort_column, bool is_default = false) {
        sort_button->set_visible(true);
        bool is_first_item = sort_items.size() == 0;
        SortItem item_ascending(sort_items.size(), "Sort By " + new_option + " (Ascending)", sort_type, true, sort_column);
        sort_items[item_ascending.id] = item_ascending;
        sort_button->get_popup()->add_radio_check_item(item_ascending.label, item_ascending.id);

        SortItem item_descending(sort_items.size(), "Sort By " + new_option + " (Descending)", sort_type, false, sort_column);
        sort_items[item_descending.id] = item_descending;
        sort_button->get_popup()->add_radio_check_item(item_descending.label, item_descending.id);

        if (is_first_item) {
              sort_button->get_popup()->set_item_checked(0, true);
        }

        SortOptionIndexes indexes;
        indexes.ascending = item_ascending.id;
        indexes.descending = item_descending.id;
        return indexes;
    }

    void clear_filter() {
        filter_edit->clear();
    }
    
    void clear() {
        sort_button->set_visible(false);
        sort_button->get_popup()->clear();
        filter_edit->clear();
    }

    void select_sort(int item_id) {
        _sort_changed(item_id);
    }

    void apply() {
        if (!managed_tree || !managed_tree->get_root()) return;
        _apply_sort();
        _apply_filter();
    } 
};


#endif // SHARED_CONTROLS_H

