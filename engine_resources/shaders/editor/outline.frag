#version 330 core

in vec2 texCoords;
out vec4 FragColor;

uniform sampler2D maskTex;
uniform vec3 outlineColor;
uniform float outlineThickness;
uniform vec2 texelSize;

void main()
{
    // if the pixel is black (we are on the mask)
    if (texture(maskTex, texCoords).xyz == vec3(0.0))
    {
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                if (i == 0 && j == 0)
                {
                    continue;
                }

                vec2 offset = vec2(i, j) * texelSize * outlineThickness;

                // and if one of the neighboring pixels is white (we are on the border)
                if (texture(maskTex, texCoords + offset).xyz == vec3(1.0))
                {
                    FragColor = vec4(outlineColor, 1.0);
                    return;
                }
            }
        }
    }
    
    discard;
}