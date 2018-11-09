#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/image.c"
#else
#include "image.h"
static inline real image_(FromIntermediate)(temp_t x) {
#ifdef TH_REAL_IS_BYTE
  x += 0.5;
  if( x <= 0 ) return 0;
  if( x >= 255 ) return 255;
#endif
  return x;
}

void image_(Main_op_validate)(THTensor *Tsrc, THTensor *Tdst){

  long src_depth = 1;
  long dst_depth = 1;

  if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
		printf("image.op_validate: src not 2 or 3 dimensional\n");
		exit(0);
	}
  if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
  	printf("image.op_validate: dst not 2 or 3 dimensional\n");
  	exit(0);
  }

  if(Tdst->nDimension == 3) dst_depth =  Tdst->size[2];
  if(Tsrc->nDimension == 3) src_depth =  Tsrc->size[2];

  if( (Tdst->nDimension==3 && ( src_depth!=dst_depth)) ||
      (Tdst->nDimension!=Tsrc->nDimension) )  {
    printf("image.op_validate: src and dst depths do not match");
    exit(0);
  }

  if( (Tdst->nDimension==3) && ( src_depth!=dst_depth) ) {
    printf("image.op_validte: src and dst depths do not match");
    exit(0);
  }
}

long image_(Main_op_stride)( THTensor *T, int i){
  if (T->nDimension == 2) {
    if (i == 2) return 0;
    else return T->stride[i];
  }
  return T->stride[i];
}

long image_(Main_op_depth)(THTensor *T){
  if(T->nDimension == 3) return T->size[2]; /* rgb or rgba */
  return 1; /* greyscale */
}

void image_(Main_scaleLinear_rowcol)(THTensor *Tsrc,
                                            THTensor *Tdst,
                                            long src_start,
                                            long dst_start,
                                            long src_stride,
                                            long dst_stride,
                                            long src_len,
                                            long dst_len ) {

  real *src= THTensor_(data)(Tsrc);
  real *dst= THTensor_(data)(Tdst);

  if ( dst_len > src_len ){
    long di;
    float si_f;
    long si_i;
    float scale = (float)(src_len - 1) / (dst_len - 1);

    if ( src_len == 1 ) {
      for( di = 0; di < dst_len - 1; di++ ) {
        long dst_pos = dst_start + di*dst_stride;
        dst[dst_pos] = src[ src_start ];
      }
    } else {
      for( di = 0; di < dst_len - 1; di++ ) {
        long dst_pos = dst_start + di*dst_stride;
        si_f = di * scale; si_i = (long)si_f; si_f -= si_i;

        dst[dst_pos] = image_(FromIntermediate)(
            (1 - si_f) * src[ src_start + si_i * src_stride ] +
            si_f * src[ src_start + (si_i + 1) * src_stride ]);
      }
    }

    dst[ dst_start + (dst_len - 1) * dst_stride ] =
      src[ src_start + (src_len - 1) * src_stride ];
  }
  else if ( dst_len < src_len ) {
    long di;
    long si0_i = 0; float si0_f = 0;
    long si1_i; float si1_f;
    long si;
    float scale = (float)src_len / dst_len;
    float acc, n;

    for( di = 0; di < dst_len; di++ )
      {
        si1_f = (di + 1) * scale; si1_i = (long)si1_f; si1_f -= si1_i;
        acc = (1 - si0_f) * src[ src_start + si0_i * src_stride ];
        n = 1 - si0_f;
        for( si = si0_i + 1; si < si1_i; si++ )
          {
            acc += src[ src_start + si * src_stride ];
            n += 1;
          }
        if( si1_i < src_len )
          {
            acc += si1_f * src[ src_start + si1_i*src_stride ];
            n += si1_f;
          }
        dst[ dst_start + di*dst_stride ] = image_(FromIntermediate)(acc / n);
        si0_i = si1_i; si0_f = si1_f;
      }
  }
  else {
    long i;
    for( i = 0; i < dst_len; i++ )
      dst[ dst_start + i*dst_stride ] = src[ src_start + i*src_stride ];
  }
}


static inline temp_t image_(Main_cubicInterpolate)(temp_t p0,
                                                   temp_t p1,
                                                   temp_t p2,
                                                   temp_t p3,
                                                   temp_t x) {
  temp_t a0 = p1;
  temp_t a1 = p2 - p0;
  temp_t a2 = 2 * p0 - 5 * p1 + 4 * p2 - p3;
  temp_t a3 = 3 * (p1 - p2) + p3 - p0;
  return a0 + 0.5 * x * (a1 + x * (a2 + x * a3));
}


void image_(Main_scaleCubic_rowcol)(THTensor *Tsrc,
                                           THTensor *Tdst,
                                           long src_start,
                                           long dst_start,
                                           long src_stride,
                                           long dst_stride,
                                           long src_len,
                                           long dst_len ) {

  real *src= THTensor_(data)(Tsrc);
  real *dst= THTensor_(data)(Tdst);

  if ( dst_len == src_len ){
    long i;
    for( i = 0; i < dst_len; i++ )
      dst[ dst_start + i*dst_stride ] = src[ src_start + i*src_stride ];
  } else if ( src_len == 1 ) {
     long i;
     for( i = 0; i < dst_len - 1; i++ ) {
       long dst_pos = dst_start + i*dst_stride;
       dst[dst_pos] = src[ src_start ];
     }
  } else {
    long di;
    float si_f;
    long si_i;
    float scale;
    if (dst_len == 1)
      scale = (float)(src_len - 1);
    else
      scale = (float)(src_len - 1) / (dst_len - 1);

    for( di = 0; di < dst_len - 1; di++ ) {
      long dst_pos = dst_start + di*dst_stride;
      si_f = di * scale; si_i = (long)si_f; si_f -= si_i;

      temp_t p0;
      temp_t p1 = src[ src_start + si_i * src_stride ];
      temp_t p2 = src[ src_start + (si_i + 1) * src_stride ];
      temp_t p3;
      if (si_i > 0) {
        p0 = src[ src_start + (si_i - 1) * src_stride ];
      } else {
        p0 = 2 * p1 - p2;
      }
      if (si_i + 2 < src_len) {
        p3 = src[ src_start + (si_i + 2) * src_stride ];
      } else {
        p3 = 2 * p2 - p1;
      }

      temp_t value = image_(Main_cubicInterpolate)(p0, p1, p2, p3, si_f);
      dst[dst_pos] = image_(FromIntermediate)(value);
    }

    dst[ dst_start + (dst_len - 1) * dst_stride ] =
      src[ src_start + (src_len - 1) * src_stride ];
  }
}

int image_(Main_scaleBilinear)(THTensor *Tsrc, THTensor *Tdst) {

  THTensor *Ttmp;
  long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height;
  long src_stride0, src_stride1, src_stride2, src_width, src_height;
  long tmp_stride0, tmp_stride1, tmp_stride2, tmp_width, tmp_height;
  long i, j, k;

  image_(Main_op_validate)(Tsrc,Tdst);

  int ndims;
  if (Tdst->nDimension == 3) ndims = 3;
  else ndims = 2;

  Ttmp = THTensor_(newWithSize2d)(Tsrc->size[0], Tdst->size[1]);

  dst_stride0= image_(Main_op_stride)(Tdst,0);
  dst_stride1= image_(Main_op_stride)(Tdst,1);
  dst_stride2= image_(Main_op_stride)(Tdst,2);
  src_stride0= image_(Main_op_stride)(Tsrc,0);
  src_stride1= image_(Main_op_stride)(Tsrc,1);
  src_stride2= image_(Main_op_stride)(Tsrc,2);
  tmp_stride0= image_(Main_op_stride)(Ttmp,0);
  tmp_stride1= image_(Main_op_stride)(Ttmp,1);
  tmp_stride2= image_(Main_op_stride)(Ttmp,2);
  dst_height=  Tdst->size[0];
  dst_width=   Tdst->size[1];
  src_height=  Tsrc->size[0];
  src_width=   Tsrc->size[1];
  tmp_height=  Ttmp->size[0];
	tmp_width=   Ttmp->size[1];
  
  for(k=0;k<image_(Main_op_depth)(Tsrc);k++) {
    /* compress/expand rows first */
    for(j = 0; j < src_height; j++) {
      image_(Main_scaleLinear_rowcol)(Tsrc,
                                      Ttmp,
                                      0*src_stride1+j*src_stride0+k*src_stride2,
                                      0*tmp_stride1+j*tmp_stride0+k*tmp_stride2,
                                      src_stride1,
                                      tmp_stride1,
                                      src_width,
                                      tmp_width );

    }

    /* then columns */
    for(i = 0; i < dst_width; i++) {
      image_(Main_scaleLinear_rowcol)(Ttmp,
                                      Tdst,
                                      i*tmp_stride1+0*tmp_stride0+k*tmp_stride2,
                                      i*dst_stride1+0*dst_stride0+k*dst_stride2,
                                      tmp_stride0,
                                      dst_stride0,
                                      tmp_height,
                                      dst_height );
    }
  }
  THTensor_(free)(Ttmp);
  return 0;
}

