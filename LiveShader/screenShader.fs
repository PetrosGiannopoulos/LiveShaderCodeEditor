#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D codeScreenTexture;
uniform sampler2D displayScreenTexture;

uniform float width;
uniform float height;

uniform vec2 caretPos;

void main()
{

	float caretWidth = 1;
	float caretHeight = 20;

	vec4 caretColor = vec4(0,0,0,1);
	vec2 posCaret = caretPos;
	//posCaret.x *= 0.5;
	
	vec2 screenCoords = gl_FragCoord.xy;

	vec2 aTexCoords = TexCoords;
	aTexCoords.x *= 2;

	vec3 col = vec3(0);
	if(screenCoords.x<(width*0.5)) col = texture(codeScreenTexture, aTexCoords).rgb;
    else col = texture(displayScreenTexture, aTexCoords).rgb;


	col = pow(col, vec3(1./2.2));

    FragColor = vec4(col, 1.0);

	float diffX = abs(gl_FragCoord.x-posCaret.x);
	float diffY = abs(gl_FragCoord.y-posCaret.y);

	//if(gl_FragCoord.y>posCaret.y && gl_FragCoord.y<(posCaret.y+caretHeight) && (gl_FragCoord.x>posCaret.x && gl_FragCoord.x<(posCaret.x+caretWidth))){
	
	vec4 bgColor = vec4(0.75,0.75,1,1)*smoothstep(length(aTexCoords*2-1)*12.5,0.01,0.02);
	
	if(screenCoords.x<(width*0.5)){
		//FragColor = mix(FragColor, bgColor,0.5);
		FragColor = pow(FragColor, vec4(1/2.2));
	}

	if(diffX<caretWidth && diffY<caretHeight){
		FragColor = caretColor;
	}

	//if(length(gl_FragCoord.xy-posCaret)<4)FragColor = vec4(0,1,0,1);


	if(gl_FragCoord.x<70)FragColor = vec4(0,0,0,1)+(aTexCoords.y*0.15);
} 