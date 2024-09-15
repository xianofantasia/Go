
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
#include "shared_controls.h"

#include "../snapshot_data.h"





SnapshotRefCountedView::SnapshotRefCountedView() {
	set_name("RefCounted");
}

void SnapshotRefCountedView::show_snapshot(GameStateSnapshot* p_data, GameStateSnapshot* p_diff_data) {
    SnapshotView::show_snapshot(p_data, p_diff_data);

    item_data_map.clear();
    data_item_map.clear();

    set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    HSplitContainer* refs_view = memnew(HSplitContainer);
    add_child(refs_view);
    refs_view->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
    
    VBoxContainer* refs_column = memnew(VBoxContainer);
    refs_column->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
    refs_view->add_child(refs_column);

    // Tree of objects
    refs_list = memnew(Tree);

    filter_bar = memnew(TreeSortAndFilterBar(refs_list, "Filter RefCounteds"));
    refs_column->add_child(filter_bar);
    int offset = diff_data ? 1 : 0;
    if (diff_data) {
        filter_bar->add_sort_option("Snapshot", TreeSortAndFilterBar::SortType::ALPHA_SORT, 0);
    }
    filter_bar->add_sort_option("Class", TreeSortAndFilterBar::SortType::ALPHA_SORT, offset + 0);
    filter_bar->add_sort_option("Name", TreeSortAndFilterBar::SortType::ALPHA_SORT, offset + 1);
    TreeSortAndFilterBar::SortOptionIndexes default_sort = filter_bar->add_sort_option(
        "Total Refs", 
        TreeSortAndFilterBar::SortType::NUMERIC_SORT, 
        offset + 2
    );
    filter_bar->add_sort_option("User Refs", TreeSortAndFilterBar::SortType::NUMERIC_SORT, offset + 3);
    filter_bar->add_sort_option("User Cycles", TreeSortAndFilterBar::SortType::NUMERIC_SORT, offset + 4);


    refs_list->set_select_mode(Tree::SelectMode::SELECT_ROW);
    refs_list->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
    refs_list->set_hide_folding(false);
    refs_column->add_child(refs_list);
    refs_list->set_hide_root(true);
    refs_list->set_columns(diff_data ? 6 : 5);
    refs_list->set_column_titles_visible(true);
    if (diff_data) {
        refs_list->set_column_title(0, "Snapshot");
        refs_list->set_column_expand(0, false);
    }
    refs_list->set_column_title(offset + 0, "Class");
    refs_list->set_column_expand(offset + 0, true);
    refs_list->set_column_title(offset + 1, "Name");
    refs_list->set_column_expand(offset + 1, true);
    refs_list->set_column_expand_ratio(offset + 1, 2);
	refs_list->set_column_title(offset + 2, "Total Refs");
	refs_list->set_column_expand(offset + 2, false);
	refs_list->set_column_title(offset + 3, "User Refs");
	refs_list->set_column_expand(offset + 3, false);
	refs_list->set_column_title(offset + 4, "User Cycles");
	refs_list->set_column_expand(offset + 4, false);
    refs_list->connect("item_selected", callable_mp(this, &SnapshotRefCountedView::_refcounted_selected));
    refs_list->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    refs_list->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

    // View of the selected refcounted
    ref_details = memnew(VBoxContainer);
    ref_details->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
    refs_view->add_child(ref_details);
    ref_details->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    ref_details->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);


	refs_list->create_item();
    _insert_data(snapshot_data, "A");
    if (diff_data) {
        _insert_data(diff_data, "B");
    }

    refs_view->set_split_offset(INFINITY);
    filter_bar->select_sort(default_sort.descending); 
	filter_bar->apply();
    refs_list->set_selected(refs_list->get_root()->get_first_child());
}