int image_(Main_scaleBicubic)(THTensor *Tsrc, THTensor *Tdst) {

  THTensor *Ttmp;
  long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height;
  long src_stride0, src_stride1, src_stride2, src_width, src_height;
  long tmp_stride0, tmp_stride1, tmp_stride2, tmp_width, tmp_height;
  long i, j, k;

  image_(Main_op_validate)(Tsrc,Tdst);

  int ndims;
  if (Tdst->nDimension == 3) ndims = 3;
  else ndims = 2;

  Ttmp = THTensor_(newWithSize2d)(Tsrc->size[0], Tdst->size[1]);

  dst_stride0= image_(Main_op_stride)(Tdst,0);
  dst_stride1= image_(Main_op_stride)(Tdst,1);
  dst_stride2= image_(Main_op_stride)(Tdst,2);
  src_stride0= image_(Main_op_stride)(Tsrc,0);
  src_stride1= image_(Main_op_stride)(Tsrc,1);
  src_stride2= image_(Main_op_stride)(Tsrc,2);
  tmp_stride0= image_(Main_op_stride)(Ttmp,0);
  tmp_stride1= image_(Main_op_stride)(Ttmp,1);
  tmp_stride2= image_(Main_op_stride)(Ttmp,2);
  dst_height=  Tdst->size[0];
  dst_width=   Tdst->size[1];
  src_height=  Tsrc->size[0];
  src_width=   Tsrc->size[1];
  tmp_height=  Ttmp->size[0];
  tmp_width=   Ttmp->size[1];
  
  for(k=0;k<image_(Main_op_depth)(Tsrc);k++) {
    /* compress/expand rows first */
    for(j = 0; j < src_height; j++) {
      image_(Main_scaleCubic_rowcol)(Tsrc,
                                     Ttmp,
                                     0*src_stride1+j*src_stride0+k*src_stride2,
                                     0*tmp_stride1+j*tmp_stride0+k*tmp_stride2,
                                     src_stride1,
                                     tmp_stride1,
                                     src_width,
                                     tmp_width );
    }

    /* then columns */
    for(i = 0; i < dst_width; i++) {
      image_(Main_scaleCubic_rowcol)(Ttmp,
                                     Tdst,
                                     i*tmp_stride1+0*tmp_stride0+k*tmp_stride2,
                                     i*dst_stride1+0*dst_stride0+k*dst_stride2,
                                     tmp_stride0,
                                     dst_stride0,
                                     tmp_height,
                                     dst_height );
    }
  }
  THTensor_(free)(Ttmp);
  return 0;
}

int image_(Main_scaleSimple)(THTensor *Tsrc, THTensor *Tdst)
{
  real *src, *dst;
  long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height, dst_depth;
  long src_stride0, src_stride1, src_stride2, src_width, src_height, src_depth;
  long i, j, k;
  float scx, scy;

  if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
		printf("image.scaleSimple: src not 2 or 3 dimensional\n");
		exit(0);
	}
  if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
  	printf("image.scaleSimple: dst not 2 or 3 dimensional\n");
  	exit(0);
  }

  src= THTensor_(data)(Tsrc);
  dst= THTensor_(data)(Tdst);

  dst_stride0 = Tdst->stride[0];
  dst_stride1 = Tdst->stride[1];
  dst_stride2 = 0;
  dst_height = Tdst->size[0];
  dst_width = Tdst->size[1];
  dst_depth =  0;
  if(Tdst->nDimension == 3) {
    dst_stride0 = Tdst->stride[2];
    dst_depth = Tdst->size[2];
  }

  src_stride0 = Tsrc->stride[0];
  src_stride1 = Tsrc->stride[1];
	src_stride2 = 0;
  src_height = Tsrc->size[0];
  src_width = Tsrc->size[1];
  src_depth =  0;
  if(Tsrc->nDimension == 3) {
    src_stride2 = Tsrc->stride[2];
    src_depth = Tsrc->size[2];
  }

  if( (Tdst->nDimension==3 && ( src_depth!=dst_depth)) ||
      (Tdst->nDimension!=Tsrc->nDimension) ) {
    printf("image.scaleSimple:%d,%d,%ld,%ld\n",Tsrc->nDimension,Tdst->nDimension,src_depth,dst_depth);
    printf("image.scaleSimple: src and dst depths do not match\n");
    exit(0);
  }

  if( Tdst->nDimension==3 && ( src_depth!=dst_depth) ) {
    printf("image.scale: src and dst depths do not match\n");
    exit(0);
	}
  /* printf("%d,%d -> %d,%d\n",src_width,src_height,dst_width,dst_height); */
  scx=((float)src_width)/((float)dst_width);
  scy=((float)src_height)/((float)dst_height);

#pragma omp parallel for private(j, i, k)
  for(j = 0; j < dst_height; j++) {
    for(i = 0; i < dst_width; i++) {
      float val = 0.0;
      long ii=(long) (((float)i)*scx);
      long jj=(long) (((float)j)*scy);
      if(ii>src_width-1) ii=src_width-1;
      if(jj>src_height-1) jj=src_height-1;

      if(Tsrc->nDimension==2)
        {
          val=src[ii*src_stride1+jj*src_stride0];
          dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
        }
      else
        {
          for(k=0;k<src_depth;k++)
            {
              val=src[ii*src_stride1+jj*src_stride0+k*src_stride2];
              dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
            }
        }
    }
  }
  return 0;
}

int image_(Main_rotate)(THTensor *Tsrc, THTensor *Tdst, float theta)
{
  float cos_theta, sin_theta;
  real *src, *dst;
  long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height, dst_depth;
  long src_stride0, src_stride1, src_stride2, src_width, src_height, src_depth;
  long i, j, k;
  float xc, yc;
  float id,jd;
  long ii,jj;

  if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
		printf("image.rotate: src not 2 or 3 dimensional\n");
		exit(0);
	}
  if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
  	printf("image.rotate: dst not 2 or 3 dimensional\n");
  	exit(0);
  }
  src= THTensor_(data)(Tsrc);
  dst= THTensor_(data)(Tdst);

  if (dst == src) {
    printf("image.rotate: in-place rotate not supported\n");
    exit(0);
  }

  dst_stride0 = Tdst->stride[0];
  dst_stride1 = Tdst->stride[1];
  dst_stride2 = 0;
  dst_height = Tdst->size[0];
  dst_width = Tdst->size[1];
  dst_depth =  0;
  if(Tdst->nDimension == 3) {
    dst_stride2 = Tdst->stride[2];
    dst_depth = Tdst->size[2];
  }

  src_stride0 = Tsrc->stride[0];
  src_stride1 = Tsrc->stride[1];
	src_stride2 = 0;
  src_height = Tsrc->size[0];
  src_width = Tsrc->size[1];
  src_depth =  0;
  if(Tsrc->nDimension == 3) {
    src_stride2 = Tsrc->stride[2];
    src_depth = Tsrc->size[2];
  }

  if( Tsrc->nDimension==3 && Tdst->nDimension==3 && ( src_depth!=dst_depth) )	{
    printf("image.rotate: src and dst depths do not match\n");
    exit(0);
  }

  if( (Tsrc->nDimension!=Tdst->nDimension) ) {
    printf("image.rotate: src and dst depths do not match\n");
    exit(0);
  }

  xc = (src_width-1)/2.0;
  yc = (src_height-1)/2.0;

  sin_theta = sin(theta);
  cos_theta = cos(theta);

  for(j = 0; j < dst_height; j++) {
    jd=j;
    for(i = 0; i < dst_width; i++) {
      float val = -1;
      id= i;

      ii = (long) round(cos_theta*(id-xc) - sin_theta*(jd-yc) + xc);
      jj = (long) round(cos_theta*(jd-yc) + sin_theta*(id-xc) + yc);

      /* rotated corners are blank */
      if(ii>src_width-1) val=0;
      if(jj>src_height-1) val=0;
      if(ii<0) val=0;
      if(jj<0) val=0;

      if(Tsrc->nDimension==2)
        {
          if(val==-1)
            val=src[ii*src_stride1+jj*src_stride0];
          dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
        }
      else
        {
          int do_copy=0; if(val==-1) do_copy=1;
          for(k=0;k<src_depth;k++)
            {
              if(do_copy)
                val=src[ii*src_stride1+jj*src_stride0+k*src_stride2];
              dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
            }
        }
    }
  }
  return 0;
}

int image_(Main_rotateBilinear)(THTensor *Tsrc, THTensor *Tdst, float theta)
{
 real *src, *dst;
  long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height, dst_depth;
  long src_stride0, src_stride1, src_stride2, src_width, src_height, src_depth;
  long i, j, k;
  float xc, yc;
  float id,jd;
  long ii_0, ii_1, jj_0, jj_1;

  if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
		printf("image.rotateBilinear: src not 2 or 3 dimensional\n");
		exit(0);
	}
  if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
  	printf("image.rotateBilinear: dst not 2 or 3 dimensional\n");
  	exit(0);
  }
  src= THTensor_(data)(Tsrc);
  dst= THTensor_(data)(Tdst);

  if (dst == src) {
    printf("image.rotateBilinear: in-place rotate not supported\n");
    exit(0);
  }

  dst_stride0 = Tdst->stride[0];
  dst_stride1 = Tdst->stride[1];
  dst_stride2 = 0;
  dst_height = Tdst->size[0];
  dst_width = Tdst->size[1];
  dst_depth =  0;
  if(Tdst->nDimension == 3) {
    dst_stride2 = Tdst->stride[2];
    dst_depth = Tdst->size[2];
  }

  src_stride0 = Tsrc->stride[0];
  src_stride1 = Tsrc->stride[1];
	src_stride2 = 0;
  src_height = Tsrc->size[0];
  src_width = Tsrc->size[1];
  src_depth =  0;
  if(Tsrc->nDimension == 3) {
    src_stride2 = Tsrc->stride[2];
    src_depth = Tsrc->size[2];
  }

  if( Tsrc->nDimension==3 && Tdst->nDimension==3 && ( src_depth!=dst_depth) ) {
    printf("image.rotateBilinear: src and dst depths do not match\n");
    exit(0);
  }

  if( (Tsrc->nDimension!=Tdst->nDimension) ) {
    printf("image.rotateBilinear: src and dst depths do not match\n");
    exit(0);
  }  

  xc = (src_width-1)/2.0;
  yc = (src_height-1)/2.0;

  for(j = 0; j < dst_height; j++) {
    jd=j;
    for(i = 0; i < dst_width; i++) {
      float val = -1;
      temp_t ri, rj, wi, wj;
      id= i;
      ri = cos(theta)*(id-xc)-sin(theta)*(jd-yc);
      rj = cos(theta)*(jd-yc)+sin(theta)*(id-xc);

      ii_0 = (long)floor(ri+xc);
      ii_1 = ii_0 + 1;
      jj_0 = (long)floor(rj+yc);
      jj_1 = jj_0 + 1;
      wi = ri+xc-ii_0;
      wj = rj+yc-jj_0;

      /* default to the closest value when interpolating on image boundaries (either image pixel or 0) */
      if(ii_1==src_width && wi<0.5) ii_1 = ii_0;
      else if(ii_1>=src_width) val=0;
      if(jj_1==src_height && wj<0.5) jj_1 = jj_0;
      else if(jj_1>=src_height) val=0;
      if(ii_0==-1 && wi>0.5) ii_0 = ii_1;
      else if(ii_0<0) val=0;
      if(jj_0==-1 && wj>0.5) jj_0 = jj_1;
      else if(jj_0<0) val=0;

      if(Tsrc->nDimension==2) {
        if(val==-1)
          val = (1.0 - wi) * (1.0 - wj) * src[ii_0*src_stride1+jj_0*src_stride0]
            + wi * (1.0 - wj) * src[ii_1*src_stride1+jj_0*src_stride0]
            + (1.0 - wi) * wj * src[ii_0*src_stride1+jj_1*src_stride0]
            + wi * wj * src[ii_1*src_stride1+jj_1*src_stride0];
        dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
      } else {
        int do_copy=0; if(val==-1) do_copy=1;
        for(k=0;k<src_depth;k++) {
          if(do_copy) {
            val = (1.0 - wi) * (1.0 - wj) * src[ii_0*src_stride1+jj_0*src_stride0+k*src_stride2]
              + wi * (1.0 - wj) * src[ii_1*src_stride1+jj_0*src_stride0+k*src_stride2]
              + (1.0 - wi) * wj * src[ii_0*src_stride1+jj_1*src_stride0+k*src_stride2]
              + wi * wj * src[ii_1*src_stride1+jj_1*src_stride0+k*src_stride2];
          }
          dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
        }
      }
    }
  }
  return 0;
}

