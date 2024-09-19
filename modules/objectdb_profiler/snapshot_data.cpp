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

#include "core/core_bind.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "core/os/time.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"

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
		current = snapshot->objects[current->extra_debug_data["node_parent"]];
	}
	return path;
}

String SnapshotDataObject::get_name() {
	String found_type_name = type_name;

	// Ideally, we will name it after the script attached to it
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

bool SnapshotDataObject::is_refcounted() {
	return is_class(RefCounted::get_class_static());
}

bool SnapshotDataObject::is_node() {
	return is_class(Node::get_class_static());
}

bool SnapshotDataObject::is_class(const String &p_base_class) {
	return ClassDB::is_parent_class(type_name, p_base_class);
}

void GameStateSnapshot::_get_outbound_references(Variant &p_var, HashMap<String, ObjectID> &r_ret_val, String p_current_path) {
	String path_divider = p_current_path.size() > 0 ? "/" : ""; // make sure we don't start with a /
	switch (p_var.get_type()) {
		case Variant::Type::INT:
		case Variant::Type::OBJECT: { // means ObjectID
			ObjectID as_id = ObjectID((uint64_t)p_var);
			if (!objects.has(as_id)) {
				return;
			}
			r_ret_val[p_current_path] = as_id;
			break;
		}
		case Variant::Type::DICTIONARY: {
			Dictionary dict = (Dictionary)p_var;
			List<Variant> keys;
			dict.get_key_list(&keys);
			for (Variant &k : keys) {
				// The dictionary key _could be_ an object. If it is, we name the key property with the same name as the value, but with _key appended to it
				_get_outbound_references(k, r_ret_val, p_current_path + path_divider + (String)k + "_key");
				Variant v = dict.get(k, Variant());
				_get_outbound_references(v, r_ret_val, p_current_path + path_divider + (String)k);
			}
			break;
		}
		case Variant::Type::ARRAY: {
			Array arr = (Array)p_var;
			int i = 0;
			for (Variant &v : arr) {
				_get_outbound_references(v, r_ret_val, p_current_path + path_divider + itos(i));
				i++;
			}
			break;
		}
		default: {
			break;
		}
	}
}

void GameStateSnapshot::_get_rc_cycles(
		SnapshotDataObject *p_obj,
		SnapshotDataObject *p_source_obj,
		HashSet<SnapshotDataObject *> p_traversed_objs,
		List<String> &r_ret_val,
		String p_current_path) {
	// we're at the end of this branch and it was a cycle
	if (p_obj == p_source_obj && p_current_path != "") {
		r_ret_val.push_back(p_current_path);
		return;
	}

	// go through each of our children and try traversing them
	for (const KeyValue<String, ObjectID> &next_child : p_obj->outbound_references) {
		SnapshotDataObject *next_obj = p_obj->snapshot->objects[next_child.value];
		String next_name = next_obj == p_source_obj ? "self" : next_obj->get_name();
		String current_name = p_obj == p_source_obj ? "self" : p_obj->get_name();
		String child_path = current_name + "[\"" + next_child.key + "\"] -> " + next_name;
		if (p_current_path != "") {
			child_path = p_current_path + "\n" + child_path;
		}

		SnapshotDataObject *next = objects[next_child.value];
		if (next != nullptr && next->is_class(RefCounted::get_class_static()) && !next->is_class(WeakRef::get_class_static()) && !p_traversed_objs.has(next)) {
			HashSet<SnapshotDataObject *> traversed_copy = p_traversed_objs;
			if (p_obj != p_source_obj) {
				traversed_copy.insert(p_obj);
			}
			_get_rc_cycles(next, p_source_obj, traversed_copy, r_ret_val, child_path);
		}
	}
}

void GameStateSnapshot::recompute_references() {
	for (const KeyValue<ObjectID, SnapshotDataObject *> &obj : objects) {
		Dictionary values;
		for (const KeyValue<StringName, Variant> &kv : obj.value->prop_values) {
			values[kv.key] = kv.value;
		}

		Variant values_variant(values);
		HashMap<String, ObjectID> refs;
		_get_outbound_references(values_variant, refs);

		obj.value->outbound_references = refs;

		for (const KeyValue<String, ObjectID> &kv : refs) {
			// get the guy we are pointing to, and indicate the name of _our_ property that is pointing to them
			if (objects.has(kv.value)) {
				objects[kv.value]->inbound_references[kv.key] = obj.key;
			}
		}
	}

	int i = 0;
	for (const KeyValue<ObjectID, SnapshotDataObject *> &obj : objects) {
		i++;
		if (!obj.value->is_class(RefCounted::get_class_static()) || obj.value->is_class(WeakRef::get_class_static()))
			continue;
		HashSet<SnapshotDataObject *> traversed_objs;
		List<String> cycles;

		_get_rc_cycles(obj.value, obj.value, traversed_objs, cycles, "");
		Array cycles_array;
		for (const String &cycle : cycles) {
			cycles_array.push_back(cycle);
		}
		obj.value->extra_debug_data["ref_cycles"] = cycles_array;
	}
}

Ref<GameStateSnapshotRef> GameStateSnapshot::create_ref(const String &p_snapshot_name, int p_snapshot_version, const String &p_snapshot_string) {
	GameStateSnapshot *sn = memnew(GameStateSnapshot);
	sn->name = p_snapshot_name;
	sn->snapshot_version = p_snapshot_version;

	// Snapshots may have been created by an older version of the editor. Handle parsing old snapshot versions here based on the version number.
	switch (sn->snapshot_version) {
		case 1: {
			Array snapshot_data = core_bind::Marshalls::get_singleton()->base64_to_variant(p_snapshot_string, true);
			sn->snapshot_context = snapshot_data.get(0);

			for (int i = 1; i < snapshot_data.size(); i += 4) {
				Array sliced = snapshot_data.slice(i);
				SceneDebuggerObject obj;
				obj.deserialize(sliced);

				if (sliced[3].get_type() != Variant::DICTIONARY) {
					goto LOAD_FAILED;
				}
				if (obj.id.is_null()) {
					continue;
				}

				sn->objects[obj.id] = memnew(SnapshotDataObject(obj, sn));
				sn->objects[obj.id]->extra_debug_data = (Dictionary)sliced[3];
				sn->objects[obj.id]->set_readonly(true);
			}

			sn->recompute_references();
			// A ref to a refcounted object which is a wrapper of a non-refcounted object.
			return Ref<GameStateSnapshotRef>(memnew(GameStateSnapshotRef(sn)));
		} break;

		default: {
			ERR_PRINT("ObjectDB Snapshot version not recognized: " + itos(p_snapshot_version));
		} break;
	}

LOAD_FAILED:
	ERR_PRINT("ObjectDB Snapshot could not be parsed");
	memfree(sn);
	return nullptr;
}

GameStateSnapshot::~GameStateSnapshot() {
	for (const KeyValue<ObjectID, SnapshotDataObject *> &item : objects) {
		memfree(item.value);
	}
}

bool GameStateSnapshotRef::unreference() {
	bool die = RefCounted::unreference();
	if (die) {
		memdelete(gamestate_snapshot);
	}
	return die;
}
