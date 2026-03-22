#version 430

in vec3 TexCoords;

out vec4 FragColor;

uniform samplerCube gCubemapTexture;

void main()
{
    vec3 color = texture(gCubemapTexture, TexCoords).rgb;

    // Gamma correction (linear to sRGB)
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}