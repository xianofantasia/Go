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

#ifndef SNAPSHOT_EDITOR_PLUGIN_H
#define SNAPSHOT_EDITOR_PLUGIN_H

// #include "editor/debugger/editor_debugger_inspector.h"
// #include "scene/debugger/scene_debugger.h"
#include "editor/plugins/editor_debugger_plugin.h"
#include "editor/plugins/editor_plugin.h"
#include "snapshot_data.h"

class SnapshotEditorPanel;


// Boostrapped by the plugin
class SnapshotEditorDebugger : public EditorDebuggerPlugin {
	GDCLASS(SnapshotEditorDebugger, EditorDebuggerPlugin);

protected:
	SnapshotEditorPanel* debugger_panel;

	void request_object_snapshot();
	void receive_object_snapshot(const Array& p_data);
	
public:
	virtual bool has_capture(const String &p_capture) const override;
	virtual bool capture(const String &p_message, const Array &p_data, int p_index) override;
	virtual void setup_session(int p_session_id) override;

	SnapshotEditorDebugger();
};

// Loaded first as a plugin. The plugin can then add the debugger when it starts up
class SnapshotEditorPlugin : public EditorPlugin {
	GDCLASS(SnapshotEditorPlugin, EditorPlugin);

protected:
	Ref<SnapshotEditorDebugger> debugger;
	void _notification(int p_what);

public:
	SnapshotEditorPlugin();
};


#endif // SNAPSHOT_EDITOR_PLUGIN_H
