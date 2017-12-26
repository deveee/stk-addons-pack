attribute vec2 coord;
uniform float progress;

void main()
{
    float pos_x = (coord.x + 1.0) * progress - 1.0;
    gl_Position = vec4(pos_x, coord.y, 0.0, 1.0);
}