void SnapshotRefCountedView::_insert_data(GameStateSnapshot* snapshot, const String& name) {
    for (const KeyValue<ObjectID, SnapshotDataObject*>& pair : snapshot->Data) {
        if (!pair.value->is_refcounted()) continue;
        
        TreeItem* item = refs_list->create_item(refs_list->get_root());
        item_data_map[item] = pair.value;
        data_item_map[pair.value] = item;
        int ref_count = pair.value->extra_debug_data.has("ref_count") ? (uint64_t)pair.value->extra_debug_data["ref_count"] : 0;
		Array ref_cycles = (Array)pair.value->extra_debug_data["ref_cycles"];

        int offset = 0;
        if (diff_data) {
            item->set_text(0, name);
            offset = 1;
        }

    	item->set_text(offset + 0,  pair.value->type_name);
    	item->set_text(offset + 1,  pair.value->get_name());
    	item->set_text(offset + 2,  String::num_uint64(ref_count));
    	item->set_text(offset + 3,  String::num_uint64(pair.value->inbound_references.size()));
    	item->set_text(offset + 4,  String::num_uint64(ref_cycles.size())); // compute cycles and attach it to refcounted object
        
		if (ref_count == ref_cycles.size()) {
			// often, references are held by the engine so we can't know if we're stuck in a cycle or not
			// But if the full cycle is visible in the ObjectDB, tell the user
			item->set_custom_bg_color(offset + 2, Color(1, 0, 0, 0.1));
			item->set_custom_bg_color(offset + 4, Color(1, 0, 0, 0.1));
		}
    }
}

void SnapshotRefCountedView::_refcounted_selected() {
    for (int i = 0; i < ref_details->get_child_count(); i++) {
        ref_details->get_child(i)->queue_free();
    }

    SnapshotDataObject* d = item_data_map[refs_list->get_selected()];
	EditorNode::get_singleton()->push_item((Object*)d);
    
    DarkPanelContainer* content_wrapper = memnew(DarkPanelContainer);
    ref_details->add_child(content_wrapper);

    VBoxContainer* vert_content = memnew(VBoxContainer);
    vert_content->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    vert_content->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    content_wrapper->add_child(vert_content);
    vert_content->add_theme_constant_override("separation", 5);

    PanelContainer* title_panel = memnew(SpanningHeader(d->get_name()));
    vert_content->add_child(title_panel);

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
        inbound_tree->set_select_mode(Tree::SelectMode::SELECT_ROW);
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
        inbound_tree->connect("item_selected", callable_mp(this, &SnapshotRefCountedView::_ref_selected).bind(inbound_tree));

        TreeItem* root = inbound_tree->create_item();
        for (const KeyValue<String, ObjectID>& ob : d->inbound_references) {
            TreeItem* i = inbound_tree->create_item(root);
            SnapshotDataObject* target = d->snapshot->Data[ob.value];
            i->set_text(0, target->get_name());
            i->set_text(1, ob.key);
            reference_item_map[i] = data_item_map[target];
        }
    }

	Array ref_cycles = (Array)d->extra_debug_data["ref_cycles"];
    if (ref_cycles.size() > 0) {
        vert_content->add_child(memnew(SpanningHeader("Cycles")));
        Tree* cycles_tree = memnew(Tree);
        cycles_tree->set_hide_folding(true);
        vert_content->add_child(cycles_tree);
        cycles_tree->set_select_mode(Tree::SelectMode::SELECT_ROW);
        cycles_tree->set_hide_root(true);
        cycles_tree->set_columns(1);
        cycles_tree->set_column_titles_visible(false);
        cycles_tree->set_column_expand(0, true);
        cycles_tree->set_column_clip_content(0, false);
        cycles_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
        cycles_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

        TreeItem* root = cycles_tree->create_item();
        for (const String& cycle : ref_cycles) {
            TreeItem* i = cycles_tree->create_item(root);
            i->set_text(0, cycle);
            i->set_text_overrun_behavior(0, TextServer::OverrunBehavior::OVERRUN_NO_TRIMMING);
            // i->set_autowrap_mode(0,TextServer::AutowrapMode::AUTOWRAP_WORD);
        }
    }
}

void SnapshotRefCountedView::_ref_selected(Tree* source_tree) {
    TreeItem* target = reference_item_map[source_tree->get_selected()];
    if (target) {
        if (!target->is_visible()) {
            // clear the filter if we can't see the node we just chose
            filter_bar->clear_filter();
        }
        target->get_tree()->deselect_all();
        target->get_tree()->set_selected(target);
        target->get_tree()->ensure_cursor_is_visible();
    }
}