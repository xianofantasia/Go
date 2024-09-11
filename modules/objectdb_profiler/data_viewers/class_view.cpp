
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
#include "scene/gui/option_button.h"
#include "../objectdb_profiler_panel.h"
#include "scene/gui/flow_container.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "../snapshot_data.h"



SnapshotClassView::SnapshotClassView() {
    set_name("Classes");
}

void SnapshotClassView::show_snapshot(GameStateSnapshot* p_data) {
    SnapshotView::show_snapshot(p_data);

    set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    VBoxContainer* buttons_and_body = memnew(VBoxContainer);
    buttons_and_body->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    buttons_and_body->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    buttons_and_body->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
    add_child(buttons_and_body);

    FlowContainer* classes_filter_buttons = memnew(FlowContainer);
    buttons_and_body->add_child(classes_filter_buttons);
    classes_filter_buttons->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_filter_buttons->add_theme_constant_override("h_separation", 10 * EDSCALE);
    classes_filter = memnew(LineEdit);
    classes_filter->set_custom_minimum_size(Size2(150 * EDSCALE, 0));
    classes_filter->set_right_icon(get_editor_theme_icon(SNAME("Search")));
    classes_filter->set_placeholder("Filter Classes");
    classes_filter->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_filter_buttons->add_child(classes_filter);
    sort_button = memnew(Button);
    sort_button->set_icon(get_editor_theme_icon(SNAME("Sort")));
    classes_filter_buttons->add_child(sort_button);


    HBoxContainer* diff_button_and_label = memnew(HBoxContainer);
    diff_button_and_label->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    diff_button_and_label->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_filter_buttons->add_child(diff_button_and_label);
    Label* diff_against = memnew(Label("Diff against:"));
    diff_button_and_label->add_child(diff_against);

    OptionButton* diff_option = memnew(OptionButton);   
    diff_option->add_item("none");
    for (const String& name : ObjectDBProfilerPanel::get_singleton()->get_snapshot_names()) {
        diff_option->add_item(ObjectDBProfilerPanel::get_singleton()->snapshot_filename_to_name(name));
    }
    diff_option->connect("item_selected", callable_mp(this, &SnapshotClassView::_apply_diff));
    diff_button_and_label->add_child(diff_option);


    HSplitContainer* classes_view = memnew(HSplitContainer);
    buttons_and_body->add_child(classes_view);
    classes_view->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
    classes_view->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_view->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_view->set_split_offset(0);
    

    // Tree of classes 
    class_tree = memnew(Tree);
    class_tree->set_custom_minimum_size(Size2(200 * EDSCALE, 0));
    class_tree->set_hide_folding(false);
    classes_view->add_child(class_tree);
    class_tree->set_hide_root(true);
    class_tree->set_columns(2);
    class_tree->set_column_titles_visible(true);
    class_tree->set_column_title(0, "Object Class");
    class_tree->set_column_expand(0, true);
    class_tree->set_column_custom_minimum_width(0, 200 * EDSCALE);
    class_tree->set_column_title(1, "Count");
    class_tree->set_column_expand(1, false);
    class_tree->connect("cell_selected", callable_mp(this, &SnapshotClassView::_class_selected));
    class_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    class_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	class_tree->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

    object_list = memnew(Tree);
    object_list->set_custom_minimum_size(Size2(150 * EDSCALE, 0));
    object_list->set_hide_folding(true);
    classes_view->add_child(object_list);
    object_list->set_hide_root(true);
    object_list->set_columns(1);
    object_list->set_column_titles_visible(true);
    object_list->set_column_title(0, "Objects");
    object_list->set_column_expand(0, true);
    object_list->connect("cell_selected", callable_mp(this, &SnapshotClassView::_object_selected));
    object_list->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    object_list->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    struct ClassData {
        ClassData() {}
        ClassData(String name, String parent): class_name(name), parent_class_name(parent) {}
        String class_name;
        String parent_class_name;
        HashSet<String> child_classes;
        List<SnapshotDataObject*> instances;
        TreeItem* tree_node;
        bool recursive_instance_count_cache_dirty = true;
        int recursive_instance_count_cached = 0;
        int get_recursive_instance_count(HashMap<String, ClassData>& data) {
            if (recursive_instance_count_cache_dirty) {
                recursive_instance_count_cached = instances.size();
                for (const String& child : child_classes) {
                    recursive_instance_count_cached += data[child].get_recursive_instance_count(data);
                }
                recursive_instance_count_cache_dirty = false;
            }
            return recursive_instance_count_cached;
        }
    };
    HashMap<String, ClassData> grouped_by_class;
    grouped_by_class["Object"] = ClassData("Object", "");

    for (const KeyValue<ObjectID, SnapshotDataObject*>& pair : snapshot_data->Data) {
        StringName class_name = StringName(pair.value->type_name);
        StringName parent_class_name = class_name != "" ? ClassDB::get_parent_class(class_name) : "";

        grouped_by_class[class_name].instances.push_back(pair.value);

        // go up the tree and insert all parents/grandparents
        while (class_name != "") {

            // String parent_full_name = parent_class_name != StringName() ? parent_class_name : "";
            if (!grouped_by_class.has(class_name)) {
                grouped_by_class[class_name] = ClassData(class_name, parent_class_name);
            }
            
            if (!grouped_by_class.has(parent_class_name)) {
                // leave our grandparent blank for now. Next iteration of the while loop will fill it in
                grouped_by_class[parent_class_name] = ClassData(parent_class_name, "");
            }
            grouped_by_class[class_name].parent_class_name = parent_class_name;
            grouped_by_class[parent_class_name].child_classes.insert(class_name);
            
            class_name = parent_class_name;
            parent_class_name = class_name != "" ? ClassDB::get_parent_class(class_name) : "";

        }
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
        next.tree_node->set_text(0, next_class_name + " (" + String::num_int64(next.instances.size()) + ")");
        next.tree_node->set_text(1,  String::num_int64(next.get_recursive_instance_count(grouped_by_class)));
        next.tree_node->set_metadata(0, next_class_name);
        for (const String& c : next.child_classes) {
            classes_todo.push_front(c);
        }
    }

    // Icons won't load until the frame after show_snapshot is called. Not sure why, but just defer the load
    callable_mp(this, &SnapshotClassView::_notification).call_deferred(NOTIFICATION_THEME_CHANGED);
}

void SnapshotClassView::_object_selected() {
	ObjectID object_id = object_list->get_selected()->get_metadata(0);
	EditorNode::get_singleton()->push_item((Object*)(snapshot_data->Data[object_id]));
}

void SnapshotClassView::_class_selected() {
	object_list->clear();
	String class_name = class_tree->get_selected()->get_metadata(0);
	TreeItem* root = object_list->create_item();
    int object_count = 0;
	for (const KeyValue<ObjectID, SnapshotDataObject*>& pair : snapshot_data->Data) {
		if (pair.value->type_name == class_name) {
			TreeItem* item = object_list->create_item(root);
            item->set_text(0,  pair.value->get_name());
			item->set_metadata(0, pair.value->remote_object_id);
            item->set_text_overrun_behavior(0, TextServer::OverrunBehavior::OVERRUN_NO_TRIMMING);
            object_count++;
		}
	}
    object_list->set_column_title(0, "Objects (" + itos(object_count) + ")");
}

void SnapshotClassView::_apply_diff() {
    print_line("applying some diff");
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

		} break;
	}
}