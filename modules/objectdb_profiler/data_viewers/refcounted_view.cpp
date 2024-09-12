
/**************************************************************************/
/*  refcounted_view.cpp                                                   */
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

#include "refcounted_view.h"

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
#include "scene/resources/style_box_flat.h"
#include "core/object/class_db.h"
#include "scene/gui/split_container.h"

#include "../snapshot_data.h"





SnapshotRefCountedView::SnapshotRefCountedView() {
	set_name("RefCounted");
}

void SnapshotRefCountedView::show_snapshot(GameStateSnapshot* p_data, GameStateSnapshot* p_diff_data) {
    SnapshotView::show_snapshot(p_data, p_diff_data);

    set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    HSplitContainer* objects_view = memnew(HSplitContainer);
    add_child(objects_view);
    objects_view->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

    // Tree of objects
    object_tree = memnew(Tree);
    object_tree->set_custom_minimum_size(Size2(500, 0) * EDSCALE);
    object_tree->set_hide_folding(false);
    objects_view->add_child(object_tree);
    object_tree->set_hide_root(true);
    object_tree->set_columns(4);
    object_tree->set_column_titles_visible(true);
    object_tree->set_column_title(0, "RefCounted Objects");
    object_tree->set_column_expand(0, true);
    object_tree->set_column_clip_content(0, false);
    object_tree->set_column_custom_minimum_width(0, 150 * EDSCALE);

    object_tree->set_column_title(1, "Total Refs");
    object_tree->set_column_expand(1, false);
    object_tree->set_column_clip_content(1, false);
	object_tree->set_column_custom_minimum_width(1, 100 * EDSCALE);

	object_tree->set_column_title(2, "User Refs");
	object_tree->set_column_expand(2, false);
	object_tree->set_column_clip_content(2, false);
	object_tree->set_column_custom_minimum_width(2, 100 * EDSCALE);
    
	object_tree->set_column_title(3, "User Cycles");
	object_tree->set_column_expand(3, false);
	object_tree->set_column_clip_content(3, false);
	object_tree->set_column_custom_minimum_width(3, 100 * EDSCALE);

    object_tree->connect("cell_selected", callable_mp(this, &SnapshotRefCountedView::_object_selected));
    object_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    object_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    // list of objects within the selected class
    object_details = memnew(VBoxContainer);
    object_details->set_custom_minimum_size(Size2(500, 0) * EDSCALE);
    objects_view->add_child(object_details);
    // object_details->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    object_details->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);


	TreeItem* root = object_tree->create_item();
    HashMap<String, TreeItem*> class_groups;

    for (const KeyValue<ObjectID, SnapshotDataObject*>& pair : snapshot_data->Data) {
        if (!pair.value->is_refcounted()) continue;


        TreeItem* item;
        if (!class_groups.has(pair.value->type_name)) {
            class_groups[pair.value->type_name] = object_tree->create_item(root);
            class_groups[pair.value->type_name]->set_text(0, pair.value->type_name);
            class_groups[pair.value->type_name]->set_collapsed(true);
        }
        item = object_tree->create_item(class_groups[pair.value->type_name]);

    	item->set_text(0,  pair.value->type_name + "#" + uitos(pair.value->remote_object_id));
        int ref_count = pair.value->extra_debug_data.has("ref_count") ? (uint64_t)pair.value->extra_debug_data["ref_count"] : 0;

    	item->set_text(1,  String::num_uint64(ref_count));
    	item->set_text(2,  String::num_uint64(pair.value->inbound_references.size()));
		Array ref_cycles = (Array)pair.value->extra_debug_data["ref_cycles"];
    	item->set_text(3,  String::num_uint64(ref_cycles.size())); // compute cycles and attach it to refcounted object
		
		if (ref_count == ref_cycles.size()) {
			// often, references are held by the engine so we can't know if we're stuck in a cycle or not
			// But if the full cycle is visible in the ObjectDB, tell the user
			item->set_custom_bg_color(1, Color(0.6, 0.37, 0.37));
			item->set_custom_bg_color(2, Color(0.6, 0.37, 0.37));
			item->set_custom_bg_color(3, Color(0.6, 0.37, 0.37));
		} else if (pair.value->inbound_references.size() == ref_cycles.size()) {
			item->set_custom_bg_color(2, Color(0.6, 0.37, 0.37));
			item->set_custom_bg_color(3, Color(0.6, 0.37, 0.37));
		}

    	item->set_metadata(0, pair.value->remote_object_id);
    }
}

