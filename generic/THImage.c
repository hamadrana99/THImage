#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/THImage.c"
#else

THTensor* image_(todepth)(THTensor* src, int depth)
{
	THTensor *dst=NULL;	
	if (depth ==1)							//desired is 1 channel, return 1xHxW
	{
		if (src->nDimension == 2)			// we have HxW, all good
		{
			dst = THTensor_(new)();
			THTensor_(unsqueeze1d)(dst, src, 0);
		}
		else if (src->size[0]==1)			//loaded image is 1xHxW, all good
		{
		    dst = src;
		}
		else if (src->size[0]==2)			//loaded image has 2 channels
		{
    		dst = THTensor_(new)();
			THTensor_(select)(dst,src,0,0);	//return just the first channel
		}
		else if (src->size[0]==3)			//loaded image has 3 channels
		{
			THTensor* y = THTensor_(newWithSize2d)(src->size[1],src->size[2]);
			image_(Main_rgb2y)(src, y);
			dst = THTensor_(new)();
			THTensor_(unsqueeze1d)(dst, y, 0);
			THTensor_(free)(src);
		}
		else if (src->size[0]==4)
		{	
			THTensor* y = THTensor_(newWithSize2d)(src->size[1],src->size[2]);
			THTensor* three_channel_slice=THTensor_(new)();
			THTensor_(narrow)(three_channel_slice, src, 0,0,3);
			image_(Main_rgb2y)(three_channel_slice , y);
			dst = THTensor_(new)();
			THTensor_(unsqueeze1d)(dst, y, 0);
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
			dst = THTensor_(newWithSize3d)(3, src->size[0], src->size[1]);
			THTensor *slice = THTensor_(new)();
			THTensor_(select)(slice, dst, 0, 0);
			THTensor_(copy)(slice, src);
			THTensor_(select)(slice, dst, 0, 1);
			THTensor_(copy)(slice, src);
			THTensor_(select)(slice, dst, 0, 2);
			THTensor_(copy)(slice, src);
			THTensor_(free)(src);
		}
		else if (src->size[0]==1)	//loaded image has 1 channel
		{
			dst = THTensor_(newWithSize3d)(3, src->size[1], src->size[2]);
			THTensor *slice = THTensor_(new)();
			THTensor_(select)(slice, dst, 0, 0);
			THTensor_(copy)(slice, src);
			THTensor_(select)(slice, dst, 0, 1);
			THTensor_(copy)(slice, src);
			THTensor_(select)(slice, dst, 0, 2);
			THTensor_(copy)(slice, src);
			THTensor_(free)(src);
		}
		else if (src->size[0]==3)			//loaded image has 3 channels
		{
			dst=src;
		}
		else if (src->size[0]==4)
		{
		    dst = THTensor_(new)();
			THTensor_(narrow)(dst,src,0,0,3);
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
	THTensor *dst=NULL;
	FILE* f = fopen(filename, "rb");
	if (f==NULL) {
		printf("Could not open file%s\n", filename);
		exit(0);
	}
	unsigned char magicBytes [4];
	fread(&magicBytes, 4, 1, f);
	fclose(f);
	if (magicBytes[0] == 0xff && magicBytes[1] == 0xd8 && magicBytes[2] == 0xff) {
		dst = libjpeg_(Main_load)(filename, NULL);
		if (sizeof(real) != 1)	//we don't have a byte tensor, normalize to 0-1
			THTensor_(div)(dst, dst, 255);
		dst=image_(todepth)(dst,depth);
	}
	else if (magicBytes[0] == 0x89 && magicBytes[1] == 0x50 && magicBytes[2] == 0x4e && magicBytes[3] == 0x47) {
		int depth_of_image=0;
		dst = libpng_(Main_load)(filename, NULL, &depth_of_image);
		if (sizeof(real) != 1)	//we don't have a byte tensor, normalize to 0-1
			THTensor_(div)(dst, dst, (depth_of_image==16)? 65535:255);
		dst=image_(todepth)(dst,depth);
	}
	else if (magicBytes[0] == '^' && magicBytes[1] == 'P' && magicBytes[2] == '[' && (magicBytes[3] == '2' || magicBytes[3] == '3') ) {
		dst = libppm_(Main_load)(filename);
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
	THLongTensor_set1d(size,1,height);
	THLongTensor_set1d(size,2,width);
	return size;
}

void image_(save)(char* filename, THTensor* src)
{
	//infer desired file format from extension in the name
	char *pt1, *ext;
	pt1 = strtok (filename,".");
	while (pt1 != NULL) {
	   ext=pt1;
	   pt1 = strtok (NULL, ".");
	}
	if (sizeof(real) != 1)	 {               //we don't have a byte tensor, remap to 0-255
	    real maxval = THTensor_(maxall)(src);
	    if (maxval >1)
		    THTensor_(mul)(src, src, 255.0/maxval);
		else
    		THTensor_(mul)(src, src, 255);
	}
	if ((strcmp(ext,"jpeg")==0) || (strcmp(ext,"JPEG")==0) || (strcmp(ext,"jpg")==0) || (strcmp(ext,"JPG")==0))
	{
		libjpeg_(Main_save)(filename, src, 75, NULL);
	}
	else if ((strcmp(ext,"png")==0) || (strcmp(ext,"PNG")==0))
	{
		libpng_(Main_save)(filename, NULL, src);
	}
	else if ((strcmp(ext,"PPM")==0) || (strcmp(ext,"ppm")==0) || (strcmp(ext,"PGM")==0) || (strcmp(ext,"pgm")==0))
	{
		libppm_(Main_save)(filename, src);
	}
	else
	{
		printf ("Invalid extension in filename\n");
		exit(0);
	}
}
THTensor* image_(specificCrop)(THTensor* src, THTensor* dst, const char *crop_type, long height, long width)
{
    long iheight, iwidth;
    if (src->nDimension == 2) {
        iheight = src->size[0];
        iwidth = src->size[1];
     }
    else if (src->nDimension == 3) {
        iheight = src->size[1];
        iwidth = src->size[2];
     }         
    else {
        printf("Image not HxW or CxHxW\n");
        exit(0);
    }   
    long startx, starty; 
    if (strcmp(crop_type,"c")==0)
    {
        starty = floor((iheight-height)/2);
        startx = floor((iwidth-width)/2);
    }
    else if (strcmp(crop_type,"tl")==0)
    {
        starty = 0;
        startx = 0;
    }
    else if (strcmp(crop_type,"tr")==0)
    {
        starty = 0;
        startx = iwidth-width;
    }
    else if (strcmp(crop_type,"bl")==0)
    {
        starty = iheight-height;
        startx = 0;
    }
    else if (strcmp(crop_type,"br")==0)
    {
        starty = iheight-height;
        startx = iwidth-width;
    }        
    else {
       printf("Invalid crop type\n");
       exit(0);
    }         
    image_(Main_cropNoScale)(src, dst, startx, starty);
    return dst;   
}
THTensor* image_(crop)(THTensor* src, THTensor* dst, long startx, long starty, long endx, long endy)
{
    image_(Main_cropNoScale)(src, dst, startx, starty);
    return dst;
}
THTensor* image_(translate)(THTensor* src, long x, long y)
{
    THTensor *dst = NULL;
    if (src->nDimension == 2) {
        dst = THTensor_(newWithSize2d)(src->size[0], src->size[1]);
    }
    else if (src->nDimension == 3) {
        dst = THTensor_(newWithSize3d)(src->size[0], src->size[1], src->size[2]);
    }    
    else {
        printf("Image not HxW or CxHxW\n");
        exit(0);
    }
    image_(Main_translate)(src, dst, x, y);
    return dst;
}
THTensor* image_(scale)(THTensor* src, long height, long width, const char* mode)
{   
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    }
    THTensor* dst = THTensor_(newWithSize3d)(src->size[0], height, width);
    if (strcmp(mode, "simple")==0)
        image_(Main_scaleSimple)(src, dst);
    else if (strcmp(mode, "bilinear")==0)
        image_(Main_scaleBilinear)(src, dst);
    else if (strcmp(mode, "bicubic")==0)
        image_(Main_scaleBicubic)(src, dst);
    else {
        printf("Invalid interpolation method specified\n");
        exit(0);
     }
    return dst;
}
THTensor* image_(rotate)(THTensor* src, float theta, const char* mode)
{
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    }
    THTensor* dst = THTensor_(newWithSize3d)(src->size[0], src->size[1], src->size[2]);
    if (strcmp(mode, "simple")==0)
        image_(Main_rotate)(src, dst, theta);
    else if (strcmp(mode, "bilinear")==0)
        image_(Main_rotateBilinear)(src, dst, theta);
    else {
        printf("Invalid rotation method specified\n");
        exit(0);
     }
    return dst;
}
THTensor* image_(polar)(THTensor* src, const char* interpolation, const char* mode)
{
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    }
    long maxDist = max(src->size[1],src->size[2]);
    THTensor* dst = THTensor_(newWithSize3d)(src->size[0], maxDist, maxDist);
    if (strcmp(interpolation, "simple")==0) {
            if (strcmp(mode, "valid")==0) {
                image_(Main_polar)(src, dst, 0);
            }
            else if (strcmp(mode, "full")==0) {
                image_(Main_polar)(src, dst, 1);
            } 
            else {
                printf("Invalid mode specified\n");
                exit(0);
             }  
    }
    else if (strcmp(interpolation, "bilinear")==0) {
            if (strcmp(mode, "valid")==0) {
                image_(Main_polarBilinear)(src, dst, 0);
            }
            else if (strcmp(mode, "full")==0) {
                image_(Main_polarBilinear)(src, dst, 1);
            } 
            else {
                printf("Invalid mode specified\n");
                exit(0);
             }              
    }    
    else {
        printf("Invalid Interpolation method specified\n");
        exit(0);
     }  
     return dst;  
}

