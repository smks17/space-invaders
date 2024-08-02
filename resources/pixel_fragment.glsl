#version 330

uniform sampler2D pixels;
noperspective in vec2 TexCoord;

out vec3 outColor;

void main(void){
    outColor = texture(pixels, TexCoord).rgb;
}