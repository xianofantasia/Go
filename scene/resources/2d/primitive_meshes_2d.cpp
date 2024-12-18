/**************************************************************************/
/*  primitive_meshes_2d.cpp                                               */
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

#include "primitive_meshes_2d.h"

#include "core/math/math_funcs.h"
#include "scene/resources/theme.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering_server.h"
#include "thirdparty/misc/polypartition.h"

/**
  PrimitiveMesh2D
*/
void PrimitiveMesh2D::_update() const {
	Array arr;
	if (GDVIRTUAL_CALL(_create_mesh_array, arr)) {
		ERR_FAIL_COND_MSG(arr.size() != RS::ARRAY_MAX, "_create_mesh_array must return an array of Mesh.ARRAY_MAX elements.");
	} else {
		arr.resize(RS::ARRAY_MAX);
		_create_mesh_array(arr);
	}

	Vector<Vector2> points = arr[RS::ARRAY_VERTEX];

	ERR_FAIL_COND_MSG(points.is_empty(), "_create_mesh_array must return at least a vertex array.");

	aabb = AABB();

	int pc = points.size();
	ERR_FAIL_COND(pc == 0);
	{
		const Vector2 *r = points.ptr();
		for (int i = 0; i < pc; i++) {
			if (i == 0) {
				aabb.position = Vector3(r[i].x, r[i].y, 0);
			} else {
				aabb.expand_to(Vector3(r[i].x, r[i].y, 0));
			}
		}
	}

	array_len = pc;

	Vector<int> indices = arr[RS::ARRAY_INDEX];
	index_array_len = indices.size();

	// Out with the old.
	RenderingServer::get_singleton()->mesh_clear(mesh);
	// In with the new.
	RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh, (RenderingServer::PrimitiveType)primitive_type, arr);

	pending_request = false;

	clear_cache();

	const_cast<PrimitiveMesh2D *>(this)->emit_changed();
}

void PrimitiveMesh2D::request_update() {
	if (pending_request) {
		return;
	}
	_update();
}

int PrimitiveMesh2D::get_surface_count() const {
	if (pending_request) {
		_update();
	}
	return 1;
}

int PrimitiveMesh2D::surface_get_array_len(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, 1, -1);
	if (pending_request) {
		_update();
	}

	return array_len;
}

int PrimitiveMesh2D::surface_get_array_index_len(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, 1, -1);
	if (pending_request) {
		_update();
	}

	return index_array_len;
}

Array PrimitiveMesh2D::surface_get_arrays(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, 1, Array());
	if (pending_request) {
		_update();
	}

	return RenderingServer::get_singleton()->mesh_surface_get_arrays(mesh, 0);
}

Dictionary PrimitiveMesh2D::surface_get_lods(int p_surface) const {
	return Dictionary(); // Not really supported.
}

TypedArray<Array> PrimitiveMesh2D::surface_get_blend_shape_arrays(int p_surface) const {
	return TypedArray<Array>(); // Not really supported.
}

BitField<Mesh::ArrayFormat> PrimitiveMesh2D::surface_get_format(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, 1, 0);

	uint64_t mesh_format = RS::ARRAY_FORMAT_VERTEX | RS::ARRAY_FORMAT_TEX_UV | RS::ARRAY_FORMAT_INDEX | RS::ARRAY_FLAG_USE_2D_VERTICES;

	return mesh_format;
}

Mesh::PrimitiveType PrimitiveMesh2D::surface_get_primitive_type(int p_idx) const {
	return primitive_type;
}

void PrimitiveMesh2D::surface_set_material(int p_idx, const Ref<Material> &p_material) {
	ERR_FAIL_INDEX(p_idx, 1);
	// Not used. The CanvasItem material is used instead.
}

Ref<Material> PrimitiveMesh2D::surface_get_material(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, 1, nullptr);
	// Not used. The CanvasItem material is used instead.
	return nullptr;
}

int PrimitiveMesh2D::get_blend_shape_count() const {
	return 0;
}

StringName PrimitiveMesh2D::get_blend_shape_name(int p_index) const {
	return StringName();
}

void PrimitiveMesh2D::set_blend_shape_name(int p_index, const StringName &p_name) {
}

AABB PrimitiveMesh2D::get_aabb() const {
	if (pending_request) {
		_update();
	}

	return aabb;
}

RID PrimitiveMesh2D::get_rid() const {
	if (pending_request) {
		_update();
	}
	return mesh;
}

