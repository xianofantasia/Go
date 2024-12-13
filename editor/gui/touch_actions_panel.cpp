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
#include "editor/editor_string_names.h"
#include "scene/resources/style_box_flat.h"

void TouchActionsPanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			drag_handle->set_texture(get_editor_theme_icon(SNAME("DragHandle")));
			save_button->set_button_icon(get_editor_theme_icon(SNAME("Save")));
			undo_button->set_button_icon(get_editor_theme_icon(SNAME("UndoRedo")));
			redo_button->set_button_icon(get_editor_theme_icon(SNAME("UndoRedo")));
		} break;
		default:
			break;
	}
}

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

void TouchActionsPanel::_on_drag_handle_gui_input(const Ref<InputEvent> &event) {
	Ref<InputEventMouseButton> mouse_button_event = event;
	if (mouse_button_event.is_valid() && mouse_button_event->get_button_index() == MouseButton::LEFT) {
		if (mouse_button_event->is_pressed()) {
			dragging = true;
			drag_offset = mouse_button_event->get_position();
		} else {
			dragging = false;
		}
	}

	Ref<InputEventMouseMotion> mouse_motion_event = event;
	if (mouse_motion_event.is_valid() && dragging) {
		Vector2 new_position = get_position() + mouse_motion_event->get_relative();
		// Clamp the position to parent bounds
		Vector2 parent_size = get_parent_area_size();
		Vector2 panel_size = get_size();
		new_position.x = CLAMP(new_position.x, 0, parent_size.x - panel_size.x);
		new_position.y = CLAMP(new_position.y, 0, parent_size.y - panel_size.y);
		set_position(new_position);
	}
}

TouchActionsPanel::TouchActionsPanel() {
	dragging = false;

	//set_custom_minimum_size(Size2(300, 50));

	// Add a StyleBoxFlat for padding and rounded corners
	Ref<StyleBoxFlat> panel_style = memnew(StyleBoxFlat);
	panel_style->set_bg_color(Color(0.1, 0.1, 0.1, 0.95));
	panel_style->set_border_color(Color(0.3, 0.3, 0.3, 1));
	panel_style->set_border_width_all(3);
	panel_style->set_corner_radius_all(10);
	panel_style->set_content_margin_all(12);
	add_theme_style_override("panel", panel_style);

	//set_anchors_preset(Control::PRESET_CENTER_BOTTOM);

	hbox = memnew(HBoxContainer);
	hbox->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	hbox->add_theme_constant_override("separation", 10);
	add_child(hbox);

	// Create drag handle
	drag_handle = memnew(TextureRect);
	drag_handle->set_custom_minimum_size(Size2(50, 30));
	drag_handle->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
	drag_handle->connect(SceneStringName(gui_input), callable_mp(this, &TouchActionsPanel::_on_drag_handle_gui_input));
	hbox->add_child(drag_handle);

	// Add a dummy control node for horizontal spacing.
	Control *spacer = memnew(Control);
	hbox->add_child(spacer);

	save_button = memnew(Button);
	save_button->set_focus_mode(Control::FOCUS_NONE);
	save_button->connect(SceneStringName(pressed), callable_mp(this, &TouchActionsPanel::_simulate_action).bind("editor/save_scene"));
	hbox->add_child(save_button);

	undo_button = memnew(Button);
	undo_button->set_text(TTR("Undo"));
	undo_button->set_focus_mode(Control::FOCUS_NONE);
	undo_button->connect(SceneStringName(pressed), callable_mp(this, &TouchActionsPanel::_simulate_action).bind("ui_undo"));
	hbox->add_child(undo_button);

	redo_button = memnew(Button);
	redo_button->set_text(TTR("Redo"));
	redo_button->set_focus_mode(Control::FOCUS_NONE);
	redo_button->set_icon_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	redo_button->connect(SceneStringName(pressed), callable_mp(this, &TouchActionsPanel::_simulate_action).bind("ui_redo"));
	hbox->add_child(redo_button);
}
