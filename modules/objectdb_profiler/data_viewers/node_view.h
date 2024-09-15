

/**************************************************************************/
/*  node_view.h                                                           */
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

#ifndef SNAPSHOT_NODE_VIEW_H
#define SNAPSHOT_NODE_VIEW_H

#include "scene/gui/tree.h"
#include "../snapshot_data.h"
#include "snapshot_view.h"
#include "shared_controls.h"

struct NodeTreeElements {
	Tree* tree;
	TreeSortAndFilterBar* filter_bar;
	VBoxContainer* root;
};

// Boostrapped by the plugin
class SnapshotNodeView : public SnapshotView {
	GDCLASS(SnapshotNodeView, SnapshotView);

protected:
	NodeTreeElements main_tree;
	NodeTreeElements diff_tree;
	Tree* active_tree = nullptr;
	bool combined_diff_view = true;
	
	PopupMenu* choose_object_menu;

	HashMap<TreeItem*, List<SnapshotDataObject*>> tree_item_owners;

	void _node_selected(Tree* tree_selected_from);
	void _notification(int p_what);	
	NodeTreeElements _make_node_tree(const String& tree_name, GameStateSnapshot* snapshot);
	void _apply_filters();
	void _refresh_icons();
	void _toggle_diff_mode(bool state);

	void _choose_object_pressed(int object_idx, bool p_confirm_override);
	void _show_choose_object_menu();

	void _add_snapshot_to_tree(Tree* tree, GameStateSnapshot* snapshot, const String& diff_group_name = "");
	void _add_object_to_tree(TreeItem* parent_item, SnapshotDataObject* data, const String& diff_group_name = "");
	TreeItem* _add_child_named(Tree* tree, TreeItem* item, SnapshotDataObject* item_owner, const String& diff_group_name = "");

	void _add_tree_item_owner(TreeItem* item, SnapshotDataObject* owner);

public:
	SnapshotNodeView();
	virtual void show_snapshot(GameStateSnapshot* data, GameStateSnapshot* p_diff_data) override;

	virtual void clear_snapshot() override;
};


#endif // SNAPSHOT_NODE_VIEW_H
