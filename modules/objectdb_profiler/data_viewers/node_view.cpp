
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
#include "scene/gui/split_container.h"
#include "shared_controls.h"
#include "scene/gui/check_button.cpp"
#include "../snapshot_data.h"


SnapshotNodeView::SnapshotNodeView() {
    set_name("Nodes");
}

void SnapshotNodeView::show_snapshot(GameStateSnapshot* p_data, GameStateSnapshot* p_diff_data) {
    SnapshotView::show_snapshot(p_data, p_diff_data);

    set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

	HSplitContainer* diff_sides = memnew(HSplitContainer);
    diff_sides->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
	add_child(diff_sides);

	bool show_diff_label = diff_data && combined_diff_view;
	main_tree = _make_node_tree(diff_data && !combined_diff_view ? "A Nodes" : "Nodes", snapshot_data);
	diff_sides->add_child(main_tree.root);
	_add_snapshot_to_tree(main_tree.tree, snapshot_data, show_diff_label ? "-" : "");

	if (diff_data) {
		CheckButton* diff_mode_toggle = memnew(CheckButton("Combine Diff"));
		diff_mode_toggle->set_pressed(combined_diff_view);
		diff_mode_toggle->connect("toggled", callable_mp(this, &SnapshotNodeView::_toggle_diff_mode));
		main_tree.filter_bar->add_child(diff_mode_toggle);
		main_tree.filter_bar->move_child(diff_mode_toggle, 0);

		if (combined_diff_view) {
			// merge the snapshots together and add a diff
			_add_snapshot_to_tree(main_tree.tree, diff_data, "+");
		} else {
			// add a second column with the diff snapshot
			diff_tree = _make_node_tree("B Nodes", diff_data);
			diff_sides->add_child(diff_tree.root);
			_add_snapshot_to_tree(diff_tree.tree, diff_data, "");
		}
	}

	_refresh_icons();
	main_tree.filter_bar->apply();
	if (diff_tree.filter_bar) {
		diff_tree.filter_bar->apply();
		diff_sides->set_split_offset(diff_sides->get_size().x * 0.5);
	}

	choose_object_menu = memnew(PopupMenu);
	add_child(choose_object_menu);
	choose_object_menu->connect(SceneStringName(id_pressed), callable_mp(this, &SnapshotNodeView::_choose_object_pressed).bind(false));

}

NodeTreeElements SnapshotNodeView::_make_node_tree(const String& tree_name, GameStateSnapshot* snapshot) {
	NodeTreeElements elements;
	elements.root = memnew(VBoxContainer);
    elements.root->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
    elements.tree = memnew(Tree);
    elements.filter_bar = memnew(TreeSortAndFilterBar(elements.tree, "Filter Nodes"));
    elements.root->add_child(elements.filter_bar);
	elements.tree->set_select_mode(Tree::SelectMode::SELECT_ROW);
    elements.tree->set_custom_minimum_size(Size2(150, 0) * EDSCALE);
    elements.tree->set_hide_folding(false);
    elements.root->add_child(elements.tree);
    elements.tree->set_hide_root(true);
	elements.tree->set_allow_reselect(true);
    elements.tree->set_columns(1);
    elements.tree->set_column_titles_visible(true);
    elements.tree->set_column_title(0, tree_name);
    elements.tree->set_column_expand(0, true);
    elements.tree->set_column_clip_content(0, false);
    elements.tree->set_column_custom_minimum_width(0, 150 * EDSCALE);
    elements.tree->connect("item_selected", callable_mp(this, &SnapshotNodeView::_node_selected).bind(elements.tree));
    elements.tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    elements.tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	elements.tree->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

	elements.tree->create_item();

	return elements;
}

void SnapshotNodeView::_node_selected(Tree* tree_selected_from) {
	active_tree = tree_selected_from;
	if (diff_tree.tree) {
		// deselect nodes in non-active tree, if needed
		if (active_tree == main_tree.tree) diff_tree.tree->deselect_all();
		if (active_tree == diff_tree.tree) main_tree.tree->deselect_all();
	}
	// for now, there is no way to choose which item you view if you're in combined snapshot view

	List<SnapshotDataObject*>& objects = tree_item_owners[tree_selected_from->get_selected()];
	if (objects.size() == 0) return;
	if (objects.size() == 1) {
		EditorNode::get_singleton()->push_item((Object*)(objects.get(0)));
	}
	if (objects.size() == 2) {
		_show_choose_object_menu();
	}	
}

void SnapshotNodeView::_toggle_diff_mode(bool state) {
	combined_diff_view = state;
	show_snapshot(snapshot_data, diff_data); // redraw the whole thing when we toggle views
}

