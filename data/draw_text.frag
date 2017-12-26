varying vec2 pos;
uniform sampler2D tex;
uniform vec4 color;

void main() 
{
    gl_FragColor = vec4(color.rgb, color.a * texture2D(tex, pos).r);
}
