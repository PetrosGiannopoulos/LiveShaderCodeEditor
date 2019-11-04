#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ssaoInput;

void main() 
{

     
	 vec4 color = texture(ssaoInput, TexCoords);
	 

	 //hdrColor = vec3(1.0) - exp(-hdrColor*exposure);

     // Gamma correction
     //hdrColor = pow(hdrColor, vec3(1.0/2.2));

	//FragColor = vec4();
	if(color.w == 0.5){
	
		vec3 hdrColor = vec3(0.0, 0.0, 0.0);
		vec2 pixelSize = 1.0 / textureSize(ssaoInput, 0);
		vec3 scattering = 0.5*color.rgb;
		scattering += texture(ssaoInput, TexCoords + vec2(-0.5*pixelSize.x, -pixelSize.y)).rgb;
		scattering += texture(ssaoInput, TexCoords + vec2(pixelSize.x, -0.5*pixelSize.y)).rgb;
		scattering += texture(ssaoInput, TexCoords + vec2(0.5*pixelSize.x, pixelSize.y)).rgb;
		scattering += texture(ssaoInput, TexCoords + vec2(-pixelSize.x, 0.5*pixelSize.y)).rgb;
		hdrColor += (2.0 / 9.0) * scattering;
		FragColor = vec4(hdrColor,1);
	
	}
	else if(color.w == 0){
		vec3 hdrColor = vec3(0.0, 0.0, 0.0);
		vec2 pixelSize = 1.0 / textureSize(ssaoInput, 0);
        vec3 scattering = 0.5*color.rgb;
        // Interpolate four pixels
        scattering += 2.0*texture(ssaoInput, TexCoords + vec2(-1.5*pixelSize.x, -1.5*pixelSize.y)).rgb;
        scattering += 2.0*texture(ssaoInput, TexCoords + vec2(1.5*pixelSize.x, -1.5*pixelSize.y)).rgb;
        scattering += 2.0*texture(ssaoInput, TexCoords + vec2(-1.5*pixelSize.x, 1.5*pixelSize.y)).rgb;
        scattering += 2.0*texture(ssaoInput, TexCoords + vec2(1.5*pixelSize.x, 1.5*pixelSize.y)).rgb;
            
        // Interpolate two pixels
        scattering += texture(ssaoInput, TexCoords + vec2(pixelSize.x, 1.5*pixelSize.y)).rgb;
        scattering += texture(ssaoInput, TexCoords + vec2(pixelSize.x, -1.5*pixelSize.y)).rgb;
        scattering += texture(ssaoInput, TexCoords + vec2(1.5*pixelSize.x, pixelSize.y)).rgb;
        scattering += texture(ssaoInput, TexCoords + vec2(-1.5*pixelSize.x, pixelSize.y)).rgb;
        hdrColor += (2.0/25.0) * scattering;

		FragColor = vec4(hdrColor,1);
	}
	else FragColor = color;
}  