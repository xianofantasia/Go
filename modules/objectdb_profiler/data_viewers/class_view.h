
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

#ifndef SNAPSHOT_CLASS_VIEW_H
#define SNAPSHOT_CLASS_VIEW_H

#include "scene/gui/tree.h"
#include "../snapshot_data.h"
#include "snapshot_view.h"
#include "scene/gui/menu_button.h"

enum FileSortOption {
	FILE_SORT_NAME_ASCENDING = 0,
	FILE_SORT_NAME_DESCENDING,
	FILE_SORT_COUNT_ASCENDING,
	FILE_SORT_COUNT_DESCENDING,
	FILE_SORT_DIFF_COUNT_ASCENDING,
	FILE_SORT_DIFF_COUNT_DESCENDING,
	FILE_SORT_DELTA_COUNT_ASCENDING,
	FILE_SORT_DELTA_COUNT_DESCENDING,
	FILE_SORT_MAX
};
const FileSortOption DEFAULT_SORT = FILE_SORT_COUNT_DESCENDING;

struct ClassData {
	ClassData() {}
	ClassData(String name, String parent): class_name(name), parent_class_name(parent) {}
	String class_name;
	String parent_class_name;
	HashSet<String> child_classes;
	List<SnapshotDataObject*> instances;
	TreeItem* tree_node;
	HashMap<GameStateSnapshot*, int> recursive_instance_count_cache;
	int instance_count(GameStateSnapshot* snapshot = nullptr) {
		int count = 0;
		for (const SnapshotDataObject* instance : instances) {
			if (!snapshot || instance->snapshot == snapshot) {
				count += 1;
			}
		}
		return count;
	}
	int get_recursive_instance_count(HashMap<String, ClassData>& all_classes, GameStateSnapshot* snapshot = nullptr) {
		if (!recursive_instance_count_cache.has(snapshot)) {
			recursive_instance_count_cache[snapshot] = instance_count(snapshot);
			for (const String& child : child_classes) {
				recursive_instance_count_cache[snapshot] += all_classes[child].get_recursive_instance_count(all_classes, snapshot);
			}
		}
		return recursive_instance_count_cache[snapshot];
	}
};

// Boostrapped by the plugin
class SnapshotClassView : public SnapshotView {
	GDCLASS(SnapshotClassView, SnapshotView);

protected:
	Tree* class_tree;
	Tree* object_list;
	Tree* diff_object_list;

	LineEdit* classes_filter;
	MenuButton* sort_button;

	template<int T_column>
	struct TreeItemAlphaComparator {
		bool operator()(const TreeItem* p_a, const TreeItem* p_b) const {
			return NoCaseComparator()(p_a->get_text(T_column), p_b->get_text(T_column));
		}
	};
	
	template<int T_column>
	struct TreeItemNumericComparator {
		bool operator()(const TreeItem* p_a, const TreeItem* p_b) const {
			return p_a->get_text(T_column).to_int() > p_b->get_text(T_column).to_int();
		}
	};

	void _object_selected(Tree* tree);
	void _class_selected();
	void _filter_changed(const String &p_filter);
	void _update_filter(TreeItem* current_node = nullptr);
	FileSortOption current_sort = DEFAULT_SORT;
	void _file_sort_popup(int p_id);
	void _update_sort();
	void _add_objects_to_class_map(HashMap<String, ClassData>& class_map, GameStateSnapshot* objects);
	void _notification(int p_what);

	Tree* _make_object_list_tree(const String& column_name);
	void _populate_object_list(GameStateSnapshot* snapshot, Tree* list, const String& name_base);

public:
	SnapshotClassView();
	virtual void show_snapshot(GameStateSnapshot* data, GameStateSnapshot* p_diff_data) override;
};



#endif // SNAPSHOT_CLASS_VIEW_H