THTensor* image_(logPolar)(THTensor* src, const char* interpolation, const char* mode)
{
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    }
    long maxDist = max(src->size[1],src->size[2]);
    THTensor* dst = THTensor_(newWithSize3d)(src->size[0], maxDist, maxDist);
    if (strcmp(interpolation, "simple")==0) {
            if (strcmp(mode, "valid")==0) {
                image_(Main_logPolar)(src, dst, 0);
            }
            else if (strcmp(mode, "full")==0) {
                image_(Main_logPolar)(src, dst, 1);
            } 
            else {
                printf("Invalid mode specified\n");
                exit(0);
             }  
    }
    else if (strcmp(interpolation, "bilinear")==0) {
            if (strcmp(mode, "valid")==0) {
                image_(Main_logPolarBilinear)(src, dst, 0);
            }
            else if (strcmp(mode, "full")==0) {
                image_(Main_logPolarBilinear)(src, dst, 1);
            } 
            else {
                printf("Invalid mode specified\n");
                exit(0);
             }              
    }    
    else {
        printf("Invalid Interpolation method specified\n");
        exit(0);
     }  
     return dst;  
}
THTensor* image_(warp)(THTensor* src, THTensor* flow_field, const char* mode, int offset_mode, const char* clamp_mode, real pad_value)
{
    if (src->nDimension != 3) {
        printf("Image should be CxWxH or 1xHxW\n");
        exit(0);
    } 
    if (flow_field->nDimension != 3) {
        printf("Flow field should be 3 dimensional\n");
        exit(0);
    }   
    THTensor *flow=NULL; 
    if (flow_field->size[0]==3)         //a 3 channel image with magnitude in the first channel, discard first channel
    {
            flow = THTensor_(new)();
			THTensor_(narrow)(flow,flow_field,0,1,2);
    }    
    else if (flow_field->size[0]==2)
        flow = flow_field;
    else {
        printf("Flow field should have only 2 or 3 channels\n");
        exit(0);
    } 
    
   
    THTensor* dst = THTensor_(newWithSize3d)(src->size[0], flow->size[1], flow->size[2]);
    int mode_select=0;
    if (strcmp(mode, "simple")==0)
        mode_select=0;
    else if (strcmp(mode, "bilinear")==0)
        mode_select=1;
    else if (strcmp(mode, "bicubic")==0)
        mode_select=2;
    else if (strcmp(mode, "lanczos")==0)
        mode_select=3;
    else {
        printf("Invalid mode specified\n");
        exit(0);
    }  
    int clamp_mode_select=0;
    if (strcmp(clamp_mode, "clamp")==0)
        clamp_mode_select=0;
    else if (strcmp(clamp_mode, "pad")==0)
        clamp_mode_select=1;
    else {
        printf("Invalid clamp mode specified\n");
        exit(0);
    }                 
    image_(Main_warp)(dst, src, flow, mode_select, offset_mode, clamp_mode_select, pad_value);
    return dst;         
}
THTensor* image_(hflip)(THTensor* src, THTensor* dst)
{
     image_(Main_hflip)(dst, src);
     return dst;
}
THTensor* image_(vflip)(THTensor* src)
{
    THTensor* dst=NULL;
    if (src->nDimension == 3)
        dst = THTensor_(newWithSize3d)(src->size[0], src->size[1], src->size[2]);
    else {
        printf("Only CxWxH dimension images supported for vflip\n");
        exit(0);
     }
     image_(Main_vflip)(dst, src);
     return dst;
}
THTensor* image_(flip)(THTensor* src, long flip_dim)
{
    THTensor* dst=NULL;
    if (src->nDimension == 5) {
        long numel = src->size[0]*src->size[1]*src->size[2]*src->size[3]*src->size[4];
        dst = THTensor_(newWithSize1d)(numel);
        THTensor_(resize5d)(dst, src->size[0],src->size[1],src->size[2],src->size[3],src->size[4]);
    }
    else {
        printf("Only 5 dimension images supported for flip with user specified flip dimension\n");
        exit(0);
     }
     image_(Main_flip)(dst, src, flip_dim);
     return dst;
}
THTensor* image_(minMax)(THTensor* src, real min, real max)
{
    THTensor* dst=THTensor_(newClone)(src);
    if (min>max) {
        printf ("Min value greater than Max value\n");
        exit(0);
     }
    real maxval = THTensor_(maxall)(src);
    real minval = THTensor_(minall)(src);
    real multiplier = (max-min)/(maxval-minval);
    THTensor_(mul)(dst, src, multiplier);
    THTensor_(sub)(dst, dst, min);
    return dst;
    
}
THTensor* image_(rgb2y)(THTensor*src)
{
    THTensor* dst = NULL;
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    }   
    THTensor* y = THTensor_(newWithSize2d)(src->size[1], src->size[2]);
	image_(Main_rgb2y)(src, y);
	dst = THTensor_(new)();
	THTensor_(unsqueeze1d)(dst, y, 0);
    return dst;
}

