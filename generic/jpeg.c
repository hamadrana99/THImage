#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/jpeg.c"
#else
/*
 * Here's the routine that will replace the standard error_exit method:
 */
 // Here's the routine that will replace the standard error_exit method:
METHODDEF(void) libjpeg_(Main_error) (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* See below. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

 // Here's the routine that will replace the standard output_message method:
METHODDEF(void) libjpeg_(Main_output_message) (j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  (*cinfo->err->format_message) (cinfo, myerr->msg);
}
int libjpeg_(Main_size)(const char *filename, int *height, int *width, int *channels)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my_error_mgr jerr;
  /* More stuff */
  FILE * infile;		/* source file */

  
  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if ((infile = fopen(filename, "rb")) == NULL)
  {
    printf("cannot open file <%s> for reading", filename);
    return -1;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = libjpeg_(Main_error);
  jerr.pub.output_message = libjpeg_(Main_output_message);
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    printf(jerr.msg);
    return -1;
  }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  (*height) = cinfo.output_height;
  (*width) = cinfo.output_width;
  (*channels) = cinfo.output_components;

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose(infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
  return 3;
}

THTensor* libjpeg_(Main_load)(const char *filename, THByteTensor *src)
{
  
#if !defined(HAVE_JPEG_MEM_SRC)
  if (src != NULL) {
     printf(JPEG_MEM_SRC_ERR_MSG);
     exit(0);
  }
#endif

  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my_error_mgr jerr;
  /* More stuff */
  FILE * infile;		    /* source file (if loading from file) */
  unsigned char * inmem;    /* source memory (if loading from memory) */
  unsigned long inmem_size; /* source memory size (bytes) */
  JSAMPARRAY buffer;		/* Output row buffer */
  /* int row_stride;		/1* physical row width in output buffer *1/ */
  int i, k;


  if (src == NULL) {
    
    /* In this example we want to open the input file before doing anything else,
     * so that the setjmp() error recovery below can assume the file is open.
     * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
     * requires it in order to read binary files.
     */

    if ((infile = fopen(filename, "rb")) == NULL)
    {
      printf("cannot open file <%s> for reading\n", filename);
      exit(0);
    }
  } else {
    /* We're loading from a ByteTensor */
    inmem = THByteTensor_data(src);
    inmem_size = src->size[0];
    infile = NULL;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = libjpeg_(Main_error);
  jerr.pub.output_message = libjpeg_(Main_output_message);
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    if (infile) {
      fclose(infile);
    }
    printf(jerr.msg);
    exit(0);
  }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */
  if (src == NULL) {
    jpeg_stdio_src(&cinfo, infile);
  } else {
    jpeg_mem_src(&cinfo, inmem, inmem_size);
  }

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */

  /* Make a one-row-high sample array that will go away when done with image */
  const unsigned int chans = cinfo.output_components;
  const unsigned int height = cinfo.output_height;
  const unsigned int width = cinfo.output_width;
  THTensor *tensor = THTensor_(newWithSize3d)(height, width, chans);
  real *tdata = THTensor_(data)(tensor);
  buffer = (*cinfo.mem->alloc_sarray)
    ((j_common_ptr) &cinfo, JPOOL_IMAGE, chans * width, 1);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
    const unsigned int j = cinfo.output_scanline-1;

   	real *td = tdata+j*width*chans;
    for(i = 0; i < chans*width; i++) {
      *td++ = (real)buffer[0][i];
    }
  }
  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  if (infile) {
    fclose(infile);
  }

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
  return tensor;
}

/*
 * save function
 *
 */
int libjpeg_(Main_save)(const char *filename,
						THTensor *tensor,
						int quality ,
						THByteTensor* tensor_dest ) {
  
#if !defined(HAVE_JPEG_MEM_DEST)
  if (tensor_dest != NULL) {					//we want to save to a tensor destination
    printf(JPEG_MEM_DEST_ERR_MSG);
    return -1;
  }
#endif

  unsigned char *inmem = NULL;  /* destination memory (if saving to memory) */
  unsigned long inmem_size = 0;  /* destination memory size (bytes) */

  THTensor *tensorc = THTensor_(newContiguous)(tensor);
  real *tensor_data = THTensor_(data)(tensorc);

 
  if (quality < 0 || quality > 100) {
    printf("quality should be between 0 and 100\n");
    return -1;
  }

  /* jpeg struct */
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  /* pointer to raw image */
  unsigned char *raw_image = NULL;

  /* dimensions of the image we want to write */
  int width=0, height=0, bytes_per_pixel=0;
  int color_space=0;
  if (tensorc->nDimension == 3) {
    height = tensorc->size[0];
    width = tensorc->size[1];
    bytes_per_pixel = tensorc->size[2];
    if (bytes_per_pixel == 3) {
      color_space = JCS_RGB;
    } else if (bytes_per_pixel == 1) {
      color_space = JCS_GRAYSCALE;
    } else {
      printf("tensor should have 1 or 3 channels (gray or RGB)\n");
	  return -1;
    }
  } else if (tensorc->nDimension == 2) {
    bytes_per_pixel = 1;
    height = tensorc->size[0];
    width = tensorc->size[1];
    color_space = JCS_GRAYSCALE;
  } else {
    printf("supports only 1 or 3 dimension tensors\n");
	return -1;
  }

  /* alloc raw image data */
  raw_image = (unsigned char *)malloc((sizeof (unsigned char))*width*height*bytes_per_pixel);

  /* convert tensor to raw bytes */
  long i;
  for (i=0; i<(height*width*bytes_per_pixel); i++) {
        raw_image[i] = tensor_data[i];
  }
  /* this is a pointer to one row of image data */
  JSAMPROW row_pointer[1];
  FILE *outfile = NULL;
  if (tensor_dest==NULL) {
    outfile = fopen( filename, "wb" );
    if ( !outfile ) {
      printf("Error opening output jpeg file %s\n!", filename );
      return -1;
    }
  }

  cinfo.err = jpeg_std_error( &jerr );
  jpeg_create_compress(&cinfo);

  /* specify data source (eg, a file) */
  if (tensor_dest==NULL) {
    jpeg_stdio_dest(&cinfo, outfile);
  } else {
    jpeg_mem_dest(&cinfo, &inmem, &inmem_size);
  }

  /* Setting the parameters of the output file here */
  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = bytes_per_pixel;
  cinfo.in_color_space = color_space;

  /* default compression parameters, we shouldn't be worried about these */
  jpeg_set_defaults( &cinfo );
  jpeg_set_quality(&cinfo, quality, (boolean)0);

  /* Now do the compression .. */
  jpeg_start_compress( &cinfo, TRUE );
  
  /* like reading a file, this time write one row at a time */
  while( cinfo.next_scanline < cinfo.image_height ) {
    row_pointer[0] = &raw_image[ cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
    jpeg_write_scanlines( &cinfo, row_pointer, 1 );
  }
  
  
	/* similar to read file, clean up after we're done compressing */
  jpeg_finish_compress( &cinfo );
  jpeg_destroy_compress( &cinfo );

  if (outfile != NULL) {
    fclose( outfile );
  }

  if (tensor_dest != NULL) {

    THByteTensor_resize1d(tensor_dest, inmem_size);  /* will fail if it's not a Byte Tensor */
    unsigned char* tensor_dest_data = THByteTensor_data(tensor_dest);
    memcpy(tensor_dest_data, inmem, inmem_size);
    free(inmem);
  }

  /* some cleanup */
  free(raw_image);
  THTensor_(free)(tensorc);

  /* success code is 1! */
  return 1;
}
#endif
