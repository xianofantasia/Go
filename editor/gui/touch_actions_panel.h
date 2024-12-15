/**************************************************************************/
/*  touch_actions_panel.h                                                 */
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

#ifndef TOUCH_ACTIONS_PANEL_H
#define TOUCH_ACTIONS_PANEL_H

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/texture_rect.h"

class TouchActionsPanel : public PanelContainer {
	GDCLASS(TouchActionsPanel, PanelContainer);

private:
	BoxContainer *hbox;
	Button *save_button;
	Button *undo_button;
	Button *redo_button;
	TextureRect *drag_handle;
	Button *layout_toggle_button;

	bool dragging;
	Vector2 drag_offset;

	void _notification(int p_what);

	void _simulate_action(const String &action_name);
	void _on_drag_handle_gui_input(const Ref<InputEvent> &event);
	void switch_layout();
	Button *add_new_action(const String &shortcut);

public:
	TouchActionsPanel();
};

#endif // TOUCH_ACTIONS_PANEL_H
