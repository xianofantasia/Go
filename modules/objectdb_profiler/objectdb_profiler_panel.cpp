
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
#include "data_viewers/json_view.h"
#include "data_viewers/node_view.h"
#include "data_viewers/summary_view.h"
#include "core/object/ref_counted.h"
#include "core/io/dir_access.h"
#include "core/error/error_macros.h"
#include "core/io/json.h"
#include "core/core_bind.h"
#include "core/config/project_settings.h"
#include "scene/gui/split_container.h"
#include "scene/gui/option_button.h"


enum RC_MENU_OPERATIONS {
	SHOW_IN_FOLDER,
	DELETE,
};

ObjectDBProfilerPanel *ObjectDBProfilerPanel::singleton = nullptr;


void ObjectDBProfilerPanel::receive_snapshot(const Array& p_data) {
	String snapshot_file_name = rtos(Time::get_singleton()->get_unix_time_from_system()).replace(".", "_"); // godot has decimal precision unix time, so replace . with _ when writing to disk
	Ref<DirAccess> snapshot_dir = _get_and_create_snapshot_storage_dir();
	if (!snapshot_dir.is_null()) {
		Error err;
		String current_dir = snapshot_dir->get_current_dir();
		String joined_dir = current_dir.path_join(snapshot_file_name) + ".odb_snapshot";
 
		Ref<FileAccess> file = FileAccess::open(joined_dir, FileAccess::WRITE, &err);
		if (err == OK) {
			String data_str = core_bind::Marshalls::get_singleton()->variant_to_base64(p_data, true);
			file->store_string(data_str);
		} else {
			ERR_PRINT("Could not persist ObjectDB Snapshot: " + String(error_names[err]));
		}
	}
	
	_add_snapshot_button(snapshot_file_name);
	snapshot_list->set_selected(snapshot_list->get_root()->get_first_child());
	_show_selected_snapshot();
}

String ObjectDBProfilerPanel::snapshot_filename_to_name(const String& filename) {
	return Time::get_singleton()->get_datetime_string_from_unix_time((String::to_float(filename.replace("_", ".").get_data())));
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

void ObjectDBProfilerPanel::_add_snapshot_button(String snapshot_file_name) {
	snapshot_names.push_front(snapshot_file_name);
	TreeItem* item = snapshot_list->create_item(snapshot_list->get_root());
	item->set_text(0, snapshot_filename_to_name(snapshot_file_name));
	item->set_metadata(0, snapshot_file_name);
	item->move_before(snapshot_list->get_root()->get_first_child());
	_update_diff_items();
}

void ObjectDBProfilerPanel::_show_selected_snapshot() {
	show_snapshot(snapshot_list->get_selected()->get_metadata(0), diff_options[diff_button->get_selected_id()]);
}

Ref<GameStateSnapshotRef> ObjectDBProfilerPanel::get_snapshot(const String& snapshot_file_name) {
	if (snapshot_cache.has(snapshot_file_name)) {
		return snapshot_cache.get(snapshot_file_name);
	} else {
		Ref<DirAccess> snapshot_dir = _get_and_create_snapshot_storage_dir();
		if (snapshot_dir.is_null()) {
			ERR_PRINT("Could not access ObjectDB Snapshot directory");
			return nullptr;
		}

		String full_file_path = snapshot_dir->get_current_dir().path_join(snapshot_file_name) + ".odb_snapshot";
		String content = FileAccess::get_file_as_string(full_file_path);
		if (content == "") {
			ERR_PRINT("ObjectDB Snapshot file is empty: " + full_file_path);
			return nullptr;
		}

		Array content_arr = core_bind::Marshalls::get_singleton()->base64_to_variant(content, true);
		Ref<GameStateSnapshotRef> snapshot = Ref<GameStateSnapshotRef>(memnew(GameStateSnapshotRef(memnew(GameStateSnapshot(snapshot_filename_to_name(snapshot_file_name), content_arr)))));
		snapshot_cache.insert(snapshot_file_name, snapshot);
		return snapshot;
	}
}

void ObjectDBProfilerPanel::show_snapshot(const String& snapshot_file_name, const String& snapshot_diff_file_name) {
	clear_snapshot();

	current_snapshot = get_snapshot(snapshot_file_name);
	if (snapshot_diff_file_name != "none") {
		diff_snapshot = get_snapshot(snapshot_diff_file_name);
	} else {
		diff_snapshot = nullptr;
	}

	for (SnapshotView* view : views) {
		view->show_snapshot(current_snapshot->ptr(), diff_snapshot == nullptr ? nullptr : diff_snapshot->ptr());
	}
}

void ObjectDBProfilerPanel::_bind_methods() {
    ADD_SIGNAL(MethodInfo("request_snapshot"));
}


void ObjectDBProfilerPanel::_request_object_snapshot() {
	emit_signal("request_snapshot");
}

void ObjectDBProfilerPanel::clear_snapshot() {
    for (SnapshotView* view : views) {
        view->clear_snapshot();
    }
	current_snapshot = nullptr;
}

void ObjectDBProfilerPanel::set_enabled(bool enabled) {
	take_snapshot->set_disabled(!enabled);
}

void ObjectDBProfilerPanel::_snapshot_rmb(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) return;
	rmb_menu->clear(false);

	// Show in folder
	// Delete
	rmb_menu->add_icon_item(get_editor_theme_icon(SNAME("Folder")), "Show In Folder", RC_MENU_OPERATIONS::SHOW_IN_FOLDER);
	rmb_menu->add_icon_item(get_editor_theme_icon(SNAME("Remove")), "Delete Snapshot", RC_MENU_OPERATIONS::DELETE);

	rmb_menu->reset_size();
	rmb_menu->set_position(get_screen_position() + p_pos);
	rmb_menu->popup();
}

