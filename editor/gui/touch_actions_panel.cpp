/**************************************************************************/
/*  touch_actions_panel.cpp                                               */
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

#include "touch_actions_panel.h"
#include "core/input/input.h"
#include "editor/editor_settings.h"

void TouchActionsPanel::_simulate_action(const String &action_name) {
	Ref<Shortcut> shortcut = ED_GET_SHORTCUT(action_name);

	if (shortcut.is_valid() && !shortcut->get_events().is_empty()) {
		Ref<InputEventKey> event = shortcut->get_events()[0];
		if (event.is_valid()) {
			event->set_pressed(true);
			Input::get_singleton()->parse_input_event(event);
		}
	}
}

TouchActionsPanel::TouchActionsPanel() {
	hbox = memnew(HBoxContainer);
	hbox->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	hbox->add_theme_constant_override("separation", 30);
	add_child(hbox);

	undo_button = memnew(Button);
	undo_button->set_text(TTR("Undo"));
	undo_button->set_focus_mode(Control::FOCUS_NONE);
	undo_button->connect(SceneStringName(pressed), callable_mp(this, &TouchActionsPanel::_simulate_action).bind("ui_undo"));
	hbox->add_child(undo_button);

	redo_button = memnew(Button);
	redo_button->set_text(TTR("Redo"));
	redo_button->set_focus_mode(Control::FOCUS_NONE);
	redo_button->connect(SceneStringName(pressed), callable_mp(this, &TouchActionsPanel::_simulate_action).bind("ui_redo"));
	hbox->add_child(redo_button);
}
