#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform vec2 renderSize;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;
    
    // Clean pixel sampling — no chromatic aberration (kills pixel art clarity)
    vec4 texelColor = texture(texture0, uv);

    // Vignette
    vec2 center = uv - vec2(0.5);
    float dist = length(center);
    float vignette = smoothstep(0.85, 0.35, dist); 
    
    // Dark fantasy color grading
    texelColor.rgb *= vignette;
    
    // CRT scanline effect (disabled — user dislikes the VHS lines)
    // if (renderSize.y > 0.0) {
    //     float scanline = sin(uv.y * renderSize.y * 1.5) * 0.05;
    //     texelColor.rgb -= scanline;
    // }
    
    // Warm infernal tint — Circulo VII (subtle red/orange)
    texelColor.rgb = mix(texelColor.rgb, vec3(0.12, 0.03, 0.02), 0.08);

    finalColor = texelColor * colDiffuse * fragColor;
}
