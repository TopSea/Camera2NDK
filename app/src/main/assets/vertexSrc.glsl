#version 300 es
out vec2 v_texcoord;
layout (location = 0) in vec4 position;
layout (location = 1) in vec2 texcoord;
uniform mat4 MVP;
void main() {
    v_texcoord = texcoord;
    gl_Position = MVP*position;
}