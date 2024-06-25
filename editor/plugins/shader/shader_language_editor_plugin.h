/**************************************************************************/
/*  shader_language_editor_plugin.h                                       */
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

#ifndef SHADER_LANGUAGE_EDITOR_PLUGIN_H
#define SHADER_LANGUAGE_EDITOR_PLUGIN_H

#include "scene/gui/control.h"
#include "scene/resources/shader.h"

class ShaderLanguageEditorBase : public Control {
	GDCLASS(ShaderLanguageEditorBase, Control);

public:
	virtual void edit_shader(const Ref<Shader> &p_shader) = 0;
	virtual void edit_shader_include(const Ref<ShaderInclude> &p_shader_inc){};

	virtual void apply_shaders() = 0;
	virtual bool is_unsaved() const = 0;
	virtual void save_external_data(const String &p_str = "") = 0;
	virtual void validate_script() = 0;
};

class ShaderLanguageEditorPlugin : public RefCounted {
	GDCLASS(ShaderLanguageEditorPlugin, RefCounted);

	static Vector<Ref<ShaderLanguageEditorPlugin>> shader_languages;
	static Vector<Vector2i> language_variation_map;

public:
	static void register_shader_language(const Ref<ShaderLanguageEditorPlugin> &p_shader_language) {
		// Allows one ShaderLanguageEditorPlugin to provide multiple dropdown options in
		// the language selection menu. For example, ShaderInclude is handled this way.
		// X is the plugin index, and Y is the language variation index.
		for (int i = 0; i < p_shader_language->get_language_variations().size(); i++) {
			language_variation_map.push_back(Vector2i(shader_languages.size(), i));
		}
		shader_languages.push_back(p_shader_language);
	}

	static const Vector<Ref<ShaderLanguageEditorPlugin>> get_shader_languages_read_only() {
		return shader_languages;
	}

	static int get_shader_language_variation_count() {
		return language_variation_map.size();
	}

	static Ref<ShaderLanguageEditorPlugin> get_shader_language_for_index(int p_index) {
		ERR_FAIL_INDEX_V(p_index, language_variation_map.size(), nullptr);
		Vector2i lang_var_indices = language_variation_map[p_index];
		return shader_languages[lang_var_indices.x];
	}

	static String get_file_extension_for_index(int p_index) {
		ERR_FAIL_INDEX_V(p_index, language_variation_map.size(), "tres");
		Vector2i lang_var_indices = language_variation_map[p_index];
		ShaderLanguageEditorPlugin *lang = shader_languages[lang_var_indices.x].ptr();
		return lang->get_file_extension(lang_var_indices.y);
	}

	virtual bool handles_shader(const Ref<Shader> &p_shader) const = 0;
	virtual bool handles_shader_include(const Ref<ShaderInclude> &p_shader_inc) const { return false; }
	virtual ShaderLanguageEditorBase *edit_shader(const Ref<Shader> &p_shader) = 0;
	virtual ShaderLanguageEditorBase *edit_shader_include(const Ref<ShaderInclude> &p_shader_inc) { return nullptr; }
	virtual Ref<Shader> create_new_shader(int p_variation_index, Shader::Mode p_shader_mode, int p_template_index) = 0;
	virtual Ref<ShaderInclude> create_new_shader_include() { return Ref<ShaderInclude>(); }
	virtual PackedStringArray get_language_variations() const = 0;
	virtual String get_file_extension(int p_variation_index) const { return "tres"; }
};

#endif // SHADER_LANGUAGE_EDITOR_PLUGIN_H