void SnapshotRefCountedView::_object_selected() {
    for (int i = 0; i < object_details->get_child_count(); i++) {
        object_details->get_child(i)->queue_free();
    }
	ObjectID object_id = object_tree->get_selected()->get_metadata(0);
    if (!snapshot_data->Data.has(object_id)) return; // user clicked on a non-leaf node probably
    SnapshotDataObject* d = snapshot_data->Data[object_id];
    
	EditorNode::get_singleton()->push_item((Object*)d);
    
    PanelContainer* content_wrapper = memnew(PanelContainer);
    content_wrapper->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    content_wrapper->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    StyleBoxFlat* content_wrapper_sbf = memnew(StyleBoxFlat);
    content_wrapper_sbf->set_bg_color(EditorNode::get_singleton()->get_editor_theme()->get_color("dark_color_2", "Editor"));
    content_wrapper->add_theme_style_override("panel", content_wrapper_sbf);
    object_details->add_child(content_wrapper);

    VBoxContainer* vert_content = memnew(VBoxContainer);
    vert_content->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    vert_content->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    content_wrapper->add_child(vert_content);
    vert_content->add_theme_constant_override("separation", 5);

    PanelContainer* title_panel = memnew(PanelContainer);
    StyleBoxFlat* title_sbf = memnew(StyleBoxFlat);
    title_sbf->set_bg_color(EditorNode::get_singleton()->get_editor_theme()->get_color("dark_color_3", "Editor"));
    title_panel->add_theme_style_override("panel", title_sbf);
    vert_content->add_child(title_panel);
    title_panel->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    Label* title = memnew(Label(d->get_name()));
    title_panel->add_child(title);
    title->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
    title->set_vertical_alignment(VerticalAlignment::VERTICAL_ALIGNMENT_CENTER);

    int ref_count = d->extra_debug_data.has("ref_count") ? (uint64_t)d->extra_debug_data["ref_count"] : 0;
    vert_content->add_child(memnew(Label("Total RC: " + String::num_uint64( ref_count ) )));
    
    if (d->inbound_references.size() > 0) {
        RichTextLabel* inbound_lbl = memnew(RichTextLabel( "[center]Inbound References[center]" ));
        inbound_lbl->set_fit_content(true);
        inbound_lbl->set_use_bbcode(true);
        vert_content->add_child(inbound_lbl);
        Tree* inbound_tree = memnew(Tree);
        inbound_tree->set_hide_folding(true);
        vert_content->add_child(inbound_tree);
        inbound_tree->set_hide_root(true);
        inbound_tree->set_columns(2);
        inbound_tree->set_column_titles_visible(true);
        inbound_tree->set_column_title(0, "Source");
        inbound_tree->set_column_expand(0, true);
        inbound_tree->set_column_clip_content(0, false);
        inbound_tree->set_column_title(1, "Property");
        inbound_tree->set_column_expand(1, true);
        inbound_tree->set_column_clip_content(1, true);
        inbound_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
        inbound_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

        TreeItem* root = inbound_tree->create_item();
        for (const KeyValue<String, ObjectID>& ob : d->inbound_references) {
            TreeItem* i = inbound_tree->create_item(root);
            i->set_text(0, snapshot_data->Data[ob.value]->get_name());
            i->set_text(1, ob.key);
        }
    }

	Array ref_cycles = (Array)d->extra_debug_data["ref_cycles"];
    if (ref_cycles.size() > 0) {
        Tree* cycles_tree = memnew(Tree);
        cycles_tree->set_hide_folding(true);
        vert_content->add_child(cycles_tree);
        cycles_tree->set_hide_root(true);
        cycles_tree->set_columns(1);
        cycles_tree->set_column_titles_visible(true);
        cycles_tree->set_column_title(0, "Cycles");
        cycles_tree->set_column_expand(0, true);
        cycles_tree->set_column_clip_content(0, false);
        cycles_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
        cycles_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

        TreeItem* root = cycles_tree->create_item();
        for (const String& cycle : ref_cycles) {
            TreeItem* i = cycles_tree->create_item(root);
            i->set_text(0, cycle);
        }
    }
}
