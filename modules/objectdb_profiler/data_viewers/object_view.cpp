
/**************************************************************************/
/*  object_view.cpp                                                       */
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

#include "object_view.h"

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
#include "scene/gui/rich_text_label.h"
#include "scene/resources/style_box_flat.h"
#include "scene/gui/split_container.h"
#include "../snapshot_data.h"





SnapshotObjectView::SnapshotObjectView() {
	set_name("Objects");
}

void SnapshotObjectView::show_snapshot(GameStateSnapshot* p_data) {
    SnapshotView::show_snapshot(p_data);

    set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    HSplitContainer* objects_view = memnew(HSplitContainer);
    add_child(objects_view);
    objects_view->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

    // Tree of objects
    object_tree = memnew(Tree);
    object_tree->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
    object_tree->set_hide_folding(false);
    objects_view->add_child(object_tree);
    object_tree->set_hide_root(true);
    object_tree->set_columns(3);
    object_tree->set_column_titles_visible(true);
    object_tree->set_column_title(0, "Object Class");
    object_tree->set_column_expand(0, true);
    object_tree->set_column_clip_content(0, false);
    object_tree->set_column_custom_minimum_width(0, 75 * EDSCALE);
    object_tree->set_column_title(1, "Inbound");
    object_tree->set_column_expand(1, false);
    object_tree->set_column_clip_content(1, false);
	object_tree->set_column_custom_minimum_width(1, 75 * EDSCALE);
	object_tree->set_column_title(2, "Outbound");
	object_tree->set_column_expand(2, false);
	object_tree->set_column_clip_content(2, false);
	object_tree->set_column_custom_minimum_width(2, 75 * EDSCALE);
    object_tree->connect("cell_selected", callable_mp(this, &SnapshotObjectView::_object_selected));
    object_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    object_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    // list of objects within the selected class
    object_details = memnew(VBoxContainer);
    object_details->set_custom_minimum_size(Size2(500, 0) * EDSCALE);
    objects_view->add_child(object_details);
    object_details->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    object_details->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);


	TreeItem* root = object_tree->create_item();
    TreeItem* isolated = object_tree->create_item(root);
    isolated->set_text(0,  "Isolated Nodes");
    isolated->set_collapsed(true);

    HashMap<String, TreeItem*> non_isolated;

    for (const KeyValue<ObjectID, SnapshotDataObject*>& pair : snapshot_data->Data) {

    	TreeItem* item;
        if (pair.value->inbound_references.size() == 0 && pair.value->outbound_references.size() == 0) {
            item = object_tree->create_item(isolated);
        } else {
            if (!non_isolated.has(pair.value->type_name)) {
                non_isolated[pair.value->type_name] = object_tree->create_item(root);
                non_isolated[pair.value->type_name]->set_text(0, pair.value->type_name);
                non_isolated[pair.value->type_name]->set_collapsed(true);
            }
            item = object_tree->create_item(non_isolated[pair.value->type_name]);
        }

    	item->set_text(0,  pair.value->get_name());
    	item->set_text(1,  String::num_uint64(pair.value->inbound_references.size()));
    	item->set_text(2,  String::num_uint64(pair.value->outbound_references.size()));
    	item->set_metadata(0, pair.value->remote_object_id);

    }
}


void SnapshotObjectView::_object_selected() {
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

    if (d->outbound_references.size() > 0) {
        RichTextLabel* outbound_lbl = memnew(RichTextLabel( "[center]Outbound References[center]" ));
        outbound_lbl->set_fit_content(true);
        outbound_lbl->set_use_bbcode(true);
        vert_content->add_child(outbound_lbl);
        Tree* outbound_tree = memnew(Tree);
        outbound_tree->set_hide_folding(true);
        vert_content->add_child(outbound_tree);
        outbound_tree->set_hide_root(true);
        outbound_tree->set_columns(2);
        outbound_tree->set_column_titles_visible(true);
        outbound_tree->set_column_title(0, "Property");
        outbound_tree->set_column_expand(0, true);
        outbound_tree->set_column_clip_content(0, false);
        outbound_tree->set_column_title(1, "Target");
        outbound_tree->set_column_expand(1, true);
        outbound_tree->set_column_clip_content(1, true);
        outbound_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
        outbound_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

        TreeItem* root = outbound_tree->create_item();
        for (const KeyValue<String, ObjectID>& ob : d->outbound_references) {
            TreeItem* i = outbound_tree->create_item(root);
            i->set_text(0, ob.key);
            i->set_text(1, snapshot_data->Data[ob.value]->get_name());
        }
    }

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
    

}