void PrimitiveMesh2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_mesh_arrays"), &PrimitiveMesh2D::get_mesh_arrays);

	ClassDB::bind_method(D_METHOD("set_custom_aabb", "aabb"), &PrimitiveMesh2D::set_custom_aabb);
	ClassDB::bind_method(D_METHOD("get_custom_aabb"), &PrimitiveMesh2D::get_custom_aabb);

	ClassDB::bind_method(D_METHOD("request_update"), &PrimitiveMesh2D::request_update);

	ADD_PROPERTY(PropertyInfo(Variant::AABB, "custom_aabb", PROPERTY_HINT_NONE, "suffix:px"), "set_custom_aabb", "get_custom_aabb");

	GDVIRTUAL_BIND(_create_mesh_array);
}

void PrimitiveMesh2D::_validate_property(PropertyInfo &p_property) const {
	// Remove unused lightmap size hint.
	if (p_property.name == "lightmap_size_hint") {
		p_property.usage = PROPERTY_USAGE_NONE;
	}
}

Array PrimitiveMesh2D::get_mesh_arrays() const {
	return surface_get_arrays(0);
}

void PrimitiveMesh2D::set_custom_aabb(const AABB &p_custom) {
	if (p_custom.is_equal_approx(custom_aabb)) {
		return;
	}
	custom_aabb = p_custom;
	RS::get_singleton()->mesh_set_custom_aabb(mesh, custom_aabb);
	emit_changed();
}

AABB PrimitiveMesh2D::get_custom_aabb() const {
	return custom_aabb;
}

PrimitiveMesh2D::PrimitiveMesh2D() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	mesh = RenderingServer::get_singleton()->mesh_create();
}

PrimitiveMesh2D::~PrimitiveMesh2D() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RenderingServer::get_singleton()->free(mesh);
}

/**
  RectangleMesh2D
*/

void RectangleMesh2D::_create_mesh_array(Array &p_arr) const {
	int i, j, prevrow, thisrow, point;
	float x, y;

	Vector2 start_pos = size * -0.5;

	Vector<Vector2> points;
	Vector<Vector2> uvs;
	Vector<int> indices;
	point = 0;

	y = start_pos.y;
	thisrow = point;
	prevrow = 0;
	for (j = 0; j <= (subdivide_h + 1); j++) {
		x = start_pos.x;
		for (i = 0; i <= (subdivide_w + 1); i++) {
			float u = i;
			float v = j;
			u /= (subdivide_w + 1.0);
			v /= (subdivide_h + 1.0);

			points.push_back(Vector2(-x, y));
			uvs.push_back(Vector2(1.0 - u, v));
			point++;

			if (i > 0 && j > 0) {
				indices.push_back(prevrow + i - 1);
				indices.push_back(prevrow + i);
				indices.push_back(thisrow + i - 1);
				indices.push_back(prevrow + i);
				indices.push_back(thisrow + i);
				indices.push_back(thisrow + i - 1);
			}

			x += size.x / (subdivide_w + 1.0);
		}

		y += size.y / (subdivide_h + 1.0);
		prevrow = thisrow;
		thisrow = point;
	}

	p_arr[RS::ARRAY_VERTEX] = points;
	p_arr[RS::ARRAY_TEX_UV] = uvs;
	p_arr[RS::ARRAY_INDEX] = indices;
}

void RectangleMesh2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_size", "size"), &RectangleMesh2D::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &RectangleMesh2D::get_size);

	ClassDB::bind_method(D_METHOD("set_subdivide_width", "subdivide"), &RectangleMesh2D::set_subdivide_width);
	ClassDB::bind_method(D_METHOD("get_subdivide_width"), &RectangleMesh2D::get_subdivide_width);
	ClassDB::bind_method(D_METHOD("set_subdivide_height", "divisions"), &RectangleMesh2D::set_subdivide_height);
	ClassDB::bind_method(D_METHOD("get_subdivide_height"), &RectangleMesh2D::get_subdivide_height);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "size", PROPERTY_HINT_NONE, "suffix:px"), "set_size", "get_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "subdivide_width", PROPERTY_HINT_RANGE, "0,100,1,or_greater"), "set_subdivide_width", "get_subdivide_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "subdivide_height", PROPERTY_HINT_RANGE, "0,100,1,or_greater"), "set_subdivide_height", "get_subdivide_height");
}

