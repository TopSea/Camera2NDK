#version 310 es
precision highp float;

in vec2 TexCoord;

uniform sampler2D uni_texture;

out vec4 FragColor;

void main() {
    FragColor = texture(uni_texture, TexCoord);
}