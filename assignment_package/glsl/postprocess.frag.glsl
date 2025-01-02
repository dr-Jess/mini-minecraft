#version 330 core

in vec4 fs_UV;

out vec4 out_Col;

uniform sampler2D u_Texture;
uniform float u_Time;
uniform int u_PostEffect;
uniform vec2 u_Resolution;

void main()
{
    vec3 color_noA = texture(u_Texture, fs_UV.xy).rgb;

    if (u_PostEffect == 1) {
        // water, tint blue
        // color_noA = color_noA * 0.8 + vec3(0.0, 0.0, 1.0) * 0.2;
        float TAU = 6.28318530718;
        float MAX_ITER = 5;

        float amplitude = .30;
        float turbulence = .1;
        vec4 newUV = fs_UV;

        newUV.xy += ((sin(newUV.x*turbulence + u_Time *.35*TAU)+1.)/2.) * amplitude;
        newUV.xy -= ((sin(newUV.y*turbulence + u_Time *.35*TAU)+1.)/2.) * amplitude;
        color_noA = texture(u_Texture, newUV.xy).rgb;

        float time = u_Time * .5+23.0;
        // uv should be the 0-1 uv of texture...
        vec2 uv = fs_UV.xy;
        vec2 p = mod(uv*TAU, TAU)-250.0;
        vec2 i = vec2(p);
        float c = 1.0;
        float inten = .005;
        for (int n = 0; n < MAX_ITER; n++)
        {
            float t = time * (1.0 - (3.5 / float(n+1)));
            i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
            c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
        }
        c /= float(MAX_ITER);
        c = 1.17-pow(c, 1.4);
        vec3 colour = vec3(pow(abs(c), 8.0));
        colour = clamp(colour/1.5 + vec3(0.0, 0.0, 1.0), 0.0, 1.0);
        color_noA = colour * 0.2 + color_noA * 0.8;
    } else if (u_PostEffect == 2) {
        // lava, tint orange
        float TAU = 6.28318530718;
        float MAX_ITER = 5;

        float amplitude = .30;
        float turbulence = .3;
        vec4 newUV = fs_UV;

        newUV.xy += ((sin(newUV.x*turbulence + u_Time *.35*TAU)+1.)/2.) * amplitude;
        newUV.xy -= ((sin(newUV.y*turbulence + u_Time *.35*TAU)+1.)/2.) * amplitude;
        color_noA = texture(u_Texture, newUV.xy).rgb;

        float time = u_Time * .5+23.0;
        // uv should be the 0-1 uv of texture...
        vec2 uv = fs_UV.xy;
        vec2 p = mod(uv*TAU, TAU)-250.0;
        vec2 i = vec2(p);
        float c = 1.0;
        float inten = .005;
        for (int n = 0; n < MAX_ITER; n++)
        {
            float t = time * (1.0 - (3.5 / float(n+1)));
            i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
            c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
        }
        c /= float(MAX_ITER);
        c = 1.17-pow(c, 1.4);
        vec3 colour = vec3(pow(abs(c), 8.0));
        colour = clamp(colour/2 + vec3(1.0f, 0.5, 0.0), 0.0, 1.0);
        color_noA = colour * 0.8 + color_noA * 0.2;
    }
    out_Col = vec4(color_noA, 1.0);
}
