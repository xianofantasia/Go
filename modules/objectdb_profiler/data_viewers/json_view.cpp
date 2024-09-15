/**************************************************************************/
/*  json_view.cpp                                                         */
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

#include "json_view.h"

#include "../snapshot_data.h"
#include "core/io/json.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "core/os/time.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "modules/gdscript/gdscript.h"
#include "scene/gui/button.h"
#include "scene/gui/code_edit.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "shared_controls.h"

SnapshotJsonView::SnapshotJsonView() {
	set_name("JSON");
}

void SnapshotJsonView::show_snapshot(GameStateSnapshot *p_data, GameStateSnapshot *p_diff_data) {
	SnapshotView::show_snapshot(p_data, p_diff_data);

	HSplitContainer *box = memnew(HSplitContainer);
	box->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
	add_child(box);

	VBoxContainer *json_box = memnew(VBoxContainer);
	json_box->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	json_box->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	box->add_child(json_box);
	json_box->add_child(memnew(SpanningHeader(diff_data ? "Snapshot A JSON" : "Snapshot JSON")));
	json_content = memnew(RichTextLabel);
	json_content->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	json_content->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	json_content->set_selection_enabled(true);
	json_box->add_child(json_content);
	json_content->add_text(_snapshot_to_json(snapshot_data));

	if (diff_data) {
		VBoxContainer *diff_json_box = memnew(VBoxContainer);
		diff_json_box->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
		diff_json_box->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
		box->add_child(diff_json_box);
		diff_json_box->add_child(memnew(SpanningHeader("Snapshot B JSON")));
		diff_json_content = memnew(RichTextLabel);
		diff_json_content->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
		diff_json_content->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
		diff_json_content->set_selection_enabled(true);
		diff_json_box->add_child(diff_json_content);
		diff_json_content->add_text(_snapshot_to_json(diff_data));
	}
}

String SnapshotJsonView::_snapshot_to_json(GameStateSnapshot *snapshot) {
	String json_view;
	Dictionary json_data;
	json_data["name"] = snapshot->name;
	Dictionary objects;
	for (const KeyValue<ObjectID, SnapshotDataObject *> &obj : snapshot->Data) {
		Dictionary obj_data;
		obj_data["type_name"] = obj.value->type_name;

		Array prop_list;
		for (const PropertyInfo &prop : obj.value->prop_list) {
			prop_list.push_back((Dictionary)prop);
		}
		objects["prop_list"] = prop_list;

		Dictionary prop_values;
		for (const KeyValue<StringName, Variant> &prop : obj.value->prop_values) {
			prop_values[prop.key] = prop.value;
		}
		obj_data["prop_values"] = prop_values;

		objects[obj.key] = obj_data;
	}
	json_data["objects"] = objects;
	return JSON::stringify(json_data, "    ", true, true);
}
