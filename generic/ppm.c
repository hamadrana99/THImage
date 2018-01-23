#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/ppm.c"
#else

static int libppm_(Main_size)(const char *filename, int *channels, int *height, int *width)
{
  FILE* fp = fopen ( filename, "r" );
  if ( !fp ) {
    printf("cannot open file <%s> for reading\n", filename);
	return -1;
  }

  long W,H,C;
  char p,n;

  // magic number
  p = (char)getc(fp);
  if ( p != 'P' ) {
    W = H = 0;
    fclose(fp);
    printf("corrupted file\n");
	return -1;
  }

  n = (char)getc(fp);

  // Dimensions
  W = ppm_get_long(fp);
  H = ppm_get_long(fp);

  if ( n=='3' || n == '6') {
    C = 3;
  } else if ( n=='2' || n=='5' ) {
    C = 1;
  } else {
    W=H=C=0;
    fclose ( fp );
    printf("unsupported magic number: P%c\n", n);
    return -1;
  }

  fclose(fp);

  (*channels) = C;
  (*height)  = H;
  (*width) = W;

  return 3;
}

static int libppm_(Main_load)(const char *filename,
								THTensor *tensor)			//destination
{
  FILE* fp = fopen ( filename, "r" );
  if ( !fp ) {
    printf("cannot open file <%s> for reading\n", filename);
    return -1;
  }

  long W,H,C;
  char p,n;
  int D, bps, bpc;

  // magic number
  p = (char)getc(fp);
  if ( p != 'P' ) {
    W = H = 0;
    fclose(fp);
    printf("corrupted file\n");
    return -1;
  }

  n = (char)getc(fp);

  // Dimensions
  W = ppm_get_long(fp);
  H = ppm_get_long(fp);

  // Max color value
  D = ppm_get_long(fp);

  // Either 8 or 16 bits per pixel
  bps = 8;
  if (D > 255) {
     bps = 16;
  }
  bpc = bps / 8;

  //printf("Loading PPM\nMAGIC: %c%c\nWidth: %ld, Height: %ld\nChannels: %d, Bits-per-pixel: %d\n", p, n, W, H, D, bps);

  // load data
  int ok = 1;
  size_t s;
  unsigned char *r = NULL;
  if ( n=='6' ) {
    C = 3;
    s = W*H*C*bpc;
    r = malloc(s);
    if (fread ( r, 1, s, fp ) < s) ok = 0;
  } else if ( n=='5' ) {
    C = 1;
    s = W*H*C*bpc;
    r = malloc(s);
    if (fread ( r, 1, s, fp ) < s) ok = 0;
  } else if ( n=='3' ) {
    int c,i;
    C = 3;
    s = W*H*C;
    r = malloc(s);
    for (i=0; i<s; i++) {
      if (fscanf ( fp, "%d", &c ) != 1) { ok = 0; break; }
      r[i] = 255*c / D;
    }
  } else if ( n=='2' ) {
    int c,i;
    C = 1;
    s = W*H*C;
    r = malloc(s);
    for (i=0; i<s; i++) {
      if (fscanf ( fp, "%d", &c ) != 1) { ok = 0; break; }
      r[i] = 255*c / D;
    }
  } else {
    W=H=C=0;
    fclose ( fp );
    printf("unsupported magic number: P%c\n", n);
    return -1;
  }

  if (!ok) {
    fclose ( fp );
    printf("corrupted file or read error\n");
	return -1;
  }

  // export tensor
  tensor = THTensor_(newWithSize3d)(C,H,W);
  real *data = THTensor_(data)(tensor);
  long i,k,j=0;
  int val;
  for (i=0; i<W*H; i++) {
    for (k=0; k<C; k++) {
       if (bpc == 1) {
          data[k*H*W+i] = (real)r[j++];
       } else if (bpc == 2) {
          val = r[j] | (r[j+1] << 8);
          j += 2;
          data[k*H*W+i] = (real)val;
       }
    }
  }

  // cleanup
  free(r);
  fclose(fp);

  // return loaded image
  return 1;
}

int libppm_(Main_save)(const char *filename,
						THTensor *tensor) 		//source
{
  // get args
  THTensor *tensorc = THTensor_(newContiguous)(tensor);
  real *data = THTensor_(data)(tensorc);

  // dimensions
  long C,H,W,N;
  if (tensorc->nDimension == 3) {
    C = tensorc->size[0];
    H = tensorc->size[1];
    W = tensorc->size[2];
  } else if (tensorc->nDimension == 2) {
    C = 1;
    H = tensorc->size[0];
    W = tensorc->size[1];
  } else {
    C=W=H=0;
    printf("can only export tensor with geometry: HxW or 1xHxW or 3xHxW\n");
    return -1;
  }
  N = C*H*W;

  // convert to chars
  unsigned char *bytes = (unsigned char*)malloc(N);
  long i,k,j=0;
  for (i=0; i<W*H; i++) {
    for (k=0; k<C; k++) {
      bytes[j++] = (unsigned char)data[k*H*W+i];
    }
  }

  // open file
  FILE* fp = fopen(filename, "w");
  if ( !fp ) {
    printf("cannot open file <%s> for writing\n", filename);
    return -1;
  }

  // write 3 or 1 channel(s) header
  if (C == 3) {
    fprintf(fp, "P6\n%ld %ld\n%d\n", W, H, 255);
  } else {
    fprintf(fp, "P5\n%ld %ld\n%d\n", W, H, 255);
  }

  // write data
  fwrite(bytes, 1, N, fp);

  // cleanup
  THTensor_(free)(tensorc);
  free(bytes);
  fclose (fp);

  // return result
  return 1;
}

#endif
