/**************************************************************************/
/*  filesystem.cpp                                                        */
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

#include "filesystem.h"

FileSystem *FileSystem::singleton = nullptr;
FileSystem *FileSystem::get_singleton() {
	return singleton;
}
FileSystem::FileSystem() {
	singleton = this;
}
FileSystem::~FileSystem() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

String FileSystem::protocol_name_os = "os";
String FileSystem::protocol_name_resources = "res";
String FileSystem::protocol_name_user_data = "user";
String FileSystem::protocol_name_memory = "mem";

bool FileSystem::protocol_exists(const String &p_name) const {
	return protocols.has(p_name);
}
bool FileSystem::remove_protocol(const String &p_name) {
	bool erased = protocols.erase(p_name);
	ERR_FAIL_COND_V_MSG(!erased, false, "No FileSystemProtocol with the name " + p_name + " is registered.");
	return erased;
}
bool FileSystem::add_protocol(const String &p_name, const Ref<FileSystemProtocol> &p_protocol) {
	ERR_FAIL_COND_V_MSG(protocols.has(p_name), false, "FileSystemProtocol with the name " + p_name + " is already registered.");
	protocols[p_name] = p_protocol;
	return true;
}
Ref<FileSystemProtocol> FileSystem::get_protocol_or_null(const String &p_name) const {
	HashMap<String, Ref<FileSystemProtocol>>::ConstIterator iter = protocols.find(p_name);
	if (!iter) {
		return Ref<FileSystemProtocol>();
	}
	return iter->value;
}
Ref<FileSystemProtocol> FileSystem::get_protocol(const String &p_name) const {
	Ref<FileSystemProtocol> protocol = get_protocol_or_null(p_name);
	ERR_FAIL_COND_V_MSG(protocol.is_null(), Ref<FileSystemProtocol>(), "FileSystemProtocol with name " + p_name + " doesn't exist!");
	return protocol;
}
bool FileSystem::split_path(const String &p_path, String *r_protocol_name, String *r_file_path) {
	int index = p_path.find("://");
	if (index < 0) {
		*r_protocol_name = "";
		*r_file_path = p_path;
		return false;
	}

	*r_protocol_name = p_path.substr(0, index);
	*r_file_path = p_path.substr(index + 3);
	return true;
}

void FileSystem::process_path(const String &p_path, String *r_protocol_name, Ref<FileSystemProtocol> *r_protocol, String *r_file_path) const {
	bool has_protcol_part = split_path(p_path, r_protocol_name, r_file_path);
	if (!has_protcol_part) {
		*r_protocol_name = protocol_name_os;
	}
	*r_protocol = get_protocol_or_null(*r_protocol_name);
	return;
}

Ref<FileAccess> FileSystem::open_file(const String &p_path, int p_mode_flags, Error *r_error) const {
	String protocol_name = String();
	Ref<FileSystemProtocol> protocol = Ref<FileSystemProtocol>();
	String file_path = String();
	process_path(p_path, &protocol_name, &protocol, &file_path);
	ERR_FAIL_COND_V_MSG(protocol.is_null(), Ref<FileAccess>(), "Unknown filesystem protocol " + protocol_name);

	return protocol->open_file(file_path, p_mode_flags, r_error);
}
bool FileSystem::file_exists(const String &p_path) const {
	String protocol_name = String();
	Ref<FileSystemProtocol> protocol = Ref<FileSystemProtocol>();
	String file_path = String();
	process_path(p_path, &protocol_name, &protocol, &file_path);
	ERR_FAIL_COND_V_MSG(protocol.is_null(), false, "Unknown filesystem protocol " + protocol_name);

	return protocol->file_exists(file_path);
}

Error FileSystem::get_open_error() const {
	return open_error;
}
Ref<FileAccess> FileSystem::_open_file(const String &p_path, int p_mode_flags) {
	return open_file(p_path, p_mode_flags, &open_error);
}

void FileSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("protocol_exists", "name"), &FileSystem::protocol_exists);
	ClassDB::bind_method(D_METHOD("remove_protocol", "name"), &FileSystem::remove_protocol);
	ClassDB::bind_method(D_METHOD("add_protocol", "name", "protocol"), &FileSystem::add_protocol);
	ClassDB::bind_method(D_METHOD("get_protocol_or_null", "name"), &FileSystem::get_protocol_or_null);
	ClassDB::bind_method(D_METHOD("get_protocol", "name"), &FileSystem::get_protocol);

	ClassDB::bind_method(D_METHOD("get_open_error"), &FileSystem::get_open_error);
	ClassDB::bind_method(D_METHOD("open_file", "path", "mode_flags"), &FileSystem::_open_file);
	ClassDB::bind_method(D_METHOD("file_exists", "name"), &FileSystem::file_exists);
}