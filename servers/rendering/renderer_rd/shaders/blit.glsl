#[vertex]

#version 450

#VERSION_DEFINES

layout(push_constant, std140) uniform Pos {
	vec4 src_rect;
	vec4 dst_rect;

	float rotation_sin;
	float rotation_cos;

	vec2 eye_center;
	float k1;
	float k2;

	float upscale;
	float aspect_ratio;
	uint layer;
	bool source_is_srgb;

	uint target_color_space;
	float reference_display_luminance;
}
data;

layout(location = 0) out vec2 uv;

void main() {
	mat4 swapchain_transform = mat4(1.0);
	swapchain_transform[0][0] = data.rotation_cos;
	swapchain_transform[0][1] = -data.rotation_sin;
	swapchain_transform[1][0] = data.rotation_sin;
	swapchain_transform[1][1] = data.rotation_cos;

	vec2 base_arr[4] = vec2[](vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(1.0, 0.0));
	uv = data.src_rect.xy + base_arr[gl_VertexIndex] * data.src_rect.zw;
	vec2 vtx = data.dst_rect.xy + base_arr[gl_VertexIndex] * data.dst_rect.zw;
	gl_Position = swapchain_transform * vec4(vtx * 2.0 - 1.0, 0.0, 1.0);
}

#[fragment]

#version 450

#VERSION_DEFINES

layout(push_constant, std140) uniform Pos {
	vec4 src_rect;
	vec4 dst_rect;

	float rotation_sin;
	float rotation_cos;

	vec2 eye_center;
	float k1;
	float k2;

	float upscale;
	float aspect_ratio;
	uint layer;
	bool source_is_srgb;

	uint target_color_space;
	float reference_display_luminance;
}
data;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 color;

#ifdef USE_LAYER
layout(binding = 0) uniform sampler2DArray src_rt;
#else
layout(binding = 0) uniform sampler2D src_rt;
#endif

#include "color_space_inc.glsl"

void main() {
#ifdef APPLY_LENS_DISTORTION
	vec2 coords = uv * 2.0 - 1.0;
	vec2 offset = coords - data.eye_center;

	// take aspect ratio into account
	offset.y /= data.aspect_ratio;

	// distort
	vec2 offset_sq = offset * offset;
	float radius_sq = offset_sq.x + offset_sq.y;
	float radius_s4 = radius_sq * radius_sq;
	float distortion_scale = 1.0 + (data.k1 * radius_sq) + (data.k2 * radius_s4);
	offset *= distortion_scale;

	// reapply aspect ratio
	offset.y *= data.aspect_ratio;

	// add our eye center back in
	coords = offset + data.eye_center;
	coords /= data.upscale;

	// and check our color
	if (coords.x < -1.0 || coords.y < -1.0 || coords.x > 1.0 || coords.y > 1.0) {
		color = vec4(0.0, 0.0, 0.0, 1.0);
	} else {
		// layer is always used here
		coords = (coords + vec2(1.0)) / vec2(2.0);
		color = texture(src_rt, vec3(coords, data.layer));
	}
#elif defined(USE_LAYER)
	color = texture(src_rt, vec3(uv, data.layer));
#else
	color = texture(src_rt, uv);
#endif

	if (data.target_color_space == COLOR_SPACE_SRGB_NONLINEAR && data.source_is_srgb == false) {
		color.rgb = linear_to_srgb(color.rgb); // linear -> sRGB conversion.
	} else if (data.target_color_space == COLOR_SPACE_HDR10_ST2084) {
		if (data.source_is_srgb == true) {
			// HACK: Converting back to linear since I'm not sure who else converts to sRGB.
			color.rgb = srgb_to_linear(color.rgb); // sRGB -> linear conversion.
		}

		color.rgb = linear_to_st2084(color.rgb, data.reference_display_luminance); // linear -> ST2084 conversion.
	}
}
