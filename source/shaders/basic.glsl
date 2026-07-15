@vs rect_vs
in vec2 position;
in vec4 color;
out vec4 frag_color;
out vec2 Position;

layout(binding = 0) uniform project_uniform { 
    mat4 proj;
};

void main() {
    gl_Position = proj * vec4(position, 0.0, 1.0);
    frag_color = color;
    Position = position;
}
@end

@fs rect_fs
in vec4 frag_color;
in vec2 Position;
out vec4 out_color;

layout(binding = 1) uniform radius_uniform {
    vec2 center;
    vec2 half_size;
    float radius;
};

void main() {
    vec2 local_pos = Position - center;
    vec2 q = abs(local_pos) - half_size + radius;
    float dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
    float px = fwidth(dist) * 0.5;
    float alpha = 1.0 - smoothstep(-px, px, dist);
    out_color = vec4(frag_color.rgb, frag_color.a * alpha);
}
@end

@vs tex_vs

in vec2 pos;
in vec2 uv;
in vec4 color;
out vec2 Uv;
out vec4 Color;

layout(binding = 0) uniform project_uniform {
    mat4 proj;
};

void main() {
    gl_Position = proj * vec4(pos, 0.0, 1.0);

    Uv = uv;
    Color = color;
}
@end

@fs tex_fs

in vec2 Uv;
in vec4 Color;
out vec4 out_color;

layout(binding = 0) uniform texture2D glyph_texture;
layout(binding = 0) uniform sampler glyph_sampler;

void main() {
    float coverage = texture(
        sampler2D(glyph_texture, glyph_sampler),
        Uv
    ).r;

    out_color = vec4(
        Color.rgb,
        Color.a * coverage
    );
}
@end

@program rectangle rect_vs rect_fs
@program text tex_vs tex_fs
