/**************************************************************************/
/*  class_view.h                                                          */
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

#ifndef CLASS_VIEW_H
#define CLASS_VIEW_H

#include "../snapshot_data.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/tree.h"
#include "snapshot_view.h"

struct ClassData {
	ClassData() {}
	ClassData(String name, String parent) :
			class_name(name), parent_class_name(parent) {}
	String class_name;
	String parent_class_name;
	HashSet<String> child_classes;
	List<SnapshotDataObject *> instances;
	TreeItem *tree_node;
	HashMap<GameStateSnapshot *, int> recursive_instance_count_cache;
	int instance_count(GameStateSnapshot *snapshot = nullptr) {
		int count = 0;
		for (const SnapshotDataObject *instance : instances) {
			if (!snapshot || instance->snapshot == snapshot) {
				count += 1;
			}
		}
		return count;
	}
	int get_recursive_instance_count(HashMap<String, ClassData> &all_classes, GameStateSnapshot *snapshot = nullptr) {
		if (!recursive_instance_count_cache.has(snapshot)) {
			recursive_instance_count_cache[snapshot] = instance_count(snapshot);
			for (const String &child : child_classes) {
				recursive_instance_count_cache[snapshot] += all_classes[child].get_recursive_instance_count(all_classes, snapshot);
			}
		}
		return recursive_instance_count_cache[snapshot];
	}
};

// Bootstrapped by the plugin
class SnapshotClassView : public SnapshotView {
	GDCLASS(SnapshotClassView, SnapshotView);

protected:
	Tree *class_tree;
	Tree *object_list;
	Tree *diff_object_list;

	void _object_selected(Tree *tree);
	void _class_selected();
	void _add_objects_to_class_map(HashMap<String, ClassData> &class_map, GameStateSnapshot *objects);
	void _notification(int p_what);

	Tree *_make_object_list_tree(const String &column_name);
	void _populate_object_list(GameStateSnapshot *snapshot, Tree *list, const String &name_base);

public:
	SnapshotClassView();
	virtual void show_snapshot(GameStateSnapshot *data, GameStateSnapshot *p_diff_data) override;
};

#endif // CLASS_VIEW_H
