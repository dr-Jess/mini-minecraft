#version 150
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

uniform vec4 u_Color = vec4(0.0, 1.0, 0.0, 1.0); // The color with which to render this instance of geometry.
uniform sampler2D u_Texture;
uniform float u_Time;
uniform sampler2D u_ShadowMap;
uniform vec3 u_CameraPos;

// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_LightVec;
in vec4 fs_UV;
in vec4 fs_ShadowPos;

out vec4 out_Col; // This is the final output color that you will see on your
// screen for the pixel that is currently being processed.

void main()
{
    vec2 uv = fs_UV.xy;
    // Material base color (before shading)
    // vec4 diffuseColor = fs_Col;
    if (fs_UV.z == 2.0) { // lava
        uv.x += mod(u_Time * 0.0007, 1.0 / 16.0);
    } else if (fs_UV.z == 1.0) { // water
        vec4 texColor = texture(u_Texture, uv);
        uv.x += mod(u_Time * 0.002, 1.0 / 10.0); //horizontal movement

        //blinn-phong
        vec3 N = normalize(vec3(fs_Nor));
        vec3 V = normalize(u_CameraPos - fs_Pos.xyz);
        vec3 L = normalize(fs_LightVec.xyz);
        vec3 H = normalize(V + L);

        vec3 ambient = 0.6 * texture(u_Texture, fs_UV.xy).rgb;

        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * texture(u_Texture, fs_UV.xy).rgb;

        const float exp = 16.0;
        vec3 specular = pow(max(dot(H, N), 0.0), exp) * vec3(1.0);

        vec3 finalColor = ambient + diffuse + specular;
        out_Col = vec4(finalColor, texColor.a);

        return;
    }

    float cosTheta = clamp(dot(fs_Nor, normalize(fs_LightVec)), 0.0, 1.0);
    vec4 texColor = texture(u_Texture, uv);

    float ambientBias = 0.001f;

    float visibility = texture( u_ShadowMap, fs_ShadowPos.xy).z < fs_ShadowPos.z-clamp(0.002*tan(acos(cosTheta)) + ambientBias, 0, 0.01)? 0.5 : 1.0;

    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(normalize(fs_Nor), normalize(fs_LightVec));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);

    float ambientTerm = 0.25;
    float lightIntensity = (diffuseTerm * visibility) + ambientTerm;
    // float lightIntensity = visibility;
    //Add a small float value to the color multiplier
    //to simulate ambient lighting. This ensures that faces that are not
    //lit by our point light are not completely black.

    // Compute final shaded color
    out_Col = vec4(texColor.rgb * lightIntensity, texColor.a);
    // if (visibility <= 0.5) {
    //     out_Col = vec4(0.0, 0.0, 0.0, texColor.a);
    // } else {
    //     out_Col = vec4(texColor.rgb * lightIntensity, texColor.a);
    // }
    // out_Col = vec4(texture(u_ShadowMap, fs_ShadowPos.xy).xyz, 1);
}
