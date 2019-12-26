#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

uniform vec2 pos;
uniform vec2 popupPos;
uniform bool isPopupOpened;
uniform int isPopupText;

void main()
{    
	//popup menu
	if(isPopupOpened && isPopupText == 0){
		if((gl_FragCoord.x>popupPos.x && gl_FragCoord.x<(popupPos.x+300)) && (gl_FragCoord.y>(popupPos.y-200) && gl_FragCoord.y<popupPos.y)){
			discard;
		}
	}

    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled;

	//if(length(gl_FragCoord.xy-pos)<4)color = mix(color, vec4(1,0,0,1),0.5);

	
} 