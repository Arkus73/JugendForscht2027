#version 430

in vec2 texCoord;
uniform sampler2D tex;

out vec4 FragColor;

void main() {
    FragColor = texture(tex, texCoord);
}