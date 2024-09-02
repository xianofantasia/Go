
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

#include "snapshot_editor_panel.h"

#include "scene/gui/control.h"
#include "core/object/object.h"
#include "core/os/memory.h"
#include "core/os/time.h"
#include "scene/gui/tree.h"
#include "scene/gui/button.h"
#include "editor/debugger/editor_debugger_node.h"
// #include "core/object/script_language.h"
#include "editor/debugger/script_editor_debugger.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/tab_container.h"
#include "editor/themes/editor_scale.h"
#include "editor/editor_node.h"
#include "core/object/ref_counted.h"
// #include "modules/gdscript/gdscript.h"
#include "snapshot_data.h"
#include "data_viewers/class_view.h"
#include "data_viewers/object_view.h"
#include "data_viewers/refcounted_view.h"
#include "data_viewers/raw_view.h"
#include "data_viewers/node_view.h"
#include "data_viewers/summary_view.h"


void SnapshotEditorPanel::add_snapshot(GameStateSnapshot* snapshot) {
	snapshots[snapshot->name] = snapshot;
	snapshot->snapshot_button = snapshot_list->create_item(snapshot_list->get_root());
	snapshot->snapshot_button->set_text(0, snapshot->name);
	snapshot->snapshot_button->set_metadata(0, snapshot->name);
	snapshot->snapshot_button->move_before(snapshot_list->get_root()->get_first_child());
	snapshot_list->set_selected(snapshot->snapshot_button);
}

void SnapshotEditorPanel::_show_selected_snapshot() {
	show_snapshot(snapshot_list->get_selected()->get_metadata(0));
}

void SnapshotEditorPanel::show_snapshot(const String& snapshot_name) {
	clear_snapshot();
	GameStateSnapshot* snapshot = snapshots[snapshot_name];
	current_snapshot = snapshot;

	for (SnapshotView* view : views) {
		view->show_snapshot(snapshot);
		if (view != summary_view) {
			summary_view->add_blurb(view->get_summary_blurb());
		}
	}
}

void SnapshotEditorPanel::_bind_methods() {
    ADD_SIGNAL(MethodInfo("request_snapshot"));
}


void SnapshotEditorPanel::_request_object_snapshot() {
	emit_signal("request_snapshot");
}

void SnapshotEditorPanel::clear_snapshot() {
    for (SnapshotView* view : views) {
        view->clear_snapshot();
    }
}

void SnapshotEditorPanel::set_enabled(bool enabled) {
	take_snapshot->set_disabled(!enabled);
	clear_snapshot();
	current_snapshot = nullptr;
	snapshot_list->get_root()->clear_children();
	snapshots.clear();
}

SnapshotEditorPanel::SnapshotEditorPanel() {
	set_name("Object Snapshot"); 

	// ===== ROOT CONTAINER =====
	HBoxContainer* root_container = memnew(HBoxContainer);
	root_container->set_anchors_preset(Control::LayoutPreset::PRESET_FULL_RECT);
	root_container->set_v_size_flags(Control::SizeFlags::SIZE_EXPAND_FILL);
	root_container->set_h_size_flags(Control::SizeFlags::SIZE_EXPAND_FILL);
	root_container->add_theme_constant_override("separation", 5);
	add_child(root_container);

	VBoxContainer* snapshot_column = memnew(VBoxContainer);
	snapshot_column->set_v_size_flags(Control::SizeFlags::SIZE_EXPAND_FILL);
	root_container->add_child(snapshot_column);
	
	// snapshot button
	take_snapshot = memnew(Button("Take Object Snapshot"));
	snapshot_column->add_child(take_snapshot);
	take_snapshot->connect(SceneStringName(pressed), callable_mp(this, &SnapshotEditorPanel::_request_object_snapshot));

	snapshot_list = memnew(Tree);
	snapshot_list->create_item();
	snapshot_list->set_custom_minimum_size(Size2(300, 0) * EDSCALE);
    snapshot_list->set_hide_folding(true);
    snapshot_column->add_child(snapshot_list);
    snapshot_list->set_hide_root(true);
    snapshot_list->set_columns(1);
    snapshot_list->set_column_titles_visible(true);
    snapshot_list->set_column_title(0, "Snapshots");
    snapshot_list->set_column_expand(0, true);
    snapshot_list->set_column_clip_content(0, false);
    snapshot_list->set_column_custom_minimum_width(0, 300 * EDSCALE);
    snapshot_list->connect("cell_selected", callable_mp(this, &SnapshotEditorPanel::_show_selected_snapshot));
    snapshot_list->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    snapshot_list->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	snapshot_list->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

	// tabs of various views right for each snapshot
	view_tabs = memnew(TabContainer);
	root_container->add_child(view_tabs);
	view_tabs->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
	view_tabs->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	view_tabs->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

	summary_view = memnew(SnapshotSummaryView);
    add_view(summary_view);
    add_view(memnew(SnapshotClassView));
    add_view(memnew(SnapshotObjectView));
    add_view(memnew(SnapshotNodeView));
    add_view(memnew(SnapshotRefCountedView));
    add_view(memnew(SnapshotRawView));

	set_enabled(false);
}

void SnapshotEditorPanel::add_view(SnapshotView* to_add) {
    views.push_back(to_add);
    view_tabs->add_child(to_add);
}

