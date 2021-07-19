/*
 This is just a simple example usage of the
 msdf generator. It generates a bitmap from the
 character you specify.

 This c file is not intended for real-world use.
 this is purely an example. Embed msdf.c/h into your
 code directly and use however you need to.

 Usage:
 ./msdf_gen 'A' glyph.png

 The resulting bitmap will be stored in glyph.png
 the MSDF bitmap will be stored in msdf_glyph.png.

 Supports multi-byte characters (utf8)
 */

#include "msdf.h"
#include <string.h>
#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define PX_RANGE 2.0

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CLAMP(x, upper, lower) (MIN(upper, MAX(x, lower)))

uint8_t* io_read_file(const char *path, const char *mode)
{
  // load the file
  FILE *file;
  file = fopen(path, mode);
  if (file == NULL) {
    printf("could not load file %s\n", path);
    return NULL;
  }

  // get file length
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  // allocate space for file data
  uint8_t *buff = malloc(size+1);

  // read file contents into buffer
  size_t l = fread(buff, size, 1, file);

  // null-terminate the buffer
  buff[size] = '\0';

  fclose(file);

  return buff;
}

float median(float r, float g, float b)
{
  return MAX(MIN(r, g), MIN(MAX(r, g), b));
}

float lerp(float s, float e, float t)
{
  return s+(e-s)*t;
}
float blerp(float c00, float c10, float c01, float c11, float tx, float ty)
{
  return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
}

int calc_index(int x, int y, int size, int num_channels)
{
  x = CLAMP(x, size-1, 0);
  y = CLAMP(y, size-1, 0);
  return num_channels*((y*size)+x);
}

void main(int argc, char **argv)
{
  char fontdefault[] = "font/OpenSans-Regular.ttf";
  int size_sdf = 26;
  int size_bitmap = 512;

  char *c     = argv[1];
  char *out   = argv[2];
  char *fon   = argv[3];
  char *fontf = !fon ? fontdefault : fon;

  // print help
  if (!c || !out || strcmp(c, "-h") == 0) {
    printf("msdf-c, a C99 multi-channel signed-distance-field generator\n");
    printf("by exezin https://github.com/exezin/msdf-c\n");
    printf("based on https://github.com/Chlumsky/msdfgen\n");
    printf("usage:\n  ./msdf_gen 'A' out.png \"path/to/font.ttf\"\n");

    return;
  }

  // load the ttf (or otf) data
  uint8_t *data = io_read_file(fontf, "rb");
  if (!data) {
    printf("Failed loading font.\n");
    return;
  }

  // process the ttf data
  stbtt_fontinfo font;
  stbtt_InitFont(&font, (const uint8_t*)data, stbtt_GetFontOffsetForIndex(data,0));

  // get baseline metrics
  int ascent, descent;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, 0);

  // Funit to pixel scale
  float scale = stbtt_ScaleForPixelHeight(&font, size_sdf);
  int baseline = (int)(ascent*scale);

  // pixel range needs inverting for otf fonts
  // not entirely sure why
  int otf = (strstr(fontf, ".otf")) ? -1 : 1;

  // generate a msdf bitmap
  // ideally you would do this in your shader
  ex_metrics_t metrics;
  float *msdf = ex_msdf_glyph(&font, ex_utf8(c), size_sdf, size_sdf, &metrics, 1);
  uint8_t *bitmap_sdf = malloc(3*size_sdf *size_sdf);
  memset(bitmap_sdf, 0, 3*size_sdf *size_sdf);
  for (int y = 0; y < size_sdf; y++) {
    for (int x = 0; x < size_sdf; x++) {
      size_t index_sdf = calc_index(x, y, size_sdf, 3);
      bitmap_sdf[index_sdf+0] = (uint8_t)(255*(msdf[index_sdf+0]+size_sdf)/size_sdf);
      bitmap_sdf[index_sdf+1] = (uint8_t)(255*(msdf[index_sdf+1]+size_sdf)/size_sdf);
      bitmap_sdf[index_sdf+2] = (uint8_t)(255*(msdf[index_sdf+2]+size_sdf)/size_sdf);
    }
  }
  
  uint8_t *bitmap = malloc(3*size_bitmap*size_bitmap);
  memset(bitmap, 0, 3*size_bitmap*size_bitmap);
  float scale_bitmap = 0.75f*otf*(float)size_bitmap/size_sdf;
  for (int y=0; y<size_bitmap; y++) {
    for (int x=0; x<size_bitmap; x++) {
      size_t index = calc_index(x, y, size_bitmap, 3);

      float gx = x/(float)size_bitmap*size_sdf;
      float gy = y/(float)size_bitmap*size_sdf;
      int gxi = (int)gx;
      int gyi = (int)gy;
#if 1
      float *c00 = &msdf[calc_index(gxi, gyi, size_sdf, 3)];
      float *c10 = &msdf[calc_index(gxi+1, gyi, size_sdf, 3)]; 
      float *c01 = &msdf[calc_index(gxi, gyi+1, size_sdf, 3)]; 
      float *c11 = &msdf[calc_index(gxi+1, gyi+1, size_sdf, 3)]; 
      float r = blerp(c00[0], c10[0], c01[0], c11[0], gx-gxi, gy-gyi);
      float g = blerp(c00[1], c10[1], c01[1], c11[1], gx-gxi, gy-gyi);
      float b = blerp(c00[2], c10[2], c01[2], c11[2], gx-gxi, gy-gyi);
#else
      float r = msdf[calc_index(gxi, gyi, size_sdf, 3)];
      float g = msdf[calc_index(gxi, gyi, size_sdf, 3)+1];
      float b = msdf[calc_index(gxi, gyi, size_sdf, 3)+2];
#endif

      float sigDist = scale_bitmap * (median(r, g, b)-0.5);
      float a = CLAMP(sigDist+0.5, 1.0, 0.0);

      bitmap[index+0] = 255*a;
      bitmap[index+1] = 255*a;
      bitmap[index+2] = 255*a;
	  }
  }

  printf("Glyph metrics for '%s':\n", c);
  printf("left_bearing: %i\n",  metrics.left_bearing);
  printf("advance:      %i\n",  metrics.advance);
  printf("glyph width:  %i\n",  metrics.ix1 - metrics.ix0);
  printf("glyph height: %i\n",  metrics.iy1 - metrics.iy0);
  printf("glyph x:   %i %i\n",  metrics.ix0,  metrics.ix1);
  printf("glyph y:   %i %i\n",  metrics.iy0,  metrics.iy1);
  printf("baseline:     %i\n",  baseline);

  // uncomment to draw a cross over the image
  // for debugging alignment issues
  /*for (int y=0; y<size_sdf; y++) {
    int index = 3*((y*size_sdf)+size_sdf/2);
    bitmap[index+0] = 0;
    bitmap[index+1] = 0;
    bitmap[index+2] = 0;
    index = 3*((size_sdf/2*size_sdf)+y);
    bitmap[index+0] = 0;
    bitmap[index+1] = 0;
    bitmap[index+2] = 0;
  }*/

  // debug output
  char buff[256];
  sprintf(buff, "msdf_%s", out);
  stbi_write_png(buff, size_sdf, size_sdf, 3, bitmap_sdf, size_sdf*3);
  stbi_write_png(out, size_bitmap, size_bitmap, 3, bitmap, size_bitmap*3);

  free(data);
  free(msdf);
  free(bitmap);
  free(bitmap_sdf);
  printf("Done generating msdf glyph.\n");
}