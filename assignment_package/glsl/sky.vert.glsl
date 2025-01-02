#version 150

in vec4 sky_Pos;
out vec4 fs_Pos;

void main() {
    fs_Pos = sky_Pos;
    gl_Position = sky_Pos;
}
