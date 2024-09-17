/**************************************************************************/
/*  objectdb_profiler_panel.cpp                                           */
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

#include "objectdb_profiler_panel.h"

#include "core/config/project_settings.h"
#include "core/core_bind.h"
#include "core/error/error_macros.h"
#include "core/io/dir_access.h"
#include "core/io/json.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "core/os/time.h"
#include "data_viewers/class_view.h"
#include "data_viewers/json_view.h"
#include "data_viewers/node_view.h"
#include "data_viewers/object_view.h"
#include "data_viewers/refcounted_view.h"
#include "data_viewers/summary_view.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "snapshot_data.h"

enum RC_MENU_OPERATIONS {
	RENAME,
	SHOW_IN_FOLDER,
	DELETE,
};

ObjectDBProfilerPanel *ObjectDBProfilerPanel::singleton = nullptr;

void ObjectDBProfilerPanel::receive_snapshot(const Array &p_data) {
	String snapshot_file_name = Time::get_singleton()->get_datetime_string_from_unix_time(Time::get_singleton()->get_unix_time_from_system()).replace("T", "_").replace(":", "-");
	Ref<DirAccess> snapshot_dir = _get_and_create_snapshot_storage_dir();
	if (!snapshot_dir.is_null()) {
		Error err;
		String current_dir = snapshot_dir->get_current_dir();
		String joined_dir = current_dir.path_join(snapshot_file_name) + ".odb_snapshot";

		Ref<FileAccess> file = FileAccess::open(joined_dir, FileAccess::WRITE, &err);
		if (err == OK) {
			// IMPORTANT: This is a version number for the .odb_snapshot file format. If you change the format of the .odb_snapshot file, increment this number.
			// Currently, the file is a version number on line 1 and serialized snapshot data on line 2.
			file->store_line("1");
			String data_str = core_bind::Marshalls::get_singleton()->variant_to_base64(p_data, true);
			file->store_string(data_str);
			file->close(); // RAII could do this typically, but we want to read the file in _show_selected_snapshot, so we have to finalize the write before that

			_add_snapshot_button(snapshot_file_name, joined_dir);
			snapshot_list->deselect_all();
			snapshot_list->set_selected(snapshot_list->get_root()->get_first_child());
			snapshot_list->ensure_cursor_is_visible();
			_show_selected_snapshot();
		} else {
			ERR_PRINT("Could not persist ObjectDB Snapshot: " + String(error_names[err]));
		}
	}
}

Ref<DirAccess> ObjectDBProfilerPanel::_get_and_create_snapshot_storage_dir() {
	String profiles_dir = "user://";
	Ref<DirAccess> da = DirAccess::open(profiles_dir);
	if (da.is_null()) {
		ERR_PRINT("Could not open user:// directory: " + profiles_dir);
		return nullptr;
	}
	Error err = da->change_dir("objectdb_snapshots");
	if (err != OK) {
		err = da->make_dir("objectdb_snapshots");
	}
	if (err != OK) {
		ERR_PRINT("Could not create ObjectDB Snapshots directory: " + da->get_current_dir());
		return nullptr;
	}
	return da;
}

TreeItem *ObjectDBProfilerPanel::_add_snapshot_button(const String &snapshot_file_name, const String &full_file_path) {
	TreeItem *item = snapshot_list->create_item(snapshot_list->get_root());
	item->set_text(0, snapshot_file_name);
	item->set_metadata(0, full_file_path);
	item->move_before(snapshot_list->get_root()->get_first_child());
	_update_diff_items();
	return item;
}

void ObjectDBProfilerPanel::_show_selected_snapshot() {
	show_snapshot(snapshot_list->get_selected()->get_text(0), diff_options[diff_button->get_selected_id()]);
}