void ObjectDBProfilerPanel::_rmb_menu_pressed(int p_tool, bool p_confirm_override) {
	String current_snapshot_filename = snapshot_list->get_selected()->get_metadata(0);
	Ref<DirAccess> da = _get_and_create_snapshot_storage_dir();
	if (da.is_null()) return;
	String file_path = da->get_current_dir().path_join(current_snapshot_filename) + ".odb_snapshot";
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
			snapshot_names.erase(current_snapshot_filename);
			_update_diff_items();
			break;
		}
	}
}

ObjectDBProfilerPanel::ObjectDBProfilerPanel() {
	singleton = this;
	set_name("ObjectDB Profiler"); 

	snapshot_cache = LRUCache<String, Ref<GameStateSnapshotRef>>(SNAPSHOT_CACHE_MAX_SIZE);

	// ===== ROOT CONTAINER =====
	HSplitContainer* root_container = memnew(HSplitContainer);
	root_container->set_anchors_preset(Control::LayoutPreset::PRESET_FULL_RECT);
	root_container->set_v_size_flags(Control::SizeFlags::SIZE_EXPAND_FILL);
	root_container->set_h_size_flags(Control::SizeFlags::SIZE_EXPAND_FILL);
	root_container->set_split_offset(300 * EDSCALE); // give the snapshot list some 'room to breath' by default
	add_child(root_container);

	VBoxContainer* snapshot_column = memnew(VBoxContainer);
	root_container->add_child(snapshot_column);
	
	// snapshot button
	take_snapshot = memnew(Button("Take ObjectDB Snapshot"));
	snapshot_column->add_child(take_snapshot);
	take_snapshot->connect(SceneStringName(pressed), callable_mp(this, &ObjectDBProfilerPanel::_request_object_snapshot));

	snapshot_list = memnew(Tree);
	snapshot_list->create_item();
    snapshot_list->set_hide_folding(true);
    snapshot_column->add_child(snapshot_list);
    snapshot_list->set_hide_root(true);
    snapshot_list->set_columns(1);
    snapshot_list->set_column_titles_visible(true);
    snapshot_list->set_column_title(0, "Snapshots");
    snapshot_list->set_column_expand(0, true);
    snapshot_list->set_column_clip_content(0, true);
    snapshot_list->connect("cell_selected", callable_mp(this, &ObjectDBProfilerPanel::_show_selected_snapshot));
    snapshot_list->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    snapshot_list->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	snapshot_list->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

	snapshot_list->set_allow_rmb_select(true);
	snapshot_list->connect(SNAME("item_mouse_selected"), callable_mp(this, &ObjectDBProfilerPanel::_snapshot_rmb));
	
	rmb_menu = memnew(PopupMenu);
	add_child(rmb_menu);
	rmb_menu->connect(SceneStringName(id_pressed), callable_mp(this, &ObjectDBProfilerPanel::_rmb_menu_pressed).bind(false));

    HBoxContainer* diff_button_and_label = memnew(HBoxContainer);
    diff_button_and_label->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    snapshot_column->add_child(diff_button_and_label);
    Label* diff_against = memnew(Label("Diff against:"));
    diff_button_and_label->add_child(diff_against);

    diff_button = memnew(OptionButton);
	diff_button->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
    diff_button->connect("item_selected", callable_mp(this, &ObjectDBProfilerPanel::_apply_diff));
    diff_button_and_label->add_child(diff_button);
	_update_diff_items();

	// tabs of various views right for each snapshot
	view_tabs = memnew(TabContainer);
	root_container->add_child(view_tabs);
	view_tabs->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	view_tabs->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);


    add_view(memnew(SnapshotSummaryView));
    add_view(memnew(SnapshotClassView));
    add_view(memnew(SnapshotObjectView));
    add_view(memnew(SnapshotNodeView));
    add_view(memnew(SnapshotRefCountedView));
    add_view(memnew(SnapshotJsonView));

	set_enabled(false);

	// load all the snapshot names from disk
	Ref<DirAccess> snapshot_dir = _get_and_create_snapshot_storage_dir();
	String last_snapshot_name = "";
	if (!snapshot_dir.is_null()) {
		for (const String& file_name : snapshot_dir->get_files()) {
			Vector<String> name_parts = file_name.split(".");
			if (name_parts.size() != 2 || name_parts[1] != "odb_snapshot") {
				ERR_PRINT("ObjectDB Snapshot file did not have .tres extension. Skipping: " + file_name);
				continue;
			}
			
			_add_snapshot_button(name_parts[0]);
			last_snapshot_name = name_parts[0];
		}
		// simulate clicking on the last snapshot we loaded from disk
		snapshot_list->set_selected(snapshot_list->get_item_with_metadata(last_snapshot_name, 0));
	}

}

void ObjectDBProfilerPanel::add_view(SnapshotView* to_add) {
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
	
	int idx = 1;
    for (const String& name : snapshot_names) {
        diff_button->add_item(snapshot_filename_to_name(name));
		diff_options[idx] = name;
		idx++;
    }
}

void ObjectDBProfilerPanel::_apply_diff(int item_idx) {
	_show_selected_snapshot();
}
