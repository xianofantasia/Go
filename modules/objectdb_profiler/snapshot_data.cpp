/**************************************************************************/
/*  snapshot_data.cpp                                                     */
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

#include "objectdb_profiler_plugin.h"

#include "core/object/object.h"
#include "core/os/memory.h"
#include "core/os/time.h"
#include "editor/debugger/editor_debugger_node.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/tree.h"
// #include "core/object/script_language.h"
#include "core/object/ref_counted.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/tab_container.h"
// #include "modules/gdscript/gdscript.h"

String SnapshotDataObject::get_node_path() {
	if (!is_node())
		return "";
	SnapshotDataObject *current = this;
	String path = "";

	while (true) {
		String current_node_name = current->extra_debug_data["node_name"];
		if (current_node_name != "") {
			if (path != "") {
				path = current_node_name + "/" + path;
			} else {
				path = current_node_name;
			}
		}
		if (!current->extra_debug_data.has("node_parent")) {
			break;
		}
		current = snapshot->Data[current->extra_debug_data["node_parent"]];
	}
	return path;
}

String SnapshotDataObject::get_name() {
	String found_type_name = type_name;

	// Ideally, we will name it after the script it's attached to
	if (get_script() != nullptr) {
		Object *maybe_script_obj = get_script().get_validated_object();

		if (maybe_script_obj->is_class(Script::get_class_static())) {
			Script *script_obj = (Script *)maybe_script_obj;

			String full_name;
			while (script_obj != nullptr) {
				String global_name = GDScript::debug_get_script_name(script_obj);
				if (global_name != "") {
					if (full_name != "") {
						full_name = global_name + "/" + full_name;
					} else {
						full_name = global_name;
					}
				}
				script_obj = script_obj->get_base_script().ptr();
			}

			found_type_name = type_name + "/" + full_name;
		}
	}

	return found_type_name + "_" + uitos(remote_object_id);
}

void GameStateSnapshot::get_outbound_references(Variant &var, HashMap<String, ObjectID> &ret_val, String current_path) {
	String path_divider = current_path.size() > 0 ? "/" : ""; // make sure we don't start with a / cuz that's weird
	switch (var.get_type()) {
		case Variant::Type::INT:
		case Variant::Type::OBJECT: { // means ObjectID
			ObjectID as_id = ObjectID((uint64_t)var);
			if (!Data.has(as_id))
				return;
			ret_val[current_path] = as_id;
			break;
		}
		case Variant::Type::DICTIONARY: {
			Dictionary dict = (Dictionary)var;
			List<Variant> keys;
			dict.get_key_list(&keys);
			for (Variant &k : keys) {
				get_outbound_references(k, ret_val, current_path + path_divider + (String)k + "_key"); // The dictionary key _could be_ an object. If it is, we name the key property with the same name as the value, but with _key appended to it
				Variant v = dict.get(k, Variant());
				get_outbound_references(v, ret_val, current_path + path_divider + (String)k);
			}
			break;
		}
		case Variant::Type::ARRAY: {
			Array arr = (Array)var;
			int i = 0;
			for (Variant &v : arr) {
				get_outbound_references(v, ret_val, current_path + path_divider + itos(i));
				i++;
			}
			break;
		}
		default: {
			break;
		}
	}
}

void GameStateSnapshot::get_rc_cycles(
		SnapshotDataObject *obj,
		SnapshotDataObject *source_obj,
		HashSet<SnapshotDataObject *> traversed_objs,
		List<String> &ret_val,
		String current_path) {
	// we're at the end of this branch and it was a cycle
	if (obj == source_obj && current_path != "") {
		ret_val.push_back(current_path);
		return;
	}

	// go through each of our children and try traversing them
	for (const KeyValue<String, ObjectID> &next_child : obj->outbound_references) {
		SnapshotDataObject *next_obj = obj->snapshot->Data[next_child.value];
		String next_name = next_obj == source_obj ? "self" : next_obj->get_name();
		String current_name = obj == source_obj ? "self" : obj->get_name();
		String child_path = current_name + "[\"" + next_child.key + "\"] -> " + next_name;
		if (current_path != "") {
			child_path = current_path + "\n" + child_path;
		}

		SnapshotDataObject *next = Data[next_child.value];
		if (next != nullptr && next->is_class(RefCounted::get_class_static()) && !next->is_class(WeakRef::get_class_static()) && !traversed_objs.has(next)) {
			HashSet<SnapshotDataObject *> traversed_copy = traversed_objs;
			if (obj != source_obj) {
				traversed_copy.insert(obj);
			}
			get_rc_cycles(next, source_obj, traversed_copy, ret_val, child_path);
		}
	}
}

void GameStateSnapshot::recompute_references() {
	for (const KeyValue<ObjectID, SnapshotDataObject *> &obj : Data) {
		Dictionary values;
		for (const KeyValue<StringName, Variant> &kv : obj.value->prop_values) {
			values[kv.key] = kv.value;
		}

		Variant values_variant(values);
		HashMap<String, ObjectID> refs;
		get_outbound_references(values_variant, refs);

		obj.value->outbound_references = refs;

		for (const KeyValue<String, ObjectID> &kv : refs) {
			// get the guy we are pointing to, and indicate the name of _our_ property that is pointing to them
			if (Data.has(kv.value)) {
				Data[kv.value]->inbound_references[kv.key] = obj.key;
			}
		}
	}

	int i = 0;
	for (const KeyValue<ObjectID, SnapshotDataObject *> &obj : Data) {
		i++;
		if (!obj.value->is_class(RefCounted::get_class_static()) || obj.value->is_class(WeakRef::get_class_static()))
			continue;
		HashSet<SnapshotDataObject *> traversed_objs;
		List<String> cycles;

		get_rc_cycles(obj.value, obj.value, traversed_objs, cycles, "");
		Array cycles_array;
		for (const String &cycle : cycles) {
			cycles_array.push_back(cycle);
		}
		obj.value->extra_debug_data["ref_cycles"] = cycles_array;
	}

	// for RefCounted objects only, go through and count how many cycles they are part of
}
