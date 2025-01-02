#version 150

in vec4 fs_Pos;
out vec4 out_Col;

uniform mat4 u_ViewProjInv;

uniform vec3 u_CameraPos;
uniform float u_Time;
uniform vec2 u_Resolution;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// Sunset palette
const vec3 sunset[5] = vec3[](vec3(255, 229, 119) / 255.0,
                               vec3(254, 192, 81) / 255.0,
                               vec3(255, 137, 103) / 255.0,
                               vec3(253, 96, 81) / 255.0,
                               vec3(57, 32, 51) / 255.0);
// Dusk palette
const vec3 dusk[5] = vec3[](vec3(144, 96, 144) / 255.0,
                            vec3(96, 72, 120) / 255.0,
                            vec3(72, 48, 120) / 255.0,
                            vec3(48, 24, 96) / 255.0,
                            vec3(0, 24, 72) / 255.0);

const vec3 sunrise[3] = vec3[](vec3(1.0, 0.8, 0.6), vec3(1.0, 0.6, 0.4), vec3(1.0, 0.4, 0.2));
const vec3 morning[3] = vec3[](vec3(0.6, 0.7, 0.8), vec3(0.75, 0.8, 0.85), vec3(0.8, 0.85, 0.9));
const vec3 noon[3] = vec3[](vec3(0.6, 0.8, 1.0), vec3(0.7, 0.9, 1.0), vec3(0.75, 0.95, 1.0));
const vec3 night[3] = vec3[](vec3(0.02, 0.02, 0.2), vec3(0.05, 0.05, 0.2), vec3(0.1, 0.1, 0.3));

const vec3 sunColor = vec3(1.0, 1.0, 0.9); // sun color
const vec3 cloudColor = sunset[3];


vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

