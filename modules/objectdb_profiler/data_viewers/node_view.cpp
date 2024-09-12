
/**************************************************************************/
/*  node_view.cpp                                                         */
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

#include "node_view.h"

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



SnapshotNodeView::SnapshotNodeView() {
    set_name("Nodes");
}

void SnapshotNodeView::show_snapshot(GameStateSnapshot* p_data, GameStateSnapshot* p_diff_data) {
    SnapshotView::show_snapshot(p_data, p_diff_data);

    set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

    // Tree of classes 
    node_tree = memnew(Tree);
    node_tree->set_custom_minimum_size(Size2(500, 0) * EDSCALE);
    node_tree->set_hide_folding(false);
    add_child(node_tree);
    node_tree->set_hide_root(true);
    node_tree->set_columns(1);
    node_tree->set_column_titles_visible(true);
    node_tree->set_column_title(0, "Nodes");
    node_tree->set_column_expand(0, true);
    node_tree->set_column_clip_content(0, false);
    node_tree->set_column_custom_minimum_width(0, 150 * EDSCALE);
    node_tree->connect("cell_selected", callable_mp(this, &SnapshotNodeView::_node_selected));
    node_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    node_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	node_tree->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);


	for (const KeyValue<ObjectID, SnapshotDataObject*>& kv : snapshot_data->Data) {
		// if it's a node AND it doesn't have a parent node
		if (kv.value->is_node() && !kv.value->extra_debug_data.has("node_parent")) {
			TreeItem* root_item = node_tree->create_item();
			root_item->set_text(0, kv.value->extra_debug_data["node_name"]);
			root_item->set_metadata(0, kv.key);
			_add_children_recursive(root_item);
		}
	}
}

void SnapshotNodeView::_add_children_recursive(TreeItem* node) {
	SnapshotDataObject* node_obj = snapshot_data->Data[node->get_metadata(0)];
	for (const Variant& v : (Array)node_obj->extra_debug_data["node_children"]) {
		SnapshotDataObject* child_obj = snapshot_data->Data[ObjectID((uint64_t)v)];
		TreeItem* child_item = node_tree->create_item(node);
		child_item->set_text(0, child_obj->extra_debug_data["node_name"]);
		child_item->set_metadata(0, child_obj->remote_object_id);
		_add_children_recursive(child_item);
	}
}

void SnapshotNodeView::_node_selected() {
	ObjectID object_id = node_tree->get_selected()->get_metadata(0);
	EditorNode::get_singleton()->push_item((Object*)(snapshot_data->Data[object_id]));
}

