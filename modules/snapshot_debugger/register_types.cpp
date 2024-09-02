/* register_types.cpp */

#include "register_types.h"

#include "core/object/class_db.h"
#include "editor/editor_node.h"
#include "editor/debugger/editor_debugger_node.h"

#include "snapshot_collector.h"
#include "snapshot_editor_plugin.h"


void initialize_snapshot_debugger_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorPlugins::add_by_type<SnapshotEditorPlugin>();
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		SnapshotCollector::initialize();
	}
}

void uninitialize_snapshot_debugger_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		// don't need to unregister the type, as there's no such thing
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		SnapshotCollector::initialize();
	}
}


