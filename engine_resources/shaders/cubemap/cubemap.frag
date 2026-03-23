#version 430

in vec3 TexCoords;

out vec4 FragColor;

uniform samplerCube gCubemapTexture;

void main()
{
    vec3 color = texture(gCubemapTexture, TexCoords).rgb;

    FragColor = vec4(color, 1.0);
}