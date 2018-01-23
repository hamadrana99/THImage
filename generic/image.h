#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/image.h"
#else

#undef MAX
#define MAX(a,b) ( ((a)>(b)) ? (a) : (b) )

#undef MIN
#define MIN(a,b) ( ((a)<(b)) ? (a) : (b) )

#undef TAPI
#define TAPI __declspec(dllimport)

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#undef temp_t
#if defined(TH_REAL_IS_FLOAT) || defined(TH_REAL_IS_DOUBLE)
#define temp_t real
#else
#define temp_t float
#endif

static inline real image_(FromIntermediate)(temp_t x);

/*
  {"scaleSimple", image_(Main_scaleSimple)},
  {"scaleBilinear", image_(Main_scaleBilinear)},
  {"scaleBicubic", image_(Main_scaleBicubic)},
  {"rotate", image_(Main_rotate)},
  {"rotateBilinear", image_(Main_rotateBilinear)},
  {"polar", image_(Main_polar)},
  {"polarBilinear", image_(Main_polarBilinear)},
  {"logPolar", image_(Main_logPolar)},
  {"logPolarBilinear", image_(Main_logPolarBilinear)},
  {"translate", image_(Main_translate)},
  {"cropNoScale", image_(Main_cropNoScale)},
  {"warp", image_(Main_warp)},
  {"saturate", image_(Main_saturate)},
  {"rgb2y",   image_(Main_rgb2y)},
  {"rgb2hsv", image_(Main_rgb2hsv)},
  {"rgb2hsl", image_(Main_rgb2hsl)},
  {"hsv2rgb", image_(Main_hsv2rgb)},
  {"hsl2rgb", image_(Main_hsl2rgb)},
  {"rgb2lab", image_(Main_rgb2lab)},
  {"lab2rgb", image_(Main_lab2rgb)},
  {"gaussian", image_(Main_gaussian)},
  {"vflip", image_(Main_vflip)},
  {"hflip", image_(Main_hflip)},
  {"flip", image_(Main_flip)},
  {"colorize", image_(Main_colorize)},
  {"text", image_(Main_drawtext)},
  {"drawRect", image_(Main_drawRect)},
*/

static void image_(Main_op_validate)(THTensor *Tsrc, THTensor *Tdst);
static long image_(Main_op_stride)( THTensor *T, int i);

static long image_(Main_op_depth)(THTensor *T);
static void image_(Main_scaleLinear_rowcol)(THTensor *Tsrc,
                                            THTensor *Tdst,
                                            long src_start,
                                            long dst_start,
                                            long src_stride,
                                            long dst_stride,
                                            long src_len,
                                            long dst_len );

static inline temp_t image_(Main_cubicInterpolate)(temp_t p0,
                                                   temp_t p1,
                                                   temp_t p2,
                                                   temp_t p3,
                                                   temp_t x) ;


static void image_(Main_scaleCubic_rowcol)(THTensor *Tsrc,
                                           THTensor *Tdst,
                                           long src_start,
                                           long dst_start,
                                           long src_stride,
                                           long dst_stride,
                                           long src_len,
                                           long dst_len );
static int image_(Main_scaleBilinear)(THTensor *Tsrc, THTensor *Tdst);
static int image_(Main_scaleBicubic)(THTensor *Tsrc, THTensor *Tdst);
static int image_(Main_scaleSimple)(THTensor *Tsrc, THTensor *Tdst);

static int image_(Main_rotate)(THTensor *Tsrc, THTensor *Tdst, float theta);
static int image_(Main_rotateBilinear)(THTensor *Tsrc, THTensor *Tdst, float theta);
static int image_(Main_polar)(THTensor *Tsrc, THTensor *Tdst, float doFull);
static int image_(Main_polarBilinear)(THTensor *Tsrc, THTensor *Tdst, float doFull);
static int image_(Main_logPolar)(THTensor *Tsrc, THTensor *Tdst, float doFull);
static int image_(Main_logPolarBilinear)(THTensor *Tsrc, THTensor *Tdst, float doFull);

static int image_(Main_cropNoScale)(THTensor *Tsrc, THTensor *Tdst, long startx, long starty);

static int image_(Main_translate)(THTensor *Tsrc, THTensor *Tdst, long shiftx, long shifty);
static int image_(Main_saturate)(THTensor *input) ;