int image_(Main_polar)(THTensor *Tsrc, THTensor *Tdst, int doFull)
{
    real *src, *dst;
    long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height, dst_depth;
    long src_stride0, src_stride1, src_stride2, src_width, src_height, src_depth;
    long i, j, k;
    float id, jd, a, r, m, midY, midX;
    long ii,jj;

		if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
			printf("image.polar: src not 2 or 3 dimensional\n");
			exit(0);
		}
		if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
			printf("image.polar: dst not 2 or 3 dimensional\n");
			exit(0);
		}
		
		src= THTensor_(data)(Tsrc);
		dst= THTensor_(data)(Tdst);

		dst_stride0 = Tdst->stride[0];
		dst_stride1 = Tdst->stride[1];
		dst_stride2 = 0;
		dst_height = Tdst->size[0];
		dst_width = Tdst->size[1];
		dst_depth =  0;
		if(Tdst->nDimension == 3) {
		  dst_stride2 = Tdst->stride[2];
		  dst_depth = Tdst->size[2];
		}

		src_stride0 = Tsrc->stride[0];
		src_stride1 = Tsrc->stride[1];
		src_stride2 = 0;
		src_height = Tsrc->size[0];
		src_width = Tsrc->size[1];
		src_depth =  0;
		if(Tsrc->nDimension == 3) {
		  src_stride2 = Tsrc->stride[2];
		  src_depth = Tsrc->size[2];
		}


    if( Tsrc->nDimension==3 && Tdst->nDimension==3 && ( src_depth!=dst_depth) ) {
        printf("image.polar: src and dst depths do not match\n"); 
        exit(0);
    }

    if( (Tsrc->nDimension!=Tdst->nDimension) ) {
        printf("image.polar: src and dst depths do not match\n"); 
        exit(0);
    }

    // compute maximum distance
    midY = (float) src_height / 2.0;
    midX = (float) src_width  / 2.0;
    if(doFull == 1) {
      m = sqrt((float) src_width * (float) src_width + (float) src_height * (float) src_height) / 2.0;
    }
    else {
      m = (src_width < src_height) ? midX : midY;
    }

    // loop to fill polar image
    for(j = 0; j < dst_height; j++) {               // orientation loop
        jd = (float) j;
        a = (2 * M_PI * jd) / (float) dst_height;   // current angle
        for(i = 0; i < dst_width; i++) {            // radius loop
            float val = -1;
            id = (float) i;
            r = (m * id) / (float) dst_width;       // current distance

            jj = (long) floor( r * cos(a) + midY);  // y-location in source image
            ii = (long) floor(-r * sin(a) + midX);  // x-location in source image

            if(ii>src_width-1) val=0;
            if(jj>src_height-1) val=0;
            if(ii<0) val=0;
            if(jj<0) val=0;

            if(Tsrc->nDimension==2)
            {
                if(val==-1)
                    val=src[ii*src_stride1+jj*src_stride0];
                dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
            }
            else
            {
                int do_copy=0; if(val==-1) do_copy=1;
                for(k=0;k<src_depth;k++)
                {
                    if(do_copy)
                        val=src[ii*src_stride1+jj*src_stride0+k*src_stride2];
                    dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
                }
            }
        }
    }
    return 0;
}
int image_(Main_polarBilinear)(THTensor *Tsrc, THTensor *Tdst, int doFull)
{
    real *src, *dst;
    long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height, dst_depth;
    long src_stride0, src_stride1, src_stride2, src_width, src_height, src_depth;
    long i, j, k;
    float id, jd, a, r, m, midY, midX;
    long ii_0, ii_1, jj_0, jj_1;

    if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
        printf("image.polarBilinear: src not 2 or 3 dimensional\n");
        exit(0);
    }
    if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
        printf("image.polarBilinear: dst not 2 or 3 dimensional\n");
        exit(0);
    }

    src= THTensor_(data)(Tsrc);
    dst= THTensor_(data)(Tdst);

		dst_stride0 = Tdst->stride[0];
		dst_stride1 = Tdst->stride[1];
		dst_stride2 = 0;
		dst_height = Tdst->size[0];
		dst_width = Tdst->size[1];
		dst_depth =  0;
		if(Tdst->nDimension == 3) {
		  dst_stride2 = Tdst->stride[2];
		  dst_depth = Tdst->size[2];
		}

		src_stride0 = Tsrc->stride[0];
		src_stride1 = Tsrc->stride[1];
		src_stride2 = 0;
		src_height = Tsrc->size[0];
		src_width = Tsrc->size[1];
		src_depth =  0;
		if(Tsrc->nDimension == 3) {
		  src_stride2 = Tsrc->stride[2];
		  src_depth = Tsrc->size[2];
		}

    if( Tsrc->nDimension==3 && Tdst->nDimension==3 && ( src_depth!=dst_depth) ) {
        printf("image.polarBilinear: src and dst depths do not match\n"); 
        exit(0);
    }

    if( (Tsrc->nDimension!=Tdst->nDimension) ) {
        printf("image.polarBilinear: src and dst depths do not match\n"); 
        exit(0);
    }

    // compute maximum distance
    midY = (float) src_height / 2.0;
    midX = (float) src_width  / 2.0;
    if(doFull == 1) {
      m = sqrt((float) src_width * (float) src_width + (float) src_height * (float) src_height) / 2.0;
    }
    else {
      m = (src_width < src_height) ? midX : midY;
    }

    // loop to fill polar image
    for(j = 0; j < dst_height; j++) {                 // orientation loop
        jd = (float) j;
        a = (2 * M_PI * jd) / (float) dst_height;     // current angle
        for(i = 0; i < dst_width; i++) {              // radius loop
            float val = -1;
            temp_t ri, rj, wi, wj;
            id = (float) i;
            r = (m * id) / (float) dst_width;         // current distance

            rj =  r * cos(a) + midY;                  // y-location in source image
            ri = -r * sin(a) + midX;                  // x-location in source image

            ii_0=(long)floor(ri);
            ii_1=ii_0 + 1;
            jj_0=(long)floor(rj);
            jj_1=jj_0 + 1;
            wi = ri - ii_0;
            wj = rj - jj_0;

            // switch to nearest interpolation when bilinear is impossible
            if(ii_1>src_width-1 || jj_1>src_height-1 || ii_0<0 || jj_0<0) {
                if(ii_0>src_width-1) val=0;
                if(jj_0>src_height-1) val=0;
                if(ii_0<0) val=0;
                if(jj_0<0) val=0;

                if(Tsrc->nDimension==2)
                {
                    if(val==-1)
                        val=src[ii_0*src_stride1+jj_0*src_stride0];
                    dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
                }
                else
                {
                    int do_copy=0; if(val==-1) do_copy=1;
                    for(k=0;k<src_depth;k++)
                    {
                        if(do_copy)
                            val=src[ii_0*src_stride1+jj_0*src_stride0+k*src_stride2];
                        dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
                    }
                }
            }

            // bilinear interpolation
            else {
                if(Tsrc->nDimension==2) {
                    if(val==-1)
                        val = (1.0 - wi) * (1.0 - wj) * src[ii_0*src_stride1+jj_0*src_stride0]
                        + wi * (1.0 - wj) * src[ii_1*src_stride1+jj_0*src_stride0]
                        + (1.0 - wi) * wj * src[ii_0*src_stride1+jj_1*src_stride0]
                        + wi * wj * src[ii_1*src_stride1+jj_1*src_stride0];
                    dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
                } else {
                    int do_copy=0; if(val==-1) do_copy=1;
                    for(k=0;k<src_depth;k++) {
                        if(do_copy) {
                            val = (1.0 - wi) * (1.0 - wj) * src[ii_0*src_stride1+jj_0*src_stride0+k*src_stride2]
                            + wi * (1.0 - wj) * src[ii_1*src_stride1+jj_0*src_stride0+k*src_stride2]
                            + (1.0 - wi) * wj * src[ii_0*src_stride1+jj_1*src_stride0+k*src_stride2]
                            + wi * wj * src[ii_1*src_stride1+jj_1*src_stride0+k*src_stride2];
                        }
                        dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
                    }
                }
            }
        }
    }
    return 0;
}

