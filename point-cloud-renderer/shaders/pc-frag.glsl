#version 410

in vec3 fragColour;

out vec4 colour;

void main()
{
    // render point as a circle
    vec2 pointCenter = gl_PointCoord - vec2(0.5);
    if(length(pointCenter) > 0.5) {
      discard;
    }

    colour = vec4(fragColour, 1.0);
}