/*
 * Converts an RGB color value to HSL. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes r, g, and b are contained in the set [0, 1] and
 * returns h, s, and l in the set [0, 1].
 */
int image_(Main_rgb2hsl)(THTensor *rgb, THTensor *hsl);

// helper
static inline temp_t image_(hue2rgb)(temp_t p, temp_t q, temp_t t) ;

/*
 * Converts an HSL color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns r, g, and b in the set [0, 1].
 */
int image_(Main_hsl2rgb)(THTensor *hsl, THTensor *rgb) ;
/*
 * Converts an RGB color value to HSV. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSV_color_space.
 * Assumes r, g, and b are contained in the set [0, 1] and
 * returns h, s, and v in the set [0, 1].
 */
int image_(Main_rgb2hsv)(THTensor *rgb, THTensor *hsv) ;

/*
 * Converts an HSV color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSV_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns r, g, and b in the set [0, 1].
 */
int image_(Main_hsv2rgb)(THTensor *hsv, THTensor *rgb) ;
#ifndef TH_REAL_IS_BYTE
/*
 * Convert an sRGB color channel to a linear sRGB color channel.
 */
static inline real image_(gamma_expand_sRGB)(real nonlinear);
/*
 * Convert a linear sRGB color channel to a sRGB color channel.
 */
static inline real image_(gamma_compress_sRGB)(real linear);
/*
 * Converts an sRGB color value to LAB.
 * Based on http://www.brucelindbloom.com/index.html?Equations.html.
 * Assumes r, g, and b are contained in the set [0, 1].
 * LAB output is NOT restricted to [0, 1]!
 */
int image_(Main_rgb2lab)(THTensor *rgb, THTensor *lab) ;

/*
 * Converts an LAB color value to sRGB.
 * Based on http://www.brucelindbloom.com/index.html?Equations.html.
 * returns r, g, and b in the set [0, 1].
 */
int image_(Main_lab2rgb)(THTensor *lab,THTensor *rgb ) ;
#else
int image_(Main_rgb2lab)(THTensor *rgb, THTensor *lab) ;
int image_(Main_lab2rgb)(THTensor *lab, THTensor *rgb) ;
#endif // TH_REAL_IS_BYTE

/* Vertically flip an image */
int image_(Main_vflip)(THTensor *dst, THTensor *src) ;
/* Horizontally flip an image */
int image_(Main_hflip)(THTensor *dst, THTensor *src) ;
/* flip an image along a specified dimension */
int image_(Main_flip)(THTensor *dst, THTensor *src, long flip_dim);

static inline void image_(Main_bicubicInterpolate)(
  real* src, long* is, long* size, temp_t ix, temp_t iy,
  real* dst, long *os,
  real pad_value, int bounds_check);

/*
 * Warps an image, according to an (x,y) flow field. The flow
 * field is in the space of the destination image, each vector
 * ponts to a source pixel in the original image.
 */
int image_(Main_warp)(THTensor *dst, THTensor *src, THTensor *flowfield, int mode, int offset_mode, int clamp_mode, real pad_value ) ;
int image_(Main_gaussian)(THTensor *dst,
                          temp_t amplitude,
                          int normalize,
                          temp_t sigma_u,
                          temp_t sigma_v,
                          temp_t mean_u,
                          temp_t mean_v);
/*
 * Borrowed from github.com/clementfarabet/lua---imgraph
 * with Clément's permission for implementing y2jet()
 */
int image_(Main_colorize)(THTensor *output, THTensor *input, THTensor *colormap) ;

int image_(Main_rgb2y)(THTensor *rgb, THTensor *yim ) ;
static inline void image_(drawPixel)(THTensor *output, int y, int x,
                                     int cr, int cg, int cb) ;
static inline void image_(drawChar)(THTensor *output, int x, int y, unsigned char c, int size,
                                    int cr, int cg, int cb,
                                    int bg_cr, int bg_cg, int bg_cb) ;

int image_(Main_drawtext)(THTensor *output,  const char* text, long x, long y, int size, int cr, 
													int cg, int cb, int bg_cr, int bg_cg, int bg_cb, int wrap) ;
int image_(Main_drawRect)(THTensor *output,
							long x1long,
							long y1long,
							long x2long,
							long y2long,
							int lineWidth,
							int cr,
							int cg,
							int cb);
#endif // TH_GENERIC_FILE
