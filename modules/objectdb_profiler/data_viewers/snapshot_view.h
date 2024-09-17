/**************************************************************************/
/*  snapshot_view.h                                                       */
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

#ifndef SNAPSHOT_VIEW_H
#define SNAPSHOT_VIEW_H

#include "../snapshot_data.h"
#include "scene/gui/control.h"
#include "scene/gui/rich_text_label.h"

// Bootstrapped by the plugin
class SnapshotView : public Control {
	GDCLASS(SnapshotView, Control);

protected:
	GameStateSnapshot *snapshot_data;
	GameStateSnapshot *diff_data;

	List<TreeItem *> get_children_recursive(Tree *tree) {
		// love writing a tree traversal just to update icons...
		List<TreeItem *> found_items;
		List<TreeItem *> items_to_check;
		if (tree && tree->get_root()) {
			items_to_check.push_back(tree->get_root());
		}
		while (items_to_check.size() > 0) {
			TreeItem *to_check = items_to_check.get(0);
			items_to_check.pop_front();
			found_items.push_back(to_check);
			for (int i = 0; i < to_check->get_child_count(); i++) {
				items_to_check.push_back(to_check->get_child(i));
			}
		}
		return found_items;
	}

public:
	String view_name;
	virtual void show_snapshot(GameStateSnapshot *data, GameStateSnapshot *p_diff_data = nullptr);
	virtual void clear_snapshot();
	bool is_showing_snapshot(GameStateSnapshot *data, GameStateSnapshot *p_diff_data);
};

#endif // SNAPSHOT_VIEW_H