Ref<GameStateSnapshotRef> ObjectDBProfilerPanel::get_snapshot(const String &snapshot_file_name) {
	if (snapshot_cache.has(snapshot_file_name)) {
		return snapshot_cache.get(snapshot_file_name);
	} else {
		Ref<DirAccess> snapshot_dir = _get_and_create_snapshot_storage_dir();
		if (snapshot_dir.is_null()) {
			ERR_PRINT("Could not access ObjectDB Snapshot directory");
			return nullptr;
		}

		String full_file_path = snapshot_dir->get_current_dir().path_join(snapshot_file_name) + ".odb_snapshot";

		Error err;
		Ref<FileAccess> snapshot_file = FileAccess::open(full_file_path, FileAccess::READ, &err);
		if (err != OK) {
			ERR_PRINT("Could not open ObjectDB Snapshot file: " + full_file_path);
			return nullptr;
		}

		String content = snapshot_file->get_as_text(true); // we wnat to split on newlines, so normalize them
		if (content == "") {
			ERR_PRINT("ObjectDB Snapshot file is empty: " + full_file_path);
			return nullptr;
		}

		// Simplest file versioning scheme known to man. First line is a version number, the rest may be anything, based on the version number.
		Vector<String> parts = content.split("\n", true, 1);
		if (parts.size() != 2) {
			ERR_PRINT("ObjectDB Snapshot file did not have at least two lines: " + full_file_path);
			return nullptr;
		}
		if (!parts[0].is_valid_int()) {
			ERR_PRINT("ObjectDB Snapshot file first line is not a version number, File: " + full_file_path + ". First Line: " + parts[0]);
			return nullptr;
		}

		int version = parts[0].to_int();
		Ref<GameStateSnapshotRef> snapshot = GameStateSnapshot::create_ref(snapshot_file_name, version, parts[1]);
		if (snapshot != nullptr) {
			// don't cache a null snapshot
			snapshot_cache.insert(snapshot_file_name, snapshot);
		}
		return snapshot;
	}
}

void ObjectDBProfilerPanel::show_snapshot(const String &snapshot_file_name, const String &snapshot_diff_file_name) {
	clear_snapshot();

	current_snapshot = get_snapshot(snapshot_file_name);
	if (snapshot_diff_file_name != "none") {
		diff_snapshot = get_snapshot(snapshot_diff_file_name);
	} else {
		diff_snapshot = nullptr;
	}

	_view_tab_changed(view_tabs->get_current_tab());
}

void ObjectDBProfilerPanel::_view_tab_changed(int tab_idx) {
	// populating tabs only on tab changed because we're handling a lot of data, and the editor freezes for while if we don't
	SnapshotView *view = cast_to<SnapshotView>(view_tabs->get_current_tab_control());
	GameStateSnapshot *snapshot = current_snapshot->ptr();
	GameStateSnapshot *diff = diff_snapshot == nullptr ? nullptr : diff_snapshot->ptr();
	if (!view->is_showing_snapshot(snapshot, diff)) {
		view->show_snapshot(snapshot, diff);
	}
}

void ObjectDBProfilerPanel::_bind_methods() {
	ADD_SIGNAL(MethodInfo("request_snapshot"));
}

void ObjectDBProfilerPanel::_request_object_snapshot() {
	emit_signal("request_snapshot");
}

void ObjectDBProfilerPanel::clear_snapshot() {
	for (SnapshotView *view : views) {
		view->clear_snapshot();
	}
	current_snapshot = nullptr;
}

void ObjectDBProfilerPanel::set_enabled(bool enabled) {
	take_snapshot->set_disabled(!enabled);
}

void ObjectDBProfilerPanel::_snapshot_rmb(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT)
		return;
	rmb_menu->clear(false);

	rmb_menu->add_icon_item(get_editor_theme_icon(SNAME("Rename")), "Rename Snapshot", RC_MENU_OPERATIONS::RENAME);
	rmb_menu->add_icon_item(get_editor_theme_icon(SNAME("Folder")), "Show In Folder", RC_MENU_OPERATIONS::SHOW_IN_FOLDER);
	rmb_menu->add_icon_item(get_editor_theme_icon(SNAME("Remove")), "Delete Snapshot", RC_MENU_OPERATIONS::DELETE);

	rmb_menu->reset_size();
	rmb_menu->set_position(get_screen_position() + p_pos);
	rmb_menu->popup();
}

