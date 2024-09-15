/**************************************************************************/
/*  objectdb_profiler_plugin.cpp                                          */
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
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "core/os/time.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "objectdb_profiler_panel.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "snapshot_data.h"

ObjectDBProfilerDebuggerPlugin::ObjectDBProfilerDebuggerPlugin() {
}

bool ObjectDBProfilerDebuggerPlugin::has_capture(const String &p_capture) const {
	return p_capture == "snapshot";
}

bool ObjectDBProfilerDebuggerPlugin::capture(const String &p_message, const Array &p_data, int p_index) {
	ERR_FAIL_COND_V(debugger_panel == nullptr, false);

	if (p_message == "snapshot:object_snapshot") {
		debugger_panel->receive_snapshot(p_data);
		return true;
	}
	return false;
}

void ObjectDBProfilerDebuggerPlugin::setup_session(int p_session_id) {
	Ref<EditorDebuggerSession> session = get_session(p_session_id);
	ERR_FAIL_COND(session.is_null());
	debugger_panel = memnew(ObjectDBProfilerPanel);
	session->connect(SNAME("started"), callable_mp(debugger_panel, &ObjectDBProfilerPanel::set_enabled).bind(true));
	session->connect(SNAME("stopped"), callable_mp(debugger_panel, &ObjectDBProfilerPanel::set_enabled).bind(false));
	debugger_panel->connect(SNAME("request_snapshot"), callable_mp(this, &ObjectDBProfilerDebuggerPlugin::request_object_snapshot));
	session->add_session_tab(debugger_panel);
}

void ObjectDBProfilerDebuggerPlugin::request_object_snapshot() {
	EditorDebuggerNode::get_singleton()->get_current_debugger()->send_message("snapshot:request_object_snapshot", Array());
}

ObjectDBProfilerPlugin::ObjectDBProfilerPlugin() {
	debugger.instantiate();
}

void ObjectDBProfilerPlugin::_notification(int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			add_debugger_plugin(debugger);
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			remove_debugger_plugin(debugger);
		}
	}
}
