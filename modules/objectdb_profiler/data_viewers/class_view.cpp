
/**************************************************************************/
/*  class_view.cpp                                                        */
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

#include "class_view.h"

#include "scene/gui/control.h"
#include "core/object/object.h"
#include "core/os/memory.h"
#include "core/os/time.h"
#include "scene/gui/tree.h"
#include "scene/gui/button.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/tab_container.h"
#include "editor/themes/editor_scale.h"
#include "editor/editor_node.h"
#include "core/object/ref_counted.h"
#include "modules/gdscript/gdscript.h"
#include "scene/gui/split_container.h"
#include "scene/gui/text_edit.h"
#include "../objectdb_profiler_panel.h"
#include "scene/gui/flow_container.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "scene/gui/menu_button.h"
#include "../snapshot_data.h"



SnapshotClassView::SnapshotClassView() {
    set_name("Classes");
}

void SnapshotClassView::show_snapshot(GameStateSnapshot* p_data, GameStateSnapshot* p_diff_data) {
    SnapshotView::show_snapshot(p_data, p_diff_data);

    set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    HSplitContainer* classes_view = memnew(HSplitContainer);
    add_child(classes_view);
    classes_view->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
    classes_view->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_view->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_view->set_split_offset(0);

    VBoxContainer* class_list_column = memnew(VBoxContainer);
    class_list_column->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    class_list_column->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_view->add_child(class_list_column);

    
    HBoxContainer* classes_filter_buttons = memnew(HBoxContainer);
    class_list_column->add_child(classes_filter_buttons);
    classes_filter_buttons->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_filter_buttons->add_theme_constant_override("h_separation", 10 * EDSCALE);
    classes_filter = memnew(LineEdit);
	classes_filter->set_clear_button_enabled(true);
    classes_filter->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_filter->set_right_icon(get_editor_theme_icon(SNAME("Search")));
    classes_filter->set_placeholder("Filter Classes");
    classes_filter_buttons->add_child(classes_filter);
    classes_filter->connect(SceneStringName(text_changed), callable_mp(this, &SnapshotClassView::_filter_changed));


	sort_button = memnew(MenuButton);
    sort_button->set_icon(get_editor_theme_icon(SNAME("Sort")));
	sort_button->set_flat(false);
	sort_button->set_theme_type_variation("FlatMenuButton");
	PopupMenu *p = sort_button->get_popup();
	p->connect(SceneStringName(id_pressed), callable_mp(this, &SnapshotClassView::_file_sort_popup));
	p->add_radio_check_item(TTR("Sort by Name (Ascending)"), FILE_SORT_NAME_ASCENDING);
	p->add_radio_check_item(TTR("Sort by Name (Descending)"), FILE_SORT_NAME_DESCENDING);
    if (!diff_data) {
        p->add_radio_check_item(TTR("Sort by Count (Ascending)"), FILE_SORT_COUNT_ASCENDING);
        p->add_radio_check_item(TTR("Sort by Count (Descending)"), FILE_SORT_COUNT_DESCENDING);
    } else {
        p->add_radio_check_item(TTR("Sort by A Count (Ascending)"), FILE_SORT_COUNT_ASCENDING);
        p->add_radio_check_item(TTR("Sort by A Count (Descending)"), FILE_SORT_DIFF_COUNT_DESCENDING);
        p->add_radio_check_item(TTR("Sort by B Count (Ascending)"), FILE_SORT_DIFF_COUNT_ASCENDING);
        p->add_radio_check_item(TTR("Sort by B Count (Descending)"), FILE_SORT_COUNT_DESCENDING);
        p->add_radio_check_item(TTR("Sort by Delta (Ascending)"), FILE_SORT_DELTA_COUNT_ASCENDING);
        p->add_radio_check_item(TTR("Sort by Delta (Descending)"), FILE_SORT_DELTA_COUNT_DESCENDING);
    }
    
    if (current_sort > p->get_item_count()) {
        current_sort = DEFAULT_SORT;
    }
    
	p->set_item_checked(current_sort, true);
    classes_filter_buttons->add_child(sort_button);
    

    // Tree of classes 
    class_tree = memnew(Tree);
    class_tree->set_custom_minimum_size(Size2(200 * EDSCALE, 0));
    class_tree->set_hide_folding(false);
    class_list_column->add_child(class_tree);
    class_tree->set_hide_root(true);
    class_tree->set_columns(diff_data ? 4 : 2);
    class_tree->set_column_titles_visible(true);
    class_tree->set_column_title(0, "Object Class");
    class_tree->set_column_expand(0, true);
    class_tree->set_column_custom_minimum_width(0, 200 * EDSCALE);
    class_tree->set_column_title(1, diff_data ? "A Count" : "Count");
    class_tree->set_column_expand(1, false);
    if (diff_data) {
        class_tree->set_column_title(2, "B Count");
        class_tree->set_column_expand(2, false);
        class_tree->set_column_title(3, "Delta");
        class_tree->set_column_expand(3, false);
    }
    class_tree->connect("cell_selected", callable_mp(this, &SnapshotClassView::_class_selected));
    class_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    class_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	class_tree->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

    VSplitContainer* object_lists = memnew(VSplitContainer);
    classes_view->add_child(object_lists);
    object_lists->set_custom_minimum_size(Size2(150 * EDSCALE, 0));
    object_lists->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    object_lists->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    if (!diff_data) {
        object_lists->add_child(object_list = _make_object_list_tree("Objects"));
    } else {
        object_lists->add_child(object_list = _make_object_list_tree("A Objects"));
        object_lists->add_child(diff_object_list = _make_object_list_tree("B Objects"));
    }

    HashMap<String, ClassData> grouped_by_class;
    grouped_by_class["Object"] = ClassData("Object", "");
    _add_objects_to_class_map(grouped_by_class, snapshot_data);
    if (diff_data != nullptr) {
        _add_objects_to_class_map(grouped_by_class, diff_data);
    }

    grouped_by_class[""].tree_node = class_tree->create_item();
    ClassData& root = grouped_by_class[""];
    List<String> classes_todo;
    for (const String& c : grouped_by_class[""].child_classes) {
        classes_todo.push_front(c);
    }
    while (classes_todo.size() > 0) {
        String next_class_name = classes_todo.get(0);
        classes_todo.pop_front();
        ClassData& next = grouped_by_class[next_class_name];
        ClassData& nexts_parent = grouped_by_class[next.parent_class_name];
        next.tree_node = class_tree->create_item(nexts_parent.tree_node);

        if (next.class_name != "") {
            next.tree_node->set_icon(0, EditorNode::get_singleton()->get_class_icon(next.class_name, ""));
        }
        next.tree_node->set_text(0, next_class_name + " (" + String::num_int64(next.instance_count(snapshot_data)) + ")");
        int a_count = next.get_recursive_instance_count(grouped_by_class, snapshot_data);
        next.tree_node->set_text(1,  String::num_int64(a_count));
        if (diff_data) {
            int b_count = next.get_recursive_instance_count(grouped_by_class, diff_data);
            next.tree_node->set_text(2, String::num_int64(b_count));
            next.tree_node->set_text(3, String::num_int64(a_count - b_count));
        }
        next.tree_node->set_metadata(0, next_class_name);
        for (const String& c : next.child_classes) {
            classes_todo.push_front(c);
        }
    }

    // Icons won't load until the frame after show_snapshot is called. Not sure why, but just defer the load
    callable_mp(this, &SnapshotClassView::_notification).call_deferred(NOTIFICATION_THEME_CHANGED);

    _update_filter();
    _update_sort();
}

