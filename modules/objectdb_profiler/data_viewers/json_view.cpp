
/**************************************************************************/
/*  json_view.cpp                                                          */
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
#include "scene/gui/code_edit.h"
#include "core/io/json.h"
#include "../snapshot_data.h"



SnapshotJsonView::SnapshotJsonView() {
	set_name("JSON");
}

void SnapshotJsonView::show_snapshot(GameStateSnapshot* p_data, GameStateSnapshot* p_diff_data) {
    SnapshotView::show_snapshot(p_data, p_diff_data);

	VBoxContainer* box = memnew(VBoxContainer);
	box->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
	add_child(box);

	json_content = memnew(RichTextLabel);
	json_content->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	json_content->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	box->add_child(json_content);
	json_content->set_selection_enabled(true);

	// Should really do this on a thread, but IDK how to get the data back onto the main thread to add it to the text box...
	String json_view;
	Dictionary json_data;
	json_data["name"] = snapshot_data->name;
	Dictionary objects;
	for (const KeyValue<ObjectID, SnapshotDataObject*>& obj : snapshot_data->Data) {
		Dictionary obj_data;
		obj_data["type_name"] = obj.value->type_name;

		Array prop_list;
		for (const PropertyInfo& prop : obj.value->prop_list) {
			prop_list.push_back((Dictionary)prop);
		}
		objects["prop_list"] = prop_list;
		
		Dictionary prop_values;
		for (const KeyValue<StringName, Variant>& prop : obj.value->prop_values) {
			prop_values[prop.key] = prop.value;
		}
		obj_data["prop_values"] = prop_values;

		objects[obj.key] = obj_data;
	}
	json_data["objects"] = objects;

	json_content->add_text(JSON::stringify(json_data, "    ", true, true));
}
