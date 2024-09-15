/**************************************************************************/
/*  summary_view.cpp                                                      */
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

#include "summary_view.h"

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "core/os/time.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "modules/gdscript/gdscript.h"
#include "scene/gui/button.h"
#include "scene/gui/center_container.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "scene/resources/style_box_flat.h"

#include "../snapshot_data.h"

SnapshotSummaryView::SnapshotSummaryView() {
	set_name("Summary");

	set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

	MarginContainer *mc = memnew(MarginContainer);
	mc->add_theme_constant_override("margin_left", 5);
	mc->add_theme_constant_override("margin_right", 5);
	mc->add_theme_constant_override("margin_top", 5);
	mc->add_theme_constant_override("margin_bottom", 5);
	mc->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
	PanelContainer *content_wrapper = memnew(PanelContainer);
	content_wrapper->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
	StyleBoxFlat *content_wrapper_sbf = memnew(StyleBoxFlat);
	content_wrapper_sbf->set_bg_color(EditorNode::get_singleton()->get_editor_theme()->get_color("dark_color_2", "Editor"));
	content_wrapper->add_theme_style_override("panel", content_wrapper_sbf);
	content_wrapper->add_child(mc);
	add_child(content_wrapper);

	VBoxContainer *content = memnew(VBoxContainer);
	mc->add_child(content);
	content->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);

	// Title
	PanelContainer *pc = memnew(PanelContainer);

	// Tree / title_button_color
	StyleBoxFlat *sbf = memnew(StyleBoxFlat);
	sbf->set_bg_color(EditorNode::get_singleton()->get_editor_theme()->get_color("dark_color_3", "Editor"));
	pc->add_theme_style_override("panel", sbf);
	content->add_child(pc);
	pc->set_anchors_preset(LayoutPreset::PRESET_TOP_WIDE);
	Label *title = memnew(Label("ObjectDB Snapshot Summary"));
	pc->add_child(title);
	title->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	title->set_vertical_alignment(VerticalAlignment::VERTICAL_ALIGNMENT_CENTER);

	explainer_text = memnew(CenterContainer);
	explainer_text->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	explainer_text->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	content->add_child(explainer_text);
	VBoxContainer *explainer_lines = memnew(VBoxContainer);
	explainer_text->add_child(explainer_lines);
	Label *l1 = memnew(Label("No snapshot selected. Press 'Take ObjectDB Snapshot' to snapshot game Objects."));
	Label *l2 = memnew(Label("ObjectDB Snapshots capture all Objects and Properties in a game."));
	Label *l3 = memnew(Label("Not all memory in Godot is exposed as an Object or Property, so ObjectDB Snapshots are a partial view of a game's memory."));
	l1->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	l2->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	l3->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	explainer_lines->add_child(l1);
	explainer_lines->add_child(l2);
	explainer_lines->add_child(l3);

	ScrollContainer *sc = memnew(ScrollContainer);
	sc->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
	sc->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	sc->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	content->add_child(sc);

	blurb_list = memnew(VBoxContainer);
	sc->add_child(blurb_list);
	blurb_list->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	blurb_list->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
}

void SnapshotSummaryView::show_snapshot(GameStateSnapshot *p_data, GameStateSnapshot *p_diff_data) {
	SnapshotView::show_snapshot(p_data, p_diff_data);
	explainer_text->set_visible(false);

	object_blurb = memnew(SummaryBlurb("Objects", "Objects with scripts not referenced by any user objects [i](a user created object has no connections to other objects)[/i]"));
	node_blurb = memnew(SummaryBlurb("Nodes", "Multiple root nodes [i](possible call to 'remove_child' without 'queue_free')[/i]"));
	refcounted_blurb = memnew(SummaryBlurb("RefCounted", "RefCounted only referenced in cycles [i](cycles often indicate a memory leaks)[/i]"));

	for (const KeyValue<ObjectID, SnapshotDataObject *> &pair : snapshot_data->Data) {
		if (pair.value->inbound_references.size() == 0 && pair.value->outbound_references.size() == 0) {
			if (pair.value->get_script() != nullptr) {
				object_blurb->add_line("ObjectID: " + String::num_uint64(pair.value->remote_object_id));
			}
		}
		if (pair.value->is_refcounted()) {
			int ref_count = (uint64_t)pair.value->extra_debug_data["ref_count"];
			Array ref_cycles = (Array)pair.value->extra_debug_data["ref_cycles"];

			if (ref_count == ref_cycles.size()) {
				refcounted_blurb->add_line("ObjectID: " + String::num_uint64(pair.key));
			}
		}
		// if it's a node AND it doesn't have a parent node
		if (pair.value->is_node() && !pair.value->extra_debug_data.has("node_parent")) {
			node_blurb->add_line(pair.value->extra_debug_data["node_name"]);
		}
	}
	object_blurb->end_list();
	node_blurb->end_list();
	refcounted_blurb->end_list();

	blurb_list->add_child(object_blurb);
	blurb_list->add_child(node_blurb);
	blurb_list->add_child(refcounted_blurb);
}

void SnapshotSummaryView::clear_snapshot() {
	// just clear out the blurbs and leave the summary
	for (int i = 0; i < blurb_list->get_child_count(); i++) {
		blurb_list->get_child(i)->queue_free();
	}
	object_blurb = nullptr;
	node_blurb = nullptr;
	refcounted_blurb = nullptr;
	explainer_text->set_visible(true);
}

SummaryBlurb::SummaryBlurb(const String &blurb_name, const String &blurb_description, bool open_list) {
	add_theme_constant_override("margin_left", 2);
	add_theme_constant_override("margin_right", 2);
	add_theme_constant_override("margin_top", 2);
	add_theme_constant_override("margin_bottom", 2);

	label = memnew(RichTextLabel);
	label->set_fit_content(true);
	label->set_use_bbcode(true);
	label->push_underline();
	label->add_text(blurb_name);
	label->pop();
	label->add_newline();
	label->append_text(blurb_description);
	add_child(label);

	set_visible(false);

	if (open_list) {
		start_list();
	}
}

void SummaryBlurb::start_list() {
	label->push_list(0, RichTextLabel::ListType::LIST_DOTS, false);
}

void SummaryBlurb::add_line(const String &line) {
	label->add_text(line);
	label->add_newline();
	set_visible(true);
}

void SummaryBlurb::end_list() {
	label->pop();
}