int image_(Main_logPolar)(THTensor *Tsrc, THTensor *Tdst, int doFull)
{
    real *src, *dst;
    long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height, dst_depth;
    long src_stride0, src_stride1, src_stride2, src_width, src_height, src_depth;
    long i, j, k;
    float id, jd, a, r, m, midY, midX, fw;
    long ii,jj;

    if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
	    printf("image.logPolar: src not 2 or 3 dimensional\n");
	    exit(0);
    }
    if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
        printf("image.logPolar: dst not 2 or 3 dimensional\n");
        exit(0);
    } 
		
    src= THTensor_(data)(Tsrc);
    dst= THTensor_(data)(Tdst);

		dst_stride0 = Tdst->stride[0];
		dst_stride1 = Tdst->stride[1];
		dst_stride2 = 0;
		dst_height = Tdst->size[0];
		dst_width = Tdst->size[1];
		dst_depth =  0;
		if(Tdst->nDimension == 3) {
		  dst_stride2 = Tdst->stride[2];
		  dst_depth = Tdst->size[2];
		}

		src_stride0 = Tsrc->stride[0];
		src_stride1 = Tsrc->stride[1];
		src_stride2 = 0;
		src_height = Tsrc->size[0];
		src_width = Tsrc->size[1];
		src_depth =  0;
		if(Tsrc->nDimension == 3) {
		  src_stride2 = Tsrc->stride[2];
		  src_depth = Tsrc->size[2];
		}

    if( Tsrc->nDimension==3 && Tdst->nDimension==3 && ( src_depth!=dst_depth) ) {
        printf("image.logPolar: src and dst depths do not match\n"); 
        exit(0);
    }

    if( (Tsrc->nDimension!=Tdst->nDimension) ) {
        printf("image.logPolar: src and dst depths do not match\n"); 
        exit(0);
    }

    // compute maximum distance
    midY = (float) src_height / 2.0;
    midX = (float) src_width  / 2.0;
    if(doFull == 1) {
        m = sqrt((float) src_width * (float) src_width + (float) src_height * (float) src_height) / 2.0;
    }
    else {
        m = (src_width < src_height) ? midX : midY;
    }

    // loop to fill polar image
    fw = log(m) / (float) dst_width;
    for(j = 0; j < dst_height; j++) {               // orientation loop
        jd = (float) j;
        a = (2 * M_PI * jd) / (float) dst_height;   // current angle
        for(i = 0; i < dst_width; i++) {            // radius loop
            float val = -1;
            id = (float) i;

            r = exp(id * fw);

            jj = (long) floor( r * cos(a) + midY);  // y-location in source image
            ii = (long) floor(-r * sin(a) + midX);  // x-location in source image

            if(ii>src_width-1) val=0;
            if(jj>src_height-1) val=0;
            if(ii<0) val=0;
            if(jj<0) val=0;

            if(Tsrc->nDimension==2)
            {
                if(val==-1)
                    val=src[ii*src_stride1+jj*src_stride0];
                dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
            }
            else
            {
                int do_copy=0; if(val==-1) do_copy=1;
                for(k=0;k<src_depth;k++)
                {
                    if(do_copy)
                        val=src[ii*src_stride1+jj*src_stride0+k*src_stride2];
                    dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
                }
            }
        }
    }
    return 0;
}
int image_(Main_logPolarBilinear)(THTensor *Tsrc, THTensor *Tdst, int doFull)
{
    real *src, *dst;
    long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height, dst_depth;
    long src_stride0, src_stride1, src_stride2, src_width, src_height, src_depth;
    long i, j, k;
    float id, jd, a, r, m, midY, midX, fw;
    long ii_0, ii_1, jj_0, jj_1;

    if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
        printf("image.logPolarBilinear: src not 2 or 3 dimensional\n");
        exit(0);
    }
    if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
        printf("image.logPolarBilinear: dst not 2 or 3 dimensional\n");
        exit(0);
    }
    src= THTensor_(data)(Tsrc);
    dst= THTensor_(data)(Tdst);

		dst_stride0 = Tdst->stride[0];
		dst_stride1 = Tdst->stride[1];
		dst_stride2 = 0;
		dst_height = Tdst->size[0];
		dst_width = Tdst->size[1];
		dst_depth =  0;
		if(Tdst->nDimension == 3) {
		  dst_stride2 = Tdst->stride[2];
		  dst_depth = Tdst->size[2];
		}

		src_stride0 = Tsrc->stride[0];
		src_stride1 = Tsrc->stride[1];
		src_stride2 = 0;
		src_height = Tsrc->size[0];
		src_width = Tsrc->size[1];
		src_depth =  0;
		if(Tsrc->nDimension == 3) {
		  src_stride2 = Tsrc->stride[2];
		  src_depth = Tsrc->size[2];
		}

    if( Tsrc->nDimension==3 && Tdst->nDimension==3 && ( src_depth!=dst_depth) ) {
        printf("image.logPolarBilinear: src and dst depths do not match\n"); 
        exit(0);
    }

    if( (Tsrc->nDimension!=Tdst->nDimension) ) {
        printf("image.logPolarBilinear: src and dst depths do not match\n"); 
        exit(0);
    }

    // compute maximum distance
    midY = (float) src_height / 2.0;
    midX = (float) src_width  / 2.0;
    if(doFull == 1) {
        m = sqrt((float) src_width * (float) src_width + (float) src_height * (float) src_height) / 2.0;
    }
    else {
        m = (src_width < src_height) ? midX : midY;
    }

    // loop to fill polar image
    fw = log(m) / (float) dst_width;
    for(j = 0; j < dst_height; j++) {                 // orientation loop
        jd = (float) j;
        a = (2 * M_PI * jd) / (float) dst_height;     // current angle
        for(i = 0; i < dst_width; i++) {              // radius loop
            float val = -1;
            float ri, rj, wi, wj;
            id = (float) i;

            r = exp(id * fw);

            rj =  r * cos(a) + midY;                  // y-location in source image
            ri = -r * sin(a) + midX;                  // x-location in source image

            ii_0=(long)floor(ri);
            ii_1=ii_0 + 1;
            jj_0=(long)floor(rj);
            jj_1=jj_0 + 1;
            wi = ri - ii_0;
            wj = rj - jj_0;

            // switch to nearest interpolation when bilinear is impossible
            if(ii_1>src_width-1 || jj_1>src_height-1 || ii_0<0 || jj_0<0) {
                if(ii_0>src_width-1) val=0;
                if(jj_0>src_height-1) val=0;
                if(ii_0<0) val=0;
                if(jj_0<0) val=0;

                if(Tsrc->nDimension==2)
                {
                    if(val==-1)
                        val=src[ii_0*src_stride1+jj_0*src_stride0];
                    dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
                }
                else
                {
                    int do_copy=0; if(val==-1) do_copy=1;
                    for(k=0;k<src_depth;k++)
                    {
                        if(do_copy)
                            val=src[ii_0*src_stride1+jj_0*src_stride0+k*src_stride2];
                        dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
                    }
                }
            }

            // bilinear interpolation
            else {
                if(Tsrc->nDimension==2) {
                    if(val==-1)
                        val = (1.0 - wi) * (1.0 - wj) * src[ii_0*src_stride1+jj_0*src_stride0]
                        + wi * (1.0 - wj) * src[ii_1*src_stride1+jj_0*src_stride0]
                        + (1.0 - wi) * wj * src[ii_0*src_stride1+jj_1*src_stride0]
                        + wi * wj * src[ii_1*src_stride1+jj_1*src_stride0];
                    dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
                } else {
                    int do_copy=0; if(val==-1) do_copy=1;
                    for(k=0;k<src_depth;k++) {
                        if(do_copy) {
                            val = (1.0 - wi) * (1.0 - wj) * src[ii_0*src_stride1+jj_0*src_stride0+k*src_stride2]
                            + wi * (1.0 - wj) * src[ii_1*src_stride1+jj_0*src_stride0+k*src_stride2]
                            + (1.0 - wi) * wj * src[ii_0*src_stride1+jj_1*src_stride0+k*src_stride2]
                            + wi * wj * src[ii_1*src_stride1+jj_1*src_stride0+k*src_stride2];
                        }
                        dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
                    }
                }
            }
        }
    }
    return 0;
}


int image_(Main_cropNoScale)(THTensor *Tsrc, THTensor *Tdst, long startx, long starty)
{
  real *src, *dst;
  long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height, dst_depth;
  long src_stride0, src_stride1, src_stride2, src_width, src_height, src_depth;
  long i, j, k;

  if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
		printf("image.cropNoScale: src not 2 or 3 dimensional\n");
		exit(0);
	}
  if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
  	printf("image.cropNoScale: dst not 2 or 3 dimensional\n");
  	exit(0);
  }
  src= THTensor_(data)(Tsrc);
  dst= THTensor_(data)(Tdst);

  dst_stride0 = Tdst->stride[0];
  dst_stride1 = Tdst->stride[1];
  dst_stride2 = 0;
  dst_height = Tdst->size[0];
  dst_width = Tdst->size[1];
  dst_depth =  0;
  if(Tdst->nDimension == 3) {
    dst_stride2 = Tdst->stride[2];
    dst_depth = Tdst->size[2];
  }

  src_stride0 = Tsrc->stride[0];
  src_stride1 = Tsrc->stride[1];
	src_stride2 = 0;
  src_height = Tsrc->size[0];
  src_width = Tsrc->size[1];
  src_depth =  0;
  if(Tsrc->nDimension == 3) {
    src_stride2 = Tsrc->stride[2];
    src_depth = Tsrc->size[2];
  }
	
  if( startx<0 || starty<0 || (startx+dst_width>src_width) || (starty+dst_height>src_height)) {
  	printf("image.cropNoScale: crop goes outside bounds of src\n");
  	exit(0);
  }

  if( Tdst->nDimension==3 && ( src_depth!=dst_depth) )	{
    printf("image.cropNoScale: src and dst depths do not match\n");
    exit(0);
  }

  for(j = 0; j < dst_height; j++) {
    for(i = 0; i < dst_width; i++) {
      float val = 0.0;

      long ii=i+startx;
      long jj=j+starty;

      if(Tsrc->nDimension==2)
        {
          val=src[ii*src_stride1+jj*src_stride0];
          dst[i*dst_stride1+j*dst_stride0] = image_(FromIntermediate)(val);
        }
      else
        {
          for(k=0;k<src_depth;k++)
            {
              val=src[ii*src_stride1+jj*src_stride0+k*src_stride2];
              dst[i*dst_stride1+j*dst_stride0+k*dst_stride2] = image_(FromIntermediate)(val);
            }
        }
    }
  }
  return 0;
}