void ObjectDBProfilerPanel::_rmb_menu_pressed(int p_tool, bool p_confirm_override) {
	String file_path = snapshot_list->get_selected()->get_metadata(0);
	String global_path = ProjectSettings::get_singleton()->globalize_path(file_path);
	switch (rmb_menu->get_item_id(p_tool)) {
		case RC_MENU_OPERATIONS::SHOW_IN_FOLDER: {
			OS::get_singleton()->shell_show_in_file_manager(global_path, true);
			break;
		}
		case RC_MENU_OPERATIONS::DELETE: {
			DirAccess::remove_file_or_error(global_path);
			snapshot_list->get_root()->remove_child(snapshot_list->get_selected());
			snapshot_list->set_selected(snapshot_list->get_root()->get_first_child());
			_update_diff_items();
			break;
		}
		case RC_MENU_OPERATIONS::RENAME: {
			snapshot_list->edit_selected(true);
			break;
		}
	}
}

void ObjectDBProfilerPanel::_edit_snapshot_name() {
	const String &new_snapshot_name = snapshot_list->get_selected()->get_text(0);
	const String &full_file_with_path = snapshot_list->get_selected()->get_metadata(0);
	Vector<String> full_path_parts = full_file_with_path.rsplit("/", false, 1);
	const String &full_file_path = full_path_parts.get(0);
	const String &file_name = full_path_parts.get(1);
	const String &old_snapshot_name = file_name.split(".").get(0);
	const String &new_full_file_path = full_file_path.path_join(new_snapshot_name) + ".odb_snapshot";

	if (new_snapshot_name.contains(":") || new_snapshot_name.contains("\\") || new_snapshot_name.contains("/") || new_snapshot_name.begins_with(".") || new_snapshot_name.length() == 0) {
		EditorNode::get_singleton()->show_warning(TTR("Invalid snapshot name"));
		snapshot_list->get_selected()->set_text(0, old_snapshot_name);
	}

	Error err = DirAccess::rename_absolute(full_file_with_path, new_full_file_path);
	if (err != OK) {
		EditorNode::get_singleton()->show_warning(TTR("Snapshot rename failed"));
		snapshot_list->get_selected()->set_text(0, old_snapshot_name);
	} else {
		snapshot_list->get_selected()->set_metadata(0, new_full_file_path);
	}

	_update_diff_items();
}