Tree* SnapshotClassView::_make_object_list_tree(const String& column_name) {
    Tree* list = memnew(Tree);
    list->set_hide_folding(true);
    list->set_hide_root(true);
    list->set_columns(1);
    list->set_column_titles_visible(true);
    list->set_column_title(0, column_name);
    list->set_column_expand(0, true);
    list->connect("cell_selected", callable_mp(this, &SnapshotClassView::_object_selected).bind(list));
    list->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    list->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    return list;
}

void SnapshotClassView::_add_objects_to_class_map(HashMap<String, ClassData>& class_map, GameStateSnapshot* objects) {
    for (const KeyValue<ObjectID, SnapshotDataObject*>& pair : objects->Data) {
        StringName class_name = StringName(pair.value->type_name);
        StringName parent_class_name = class_name != "" ? ClassDB::get_parent_class(class_name) : "";

        class_map[class_name].instances.push_back(pair.value);

        // go up the tree and insert all parents/grandparents
        while (class_name != "") {

            // String parent_full_name = parent_class_name != StringName() ? parent_class_name : "";
            if (!class_map.has(class_name)) {
                class_map[class_name] = ClassData(class_name, parent_class_name);
            }
            
            if (!class_map.has(parent_class_name)) {
                // leave our grandparent blank for now. Next iteration of the while loop will fill it in
                class_map[parent_class_name] = ClassData(parent_class_name, "");
            }
            class_map[class_name].parent_class_name = parent_class_name;
            class_map[parent_class_name].child_classes.insert(class_name);
            
            class_name = parent_class_name;
            parent_class_name = class_name != "" ? ClassDB::get_parent_class(class_name) : "";

        }
    }
}

void SnapshotClassView::_object_selected(Tree* tree) {
    GameStateSnapshot* snapshot = snapshot_data;
    if (diff_data) {
        Tree* other = tree == diff_object_list ? object_list : diff_object_list;
        TreeItem* selected = other->get_selected();
        if (selected) {
            selected->deselect(0);
        }
        if (tree == diff_object_list) {
            snapshot = diff_data;
        }
    }
	ObjectID object_id = tree->get_selected()->get_metadata(0);
	EditorNode::get_singleton()->push_item((Object*)(snapshot->Data[object_id]));
}

