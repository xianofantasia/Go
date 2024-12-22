/**************************************************************************/
/*  color_channel_selector.cpp                                            */
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

#include "color_channel_selector.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/resources/style_box_flat.h"

ColorChannelSelector::ColorChannelSelector() :
		buttons() {
	HBoxContainer *container = memnew(HBoxContainer);
	container->add_theme_constant_override("separation", 0);

	create_button(0, "R", container);
	create_button(1, "G", container);
	create_button(2, "B", container);
	create_button(3, "A", container);

	add_child(container);
}

void ColorChannelSelector::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		// It is a common pattern in the editor to use `NOTIFICATION_THEME_CHANGED` to setup theme overrides that need
		// access to the editor's theme. Unfortunately, in our case, this leads to infinite recursion.
		if (_setting_up_theme) {
			return;
		}
		_setting_up_theme = true;
		// PanelContainer's background is invisible in the editor. We need a background.
		// And we need this in turn because buttons don't look good without background (for example, hover is transparent).
		Ref<StyleBox> bg_style = get_theme_stylebox("panel", "TabContainer");
		ERR_FAIL_COND(bg_style.is_null());
		bg_style = bg_style->duplicate();
		// The default content margin makes the widget become a bit too large. It should be like mini-toolbar.
		const float editor_scale = EditorScale::get_scale();
		bg_style->set_content_margin(SIDE_LEFT, 1.f * editor_scale);
		bg_style->set_content_margin(SIDE_RIGHT, 1.f * editor_scale);
		bg_style->set_content_margin(SIDE_TOP, 1.f * editor_scale);
		bg_style->set_content_margin(SIDE_BOTTOM, 1.f * editor_scale);
		add_theme_style_override("panel", bg_style);
		_setting_up_theme = false;
	}
}

void ColorChannelSelector::set_available_channels_mask(const uint32_t mask) {
	for (unsigned int i = 0; i < CHANNEL_COUNT; ++i) {
		const bool available = (mask & (1 << i)) != 0;
		Button *button = buttons[i];
		button->set_visible(available);
	}
}

void ColorChannelSelector::on_button_toggled(const bool unused_pressed) {
	emit_signal("selected_channels_changed");
}

uint32_t ColorChannelSelector::get_selected_channels_mask() const {
	uint32_t mask = 0;
	for (unsigned int i = 0; i < CHANNEL_COUNT; ++i) {
		Button *button = buttons[i];
		if (button->is_visible() && buttons[i]->is_pressed()) {
			mask |= (1 << i);
		}
	}
	return mask;
}

// Helper
Vector4 ColorChannelSelector::get_selected_channel_factors() const {
	Vector4 channel_factors;
	const uint32_t mask = get_selected_channels_mask();
	for (unsigned int i = 0; i < 4; ++i) {
		if ((mask & (1 << i)) != 0) {
			channel_factors[i] = 1;
		}
	}
	return channel_factors;
}

void ColorChannelSelector::create_button(const unsigned int p_channel_index, const String p_text, Control *p_parent) {
	CRASH_COND(p_channel_index >= CHANNEL_COUNT);
	CRASH_COND(buttons[p_channel_index] != nullptr);
	Button *button = memnew(Button);
	button->set_text(p_text);
	button->set_toggle_mode(true);
	button->set_pressed(true);
	// button->set_flat(true);

	// Don't show focus, it stands out too much and remains visible which can be confusing
	button->add_theme_style_override("focus", memnew(StyleBoxEmpty()));

	// Make it look similar to toolbar buttons
	button->set_theme_type_variation("FlatButton");

	button->connect("toggled", callable_mp(this, &ColorChannelSelector::on_button_toggled));
	p_parent->add_child(button);
	buttons[p_channel_index] = button;
}

void ColorChannelSelector::_bind_methods() {
	ADD_SIGNAL(MethodInfo("selected_channels_changed"));
}
