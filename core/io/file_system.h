/**************************************************************************/
/*  file_system.h                                                         */
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

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "core/io/file_access.h"
#include "core/object/ref_counted.h"
#include "file_system_protocol.h"

class FileSystem : public Object {
	GDCLASS(FileSystem, Object);

protected:
	static void _bind_methods();

private:
	static FileSystem *singleton;

	HashMap<String, Ref<FileSystemProtocol>> protocols;

	// returns whether it found a protocol delimiter
	static bool split_path(const String &p_path, String *r_protocol_name, String *r_file_path);
	// for paths that doesn't have the protocol part, it fallsback to host://
	// if r_protocol returns null, the protocol of the path targeted is invalid.
	void process_path(const String &p_path, String *r_protocol_name, Ref<FileSystemProtocol> *r_protocol, String *r_file_path) const;

	Error open_error;
	Ref<FileAccess> _open_file(const String &p_path, int p_mode_flags);

public:
	static String protocol_name_os;
	static String protocol_name_resources;
	static String protocol_name_user_data;
    static String protocol_name_memory;

	FileSystem();
	~FileSystem();
	static FileSystem *get_singleton();

	bool protocol_exists(const String &p_protocol) const;
	bool add_protocol(const String &p_name, const Ref<FileSystemProtocol> &p_protocol);
	bool remove_protocol(const String &p_name);
	Ref<FileSystemProtocol> get_protocol(const String &p_name) const;
	Ref<FileSystemProtocol> get_protocol_or_null(const String &p_name) const;

	Error get_open_error() const;

	Ref<FileAccess> open_file(const String &p_path, int p_mode_flags, Error *r_error) const;
	bool file_exists(const String &p_path) const;
};

#endif // FILE_SYSTEM_H