int image_(Main_translate)(THTensor *Tsrc, THTensor *Tdst, long shiftx, long shifty)
{
  real *src, *dst;
  long dst_stride0, dst_stride1, dst_stride2, dst_width, dst_height, dst_depth;
  long src_stride0, src_stride1, src_stride2, src_width, src_height, src_depth;
  long i, j, k;

  if (Tsrc->nDimension!=2 && Tsrc->nDimension!=3) {
		printf("image.translate: src not 2 or 3 dimensional\n");
		exit(0);
	}
  if (Tdst->nDimension!=2 && Tdst->nDimension!=3) {
  	printf("image.translate: dst not 2 or 3 dimensional\n");
  	exit(0);
  } 
  src= THTensor_(data)(Tsrc);
  dst= THTensor_(data)(Tdst);

  dst_stride0 = Tdst->stride[0];
  dst_stride1 = Tdst->stride[1];
  dst_stride2 = 1;
  dst_height = Tdst->size[0];
  dst_width = Tdst->size[1];
  dst_depth =  1;
  if(Tdst->nDimension == 3) {
    dst_stride2 = Tdst->stride[2];
    dst_depth = Tdst->size[2];
  }

  src_stride0 = Tsrc->stride[0];
  src_stride1 = Tsrc->stride[1];
	src_stride2 = 1;
  src_height = Tsrc->size[0];
  src_width = Tsrc->size[1];
  src_depth =  1;
  if(Tsrc->nDimension == 3) {
    src_stride2 = Tsrc->stride[2];
    src_depth = Tsrc->size[2];
  }

  if( Tdst->nDimension==3 && ( src_depth!=dst_depth) )  {
    printf("image.translate: src and dst depths do not match\n");
    exit(0);
  }

  for(j = 0; j < src_height; j++) {
    for(i = 0; i < src_width; i++) {
      long ii=i+shiftx;
      long jj=j+shifty;

      // Check it's within destination bounds, else crop
      if(ii<dst_width && jj<dst_height && ii>=0 && jj>=0) {
        for(k=0;k<src_depth;k++) {
          dst[ii*dst_stride1+jj*dst_stride0+k*dst_stride2] = src[i*src_stride1+j*src_stride0+k*src_stride2];
        }
      }
    }
  }
  return 0;
}

int image_(Main_saturate)(THTensor *input) {
#ifdef TH_REAL_IS_BYTE
  // Noop since necessarily constrained to [0, 255].
#else
  THTensor *output = input;

  TH_TENSOR_APPLY2(real, output, real, input,                       \
                   *output_data = (*input_data < 0) ? 0 : (*input_data > 1) ? 1 : *input_data;)
#endif
  return 1;
}

/*
 * Converts an RGB color value to HSL. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes r, g, and b are contained in the set [0, 1] and
 * returns h, s, and l in the set [0, 1].
 */
int image_(Main_rgb2hsl)(THTensor *rgb, THTensor *hsl) {
  int y,x;
  temp_t r, g, b, h, s, l;
  for (y=0; y<rgb->size[0]; y++) {
    for (x=0; x<rgb->size[1]; x++) {
      // get Rgb
      r = THTensor_(get3d)(rgb, y, x, 0);
      g = THTensor_(get3d)(rgb, y, x, 1);
      b = THTensor_(get3d)(rgb, y, x, 2);
#ifdef TH_REAL_IS_BYTE
      r /= 255;
      g /= 255;
      b /= 255;
#endif
      temp_t mx = max(max(r, g), b);
      temp_t mn = min(min(r, g), b);
      if(mx == mn) {
        h = 0; // achromatic
        s = 0;
        l = mx;
      } else {
        temp_t d = mx - mn;
        if (mx == r) {
          h = (g - b) / d + (g < b ? 6 : 0);
        } else if (mx == g) {
          h = (b - r) / d + 2;
        } else {
          h = (r - g) / d + 4;
        }
        h /= 6;
        l = (mx + mn) / 2;
        s = l > 0.5 ? d / (2 - mx - mn) : d / (mx + mn);
      }

      // set hsl
#ifdef TH_REAL_IS_BYTE
      h *= 255;
      s *= 255;
      l *= 255;
#endif
      THTensor_(set3d)(hsl, y, x, 0, image_(FromIntermediate)(h));
      THTensor_(set3d)(hsl, y, x, 1, image_(FromIntermediate)(s));
      THTensor_(set3d)(hsl, y, x, 2, image_(FromIntermediate)(l));
    }
  }
  return 0;
}

// helper
static inline temp_t image_(hue2rgb)(temp_t p, temp_t q, temp_t t) {
  if (t < 0.) t += 1;
  if (t > 1.) t -= 1;
  if (t < 1./6)
    return p + (q - p) * 6. * t;
  else if (t < 1./2)
    return q;
  else if (t < 2./3)
    return p + (q - p) * (2./3 - t) * 6.;
  else
    return p;
}

/*
 * Converts an HSL color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns r, g, and b in the set [0, 1].
 */
int image_(Main_hsl2rgb)(THTensor *hsl, THTensor *rgb) {
  
  int y,x;
  temp_t r, g, b, h, s, l;
  for (y=0; y<hsl->size[0]; y++) {
    for (x=0; x<hsl->size[1]; x++) {
      // get hsl
      h = THTensor_(get3d)(hsl, y, x, 0);
      s = THTensor_(get3d)(hsl, y, x, 1);
      l = THTensor_(get3d)(hsl, y, x, 2);
#ifdef TH_REAL_IS_BYTE
      h /= 255;
      s /= 255;
      l /= 255;
#endif

      if(s == 0) {
        // achromatic
        r = l;
        g = l;
        b = l;
      } else {
        temp_t q = (l < 0.5) ? (l * (1 + s)) : (l + s - l * s);
        temp_t p = 2 * l - q;
        temp_t hr = h + 1./3;
        temp_t hg = h;
        temp_t hb = h - 1./3;
        r = image_(hue2rgb)(p, q, hr);
        g = image_(hue2rgb)(p, q, hg);
        b = image_(hue2rgb)(p, q, hb);
      }

      // set rgb
#ifdef TH_REAL_IS_BYTE
      r *= 255;
      g *= 255;
      b *= 255;
#endif
      THTensor_(set3d)(rgb, y, x, 0, image_(FromIntermediate)(r));
      THTensor_(set3d)(rgb, y, x, 1, image_(FromIntermediate)(g));
      THTensor_(set3d)(rgb, y, x, 2, image_(FromIntermediate)(b));
    }
  }
  return 0;
}

/*
 * Converts an RGB color value to HSV. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSV_color_space.
 * Assumes r, g, and b are contained in the set [0, 1] and
 * returns h, s, and v in the set [0, 1].
 */
int image_(Main_rgb2hsv)(THTensor *rgb, THTensor *hsv) {

  int y, x;
  temp_t r, g, b, h, s, v;
  for (y=0; y<rgb->size[0]; y++) {
    for (x=0; x<rgb->size[1]; x++) {
      // get Rgb
      r = THTensor_(get3d)(rgb, y, x, 0);
      g = THTensor_(get3d)(rgb, y, x, 1);
      b = THTensor_(get3d)(rgb, y, x, 2);
#ifdef TH_REAL_IS_BYTE
      r /= 255;
      g /= 255;
      b /= 255;
#endif

      temp_t mx = max(max(r, g), b);
      temp_t mn = min(min(r, g), b);
      if(mx == mn) {
        // achromatic
        h = 0;
        s = 0;
        v = mx;
      } else {
        temp_t d = mx - mn;
        if (mx == r) {
          h = (g - b) / d + (g < b ? 6 : 0);
        } else if (mx == g) {
          h = (b - r) / d + 2;
        } else {
          h = (r - g) / d + 4;
        }
        h /= 6;
        s = d / mx;
        v = mx;
      }

      // set hsv
#ifdef TH_REAL_IS_BYTE
      h *= 255;
      s *= 255;
      v *= 255;
#endif
      THTensor_(set3d)(hsv, y, x, 0, image_(FromIntermediate)(h));
      THTensor_(set3d)(hsv, y, x, 1, image_(FromIntermediate)(s));
      THTensor_(set3d)(hsv, y, x, 2, image_(FromIntermediate)(v));
    }
  }
  return 0;
}

/*
 * Converts an HSV color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSV_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns r, g, and b in the set [0, 1].
 */