void SnapshotClassView::_class_selected() {
    if (!diff_data) {
        _populate_object_list(snapshot_data, object_list, "Objects");
    } else {
        _populate_object_list(snapshot_data, object_list, "A Objects");
        _populate_object_list(diff_data, diff_object_list, "B Objects");
    }
}

void SnapshotClassView::_populate_object_list(GameStateSnapshot* snapshot, Tree* list, const String& name_base) {
	list->clear();
	String class_name = class_tree->get_selected()->get_metadata(0);
	TreeItem* root = list->create_item();
    int object_count = 0;
	for (const KeyValue<ObjectID, SnapshotDataObject*>& pair : snapshot->Data) {
		if (pair.value->type_name == class_name) {
			TreeItem* item = list->create_item(root);
            item->set_text(0,  pair.value->get_name());
			item->set_metadata(0, pair.value->remote_object_id);
            item->set_text_overrun_behavior(0, TextServer::OverrunBehavior::OVERRUN_NO_TRIMMING);
            object_count++;
		}
	}
    list->set_column_title(0, name_base + " (" + itos(object_count) + ")");
}

void SnapshotClassView::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_TRANSLATION_CHANGED: {
            // Icon caching, how does that work? (I copied this from editor_profiler.cpp obviously)
            classes_filter->set_right_icon(get_editor_theme_icon(SNAME("Search")));
            sort_button->set_icon(get_editor_theme_icon(SNAME("Sort")));

            // love writing a tree traversal just to update icons...
            List<TreeItem*> items;
            items.push_back(class_tree->get_root());
            while (items.size() > 0) {
                TreeItem* top = items.get(0);
                items.pop_front();
                TypedArray<TreeItem> ti = top->get_children();
                for (int i = 0; i < ti.size(); i++) {
                    Object* obj = ti.get(i);
                    items.push_back((TreeItem*)obj);
                }
                top->set_icon(0, EditorNode::get_singleton()->get_class_icon(top->get_metadata(0), ""));
            }

            _update_filter();
            _update_sort();

		} break;
	}
}

void SnapshotClassView::_filter_changed(const String &p_filter) {
    _update_filter();
}

void SnapshotClassView::_update_filter(TreeItem* current_node) {
    if (!current_node) {
        current_node = class_tree->get_root();
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
        _update_filter(child);
        if (child->is_visible()) {
            any_child_visible = true;
        }
    }

    // check if we match the filter
    String filter_str = classes_filter->get_text().strip_edges(true, true).to_lower();

    // we are visible
    if (current_node->get_text(0).to_lower().contains(filter_str) || filter_str.is_empty()) {
        current_node->set_visible(true);
    // we have a visible child
    } else if (any_child_visible) {
		current_node->set_custom_color(0, get_theme_color(SNAME("font_disabled_color"), EditorStringName(Editor)));

    // we and out children aren't visible
    } else {
        current_node->set_visible(false);
    }
}

void SnapshotClassView::_update_sort() {
    for (int i = 0; i != sort_button->get_popup()->get_item_count(); i++) {
		sort_button->get_popup()->set_item_checked(i, (i == (int)current_sort));
	}

    List<TreeItem*> items_to_sort;
    items_to_sort.push_back(class_tree->get_root());

    while (items_to_sort.size() > 0) {
        TreeItem* to_sort = items_to_sort.get(0);
        items_to_sort.pop_front();
        
        List<TreeItem*> items;
        for (int i = 0; i < to_sort->get_child_count(); i++) {
            items.push_back(to_sort->get_child(i));
        }
        switch(current_sort) {
            case FILE_SORT_NAME_ASCENDING:
                items.sort_custom<TreeItemAlphaComparator<0>>();
                break;
            case FILE_SORT_NAME_DESCENDING:
                items.sort_custom<TreeItemAlphaComparator<0>>();
                items.reverse();
                break;
            case FILE_SORT_COUNT_ASCENDING:
                items.sort_custom<TreeItemNumericComparator<1>>();
                items.reverse();
                break;
            case FILE_SORT_COUNT_DESCENDING:
                items.sort_custom<TreeItemNumericComparator<1>>();
                break;
            case FILE_SORT_DIFF_COUNT_ASCENDING:
                items.sort_custom<TreeItemNumericComparator<2>>();
                items.reverse();
                break;
            case FILE_SORT_DIFF_COUNT_DESCENDING:
                items.sort_custom<TreeItemNumericComparator<2>>();
                break;
            case FILE_SORT_DELTA_COUNT_ASCENDING:
                items.sort_custom<TreeItemNumericComparator<3>>();
                items.reverse();
                break;
            case FILE_SORT_DELTA_COUNT_DESCENDING:
                items.sort_custom<TreeItemNumericComparator<3>>();
                break;
        }

        for (int i = 0; i < items.size(); i++) {
            items.get(i)->move_before(to_sort->get_child(i));
            items_to_sort.push_back(items.get(i));
        }
    }
}


void SnapshotClassView::_file_sort_popup(int p_id) {
    current_sort = (FileSortOption)p_id;
    _update_sort();
}
