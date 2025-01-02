#version 330 core

in vec4 vs_Pos;
in vec4 vs_Nor;
in vec4 vs_UV;
out vec4 fs_UV;

void main()
{
    fs_UV = vs_UV;
    gl_Position = vs_Pos;
}