int image_(Main_hsv2rgb)(THTensor *hsv, THTensor *rgb) {
  int y, x;
  temp_t r, g, b, h, s, v;
  for (y=0; y<hsv->size[0]; y++) {
    for (x=0; x<hsv->size[1]; x++) {
      // get hsv
      h = THTensor_(get3d)(hsv, y, x, 0);
      s = THTensor_(get3d)(hsv, y, x, 1);
      v = THTensor_(get3d)(hsv, y, x, 2);
#ifdef TH_REAL_IS_BYTE
      h /= 255;
      s /= 255;
      v /= 255;
#endif

      int i = floor(h*6.);
      temp_t f = h*6-i;
      temp_t p = v*(1-s);
      temp_t q = v*(1-f*s);
      temp_t t = v*(1-(1-f)*s);

      switch (i % 6) {
      case 0: r = v, g = t, b = p; break;
      case 1: r = q, g = v, b = p; break;
      case 2: r = p, g = v, b = t; break;
      case 3: r = p, g = q, b = v; break;
      case 4: r = t, g = p, b = v; break;
      case 5: r = v, g = p, b = q; break;
      default: r=0; g = 0, b = 0; break;
      }

      // set rgb
#ifdef TH_REAL_IS_BYTE
      r *= 255;
      g *= 255;
      b *= 255;
#endif
      THTensor_(set3d)(rgb, y, x, 0, image_(FromIntermediate)(r));
      THTensor_(set3d)(rgb, y, x, 1, image_(FromIntermediate)(g));
      THTensor_(set3d)(rgb, y, x, 2, image_(FromIntermediate)(b));
    }
  }
  return 0;
}

#ifndef TH_REAL_IS_BYTE
/*
 * Convert an sRGB color channel to a linear sRGB color channel.
 */
static inline real image_(gamma_expand_sRGB)(real nonlinear)
{
  return (nonlinear <= 0.04045) ? (nonlinear / 12.92)
                                : (pow((nonlinear+0.055)/1.055, 2.4));
}

/*
 * Convert a linear sRGB color channel to a sRGB color channel.
 */
static inline real image_(gamma_compress_sRGB)(real linear)
{
  return (linear <= 0.0031308) ? (12.92 * linear)
                               : (1.055 * pow(linear, 1.0/2.4) - 0.055);
}

/*
 * Converts an sRGB color value to LAB.
 * Based on http://www.brucelindbloom.com/index.html?Equations.html.
 * Assumes r, g, and b are contained in the set [0, 1].
 * LAB output is NOT restricted to [0, 1]!
 */
int image_(Main_rgb2lab)(THTensor *rgb, THTensor *lab) {
  
  // CIE Standard
  double epsilon = 216.0/24389.0;
  double k = 24389.0/27.0;
  // D65 white point
  double xn = 0.950456;
  double zn = 1.088754;

  int y,x;
  real r,g,b,l,a,_b;
  for (y=0; y<rgb->size[0]; y++) {
    for (x=0; x<rgb->size[1]; x++) {
      // get RGB
      r = image_(gamma_expand_sRGB)(THTensor_(get3d)(rgb, y, x, 0));
      g = image_(gamma_expand_sRGB)(THTensor_(get3d)(rgb, y, x, 1));
      b = image_(gamma_expand_sRGB)(THTensor_(get3d)(rgb, y, x, 2));

      // sRGB to XYZ
      double X = 0.412453 * r + 0.357580 * g + 0.180423 * b;
      double Y = 0.212671 * r + 0.715160 * g + 0.072169 * b;
      double Z = 0.019334 * r + 0.119193 * g + 0.950227 * b;

      // normalize for D65 white point
      X /= xn;
      Z /= zn;

      // XYZ normalized to CIE Lab
      double fx = X > epsilon ? pow(X, 1/3.0) : (k * X + 16)/116;
      double fy = Y > epsilon ? pow(Y, 1/3.0) : (k * Y + 16)/116;
      double fz = Z > epsilon ? pow(Z, 1/3.0) : (k * Z + 16)/116;
      l = 116 * fy - 16;
      a = 500 * (fx - fy);
      _b = 200 * (fy - fz);

      // set lab
      THTensor_(set3d)(lab, y, x, 0, l);
      THTensor_(set3d)(lab, y, x, 1, a);
      THTensor_(set3d)(lab, y, x, 2, _b);
    }
  }
  return 0;
}

/*
 * Converts an LAB color value to sRGB.
 * Based on http://www.brucelindbloom.com/index.html?Equations.html.
 * returns r, g, and b in the set [0, 1].
 */
int image_(Main_lab2rgb)(THTensor *lab,THTensor *rgb ) {
  
  int y,x;
  real r,g,b,l,a,_b;

  // CIE Standard
  double epsilon = 216.0/24389.0;
  double k = 24389.0/27.0;
  // D65 white point
  double xn = 0.950456;
  double zn = 1.088754;

  for (y=0; y<lab->size[0]; y++) {
    for (x=0; x<lab->size[1]; x++) {
      // get lab
      l = THTensor_(get3d)(lab, y, x, 0);
      a = THTensor_(get3d)(lab, y, x, 1);
      _b = THTensor_(get3d)(lab, y, x, 2);

      // LAB to XYZ
      double fy = (l + 16) / 116;
      double fz = fy - _b / 200;
      double fx = (a / 500) + fy;
      double X = pow(fx, 3);
      if (X <= epsilon)
        X = (116 * fx - 16) / k;
      double Y = l > (k * epsilon) ? pow((l + 16) / 116, 3) : l/k;
      double Z = pow(fz, 3);
      if (Z <= epsilon)
        Z = (116 * fz - 16) / k;

      X *= xn;
      Z *= zn;

      // XYZ to sRGB
      r =  3.2404542 * X - 1.5371385 * Y - 0.4985314 * Z;
      g = -0.9692660 * X + 1.8760108 * Y + 0.0415560 * Z;
      b =  0.0556434 * X - 0.2040259 * Y + 1.0572252 * Z;

      // set rgb
      THTensor_(set3d)(rgb, y, x, 0, image_(gamma_compress_sRGB(r)));
      THTensor_(set3d)(rgb, y, x, 1, image_(gamma_compress_sRGB(g)));
      THTensor_(set3d)(rgb, y, x, 2, image_(gamma_compress_sRGB(b)));
    }
  }
  return 0;
}
#else
int image_(Main_rgb2lab)(THTensor *rgb, THTensor *lab) {
  printf("image.rgb2lab: not supported for torch.ByteTensor\n");
  return -1;
}

int image_(Main_lab2rgb)(THTensor *lab, THTensor *rgb) {
  printf("image.lab2rgb: not supported for torch.ByteTensor\n");
  return -1;
}
#endif // TH_REAL_IS_BYTE

/* Vertically flip an image */
int image_(Main_vflip)(THTensor *dst, THTensor *src) {
  int height = dst->size[0];
  int width = dst->size[1];
  int channels = dst->size[2];
  long *is = src->stride;
  long *os = dst->stride;

  // get raw pointers
  real *dst_data = THTensor_(data)(dst);
  real *src_data = THTensor_(data)(src);

  long k, x, y;
  if (dst_data != src_data) {
      /* not in-place.
       * this branch could be removed by first duplicating the src into dst then doing inplace */
#pragma omp parallel for private(k, x, y)
      for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
      	  for(k=0; k<channels; k++) {
             dst_data[ k*os[2] + (height-1-y)*os[0] + x*os[1] ] = src_data[ k*is[2] + y*is[0] + x*is[1] ];
          }
        }
      }
  } else {
      /* in-place  */
      real swap, * src_px,  * dst_px;
      long half_height = height >> 1;
      for (y=0; y < half_height; y++) {
        for (x=0; x<width; x++) {
	        for(k=0; k<channels; k++) {
                src_px = src_data + k*is[2] + y*is[0] + x*is[1];
                dst_px =  dst_data + k*is[2] + (height-1-y)*is[0] + x*is[1];
                swap = *dst_px;
                *dst_px = *src_px;
                *src_px = swap;
            }
          }
      }
  }

  return 0;
}

/* Horizontally flip an image */
int image_(Main_hflip)(THTensor *dst, THTensor *src) {
  
  int height = dst->size[0];
  int width = dst->size[1];
  int channels = dst->size[2];
  long *is = src->stride;
  long *os = dst->stride;

  // get raw pointers
  real *dst_data = THTensor_(data)(dst);
  real *src_data = THTensor_(data)(src);

  long k, x, y;
  if (dst_data != src_data) {
      /* not in-place.
       * this branch could be removed by first duplicating the src into dst then doing inplace */
#pragma omp parallel for private(k, x, y)
      for (y=0; y<height; y++) {
         for (x=0; x<width; x++) {
      		   for(k=0; k<channels; k++) {
                 dst_data[ k*os[2] + y*os[0] + (width-x-1)*os[1] ] = src_data[ k*is[2] + y*is[0] + x*is[1] ];
              }
          }
      }
  } else {
      /* in-place  */
      real swap, * src_px,  * dst_px;
      long half_width = width >> 1;
      for (y=0; y < height; y++) {
        for (x=0; x<half_width; x++) {
      	  for(k=0; k<channels; k++) {
                src_px = src_data + k*is[2] + y*is[0] + x*is[1];
                dst_px =  dst_data + k*is[2] + y*is[0] + (width-x-1)*is[1];
                swap = *dst_px;
                *dst_px = *src_px;
                *src_px = swap;
            }
          }
      }
  }

  return 0;
}

