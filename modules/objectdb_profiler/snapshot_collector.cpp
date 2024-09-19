/**************************************************************************/
/*  snapshot_collector.cpp                                                */
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

#include "snapshot_collector.h"
#include "snapshot_data.h"

#include "core/core_bind.h"
#include "core/debugger/engine_debugger.h"
#include "core/object/object.h"

HashMap<int, String> SnapshotCollector::pending_snapshots;

void SnapshotCollector::initialize() {
	EngineDebugger::register_message_capture("snapshot", EngineDebugger::Capture(nullptr, SnapshotCollector::parse_message));
}

void SnapshotCollector::deinitialize() {
}

void SnapshotCollector::snapshot_objects(Array *p_arr) {
	print_line("Starting to snapshot");
	List<SnapshotDataTransportObject> debugger_objects;
	p_arr->clear();
	ObjectDB::debug_objects([](Object *p_obj, void *user_data) {
		List<SnapshotDataTransportObject> *debugger_objects = (List<SnapshotDataTransportObject> *)user_data;
		// This is the same way objects in the remote scene tree are seialized,
		// but here we add a few extra properties via the extra_debug_data dictionary
		SnapshotDataTransportObject debug_data(p_obj);

		// If we're RefCounted, send over our RefCount too. Could add code here to add a few other interesting properties
		if (ClassDB::is_parent_class(p_obj->get_class_name(), RefCounted::get_class_static())) {
			RefCounted *ref = (RefCounted *)p_obj;
			debug_data.extra_debug_data["ref_count"] = ref->get_reference_count();
		}

		if (ClassDB::is_parent_class(p_obj->get_class_name(), Node::get_class_static())) {
			Node *node = (Node *)p_obj;
			debug_data.extra_debug_data["node_name"] = node->get_name();
			if (node->get_parent() != nullptr) {
				debug_data.extra_debug_data["node_parent"] = node->get_parent()->get_instance_id();
			}

			debug_data.extra_debug_data["node_is_scene_root"] = SceneTree::get_singleton()->get_root() == node;

			Array children;
			for (int i = 0; i < node->get_child_count(); i++) {
				children.push_back(node->get_child(i)->get_instance_id());
			}
			debug_data.extra_debug_data["node_children"] = children;
		}

		debugger_objects->push_back(debug_data);
	},
			(void *)&debugger_objects);

	// Add a header to the snapshot with general data about the state of the game, not tied to any particular object
	Dictionary snapshot_context;
	snapshot_context["mem_available"] = Memory::get_mem_available();
	snapshot_context["mem_usage"] = Memory::get_mem_usage();
	snapshot_context["mem_max_usage"] = Memory::get_mem_max_usage();
	snapshot_context["timestamp"] = Time::get_singleton()->get_unix_time_from_system();
	p_arr->push_back(snapshot_context);
	for (SnapshotDataTransportObject &debug_data : debugger_objects) {
		debug_data.serialize(*p_arr);
		p_arr->push_back(debug_data.extra_debug_data);
	}

	print_line("snapshot length: " + String::num_uint64(p_arr->size()));
}

Error SnapshotCollector::parse_message(void *p_user, const String &p_msg, const Array &p_args, bool &r_captured) {
	r_captured = true;
	if (p_msg == "request_prepare_snapshot") {
		int request_id = (int)p_args.get(0);
		Array objects;
		SnapshotCollector::snapshot_objects(&objects);
		// Debugger networking has a limit on both how many objects can be queued to send and how
		// many bytes can be queued to send. Serializing to a string means we never hit the object
		// limit, and only have to deal with the byte limit.
		pending_snapshots[request_id] = core_bind::Marshalls::get_singleton()->variant_to_base64(objects);

		// tell the editor how long the snapshot is
		Array resp;
		resp.push_back(request_id);
		resp.push_back(pending_snapshots[request_id].length());
		EngineDebugger::get_singleton()->send_message("snapshot:snapshot_prepared", resp);

	} else if (p_msg == "request_snapshot_chunk") {
		int request_id = (int)p_args.get(0);
		int offset = (int)p_args.get(1);
		int length = (int)p_args.get(2);
		const String &snapshot_str = pending_snapshots[request_id];

		Array resp;
		resp.push_back(request_id);
		resp.push_back(snapshot_str.substr(offset, length));
		EngineDebugger::get_singleton()->send_message("snapshot:snapshot_chunk", resp);

		// If we sent the last part of the string, delete it locally
		if (offset + length >= snapshot_str.length()) {
			pending_snapshots.erase(request_id);
		}
	} else {
		r_captured = false;
	}
	return OK;
}