ObjectDBProfilerPanel::ObjectDBProfilerPanel() {
	singleton = this;
	set_name("ObjectDB Profiler");

	snapshot_cache = LRUCache<String, Ref<GameStateSnapshotRef>>(SNAPSHOT_CACHE_MAX_SIZE);

	// ===== ROOT CONTAINER =====
	HSplitContainer *root_container = memnew(HSplitContainer);
	root_container->set_anchors_preset(Control::LayoutPreset::PRESET_FULL_RECT);
	root_container->set_v_size_flags(Control::SizeFlags::SIZE_EXPAND_FILL);
	root_container->set_h_size_flags(Control::SizeFlags::SIZE_EXPAND_FILL);
	root_container->set_split_offset(300 * EDSCALE); // give the snapshot list some 'room to breath' by default
	add_child(root_container);

	VBoxContainer *snapshot_column = memnew(VBoxContainer);
	root_container->add_child(snapshot_column);

	// snapshot button
	take_snapshot = memnew(Button("Take ObjectDB Snapshot"));
	snapshot_column->add_child(take_snapshot);
	take_snapshot->connect(SceneStringName(pressed), callable_mp(this, &ObjectDBProfilerPanel::_request_object_snapshot));

	snapshot_list = memnew(Tree);
	snapshot_list->create_item();
	snapshot_list->set_hide_folding(true);
	snapshot_column->add_child(snapshot_list);
	snapshot_list->set_select_mode(Tree::SelectMode::SELECT_ROW);
	snapshot_list->set_hide_root(true);
	snapshot_list->set_columns(1);
	snapshot_list->set_column_titles_visible(true);
	snapshot_list->set_column_title(0, "Snapshots");
	snapshot_list->set_column_expand(0, true);
	snapshot_list->set_column_clip_content(0, true);
	snapshot_list->connect("item_selected", callable_mp(this, &ObjectDBProfilerPanel::_show_selected_snapshot));
	snapshot_list->connect("item_edited", callable_mp(this, &ObjectDBProfilerPanel::_edit_snapshot_name));
	snapshot_list->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	snapshot_list->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	snapshot_list->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

	snapshot_list->set_allow_rmb_select(true);
	snapshot_list->connect(SNAME("item_mouse_selected"), callable_mp(this, &ObjectDBProfilerPanel::_snapshot_rmb));

	rmb_menu = memnew(PopupMenu);
	add_child(rmb_menu);
	rmb_menu->connect(SceneStringName(id_pressed), callable_mp(this, &ObjectDBProfilerPanel::_rmb_menu_pressed).bind(false));

	HBoxContainer *diff_button_and_label = memnew(HBoxContainer);
	diff_button_and_label->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	snapshot_column->add_child(diff_button_and_label);
	Label *diff_against = memnew(Label("Diff against:"));
	diff_button_and_label->add_child(diff_against);

	diff_button = memnew(OptionButton);
	diff_button->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	diff_button->connect("item_selected", callable_mp(this, &ObjectDBProfilerPanel::_apply_diff));
	diff_button_and_label->add_child(diff_button);

	// tabs of various views right for each snapshot
	view_tabs = memnew(TabContainer);
	root_container->add_child(view_tabs);
	view_tabs->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	view_tabs->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	view_tabs->connect("tab_changed", callable_mp(this, &ObjectDBProfilerPanel::_view_tab_changed));

	add_view(memnew(SnapshotSummaryView));
	add_view(memnew(SnapshotClassView));
	add_view(memnew(SnapshotObjectView));
	add_view(memnew(SnapshotNodeView));
	add_view(memnew(SnapshotRefCountedView));
	add_view(memnew(SnapshotJsonView));

	set_enabled(false);

	// load all the snapshot names from disk
	Ref<DirAccess> snapshot_dir = _get_and_create_snapshot_storage_dir();
	TreeItem *last_snapshot_button = nullptr;
	if (!snapshot_dir.is_null()) {
		for (const String &file_name : snapshot_dir->get_files()) {
			Vector<String> name_parts = file_name.split(".");
			if (name_parts.size() != 2 || name_parts[1] != "odb_snapshot") {
				ERR_PRINT("ObjectDB Snapshot file did not have .tres extension. Skipping: " + file_name);
				continue;
			}

			last_snapshot_button = _add_snapshot_button(name_parts[0], snapshot_dir->get_current_dir().path_join(file_name));
		}
		// simulate clicking on the last snapshot we loaded from disk
		if (last_snapshot_button != nullptr) {
			snapshot_list->set_selected(last_snapshot_button);
			snapshot_list->ensure_cursor_is_visible();
		}
	}
}

void ObjectDBProfilerPanel::add_view(SnapshotView *to_add) {
	views.push_back(to_add);
	view_tabs->add_child(to_add);
}

ObjectDBProfilerPanel::~ObjectDBProfilerPanel() {
	singleton = nullptr;
}

void ObjectDBProfilerPanel::_update_diff_items() {
	diff_button->clear();
	diff_button->add_item("none", 0);
	diff_options[0] = "none";

	for (int i = 0; i < snapshot_list->get_root()->get_child_count(); i++) {
		const String &name = snapshot_list->get_root()->get_child(i)->get_text(0);
		diff_button->add_item(name);
		diff_options[i + 1] = name; // 0 = none, so i + 1
	}
}

void ObjectDBProfilerPanel::_apply_diff(int item_idx) {
	_show_selected_snapshot();
}