/* flip an image along a specified dimension */
int image_(Main_flip)(THTensor *dst, THTensor *src, long flip_dim) {

  if ((dst->nDimension != 5) || (src->nDimension != 5)) {
    printf("image.flip: expected 5 dimensions for src and dst\n");
    exit(0);
  }

  if (flip_dim < 1 || flip_dim > dst->nDimension || flip_dim > 5) {
    printf("image.flip: flip_dim out of bounds\n");
    exit(0);
  }
  flip_dim--;  //  Make it zero indexed

  // get raw pointers
  real *dst_data = THTensor_(data)(dst);
  real *src_data = THTensor_(data)(src);
  if (dst_data == src_data) {
    printf("image.flip: in-place flip not supported\n");
    exit(0);
  }

  long size0 = dst->size[0];
  long size1 = dst->size[1];
  long size2 = dst->size[2];
  long size3 = dst->size[3];
  long size4 = dst->size[4];

  if (src->size[0] != size0 || src->size[1] != size1 ||
      src->size[2] != size2 || src->size[3] != size3 ||
      src->size[4] != size4) {
    printf("image.flip: src and dst are not the same size\n");
    exit(0);
  }

  long *is = src->stride;
  long *os = dst->stride;

  long x, y, z, d, t, isrc, idst = 0;
  for (t = 0; t < size0; t++) {
    for (d = 0; d < size1; d++) {
      for (z = 0; z < size2; z++) {
        for (y = 0; y < size3; y++) {
          for (x = 0; x < size4; x++) {
            isrc = t*is[0] + d*is[1] + z*is[2] + y*is[3] + x*is[4];
            // The big switch statement here looks ugly, however on my machine
            // gcc compiles it to a skip list, so it should be fast.
            switch (flip_dim) {
              case 0:
                idst = (size0 - t - 1)*os[0] + d*os[1] + z*os[2] + y*os[3] + x*os[4];
                break;
              case 1:
                idst = t*os[0] + (size1 - d - 1)*os[1] + z*os[2] + y*os[3] + x*os[4];
                break;
              case 2:
                idst = t*os[0] + d*os[1] + (size2 - z - 1)*os[2] + y*os[3] + x*os[4];
                break;
              case 3:
                idst = t*os[0] + d*os[1] + z*os[2] + (size3 - y - 1)*os[3] + x*os[4];
                break;
              case 4:
                idst = t*os[0] + d*os[1] + z*os[2] + y*os[3] + (size4 - x - 1)*os[4];
                break;
            }
            dst_data[ idst ] = src_data[  isrc ];
          }
        }
      }
    }
  }

  return 0;
}

static inline void image_(Main_bicubicInterpolate)(
  real* src, long* is, long* size, temp_t ix, temp_t iy,
  real* dst, long *os,
  real pad_value, int bounds_check)
{
  int i, j, k;
  temp_t arr[4], p[4];

  // Calculate fractional and integer components
  long x_pix = floor(ix);
  long y_pix = floor(iy);
  temp_t dx = ix - x_pix;
  temp_t dy = iy - y_pix;

  for (k=0; k<size[2]; k++) {
    #pragma unroll
    for (i = 0; i < 4; i++) {
      long v = y_pix + i - 1;
      real* data = &src[k * is[2] + v * is[0]];

      #pragma unroll
      for (j = 0; j < 4; j++) {
        long u = x_pix + j - 1;
        if (bounds_check && (v < 0 || v >= size[0] || u < 0 || u >= size[1])) {
          p[j] = pad_value;
        } else {
          p[j] = data[u * is[0]];
        }
      }

      arr[i] = image_(Main_cubicInterpolate)(p[0], p[1], p[2], p[3], dx);
    }

    temp_t value = image_(Main_cubicInterpolate)(arr[0], arr[1], arr[2], arr[3], dy);
    dst[k * os[2]] = image_(FromIntermediate)(value);
  }
}

/*
 * Warps an image, according to an (x,y) flow field. The flow
 * field is in the space of the destination image, each vector
 * ponts to a source pixel in the original image.
 */
int image_(Main_warp)(THTensor *dst, THTensor *src, THTensor *flowfield, int mode, int offset_mode, int clamp_mode, real pad_value ) {
  
  // dims
  int height = dst->size[0];
  int width = dst->size[1];
  int src_height = src->size[0];
  int src_width = src->size[1];
  int channels = dst->size[2];
  long *is = src->stride;
  long *os = dst->stride;
  long *fs = flowfield->stride;

  // get raw pointers
  real *dst_data = THTensor_(data)(dst);
  real *src_data = THTensor_(data)(src);
  real *flow_data = THTensor_(data)(flowfield);

  // resample
  long k,x,y,v,u,i,j;
#pragma omp parallel for private(k, x, y, v, u, i, j)
  for (y=0; y<height; y++) {
    for (x=0; x<width; x++) {
      // subpixel position:
      float flow_y = flow_data[ 0*fs[2] + y*fs[0] + x*fs[1] ];
      float flow_x = flow_data[ 1*fs[2] + y*fs[0] + x*fs[1] ];
      float iy = offset_mode*y + flow_y;
      float ix = offset_mode*x + flow_x;

      // borders
      int off_image = 0;
      if (iy < 0 || iy > src_height - 1 ||
          ix < 0 || ix > src_width - 1) {
        off_image = 1;
      }

      if (off_image == 1 && clamp_mode == 1) {
        // We're off the image and we're clamping the input image to 0
        for (k=0; k<channels; k++) {
          dst_data[ k*os[2] + y*os[0] + x*os[1] ] = pad_value;
        }
      } else {
        ix = MAX(ix,0); ix = MIN(ix,src_width-1);
        iy = MAX(iy,0); iy = MIN(iy,src_height-1);

        // bilinear?
        switch (mode) {
        case 1:  // Bilinear interpolation
          {
            // 4 nearest neighbors:
            long ix_nw = floor(ix);
            long iy_nw = floor(iy);
            long ix_ne = ix_nw + 1;
            long iy_ne = iy_nw;
            long ix_sw = ix_nw;
            long iy_sw = iy_nw + 1;
            long ix_se = ix_nw + 1;
            long iy_se = iy_nw + 1;

            // get surfaces to each neighbor:
            temp_t nw = (ix_se-ix)*(iy_se-iy);
            temp_t ne = (ix-ix_sw)*(iy_sw-iy);
            temp_t sw = (ix_ne-ix)*(iy-iy_ne);
            temp_t se = (ix-ix_nw)*(iy-iy_nw);

            // weighted sum of neighbors:
            for (k=0; k<channels; k++) {
              dst_data[ k*os[2] + y*os[0] + x*os[1] ] = image_(FromIntermediate)(
                  src_data[ k*is[2] +               iy_nw*is[0] +              ix_nw*is[1] ] * nw
                + src_data[ k*is[2] +               iy_ne*is[0] + MIN(ix_ne,src_width-1)*is[1] ] * ne
                + src_data[ k*is[2] + MIN(iy_sw,src_height-1)*is[0] +              ix_sw*is[1] ] * sw
                + src_data[ k*is[2] + MIN(iy_se,src_height-1)*is[0] + MIN(ix_se,src_width-1)*is[1] ] * se);
            }
          }
          break;
        case 0:  // Simple (i.e., nearest neighbor)
          {
            // 1 nearest neighbor:
            long ix_n = floor(ix+0.5);
            long iy_n = floor(iy+0.5);

            // weighted sum of neighbors:
            for (k=0; k<channels; k++) {
              dst_data[ k*os[2] + y*os[0] + x*os[1] ] = src_data[ k*is[2] + iy_n*is[0] + ix_n*is[1] ];
            }
          }
          break;
        case 2:  // Bicubic
          {
            // We only need to do bounds checking if ix or iy are near the edge
            int edge = !(iy >= 1 && iy < src_height - 2 && ix >= 1 && ix < src_width - 2);

            real* dst = dst_data + y*os[0] + x*os[1];
            if (edge) {
              image_(Main_bicubicInterpolate)(src_data, is, src->size, ix, iy, dst, os, pad_value, 1);
            } else {
              image_(Main_bicubicInterpolate)(src_data, is, src->size, ix, iy, dst, os, pad_value, 0);
            }
          }
          break;
        case 3:  // Lanczos
          {
            // Note: Lanczos can be made fast if the resampling period is
            // constant... and therefore the Lu, Lv can be cached and reused.
            // However, unfortunately warp makes no assumptions about resampling
            // and so we need to perform the O(k^2) convolution on each pixel AND
            // we have to re-calculate the kernel for every pixel.
            // See wikipedia for more info.
            // It is however an extremely good approximation to to full sinc
            // interpolation (IIR) filter.
            // Another note is that the version here has been optimized using
            // pretty aggressive code flow and explicit inlining.  It might not
            // be very readable (contact me, Jonathan Tompson, if it is not)

            // Calculate fractional and integer components
            long x_pix = floor(ix);
            long y_pix = floor(iy);

            // Precalculate the L(x) function evaluations in the u and v direction
            #define rad (3)  // This is a tunable parameter: 2 to 3 is OK
            float Lu[2 * rad];  // L(x) for u direction
            float Lv[2 * rad];  // L(x) for v direction
            for (u=x_pix-rad+1, i=0; u<=x_pix+rad; u++, i++) {
              float du = ix - (float)u;  // Lanczos kernel x value
              du = du < 0 ? -du : du;  // prefer not to used std absf
              if (du < 0.000001f) {  // TODO: Is there a real eps standard?
                Lu[i] = 1;
              } else if (du > (float)rad) {
                Lu[i] = 0;
              } else {
                Lu[i] = ((float)rad * sin((float)M_PI * du) *
                  sin((float)M_PI * du / (float)rad)) /
                  ((float)(M_PI * M_PI) * du * du);
              }
            }
            for (v=y_pix-rad+1, i=0; v<=y_pix+rad; v++, i++) {
              float dv = iy - (float)v;  // Lanczos kernel x value
              dv = dv < 0 ? -dv : dv;  // prefer not to used std absf
              if (dv < 0.000001f) {  // TODO: Is there a real eps standard?
                Lv[i] = 1;
              } else if (dv > (float)rad) {
                Lv[i] = 0;
              } else {
                Lv[i] = ((float)rad * sin((float)M_PI * dv) *
                  sin((float)M_PI * dv / (float)rad)) /
                  ((float)(M_PI * M_PI) * dv * dv);
              }
            }
            float sum_weights = 0;
            for (u=0; u<2*rad; u++) {
              for (v=0; v<2*rad; v++) {
                sum_weights += (Lu[u] * Lv[v]);
              }
            }

            for (k=0; k<channels; k++) {
              temp_t result = 0;
              for (u=x_pix-rad+1, i=0; u<=x_pix+rad; u++, i++) {
                long curu = MAX(MIN((long)(src_width-1), u), 0);
                for (v=y_pix-rad+1, j=0; v<=y_pix+rad; v++, j++) {
                  long curv = MAX(MIN((long)(src_height-1), v), 0);
                  temp_t Suv = src_data[k * is[2] + curv * is[0] + curu * is[1]];

                  temp_t weight = Lu[i] * Lv[j];
                  result += (Suv * weight);
                }
              }
              // Normalize by the sum of the weights
              result = result / (float)sum_weights;

              // Again,  I assume that since the image is stored as reals we
              // don't have to worry about clamping to min and max int (to
              // prevent over or underflow)
              dst_data[ k*os[2] + y*os[0] + x*os[1] ] = image_(FromIntermediate)(result);
            }
          }
          break;
        }  // end switch (mode)
      }  // end else
    }
  }

  // done
  return 0;
}


