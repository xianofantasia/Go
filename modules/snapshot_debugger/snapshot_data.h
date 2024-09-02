/**************************************************************************/
/*  multiplayer_editor_plugin.h                                           */
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

#ifndef SNAPSHOT_DATA_H
#define SNAPSHOT_DATA_H

#include "editor/debugger/editor_debugger_inspector.h"
#include "scene/debugger/scene_debugger.h"
#include "core/object/script_language.h"
#include "modules/gdscript/gdscript.h"

#include "core/io/missing_resource.h"
#include "scene/gui/tree.h"


struct SnapshotDataTransportObject : public SceneDebuggerObject {
	SnapshotDataTransportObject(): SceneDebuggerObject() {}
	SnapshotDataTransportObject(Object* obj): SceneDebuggerObject(obj) {}
	Dictionary extra_debug_data;
};


class SnapshotDataObject : public EditorDebuggerRemoteObject {
	GDCLASS(SnapshotDataObject, EditorDebuggerRemoteObject);
	

public:
	class GameStateSnapshot* snapshot;
	SnapshotDataObject(SceneDebuggerObject& obj, GameStateSnapshot* snapshot): EditorDebuggerRemoteObject(obj), snapshot(snapshot) {}
	Dictionary extra_debug_data;

	HashMap<String, ObjectID> outbound_references;
	HashMap<String, ObjectID> inbound_references;

	String get_name();
	String get_node_path();

	bool is_refcounted() {
		return is_class(RefCounted::get_class_static());
	}
	
	bool is_node() {
		return is_class(Node::get_class_static());
	}
	
	bool is_class(const String& base_class) {
		return ClassDB::is_parent_class(type_name, base_class);
	}
};

class GameStateSnapshot : public Object {
	GDCLASS(GameStateSnapshot, Object);

	void get_outbound_references(Variant& var, HashMap<String, ObjectID>& ret_val, String current_path = "");
	void get_rc_cycles(SnapshotDataObject* obj, SnapshotDataObject* source_obj, HashSet<SnapshotDataObject*> traversed_objs, List<String>& ret_val, String current_path = "");

public:
	GameStateSnapshot() {}

	String name;
	TreeItem* snapshot_button;
	HashMap<ObjectID, SnapshotDataObject*> Data;

	SnapshotDataObject* get_object(ObjectID id) { return Data[id]; }
	void recompute_references();


	bool do_nulls_exist() {
		for (const KeyValue<ObjectID, SnapshotDataObject*>& obj : Data) {
			if (obj.value == nullptr) {
				return true;
			}
		}
		return false;
	}
};


#endif // SNAPSHOT_DATA_H
