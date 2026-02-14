// LOD1 Triplanar Texture Projection Shader
//
// LOD2モデルを5方向(前後左右上)からキャプチャしたテクスチャを
// LOD1モデルにトライプラナー投影するシェーダ。
//
// 使い方:
// 1. LOD2モデルをSubViewportで5方向からキャプチャ
// 2. キャプチャテクスチャを各uniformにセット
// 3. aabb_position/aabb_size にLOD2のバウンディングボックスをセット
// 4. LOD1のMeshInstance3DにShaderMaterialとして適用

shader_type spatial;
render_mode cull_back;

group_uniforms textures;
uniform sampler2D texture_front : source_color, filter_linear, repeat_disable;
uniform sampler2D texture_back : source_color, filter_linear, repeat_disable;
uniform sampler2D texture_left : source_color, filter_linear, repeat_disable;
uniform sampler2D texture_right : source_color, filter_linear, repeat_disable;
uniform sampler2D texture_top : source_color, filter_linear, repeat_disable;

group_uniforms bounds;
uniform vec3 aabb_position;
uniform vec3 aabb_size;

group_uniforms material;
uniform float blend_sharpness : hint_range(1.0, 16.0) = 4.0;
uniform float roughness : hint_range(0.0, 1.0) = 0.8;
uniform float metallic : hint_range(0.0, 1.0) = 0.0;

// キャプチャ時のマージンと一致させる
const float CAPTURE_MARGIN = 1.01;

varying vec3 world_pos;
varying vec3 world_norm;

void vertex() {
	world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	world_norm = normalize(mat3(MODEL_MATRIX) * NORMAL);
}

void fragment() {
	vec3 wn = normalize(world_norm);
	vec3 wp = world_pos;
	vec3 center = aabb_position + aabb_size * 0.5;

	// 各方向のキャプチャ時の正射影サイズ（キャプチャ設定と一致）
	float s_fb = max(aabb_size.x, aabb_size.y) * CAPTURE_MARGIN;
	float s_lr = max(aabb_size.z, aabb_size.y) * CAPTURE_MARGIN;
	float s_t = max(aabb_size.x, aabb_size.z) * CAPTURE_MARGIN;

	// 各方向のUV座標（カメラの向きに合わせた投影）
	// Front (+Z): right=+X, up=+Y
	vec2 uv_f = vec2(
		(wp.x - center.x) / s_fb + 0.5,
		0.5 - (wp.y - center.y) / s_fb
	);
	// Back (-Z): right=-X, up=+Y
	vec2 uv_b = vec2(
		0.5 - (wp.x - center.x) / s_fb,
		0.5 - (wp.y - center.y) / s_fb
	);
	// Right (+X): right=-Z, up=+Y
	vec2 uv_r = vec2(
		0.5 - (wp.z - center.z) / s_lr,
		0.5 - (wp.y - center.y) / s_lr
	);
	// Left (-X): right=+Z, up=+Y
	vec2 uv_l = vec2(
		(wp.z - center.z) / s_lr + 0.5,
		0.5 - (wp.y - center.y) / s_lr
	);
	// Top (+Y): right=+X, up=+Z (camera rotated -90 on X)
	vec2 uv_t = vec2(
		(wp.x - center.x) / s_t + 0.5,
		0.5 - (wp.z - center.z) / s_t
	);

	// テクスチャサンプリング
	vec4 c_f = texture(texture_front, uv_f);
	vec4 c_b = texture(texture_back, uv_b);
	vec4 c_r = texture(texture_right, uv_r);
	vec4 c_l = texture(texture_left, uv_l);
	vec4 c_t = texture(texture_top, uv_t);

	// 法線方向に基づく重み付け
	float w_f = pow(max(wn.z, 0.0), blend_sharpness);
	float w_b = pow(max(-wn.z, 0.0), blend_sharpness);
	float w_r = pow(max(wn.x, 0.0), blend_sharpness);
	float w_l = pow(max(-wn.x, 0.0), blend_sharpness);
	float w_t = pow(abs(wn.y), blend_sharpness);  // 上下両方にTopテクスチャを使用

	float total = w_f + w_b + w_r + w_l + w_t + 1e-4;

	vec3 albedo = (c_f.rgb * w_f + c_b.rgb * w_b + c_r.rgb * w_r
		+ c_l.rgb * w_l + c_t.rgb * w_t) / total;

	ALBEDO = albedo;
	ROUGHNESS = roughness;
	METALLIC = metallic;
}
