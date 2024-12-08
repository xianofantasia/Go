/**************************************************************************/
/*  span.h                                                                */
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

#ifndef SPAN_H
#define SPAN_H

#include "core/typedefs.h"

// Equivalent of std::span.
// Represents a view into a contiguous memory space.
// DISCLAIMER: This data type does not own the underlying buffer. DO NOT STORE IT.
//  Additionally, for the lifetime of the Span, do not resize the buffer, and do not insert or remove elements from it.
//  Failure to respect this may lead to crashes or undefined behavior.
template <typename T>
class Span {
	T *_ptr = nullptr;
	size_t _len = 0;

public:
	Span() = default;
	Span(T *p_ptr, size_t p_len) :
			_ptr(p_ptr), _len(p_len) {}

	// Copy semantics are explicitly disabled, because Span should usually not be stored.
	// When passing around Span instances across functions, use references to emphasize it's not owned.
	Span(const Span &p_span) = delete;
	void operator=(const Span &span) = delete;

	// A ptrw() call explicitly asks for write access.
	// As such, it makes no sense to call it on a read-only span (e.g. Span<const int>).
	// To help reduce confusion, it is disabled with SFINAE if the element type is const.
	std::enable_if<std::negation_v<std::is_const<T>>, T> *ptrw() { return _ptr; }
	const T *ptr() const { return _ptr; }

	T &operator[](int i) { return _ptr[i]; }
	const T &operator[](int i) const { return _ptr[i]; }

	size_t size() const { return _len; }
	bool is_empty() const { return _len == 0; }

	// Algorithms.
	size_t find(const T &p_val, std::ptrdiff_t p_from = 0) const;
	size_t rfind(const T &p_val, std::ptrdiff_t p_from = -1) const;
	size_t count(const T &p_val) const;
};

template <typename T>
size_t Span<T>::find(const T &p_val, std::ptrdiff_t p_from) const {
	if (p_from < 0 || _len == 0) {
		return -1;
	}

	for (size_t i = p_from; i < _len; i++) {
		if (_ptr[i] == p_val) {
			return i;
		}
	}

	return -1;
}

template <typename T>
size_t Span<T>::rfind(const T &p_val, std::ptrdiff_t p_from) const {
	if (p_from < 0) {
		p_from = _len + p_from;
	}
	if (p_from < 0 || p_from >= (std::ptrdiff_t)_len) {
		p_from = _len - 1;
	}

	for (std::ptrdiff_t i = p_from; i >= 0; i--) {
		if (_ptr[i] == p_val) {
			return i;
		}
	}
	return -1;
}

template <typename T>
size_t Span<T>::count(const T &p_val) const {
	size_t amount = 0;
	for (size_t i = 0; i < _len; i++) {
		if (_ptr[i] == p_val) {
			amount++;
		}
	}
	return amount;
}

#endif // SPAN_H
