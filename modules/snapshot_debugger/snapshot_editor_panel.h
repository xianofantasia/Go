


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

#ifndef SNAPSHOT_EDITOR_PANEL_H
#define SNAPSHOT_EDITOR_PANEL_H

#include "editor/debugger/editor_debugger_inspector.h"
#include "scene/debugger/scene_debugger.h"
#include "editor/plugins/editor_debugger_plugin.h"
#include "scene/gui/tree.h"
#include "scene/gui/tab_container.h"
#include "snapshot_data.h"
#include "editor/plugins/editor_plugin.h"

#include "snapshot_data.h"
#include "data_viewers/snapshot_view.h"
#include "data_viewers/summary_view.h"


// UI loaded by the debugger
class SnapshotEditorPanel : public Control {
	GDCLASS(SnapshotEditorPanel, Control);

protected:
	Tree* snapshot_list;
	Button* take_snapshot;

	TabContainer* view_tabs;
    List<SnapshotView*> views;
	SnapshotSummaryView* summary_view;

	HashMap<String, GameStateSnapshot*> snapshots;
    GameStateSnapshot* current_snapshot;

	void _request_object_snapshot();
	void _show_selected_snapshot();
	
public:
	SnapshotEditorPanel();
	static void _bind_methods();

    void add_snapshot(GameStateSnapshot* snapshot);
    void show_snapshot(const String& snapshot_name);
	void clear_snapshot();
	void set_enabled(bool enabled);
    
    void add_view(SnapshotView* to_add);
};


#endif // SNAPSHOT_EDITOR_PANEL_H