THTensor* image_(rgb2hsv)(THTensor*src)
{
    THTensor* dst = NULL;
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    } 
    dst = THTensor_(newWithSize3d)(src->size[0], src->size[1], src->size[2]);
    image_(Main_rgb2hsv)(src,dst);
    return dst;
}
THTensor* image_(hsv2rgb)(THTensor*src)
{
    THTensor* dst = NULL;
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    } 
    dst = THTensor_(newWithSize3d)(src->size[0], src->size[1], src->size[2]);
    image_(Main_hsv2rgb)(src,dst);
    return dst;
}
THTensor* image_(rgb2hsl)(THTensor*src)
{
    THTensor* dst = NULL;
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    } 
    dst = THTensor_(newWithSize3d)(src->size[0], src->size[1], src->size[2]);
    image_(Main_rgb2hsl)(src,dst);
    return dst;
}
THTensor* image_(hsl2rgb)(THTensor*src)
{
    THTensor* dst = NULL;
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    } 
    dst = THTensor_(newWithSize3d)(src->size[0], src->size[1], src->size[2]);
    image_(Main_hsl2rgb)(src,dst);
    return dst;
}
THTensor* image_(rgb2lab)(THTensor*src)
{
    THTensor* dst = NULL;
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    } 
    dst = THTensor_(newWithSize3d)(src->size[0], src->size[1], src->size[2]);
    image_(Main_rgb2lab)(src,dst);
    return dst;
}
THTensor* image_(lab2rgb)(THTensor*src)
{
    THTensor* dst = NULL;
    if (src->nDimension != 3) {
        printf("Image not CxWxH\n");
        exit(0);
    } 
    dst = THTensor_(newWithSize3d)(src->size[0], src->size[1], src->size[2]);
    image_(Main_lab2rgb)(src,dst);
    return dst;
}

void image_(drawText)(THTensor* src, const char* text, long x, long y, long size, int color_r, int color_g, int color_b, int wrap)
{
    image_(Main_drawtext)(src, text, x, y, size, color_r, color_g, color_b, -1, -1, -1, wrap);
}
void image_(drawRect)(THTensor* src, long startx, long starty, long endx, long endy, int line_width, int color_r, int color_g, int color_b)
{
    image_(Main_drawRect)(src, startx, starty, endx, endy, line_width, color_r, color_g, color_b);
}




#endif