void RectangleMesh2D::set_size(const Vector2 &p_size) {
	if (p_size.is_equal_approx(size)) {
		return;
	}

	size = p_size;
	request_update();
}

Vector2 RectangleMesh2D::get_size() const {
	return size;
}

void RectangleMesh2D::set_subdivide_width(const int p_divisions) {
	if (p_divisions == subdivide_w) {
		return;
	}

	subdivide_w = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int RectangleMesh2D::get_subdivide_width() const {
	return subdivide_w;
}

void RectangleMesh2D::set_subdivide_height(const int p_divisions) {
	if (p_divisions == subdivide_h) {
		return;
	}

	subdivide_h = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int RectangleMesh2D::get_subdivide_height() const {
	return subdivide_h;
}

RectangleMesh2D::RectangleMesh2D() {}

/**
  CircleMesh2D
*/

void CircleMesh2D::_create_mesh_array(Array &p_arr) const {
	int i, thisrow, point;
	float x, y, u, v;

	Vector<Vector2> points;
	Vector<Vector2> uvs;
	Vector<int> indices;
	point = 0;

	float circle_radius = radius;
	if (radial_segments == 3) {
		// Make sure the triangle respects the given radius.
		circle_radius *= 2.0f / Math::sqrt(3.0f);
	}

	if (radius > 0.0) {
		thisrow = point;
		points.push_back(Vector2(0.0, 0.0));
		uvs.push_back(Vector2(0.5, 0.5));
		point++;

		for (i = 0; i <= radial_segments; i++) {
			float r = i;
			r /= radial_segments;

			if (i == radial_segments) {
				x = 0.0;
				y = 1.0;
			} else {
				x = Math::sin(r * Math_TAU);
				y = Math::cos(r * Math_TAU);
			}

			u = (x + 1.0) * 0.5;
			v = (y + 1.0) * 0.5;

			Vector2 p = Vector2(x * circle_radius, y * circle_radius);
			points.push_back(p);
			uvs.push_back(Vector2(u, v));
			point++;

			if (i > 0) {
				indices.push_back(thisrow);
				indices.push_back(point - 2);
				indices.push_back(point - 1);
			}
		}
	}

	p_arr[RS::ARRAY_VERTEX] = points;
	p_arr[RS::ARRAY_TEX_UV] = uvs;
	p_arr[RS::ARRAY_INDEX] = indices;
}

void CircleMesh2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &CircleMesh2D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &CircleMesh2D::get_radius);
	ClassDB::bind_method(D_METHOD("set_radial_segments", "segments"), &CircleMesh2D::set_radial_segments);
	ClassDB::bind_method(D_METHOD("get_radial_segments"), &CircleMesh2D::get_radial_segments);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "0,1024,0.001,or_greater,suffix:px"), "set_radius", "get_radius");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "radial_segments", PROPERTY_HINT_RANGE, "1,100,1,or_greater"), "set_radial_segments", "get_radial_segments");
}

void CircleMesh2D::set_radius(const float p_radius) {
	if (Math::is_equal_approx(p_radius, radius)) {
		return;
	}

	radius = p_radius;
	request_update();
}

float CircleMesh2D::get_radius() const {
	return radius;
}

void CircleMesh2D::set_radial_segments(const int p_segments) {
	if (p_segments == radial_segments) {
		return;
	}

	radial_segments = p_segments > 3 ? p_segments : 3;
	request_update();
}

int CircleMesh2D::get_radial_segments() const {
	return radial_segments;
}

CircleMesh2D::CircleMesh2D() {}

/**
  CapsuleMesh2D
*/

