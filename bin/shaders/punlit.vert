#version 410

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;

uniform mat4 MVP;
uniform float pointSize;

uniform vec3 hmdPos;
uniform vec3 leftController;
uniform vec3 rightController;

out vec3 fragmentColor;

void main()
{
    gl_Position = MVP * vec4(vertexPosition, 1.0f);

    gl_PointSize = pointSize;

    //float hdist = smoothstep(5, 10, distance(leftController,vertexPosition));

    // increase point size by 10 in the zone
    //gl_PointSize = pointSize + (1.0-hdist)*10;

    fragmentColor = vertexColor/vec3(255.0,255.0,255.0);

    //float rmx = smoothstep(5, 10, distance(rightController,vertexPosition));
    //rmx = abs(2.0*rmx-1.0);
    //
    //float lmx = smoothstep(5, 10, distance(leftController,vertexPosition));
    //lmx = abs(2.0*lmx-1.0);
    //
    //fragmentColor = mix(vec3(1.0, 0.5, 0.0), fragmentColor, rmx);
    //fragmentColor = mix(vec3(0.0, 0.5, 1.0), fragmentColor, lmx);
}