//worley noise (distance to nearest seed (seeds are evenly distributed))
float WorleyNoise2D(vec2 p)
{
    vec2 pointInt = floor(p);
    vec2 pointFract = fract(p);

    float minDist = 1.0;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            vec2 neighbor = vec2(float(x), float(y));

            vec2 point = random2(pointInt + neighbor);

            vec2 diff = neighbor + point - pointFract;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

//worley fractal brownian motion that layers multiple instances of noise to generate more cloudlike noise
float WorleyFBM(vec2 uv) {
    float sum = 0.0;
    float freq = 1.0;
    float amp = 0.5;

    for (int i = 0; i < 5; i++) {
        sum += WorleyNoise2D(uv * freq) * amp;
        freq *= 2.0;
        amp *= 0.5;
    }

    return sum;
}


//cool pixel effect
vec2 pixelizeUV(vec2 uv, float pixelSize) {
    uv *= pixelSize; //shrink uvs to box size
    uv = floor(uv); //remove fractional parts
    uv /= pixelSize; //scale back up
    return uv;
}

// convert spherical coordinates to uv coordinates in sky dome
vec2 sphereToUV(vec3 p) {
    float phi = atan(p.z, p.x);
    phi = mod(phi + TWO_PI, TWO_PI); //smooth wrapping

    if (phi < 0) {
        phi += TWO_PI;
    }
    float theta = acos(p.y);
    return vec2(1 - phi / TWO_PI, 1 - theta / PI);
}

//from example - functions to interpolate sky color using mix to handle gradients based on y coord
vec3 uvToSunset(vec2 uv) {
    if (uv.y < 0.5) {
        return sunset[0];
    }
    else if (uv.y < 0.55) {
        return mix(sunset[0], sunset[1], (uv.y - 0.5) / 0.05);
    }
    else if (uv.y < 0.6) {
        return mix(sunset[1], sunset[2], (uv.y - 0.55) / 0.05);
    }
    else if (uv.y < 0.65) {
        return mix(sunset[2], sunset[3], (uv.y - 0.6) / 0.05);
    }
    else if (uv.y < 0.75) {
        return mix(sunset[3], sunset[4], (uv.y - 0.65) / 0.1);
    }
    return sunset[4];
}

vec3 uvToDusk(vec2 uv) {
    if (uv.y < 0.5) {
        return dusk[0];
    }
    else if (uv.y < 0.55) {
        return mix(dusk[0], dusk[1], (uv.y - 0.5) / 0.05);
    }
    else if (uv.y < 0.6) {
        return mix(dusk[1], dusk[2], (uv.y - 0.55) / 0.05);
    }
    else if (uv.y < 0.65) {
        return mix(dusk[2], dusk[3], (uv.y - 0.6) / 0.05);
    }
    else if (uv.y < 0.75) {
        return mix(dusk[3], dusk[4], (uv.y - 0.65) / 0.1);
    }
    return dusk[4];
}

vec3 interpolateSkyColor(float time, vec3 rayDir) {
    vec2 uv = sphereToUV(rayDir);
    vec2 pixelizedUV = pixelizeUV(uv, 300);
    vec2 pixelCenter = floor(uv * 300) / 300;

    float noiseValue = WorleyFBM(pixelCenter * 10.0);

    //sharper edges
    float definedNoise = smoothstep(0.3, 0.5, noiseValue);

    //random whiteness to get rid fo rings
    float randomWhiteness = random2(pixelCenter).x * 0.1; // Subtle randomness
    float opacityFactor = clamp(definedNoise + randomWhiteness, 0.0, 1.0);

    float edgeBlend = smoothstep(0.4, 0.5, abs(pixelizedUV.x - 0.5));

    //fade as goes up
    float fadeFactor = pow(1.0 - pixelizedUV.y, 3.0);
    fadeFactor = clamp(fadeFactor, 0.0, 1.0);

    vec3 baseColor;
    if (time < 0.03) {
        baseColor = uvToDusk(pixelizedUV);
    } else if (time < 0.07) {
        baseColor = mix(uvToDusk(pixelizedUV), morning[0], smoothstep(0.03, 0.07, time));
    } else if (time < 0.25) {
        baseColor = mix(morning[0], noon[0], smoothstep(0.07, 0.25, time));
    } else if (time < 0.35) {
        baseColor = noon[0];
    } else if (time < 0.45) {
        baseColor = mix(noon[0], uvToSunset(pixelizedUV), smoothstep(0.35, 0.45, time));
    } else if (time < 0.55) {
        baseColor = uvToSunset(pixelizedUV);
    } else if (time < 0.60) {
        baseColor = mix(uvToSunset(pixelizedUV), night[0], smoothstep(0.55, 0.60, time));
    } else if (time > 0.98 || time < 0.03) {
        baseColor = mix(night[0], uvToDusk(pixelizedUV), smoothstep(0.0, 0.03, time));
    } else {
        baseColor = night[0];
    }

    // blend base color with white
    vec3 white = vec3(1.0, 1.0, 1.0);
    return mix(baseColor, white, opacityFactor * fadeFactor);
}

// sun color based on time
vec3 sunColorBasedOnTime(float time) {
    if (time < 0.05) {
        return mix(sunColor, vec3(1.0, 0.6, 0.4), smoothstep(0.0, 0.1, time)); // Sunrise
    } else if (time < 0.2) {
        return mix(vec3(1.0, 0.6, 0.4), vec3(1.0, 1.0, 0.9), smoothstep(0.1, 0.2, time)); // Morning
    } else if (time < 0.55) {
        return mix(vec3(1.0, 1.0, 0.9), vec3(1.0, 1.0, 0.9), smoothstep(0.2, 0.55, time)); // Noon
    } else if (time < 0.65) {
        return mix(vec3(1.0, 1.0, 0.9), vec3(1.0, 0.4, 0.3), smoothstep(0.55, 0.6, time)); // Sunset
    } else {
        return vec3(0.02, 0.02, 0.2); // Night
    }
}

// sun effect that expands as it nears the horizon
vec3 sunEffect(vec3 rayDir, vec3 sunDir, vec3 sunBaseColor) {
    float sunSize = 0.1;
    float outerGlowSize = 0.3;

    // calculate angle between ray direction and sun direction
    float angle = acos(dot(rayDir, sunDir));
    // increase sun size as it nears the horizon
    float sizeFactor = smoothstep(PI * 0.25, PI * 0.5, angle);
    sunSize = mix(0.07, 0.5, sizeFactor); //grow

    // intensity + sun corona
    float sunIntensity = exp(-pow(length(rayDir - sunDir) / sunSize, 2.0));
    float glowIntensity = exp(-pow(length(rayDir - sunDir) / outerGlowSize, 4.0)) * sizeFactor;
    vec3 outerGlowColor = mix(sunBaseColor, vec3(1.0, 0.8, 0.6), 0.3); // slightly yellowish glow

    sunIntensity = smoothstep(0.0, 0.15, sunIntensity);

    return sunIntensity * sunBaseColor + glowIntensity * outerGlowColor;
}

// compute ray direction from screen position
vec3 rayDirection(vec4 screenPos) {
    vec4 worldPos = u_ViewProjInv * screenPos;  // convert to world space
    worldPos /= worldPos.w;  // convert from homogeneous coordinates
    return normalize(worldPos.xyz - u_CameraPos); // direction vector from camera to screen position
}

// inspo from reddit channel and https://www.shadertoy.com/view/3dSXWt
float starField(vec3 rayDir) {
    vec3 noisePos = rayDir * 300.0;
    // grid coords to generate stars at
    vec3 grid = floor(noisePos);
    vec2 grid2D = vec2(grid.x, grid.z);
    vec2 randomOffsets = random2(grid2D);
    float brightness = step(0.999, randomOffsets.x) * randomOffsets.y;
    return brightness * smoothstep(0.0, -0.1, rayDir.y);
}

void main() {
    vec3 rayDir = rayDirection(vec4(fs_Pos.xy, 1.0, 1.0));

    // wraparound time for animation (cycle day-night)
    float dayTime = mod(u_Time / 2400.0, 1.0); // Full day-night cycle

    // sn position based on time
    // float sunAzimuth = dayTime * TWO_PI;
    float sunElevation = dayTime * TWO_PI;
    // vec3 sunDir = vec3(cos(sunElevation) * cos(sunAzimuth),
    //                    sin(sunElevation),
    //                    cos(sunElevation) * sin(sunAzimuth));
    vec3 sunDir = vec3(0.0,
                       sin(sunElevation),
                       cos(sunElevation));

    // sky color based on time
    vec3 skyColor = interpolateSkyColor(dayTime, rayDir);

    // sun color
    vec3 currentSunColor = sunColorBasedOnTime(dayTime);
    // sun's effect on the scene
    vec3 sun = sunEffect(rayDir, sunDir, currentSunColor);
\
    // sun angle
    float angle = acos(dot(rayDir, sunDir)) * 180.0 / PI;

    // glow
    float sunSize = 5;
    if (angle < sunSize) {
        if (angle < 7.5) {
            sun = sunColor; //yellow
        }
        //corona
        else {
            sun = mix(sunColor, skyColor, (angle - 7.5) / (sunSize - 7.5));
        }
    }

    // stars appear after 60% of the day cycle
    float stars = step(0.60, dayTime) * starField(rayDir);
    vec3 starColor = vec3(stars);

    // combine sky + sun + stars
    vec3 finalColor = skyColor + sun + starColor;

    out_Col = vec4(finalColor, 1.0);
}
