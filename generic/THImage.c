#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/THImage.c"
#else

THTensor* image_(todepth)(THTensor* src, int depth)
{
	THTensor *dst;
	if (depth ==1)							//desired is 1 channel, return HxW
	{
		if (src->nDimension == 2)			// we have HxW, all good
		{
			dst=src;
		}
		else if (src->size[0]==1)			//loaded image is 1xHxW
		{
			THTensor_(select)(dst,src,0,0);
		}
		else if (src->size[0]==2)			//loaded image has 2 channels
		{
			THTensor_(select)(dst,src,0,0);	//return just the first channel
		}
		else if (src->size[0]==3)			//loaded image has 3 channels
		{
			THTensor* dst = THTensor_(newWithSize2d)(src->size[1],src->size[2]);
			image_(Main_rgb2y)(src, dst);
			THTensor_(free)(src);
		}
		else if (src->size[0]==4)
		{
			THTensor* dst = THTensor_(newWithSize2d)(src->size[1],src->size[2]);
			THTensor* three_channel_slice;
			THTensor_(narrow)(three_channel_slice, src, 0,0,2);
			image_(Main_rgb2y)(three_channel_slice , dst);
			THTensor_(free)(src);
		}
		else
		{
			printf ("Loaded Image has incorrect number of channels");
			exit(0);
		}
	}
	else if (depth==3)
	{
		if (src->nDimension == 2)			// we have WxH, copy to 3
		{
			THTensor *dst = THTensor_(newWithSize3d)(3, src->size[0], src->size[1]);
			THTensor *slice;
			THTensor_(select)(slice, src, 0, 0);
			THTensor_(copy)(dst, slice);
			THTensor_(select)(slice, src, 0, 1);
			THTensor_(copy)(dst, slice);
			THTensor_(select)(slice, src, 0, 2);
			THTensor_(copy)(dst, slice);
			THTensor_(free)(src);
		}
		else if (src->size[0]==1)	//loaded image has 3 or 4 channels
		{
			THTensor *dst = THTensor_(newWithSize3d)(3, src->size[1], src->size[2]);
			THTensor *slice;
			THTensor_(select)(slice, src, 0, 0);
			THTensor_(copy)(dst, slice);
			THTensor_(select)(slice, src, 0, 1);
			THTensor_(copy)(dst, slice);
			THTensor_(select)(slice, src, 0, 2);
			THTensor_(copy)(dst, slice);
			THTensor_(free)(src);
		}
		else if (src->size[0]==3)			//loaded image has 3 channels
		{
			dst=src;
		}
		else if (src->size[0]==4)
		{
			THTensor_(narrow)(dst,src,0,0,2);
		}
		else
		{
			printf ("Loaded Image has incorrect number of channels");
			exit(0);
		}
	}
	else
	{
		printf("Unsupported Depth\n");
		exit(0);
	}
	return dst;
}

THTensor* image_(load)(const char* filename, int depth)
{
	//read the magic bytes to determine image extension (supported: jpeg, png, pgm, ppm)
	THTensor *dst;
	FILE* f = fopen(filename, "rb");
	if (f==NULL) {
		printf("Could not open file%s\n", filename);
		exit(0);
	}
	unsigned char magicBytes [4];
	fread(&magicBytes, 4, 1, f);
	fclose(f);
	if (magicBytes[0] == 0xff && magicBytes[1] == 0xd8 && magicBytes[2] == 0xff) {
		libjpeg_(Main_load)(filename, NULL, dst);
		if (sizeof(real) != 1)	//we don't have a byte tensor, normalize to 0-1
			THTensor_(div)(dst, dst, 255);
		dst=image_(todepth)(dst,depth);
	}
	else if (magicBytes[0] == 0x89 && magicBytes[1] == 0x50 && magicBytes[2] == 0x4e && magicBytes[3] == 0x47) {
		int depth_of_image=0;
		libpng_(Main_load)(filename, NULL, dst, &depth_of_image);
		if (sizeof(real) != 1)	//we don't have a byte tensor, normalize to 0-1
			THTensor_(div)(dst, dst, (depth_of_image==16)? 65535:255);
		dst=image_(todepth)(dst,depth);
	}
	else if (magicBytes[0] == '^' && magicBytes[1] == 'P' && magicBytes[2] == '[' && (magicBytes[3] == '2' || magicBytes[3] == '3') ) {
		libppm_(Main_load)(filename, dst);
		if (sizeof(real) != 1)	//we don't have a byte tensor, normalize to 0-1
			THTensor_(div)(dst, dst, 255);
		dst=image_(todepth)(dst,depth);
	}
	else {
		printf("Invalid file format.\n");
		exit(0);
	}
	return dst;

}