int image_(Main_gaussian)(THTensor *dst,
		temp_t amplitude,
		int normalize,
		temp_t sigma_u,
		temp_t sigma_v,
		temp_t mean_u,
		temp_t mean_v)
{
	long height = dst->size[0];
  long width = dst->size[1];
  long *os = dst->stride;

  real *dst_data = THTensor_(data)(dst);


  mean_u = mean_u * width + 0.5;
  mean_v = mean_v * height + 0.5;

  // Precalculate 1/(sigma*size) for speed (for some stupid reason the pragma
  // omp declaration prevents gcc from optimizing the inside loop on my macine:
  // verified by checking the assembly output)
  temp_t over_sigmau = 1.0 / (sigma_u * width);
  temp_t over_sigmav = 1.0 / (sigma_v * height);

  long v, u;
  temp_t du, dv;
#pragma omp parallel for private(v, u, du, dv)
  for (v = 0; v < height; v++) {
    for (u = 0; u < width; u++) {
      du = (u + 1 - mean_u) * over_sigmau;
      dv = (v + 1 - mean_v) * over_sigmav;
      temp_t value = amplitude * exp(-0.5 * (du*du + dv*dv));
      dst_data[ v*os[0] + u*os[1] ] = image_(FromIntermediate)(value);
    }
  }

  if (normalize) {
    temp_t sum = 0;
    // We could parallelize this, but it's more trouble than it's worth
    for(v = 0; v < height; v++) {
      for(u = 0; u < width; u++) {
        sum += dst_data[ v*os[0] + u*os[1] ];
      }
    }
    temp_t one_over_sum = 1.0 / sum;
#pragma omp parallel for private(v, u)
    for(v = 0; v < height; v++) {
      for(u = 0; u < width; u++) {
        dst_data[ v*os[0] + u*os[1] ] *= one_over_sum;
      }
    }
  }
  return 0;
}

/*
 * Borrowed from github.com/clementfarabet/lua---imgraph
 * with Clément's permission for implementing y2jet()
 */
int image_(Main_colorize)(THTensor *output, THTensor *input, THTensor *colormap) {
  
  // dims
  long height = input->size[0];
  long width = input->size[1];

  // generate color map if not given
  int noColorMap = THTensor_(nElement)(colormap) == 0;
  if (noColorMap) {
    THTensor_(resize2d)(colormap, width*height, 3);
    THTensor_(fill)(colormap, -1);
  }

  // colormap channels
  int channels = colormap->size[1];

  // generate output
  THTensor_(resize3d)(output, height, width, channels);
  int x,y,k;
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      int id = THTensor_(get2d)(input, y, x);
      if (noColorMap) {
        for (k = 0; k < channels; k++) {
          temp_t value = (float)rand() / (float)RAND_MAX;
#ifdef TH_REAL_IS_BYTE
          value *= 255;
#endif
          THTensor_(set2d)(colormap, id, k, image_(FromIntermediate)(value));
        }
      }
      for (k = 0; k < channels; k++) {
        real color = THTensor_(get2d)(colormap, id, k);
        THTensor_(set3d)(output, y, x, k, color);
      }
    }
  }
  // return nothing
  return 0;
}

int image_(Main_rgb2y)(THTensor *rgb, THTensor *yim ) {
  
  if (rgb->nDimension != 3)	{
  	printf("image.rgb2y: src not 3D\n");
  	exit(0);
 	}
  if (yim->nDimension != 2)  {
  	printf("image.rgb2y: dst not 2D\n");
  	exit(0);
  }
  if (rgb->size[0] != yim->size[0])	{
  	printf("image.rgb2y: src and dst not of same height\n");
  	exit(0);
  }
  if (rgb->size[1] != yim->size[1])	{
    printf("image.rgb2y: src and dst not of same width\n");
    exit(0);
  }

  int y, x;
  temp_t r, g, b, yc;
  const int height = rgb->size[0];
  const int width = rgb->size[1];
  for (y=0; y<height; y++) {
    for (x=0; x<width; x++) {
      // get Rgb
      r = THTensor_(get3d)(rgb, y, x, 0);
      g = THTensor_(get3d)(rgb, y, x, 1);
      b = THTensor_(get3d)(rgb, y, x, 2);

      yc = 0.299 * r + 0.587 * g + 0.114 * b;
      THTensor_(set2d)(yim, y, x, image_(FromIntermediate)(yc));
    }
  }
  return 0;
}

static inline void image_(drawPixel)(THTensor *output, int y, int x,
                                     int cr, int cg, int cb) {
#ifdef TH_REAL_IS_BYTE
  THTensor_(set3d)(output, y, x, 0, cr);
  THTensor_(set3d)(output, y, x, 1, cg);
  THTensor_(set3d)(output, y, x, 2, cb);
#else
  THTensor_(set3d)(output, y, x, 0, cr / 255.0f);
  THTensor_(set3d)(output, y, x, 1, cg / 255.0f);
  THTensor_(set3d)(output, y, x, 2, cb / 255.0f);
#endif
}
static inline void image_(drawChar)(THTensor *output, int x, int y, unsigned char c, int size,
                                    int cr, int cg, int cb,
                                    int bg_cr, int bg_cg, int bg_cb) {
  long height = output->size[0];
  long width  = output->size[1];
	long channels = output->size[2];
  
  /* out of bounds condition, return without drawing */
  if((x >= width)            || // Clip right
     (y >= height)           || // Clip bottom
     ((x + 6 * size - 1) < 0) || // Clip left
     ((y + 8 * size - 1) < 0))   // Clip top
    return;

  for(char i = 0; i < 6; i++ ) {
    unsigned char line;
    if (i < 5) {
      line = *(const unsigned char *)(image_ada_font+(c*5) + i);
    } else {
      line = 0x0;
    }
    for(char j = 0; j < 8; j++, line >>= 1) {
      if(line & 0x1) {
        if (size == 1) {
          image_(drawPixel)(output, y+j, x+i, cr, cg, cb);
        }
        else {
          for (int ii = x+(i*size); ii < x+(i*size) + size; ii++) {
            for (int jj = y+(j*size); jj < y+(j*size) + size; jj++) {
              image_(drawPixel)(output, jj, ii, cr, cg, cb);
            }
          }
        }
      } else if (bg_cr != -1 && bg_cg != -1 && bg_cb != -1) {
        if (size == 1) {
          image_(drawPixel)(output, y+j, x+i, bg_cr, bg_cg, bg_cb);
        } else {
          for (int ii = x+(i*size); ii < x+(i*size) + size; ii++) {
            for (int jj = y+(j*size); jj < y+(j*size) + size; jj++) {
              image_(drawPixel)(output, jj, ii, bg_cr, bg_cg, bg_cb);
            }
          }
        }
      }
    }
  }
}

int image_(Main_drawtext)(THTensor *output,  const char* text, long x, long y, int size, int cr, 
													int cg, int cb, int bg_cr, int bg_cg, int bg_cb, int wrap) 
{
 
  long len = strlen(text);

  // dims
  long height = output->size[0];
  long width  = output->size[1];
	long channels = output->size[2];
  
  long cursor_y = y;
  long cursor_x = x;

  for (long cnt = 0; cnt < len; cnt++) {
    unsigned char c = text[cnt];
    if(c == '\n') {
      cursor_y += size*8;
      cursor_x  = x;
    } else if(c == '\r') {
      // skip em
    } else {
      if(wrap && ((cursor_x + size * 6) >= width)) { // Heading off edge?
        cursor_x  = 0;            // Reset x to zero
        cursor_y += size * 8; // Advance y one line
      }
      image_(drawChar)(output, cursor_x, cursor_y, c, size,
                       cr, cg, cb,
                       bg_cr, bg_cg, bg_cb);
      cursor_x += size * 6;
    }
  }

  return 0;
}

int image_(Main_drawRect)(THTensor *output,
							long x1long,
							long y1long,
							long x2long,
							long y2long,
							int lineWidth,
							int cr,
							int cg,
							int cb)
{
  int loffset = lineWidth / 2 + 1;
  int uoffset = lineWidth - loffset - 1;
  int x1l = (int) MAX(0, x1long - loffset);
  int y1l = (int) MAX(0, y1long - loffset);
  int x1u = (int) MIN(output->size[1], x1long + uoffset + 1);
  int y1u = (int) MIN(output->size[0], y1long + uoffset + 1);
  int x2l = (int) MAX(0, x2long - loffset);
  int y2l = (int) MAX(0, y2long - loffset);
  int x2u = (int) MIN(output->size[1], x2long + uoffset + 1);
  int y2u = (int) MIN(output->size[0], y2long + uoffset + 1);

  for (int y = y1l; y < y2u; y++) {
    for (int x = x1l; x < x1u; x++) {
      image_(drawPixel)(output, y, x, cr, cg, cb);
    }
    for (int x = x2l; x < x2u; x++) {
      image_(drawPixel)(output, y, x, cr, cg, cb);
    }
  }
  for (int x = x1l; x < x2u; x++) {
    for (int y = y1l; y < y1u; y++) {
      image_(drawPixel)(output, y, x, cr, cg, cb);
    }
    for (int y = y2l; y < y2u; y++) {
      image_(drawPixel)(output, y, x, cr, cg, cb);
    }
  }

  return 0;
}

#endif // TH_GENERIC_FILE
