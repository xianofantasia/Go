
/**************************************************************************/
/*  objectdb_profiler_panel.h                                             */
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

#ifndef OBJECTDB_PROFILER_PANEL_H
#define OBJECTDB_PROFILER_PANEL_H

#include "editor/debugger/editor_debugger_inspector.h"
#include "scene/debugger/scene_debugger.h"
#include "editor/plugins/editor_debugger_plugin.h"
#include "scene/gui/tree.h"
#include "scene/gui/tab_container.h"
#include "snapshot_data.h"
#include "editor/plugins/editor_plugin.h"

#include "snapshot_data.h"
#include "data_viewers/snapshot_view.h"
#include "core/io/dir_access.h"
#include "core/templates/lru.h"

const int SNAPSHOT_CACHE_MAX_SIZE = 10;

// UI loaded by the debugger
class ObjectDBProfilerPanel : public Control {
	GDCLASS(ObjectDBProfilerPanel, Control);

protected:
	Tree* snapshot_list;
	Button* take_snapshot;

	TabContainer* view_tabs;
    List<SnapshotView*> views;

	PopupMenu* rmb_menu;

	List<String> snapshot_names;
    Ref<GameStateSnapshotRef> current_snapshot;
    Ref<GameStateSnapshotRef> diff_snapshot;

	void _request_object_snapshot();
	void _show_selected_snapshot();

	Ref<DirAccess> _get_and_create_snapshot_storage_dir();

	void _add_snapshot_button(String snapshot_name);
	String _snapshot_filename_to_name(const String& filename);

	LRUCache<String, Ref<GameStateSnapshotRef>> snapshot_cache;

	void _snapshot_rmb(const Vector2 &p_pos, MouseButton p_button);
	void _rmb_menu_pressed(int p_tool, bool p_confirm_override);
	
public:
	ObjectDBProfilerPanel();
	static void _bind_methods();

	void receive_snapshot(const Array& p_data);
    void show_snapshot(const String& snapshot_file_name);
	void clear_snapshot();
	void set_enabled(bool enabled);

    void add_view(SnapshotView* to_add);
};


#endif // OBJECTDB_PROFILER_PANEL_H