void CapsuleMesh2D::_create_mesh_array(Array &p_arr) const {
	int i, point;
	float x, y, u, v;

	Vector<Vector2> points;
	Vector<Vector2> uvs;
	Vector<int> indices;
	point = 0;

	if (radius > 0.0) {
		float circle_center_offset = 0.5 * height - radius;
		float radius_v = radius / height;
		float circle_center_offset_v = 0.5 - radius_v;

		// Bottom.
		int circle_center_idx = point;
		points.push_back(Vector2(0.0, circle_center_offset));
		uvs.push_back(Vector2(0.5, 0.5 + circle_center_offset_v));
		point++;

		for (i = 0; i <= radial_segments; i++) {
			float r = i;
			r /= radial_segments;

			if (i == radial_segments) {
				x = -1.0;
				y = 0.0;
			} else {
				x = Math::cos(r * Math_PI);
				y = Math::sin(r * Math_PI);
			}

			u = 0.5 + x * 0.5;
			v = 0.5 + circle_center_offset_v + y * radius_v;

			Vector2 p = Vector2(x * radius, circle_center_offset + y * radius);
			points.push_back(p);
			uvs.push_back(Vector2(u, v));
			point++;

			if (i > 0) {
				indices.push_back(circle_center_idx);
				indices.push_back(point - 1);
				indices.push_back(point - 2);
			}
		}

		// Middle.
		if (height > 2.0f * radius) {
			points.push_back(Vector2(-radius, circle_center_offset));
			uvs.push_back(Vector2(0, 0.5 + circle_center_offset_v));
			point++;
			points.push_back(Vector2(radius, circle_center_offset));
			uvs.push_back(Vector2(1, 0.5 + circle_center_offset_v));
			point++;
			points.push_back(Vector2(radius, -circle_center_offset));
			uvs.push_back(Vector2(1, 0.5 - circle_center_offset_v));
			point++;
			points.push_back(Vector2(-radius, -circle_center_offset));
			uvs.push_back(Vector2(0, 0.5 - circle_center_offset_v));
			point++;
			indices.push_back(point - 4);
			indices.push_back(point - 3);
			indices.push_back(point - 2);
			indices.push_back(point - 2);
			indices.push_back(point - 1);
			indices.push_back(point - 4);
		}

		// Top.
		circle_center_idx = point;
		points.push_back(Vector2(0.0, -circle_center_offset));
		uvs.push_back(Vector2(0.5, 0.5 - circle_center_offset_v));
		point++;

		for (i = 0; i <= radial_segments; i++) {
			float r = i;
			r /= radial_segments;

			if (i == radial_segments) {
				x = -1.0;
				y = 0.0;
			} else {
				x = Math::cos(r * Math_PI);
				y = Math::sin(r * Math_PI);
			}

			u = 0.5 + x * 0.5;
			v = 0.5 - circle_center_offset_v - y * radius_v;

			Vector2 p = Vector2(x * radius, -circle_center_offset - y * radius);
			points.push_back(p);
			uvs.push_back(Vector2(u, v));
			point++;

			if (i > 0) {
				indices.push_back(circle_center_idx);
				indices.push_back(point - 2);
				indices.push_back(point - 1);
			}
		}
	}

	p_arr[RS::ARRAY_VERTEX] = points;
	p_arr[RS::ARRAY_TEX_UV] = uvs;
	p_arr[RS::ARRAY_INDEX] = indices;
}

void CapsuleMesh2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &CapsuleMesh2D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &CapsuleMesh2D::get_radius);
	ClassDB::bind_method(D_METHOD("set_height", "height"), &CapsuleMesh2D::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &CapsuleMesh2D::get_height);

	ClassDB::bind_method(D_METHOD("set_radial_segments", "segments"), &CapsuleMesh2D::set_radial_segments);
	ClassDB::bind_method(D_METHOD("get_radial_segments"), &CapsuleMesh2D::get_radial_segments);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "0.001,1024.0,0.001,or_greater,suffix:px"), "set_radius", "get_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height", PROPERTY_HINT_RANGE, "0.001,1024.0,0.001,or_greater,suffix:px"), "set_height", "get_height");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "radial_segments", PROPERTY_HINT_RANGE, "1,100,1,or_greater"), "set_radial_segments", "get_radial_segments");

	ADD_LINKED_PROPERTY("radius", "height");
	ADD_LINKED_PROPERTY("height", "radius");
}

void CapsuleMesh2D::set_radius(const float p_radius) {
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}

	radius = p_radius;
	if (radius > height * 0.5) {
		height = radius * 2.0;
	}
	request_update();
}

float CapsuleMesh2D::get_radius() const {
	return radius;
}

void CapsuleMesh2D::set_height(const float p_height) {
	if (Math::is_equal_approx(height, p_height)) {
		return;
	}

	height = p_height;
	if (radius > height * 0.5) {
		radius = height * 0.5;
	}
	request_update();
}

float CapsuleMesh2D::get_height() const {
	return height;
}

void CapsuleMesh2D::set_radial_segments(const int p_segments) {
	if (radial_segments == p_segments) {
		return;
	}

	radial_segments = p_segments > 2 ? p_segments : 2;
	request_update();
}

int CapsuleMesh2D::get_radial_segments() const {
	return radial_segments;
}

CapsuleMesh2D::CapsuleMesh2D() {}
