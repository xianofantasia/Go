// Keep in sync with RenderingDeviceCommons::ColorSpace
const uint COLOR_SPACE_LINEAR = 0;
const uint COLOR_SPACE_SRGB_NONLINEAR = 1;
const uint COLOR_SPACE_HDR10_ST2084 = 2;

vec3 srgb_to_linear(vec3 color) {
	return mix(pow((color.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)), color.rgb * (1.0 / 12.92), lessThan(color.rgb, vec3(0.04045)));
}

vec3 linear_to_srgb(vec3 color) {
	//if going to srgb, clamp from 0 to 1.
	color = clamp(color, vec3(0.0), vec3(1.0));
	const vec3 a = vec3(0.055f);
	return mix((vec3(1.0f) + a) * pow(color.rgb, vec3(1.0f / 2.4f)) - a, 12.92f * color.rgb, lessThan(color.rgb, vec3(0.0031308f)));
}

vec3 linear_to_st2084(vec3 color, float reference_luminance) {
	// Prevent negative values from messing with the transfer function.
	color = max(color, vec3(0.0, 0.0, 0.0));

	// Rotate from Rec.709 to Rec.2020 primaries
	// Color transformation matrix values taken from DirectXTK, may need verification.
	const mat3 from709to2020 = mat3(
			0.6274040f, 0.0690970f, 0.0163916f,
			0.3292820f, 0.9195400f, 0.0880132f,
			0.0433136f, 0.0113612f, 0.8955950f);
	color = from709to2020 * color;

	// Scale 1.0 to match the max luminance of SDR.
	// This is controlled by the reference_luminance parameter, which should be matched to the SDR reference luminance in the OS.
	float adjustment = reference_luminance * 0.0001f;
	color = color * adjustment;

	// TODO: Scale to avoid clipping via max and min luminance?

	// Apply ST2084 curve
	const float c1 = 0.8359375;
	const float c2 = 18.8515625;
	const float c3 = 18.6875;
	const float m1 = 0.1593017578125;
	const float m2 = 78.84375;
	vec3 cp = pow(abs(color), vec3(m1));

	return pow((c1 + c2 * cp) / (1 + c3 * cp), vec3(m2));
}