THLongTensor* image_(getSize)(const char* filename)
{
	THLongTensor* size = THLongTensor_newWithSize1d(3);
	int channels,height,width;
	FILE* f = fopen(filename, "rb");
	if (f==NULL) {
		printf("Could not open file%s\n", filename);
		exit(0);
	}
	unsigned char magicBytes [4];
	fread(&magicBytes, 4, 1, f);
	fclose(f);
	if (magicBytes[0] == 0xff && magicBytes[1] == 0xd8 && magicBytes[2] == 0xff) {
		libjpeg_(Main_size)(filename, &channels, &height, &width);
	}
	else if (magicBytes[0] == 0x89 && magicBytes[1] == 0x50 && magicBytes[2] == 0x4e && magicBytes[3] == 0x47) {
		libpng_(Main_size)(filename, &channels, &height, &width);
	}
	else if (magicBytes[0] == '^' && magicBytes[1] == 'P' && magicBytes[2] == '[' && (magicBytes[3] == '2' || magicBytes[3] == '3') ) {
		libppm_(Main_size)(filename, &channels, &height, &width);
	}
	else {
		printf("Invalid file format.\n");
		exit(0);
	}
	THLongTensor_set1d(size,0,channels);
	THLongTensor_set1d(size,0,height);
	THLongTensor_set1d(size,0,width);
	return size;
}

void image_(save)(char* filename, THTensor* src)
{
	//infer desired file format from extension in the name
	char *pt1, *ext;
	pt1 = strtok (filename,".");
	while (pt1 != NULL) {
	   printf("%s\n", pt1);
	   ext=pt1;
	   pt1 = strtok (NULL, ".");
	}
	if (strcmp(ext,"jpeg") || strcmp(ext,"JPEG")|| strcmp(ext,"jpg") || strcmp(ext,"JPG"))
	{
		libjpeg_(Main_save)(filename, src, 75, NULL);
	}
	else if (strcmp(ext,"png") || strcmp(ext,"PNG"))
	{
		libpng_(Main_save)(filename, NULL, src);
	}
	else if (strcmp(ext,"PPM") || strcmp(ext,"ppm") || strcmp(ext,"PGM") || strcmp(ext,"pgm"))
	{
		libppm_(Main_save)(filename, src);
	}
	else
	{
		printf ("Invalid extension in filename\n");
		exit(0);
	}
}
void image_(crop)(THTensor* src, THTensor*dst, long startx, long starty, long endx, long endy){}
void image_(translate)(THTensor* src, THTensor* dst, long x, long y){}
void image_(scale)(THTensor* src, THTensor* dst, long width, long height, const char* mode){}
void image_(rotate)(THTensor* src, THTensor* dst, float theta, const char* mode){}
void image_(polar)(THTensor* src, THTensor* dst, const char* interpolation, const char* mode){}
void image_(logPolar)(THTensor* src, THTensor* dst, const char* interpolation, const char* mode){}
void image_(warp)(THTensor* src, THTensor* dst, const char* warp){}
void image_(affineTransorm)(THTensor* src, THTensor* dst, THTensor* matrix){}
void image_(hflip)(THTensor* src, THTensor* dst){}
void image_(vflip)(THTensor* src, THTensor* dst){}
void image_(flip)(THTensor* src, THTensor* dst, long flip_dim){}
void image_(minMax)(THTensor* src, THTensor* dst, real max, real min){}

#endif
