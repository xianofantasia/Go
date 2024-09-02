
/**************************************************************************/
/*  multiplayer_editor_plugin.cpp                                         */
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
#include "../snapshot_data.h"



SnapshotClassView::SnapshotClassView() {
    set_name("Classes");
}

void SnapshotClassView::show_snapshot(GameStateSnapshot* p_data) {
    SnapshotView::show_snapshot(p_data);

    set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    HBoxContainer* classes_view = memnew(HBoxContainer);
    add_child(classes_view);
    classes_view->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
    classes_view->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    classes_view->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    // Tree of classes 
    class_tree = memnew(Tree);
    class_tree->set_custom_minimum_size(Size2(500, 0) * EDSCALE);
    class_tree->set_hide_folding(false);
    classes_view->add_child(class_tree);
    class_tree->set_hide_root(true);
    class_tree->set_columns(2);
    class_tree->set_column_titles_visible(true);
    class_tree->set_column_title(0, "Object Class");
    class_tree->set_column_expand(0, true);
    class_tree->set_column_clip_content(0, false);
    class_tree->set_column_custom_minimum_width(0, 150 * EDSCALE);
    class_tree->set_column_title(1, "Count");
    class_tree->set_column_expand(1, false);
    class_tree->set_column_clip_content(1, false);
    class_tree->set_column_custom_minimum_width(1, 100 * EDSCALE);
    class_tree->connect("cell_selected", callable_mp(this, &SnapshotClassView::_class_selected));
    class_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    class_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    // list of objects within the selected class
    object_list = memnew(Tree);
    object_list->set_custom_minimum_size(Size2(500, 0) * EDSCALE);
    object_list->set_hide_folding(true);
    classes_view->add_child(object_list);
    object_list->set_hide_root(true);
    object_list->set_columns(1);
    object_list->set_column_titles_visible(true);
    object_list->set_column_title(0, "Objects");
    object_list->set_column_expand(0, true);
    object_list->set_column_clip_content(0, false);
    object_list->set_column_custom_minimum_width(0, 150 * EDSCALE);
    object_list->connect("cell_selected", callable_mp(this, &SnapshotClassView::_object_selected));
    // object_list->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    object_list->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    struct ClassData {
        ClassData() {}
        ClassData(String name, String parent): class_name(name), parent_class_name(parent) {}
        String class_name;
        String parent_class_name;
        HashSet<String> child_classes;
        List<SnapshotDataObject*> instances;
        TreeItem* tree_node;
    };
    HashMap<String, ClassData> grouped_by_class;
    grouped_by_class[""] = ClassData("", "");

    for (const KeyValue<ObjectID, SnapshotDataObject*>& pair : snapshot_data->Data) {
        StringName class_name = StringName(pair.value->type_name);
        StringName parent_class_name = class_name != "" ? ClassDB::get_parent_class(class_name) : "";

        String origional_class_name = class_name;
        String origional_parent_class_name = parent_class_name;

        // go up the tree and insert all parents/grandparents
        while (class_name != "") {
            String parent_full_name = parent_class_name != StringName() ? parent_class_name : "";
            if (!grouped_by_class.has(class_name)) {
                grouped_by_class[class_name] = ClassData(class_name, parent_full_name);
            }
            class_name = parent_class_name;
            parent_class_name = class_name != "" ? ClassDB::get_parent_class(class_name) : "";
        }

        if (!grouped_by_class[origional_parent_class_name].child_classes.has(origional_class_name)) {
            grouped_by_class[origional_parent_class_name].child_classes.insert(origional_class_name);
        }		
        grouped_by_class[origional_class_name].instances.push_back(pair.value);
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
        next.tree_node->set_text(0, next_class_name);
        next.tree_node->set_text(1, String::num_int64(next.instances.size()));
        next.tree_node->set_metadata(0, next_class_name);
        for (const String& c : next.child_classes) {
            classes_todo.push_front(c);
        }
    }
}


void SnapshotClassView::_object_selected() {
	ObjectID object_id = object_list->get_selected()->get_metadata(0);
	EditorNode::get_singleton()->push_item((Object*)(snapshot_data->Data[object_id]));
}


void SnapshotClassView::_class_selected() {
	object_list->clear();
	String class_name = class_tree->get_selected()->get_metadata(0);
	TreeItem* root = object_list->create_item();
	for (const KeyValue<ObjectID, SnapshotDataObject*>& pair : snapshot_data->Data) {
		if (pair.value->type_name == class_name) {
			TreeItem* item = object_list->create_item(root);
            item->set_text(0,  pair.value->get_name());
			item->set_metadata(0, pair.value->remote_object_id);
		}
	}
}

RichTextLabel* SnapshotClassView::get_summary_blurb() {
    return nullptr;
}