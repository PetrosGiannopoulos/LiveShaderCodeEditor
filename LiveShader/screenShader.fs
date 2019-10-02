#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D codeScreenTexture;
uniform sampler2D displayScreenTexture;

uniform float width;
uniform float height;

void main()
{

	vec2 screenCoords = gl_FragCoord.xy;

	vec2 aTexCoords = TexCoords;
	aTexCoords.x *= 2;

	vec3 col = vec3(0);
	if(screenCoords.x<(width*0.5)) col = texture(codeScreenTexture, aTexCoords).rgb;
    else col = texture(displayScreenTexture, aTexCoords).rgb;


	col = pow(col, vec3(1./2.2));

    FragColor = vec4(col, 1.0);
} 