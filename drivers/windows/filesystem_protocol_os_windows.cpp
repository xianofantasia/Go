/**************************************************************************/
/*  filesystem_protocol_os_windows.h                                      */
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

#ifdef WINDOWS_ENABLED

#include "filesystem_protocol_os_windows.h"

#include <share.h> // _SH_DENYNO
#include <shlwapi.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

HashSet<String> FileSystemProtocolOSWindows::invalid_files;
void FileSystemProtocolOSWindows::initialize() {
	static const char *reserved_files[]{
		"con", "prn", "aux", "nul", "com0", "com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9", "lpt0", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9", nullptr
	};
	int reserved_file_index = 0;
	while (reserved_files[reserved_file_index] != nullptr) {
		invalid_files.insert(reserved_files[reserved_file_index]);
		reserved_file_index++;
	}
}
void FileSystemProtocolOSWindows::finalize() {
	invalid_files.clear();
}

bool FileSystemProtocolOSWindows::is_path_invalid(const String &p_path) {
	// Check for invalid operating system file.
	String fname = p_path.get_file().to_lower();

	int dot = fname.find(".");
	if (dot != -1) {
		fname = fname.substr(0, dot);
	}
	return invalid_files.has(fname);
}

String FileSystemProtocolOSWindows::fix_path(const String &p_path) {
	String r_path = p_path;

	if (r_path.is_relative_path()) {
		Char16String current_dir_name;
		size_t str_len = GetCurrentDirectoryW(0, nullptr);
		current_dir_name.resize(str_len + 1);
		GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());
		r_path = String::utf16((const char16_t *)current_dir_name.get_data()).trim_prefix(R"(\\?\)").replace("\\", "/").path_join(r_path);
	}
	r_path = r_path.simplify_path();
	r_path = r_path.replace("/", "\\");
	if (!r_path.is_network_share_path() && !r_path.begins_with(R"(\\?\)")) {
		r_path = R"(\\?\)" + r_path;
	}
	return r_path;
}

bool FileSystemProtocolOSWindows::file_exists_static(const String &p_path) {
	if (is_path_invalid(p_path)) {
		return false;
	}

	String filename = fix_path(p_path);
	FILE *g = _wfsopen((LPCWSTR)(filename.utf16().get_data()), L"rb", _SH_DENYNO);
	if (g == nullptr) {
		return false;
	} else {
		fclose(g);
		return true;
	}
}

Ref<FileAccess> FileSystemProtocolOSWindows::open_file(const String &p_path, int p_mode_flags, Error *r_error) const {
	*r_error = ERR_UNAVAILABLE;
	return Ref<FileAccess>();
}
bool FileSystemProtocolOSWindows::file_exists(const String &p_path) const {
	return file_exists_static(p_path);
}
#endif // WINDOWS_ENABLED
