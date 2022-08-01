#version 300 es
precision highp float;

in vec2 TexCoord;

uniform sampler2D s_textureY;
uniform sampler2D s_textureU;
uniform sampler2D s_textureV;
out vec4 FragColor;

void main() {
 //   float y, u, v, r, g, b;
 //   y = texture(s_textureY, TexCoord).r;
 //   u = texture(s_textureU, TexCoord).r;
 //   v = texture(s_textureV, TexCoord).r;
 //   u = u - 0.5;
 //   v = v - 0.5;
 //   r = y + 1.403 * v;
 //   g = y - 0.344 * u - 0.714 * v;
 //   b = y + 1.770 * u;
    FragColor = texture(s_textureY, TexCoord);
}