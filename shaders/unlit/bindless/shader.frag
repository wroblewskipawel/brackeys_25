#version 460 core

#extension GL_ARB_bindless_texture : require

out vec4 frag_color;

in VS_OUT { vec2 tex_coord; }
fs_in;

struct UnlitMaterial {
    sampler2D albedo_tex;
};

layout(std140, binding = 0) readonly buffer MaterialPack {
    UnlitMaterial materials[];
}
material_pack;

uniform uint material;

void main() {
    vec4 albedo =
        texture(material_pack.materials[material].albedo_tex, fs_in.tex_coord);
    frag_color = albedo;
}
