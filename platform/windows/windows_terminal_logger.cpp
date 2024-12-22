/**************************************************************************/
/*  windows_terminal_logger.cpp                                           */
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

#include "windows_terminal_logger.h"

#ifdef WINDOWS_ENABLED

#include "core/os/os.h"

#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void WindowsTerminalLogger::logv(const char *p_format, va_list p_list, bool p_err) {
	if (!should_log(p_err)) {
		return;
	}

	const int static_buffer_size = 1024;
	char static_buf[static_buffer_size];
	char *buf = static_buf;
	va_list list_copy;
	va_copy(list_copy, p_list);
	int len = vsnprintf(buf, static_buffer_size, p_format, p_list);
	if (len >= static_buffer_size) {
		buf = (char *)memalloc(len + 1);
		len = vsnprintf(buf, len + 1, p_format, list_copy);
	}
	va_end(list_copy);

	String str_buf = String::utf8(buf, len).replace("\r\n", "\n").replace("\n", "\r\n");
	if (len >= static_buffer_size) {
		memfree(buf);
	}
	CharString cstr_buf = str_buf.utf8();
	if (cstr_buf.length() == 0) {
		return;
	}

	DWORD written = 0;
	HANDLE h = p_err ? GetStdHandle(STD_ERROR_HANDLE) : GetStdHandle(STD_OUTPUT_HANDLE);
	WriteFile(h, cstr_buf.ptr(), cstr_buf.length(), &written, nullptr);

#ifdef DEBUG_ENABLED
	FlushFileBuffers(h);
#endif
}

void WindowsTerminalLogger::log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, bool p_editor_notify, ErrorType p_type) {
	if (!should_log(true)) {
		return;
	}

	const char *err_details = p_rationale && p_rationale[0] ? p_rationale : p_code;

	// Disable color codes if stdout is not a TTY or CI.
	// This prevents Godot from writing ANSI escape codes when redirecting
	// stdout and stderr to a file.
	const bool color = OS::get_singleton()->has_environment("CI") || OS::get_singleton()->get_stdout_type() == OS::STD_HANDLE_CONSOLE;
	const char *gray = color ? "\u001b[0;90m" : "";
	const char *red = color ? "\u001b[0;31m" : "";
	const char *red_bold = color ? "\u001b[1;31m" : "";
	const char *yellow = color ? "\u001b[0;33m" : "";
	const char *yellow_bold = color ? "\u001b[1;33m" : "";
	const char *magenta = color ? "\u001b[0;35m" : "";
	const char *magenta_bold = color ? "\u001b[1;35m" : "";
	const char *cyan = color ? "\u001b[0;36m" : "";
	const char *cyan_bold = color ? "\u001b[1;36m" : "";
	const char *reset = color ? "\u001b[0m" : "";

	switch (p_type) {
		case ERR_WARNING:
			logf_error("%sWARNING:%s %s\n", yellow_bold, yellow, err_details);
			logf_error("%s     at: %s (%s:%i)%s\n", gray, p_function, p_file, p_line, reset);
			break;
		case ERR_SCRIPT:
			logf_error("%sSCRIPT ERROR:%s %s\n", magenta_bold, magenta, err_details);
			logf_error("%s          at: %s (%s:%i)%s\n", gray, p_function, p_file, p_line, reset);
			break;
		case ERR_SHADER:
			logf_error("%sSHADER ERROR:%s %s\n", cyan_bold, cyan, err_details);
			logf_error("%s          at: %s (%s:%i)%s\n", gray, p_function, p_file, p_line, reset);
			break;
		case ERR_ERROR:
		default:
			logf_error("%sERROR:%s %s\n", red_bold, red, err_details);
			logf_error("%s   at: %s (%s:%i)%s\n", gray, p_function, p_file, p_line, reset);
			break;
	}
}

WindowsTerminalLogger::~WindowsTerminalLogger() {}

#endif // WINDOWS_ENABLED
