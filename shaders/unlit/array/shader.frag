#version 460 core

#extension GL_ARB_bindless_texture : require

out vec4 frag_color;

in VS_OUT { vec2 tex_coord; }
fs_in;

struct ArrayUnlit {
    sampler2DArray albedo;
};

uniform ArrayUnlit arrayUnlit;

uniform uint material;

void main() {
    vec4 albedo =
        texture(arrayUnlit.albedo, vec3(fs_in.tex_coord, material));
    frag_color = albedo;
}
