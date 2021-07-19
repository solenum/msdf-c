# msdf-c
A pure C99 multi-channel signed distance field generator.  Handles MSDF bitmap
generation from given ttf/otf font.

---

**This is in an unstable state, ymmv.**

[Based on the C++ implementation by Viktor Chlumský.](https://github.com/Chlumsky/msdfgen)

[Example of using this with GL (from my C engine)](https://github.com/exezin/exengine/blob/master/src/exengine/render/text.c)


~~~
// example fragment shader
// scale is render_width/glyph_width
// render_width being the width of each rendered glyph
float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
  vec3 sample = texture(u_texture, uv).rgb;
  float dist = scale * (median(sample.r, sample.g, sample.b) - 0.5);
  float o = clamp(dist + 0.5, 0.0, 1.0);
  color = vec4(vec3(1.0), o);
}
~~~


Current issues:

* ~~Glyph alignment seems off~~ [X]

* ~~Lack of multi-byte character support~~ [X]

* ~~Error correction appears to be wrong (pixel clash)~~ [X]

* ~~Lacking a sensible API~~ [X], and code is a bit messy