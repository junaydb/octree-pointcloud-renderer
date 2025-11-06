#version 410

layout(location=0) in vec3 position;
layout(location=1) in vec3 colour;

uniform mat4 MVP;
uniform float pointSize;
 
out vec3 fragColour;

void main()
{
    gl_Position = MVP * vec4(position, 1.0);
    gl_PointSize = pointSize;
    fragColour = colour;
}