void SnapshotNodeView::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_TRANSLATION_CHANGED: {

			_refresh_icons();
		} break;
	}
}

void SnapshotNodeView::_add_snapshot_to_tree(Tree* tree, GameStateSnapshot* snapshot, const String& diff_group_name) {
	for (const KeyValue<ObjectID, SnapshotDataObject*>& kv : snapshot->Data) {
		if (kv.value->is_node() && !kv.value->extra_debug_data.has("node_parent")) {
			TreeItem* root_item = _add_child_named(tree, tree->get_root(), kv.value, diff_group_name);
			_add_object_to_tree(root_item, kv.value, diff_group_name);
		}
	}
}

void SnapshotNodeView::_add_object_to_tree(TreeItem* parent_item, SnapshotDataObject* data, const String& diff_group_name) {
	for (const Variant& v : (Array)data->extra_debug_data["node_children"]) {
		SnapshotDataObject* child_object = data->snapshot->Data[ObjectID((uint64_t)v)];	
		TreeItem* child_item = _add_child_named(parent_item->get_tree(), parent_item, child_object, diff_group_name);
		_add_object_to_tree(child_item, child_object, diff_group_name);
	}
}

TreeItem* SnapshotNodeView::_add_child_named(Tree* tree, TreeItem* item, SnapshotDataObject* item_owner, const String& diff_group_name) {
	bool has_group = !diff_group_name.is_empty();
	const String& item_name = item_owner->extra_debug_data["node_name"];
	// Find out if this node already exists
	TreeItem* child_item = nullptr;
	if (has_group) {
		for (int idx = 0; idx < item->get_child_count(); idx++) {
			TreeItem* child = item->get_child(idx);
			if (child->get_text(0) == item_name) {
				child_item = child;
				break;
			}
		}
	}
	
	if (child_item) {
		// if it exists, clear out the diff flag because we now know it exists in both trees
		child_item->clear_custom_bg_color(0); // if the item already exists, clear out the delta indicator because it's the same in both trees
	} else {
		if (item_owner->extra_debug_data["node_is_scene_root"]) {
			child_item = tree->get_root() ? tree->get_root() : tree->create_item();
		} else {
			child_item = tree->create_item(item);
		}
		 // item ? tree->create_item(item) : (tree->get_root() ? tree->get_root() : tree->create_item());
		if (has_group) {
			if (diff_group_name == "+") {
				child_item->set_custom_bg_color(0, Color(0, 1, 0, 0.1));
			}
			if (diff_group_name == "-") {
				child_item->set_custom_bg_color(0, Color(1, 0, 0, 0.1));
			}
		}
	}
	
	child_item->set_text(0, item_name);
	_add_tree_item_owner(child_item, item_owner);
	return child_item;
}

void SnapshotNodeView::_add_tree_item_owner(TreeItem* item, SnapshotDataObject* owner) {

	if (!tree_item_owners.has(item)) {
		tree_item_owners.insert(item, List<SnapshotDataObject*>());
	}
	tree_item_owners[item].push_back(owner);
}


void SnapshotNodeView::_refresh_icons() {
	for (TreeItem* item : get_children_recursive(main_tree.tree)) {
		item->set_icon(0, EditorNode::get_singleton()->get_class_icon(tree_item_owners[item].get(0)->type_name, ""));
	}
	if (diff_tree.tree) {
		for (TreeItem* item : get_children_recursive(diff_tree.tree)) {
			item->set_icon(0, EditorNode::get_singleton()->get_class_icon(tree_item_owners[item].get(0)->type_name, ""));
		}
	}
}

void SnapshotNodeView::clear_snapshot() {
	SnapshotView::clear_snapshot();

	tree_item_owners.clear();
	main_tree.tree = nullptr;
	main_tree.filter_bar = nullptr;
	main_tree.root = nullptr;
	diff_tree.tree = nullptr;
	diff_tree.filter_bar = nullptr;
	diff_tree.root = nullptr;
	active_tree = nullptr;
}

void SnapshotNodeView::_choose_object_pressed(int object_idx, bool p_confirm_override) {
	List<SnapshotDataObject*>& objects = tree_item_owners[active_tree->get_selected()];
	EditorNode::get_singleton()->push_item((Object*)(objects.get(object_idx)));
}

void SnapshotNodeView::_show_choose_object_menu() {
	remove_child(choose_object_menu);
	add_child(choose_object_menu);
	choose_object_menu->clear(false);
	choose_object_menu->add_item("Snapshot A", 0);
	choose_object_menu->add_item("Snapshot B", 1);
	choose_object_menu->reset_size();
	choose_object_menu->set_position(get_screen_position() + get_local_mouse_position());
	choose_object_menu->popup();
}