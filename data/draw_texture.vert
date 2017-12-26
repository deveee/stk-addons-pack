attribute vec4 coord;
varying vec2 pos;

void main() 
{
    gl_Position = vec4(coord.xy, 0.0, 1.0);
    pos = coord.zw;
}
