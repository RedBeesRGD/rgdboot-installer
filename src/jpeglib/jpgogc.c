/****************************************************************************
* libjpeg - 6b wrapper
*
* The version of libjpeg used in libOGC has been modified to include a memory
* source data manager (jmemsrc.c).
*
* softdev November 2006
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jpgogc.h"

/** Internal ***/
typedef struct {
    JSAMPARRAY      dstbuffer;
    jvirt_sarray_ptr whole_image;
    JDIMENSION      row_width;
    JDIMENSION      data_width;
    int             screen_width;
    int             cur_output_row;
    int             padding;
    int             decoder;
    char           *imgbuffer;
} DSTMEM;

static DSTMEM          dstmgr;

enum {
    JPG_GREY,
    JPG_RGB,
    JPG_NULL
};

static unsigned int colourmap[256]; /*** Used for 256 or greyscale ***/

/****************************************************************************
* CvtRGB
*
* Change two pixels at a time.
****************************************************************************/
static unsigned int
CvtRGB(unsigned char r1, unsigned char g1, unsigned char b1,
       unsigned char r2, unsigned char g2, unsigned char b2)
{
    int             y1,
                    cb1,
                    cr1,
                    y2,
                    cb2,
                    cr2,
                    cb,
                    cr;

    y1 = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
    cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
    cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;

    y2 = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
    cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
    cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;

    cb = (cb1 + cb2) >> 1;
    cr = (cr1 + cr2) >> 1;

    return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}

/****************************************************************************
* JPEG_Colourmap
*
* Set a standard 256 colour palette or greyscale
****************************************************************************/
static void
JPEG_Colourmap(struct jpeg_decompress_struct *cinfo)
{
    JSAMPARRAY      colormap = cinfo->colormap;
    int             num_colours = cinfo->actual_number_of_colors;
    int             i;

    if ( num_colours > 256 )
	    num_colours = 256;
    
    if (colormap != NULL) {
	if (cinfo->out_color_components == 3) {
			/*** RGB Colour Map ***/
	    for (i = 0; i < num_colours; i++)
		colourmap[i] = CvtRGB(GETJSAMPLE(colormap[0][i]),
				      GETJSAMPLE(colormap[1][i]),
				      GETJSAMPLE(colormap[2][i]),
				      GETJSAMPLE(colormap[0][i]),
				      GETJSAMPLE(colormap[1][i]),
				      GETJSAMPLE(colormap[2][i]));
	} else {
			/*** Greyscale colour map - quantized ***/
	    for (i = 0; i < num_colours; i++)
		colourmap[i] = CvtRGB(GETJSAMPLE(colormap[0][i]),
				      GETJSAMPLE(colormap[0][i]),
				      GETJSAMPLE(colormap[0][i]),
				      GETJSAMPLE(colormap[0][i]),
				      GETJSAMPLE(colormap[0][i]),
				      GETJSAMPLE(colormap[0][i]));

	}
    } else {
		/*** MUST be greyscale ***/
	for (i = 0; i < 256; i++)
	    colourmap[i] = CvtRGB(i, i, i, i, i, i);
    }
}

/****************************************************************************
* JPEG_DecodeLine
*
* Decode one scanline from the JPEG image.
****************************************************************************/
static void
JPEG_DecodeLine(struct jpeg_decompress_struct *cinfo,
		JDIMENSION num_scanlines)
{
    JSAMPARRAY      image_ptr;
    register JSAMPROW inptr,
                    outptr;
    register JDIMENSION col;

    if (dstmgr.decoder == JPG_NULL)
	return;

    /*
     * Access next row in virtual array 
     */
    image_ptr = (*cinfo->mem->access_virt_sarray)
	((j_common_ptr) cinfo, dstmgr.whole_image,
	 dstmgr.cur_output_row, (JDIMENSION) 1, TRUE);

    /*
     * Increment row 
     */
    dstmgr.cur_output_row++;

    inptr = dstmgr.dstbuffer[0];
    outptr = image_ptr[0];

    if (dstmgr.decoder == JPG_GREY) {
		/*** Low colour or greyscale ***/
	for (col = cinfo->output_width; col > 0; col--) {
	    *outptr++ = *inptr++;	/* can omit GETJSAMPLE() safely */
	}
    } else {
		/*** 24 bit colour ***/
	for (col = cinfo->output_width; col > 0; col--) {
	    outptr[0] = *inptr++;	/* can omit GETJSAMPLE() safely */
	    outptr[1] = *inptr++;
	    outptr[2] = *inptr++;
	    outptr += 3;
	}
    }

    if (dstmgr.padding)
	*outptr++ = 0;
}

/****************************************************************************
* JPEG_CopyToBuffer
*
* Output the completed image to a memory buffer
****************************************************************************/
void
JPEG_CopyToBuffer(struct jpeg_decompress_struct *cinfo);

void
JPEG_CopyToBuffer(struct jpeg_decompress_struct *cinfo)
{
    JDIMENSION      row,
                    col;
    JSAMPARRAY      image_ptr;
    register JSAMPROW data_ptr;
    unsigned int   *buffer;
    int             offset = 0;

    if (dstmgr.decoder == JPG_NULL)
	return;

	/*** Set output ***/
    buffer = (unsigned int *) dstmgr.imgbuffer;

    /*
     * Write the file body from our virtual array 
     */
    for (row = 0; row < cinfo->output_height; row++) {

	image_ptr = (*cinfo->mem->access_virt_sarray)
	    ((j_common_ptr) cinfo, dstmgr.whole_image, row, (JDIMENSION) 1,
	     FALSE);

	data_ptr = image_ptr[0];

	offset = ((row * dstmgr.screen_width) >> 1);

		/*** Decode to NGC format ***/
	if (dstmgr.decoder == JPG_GREY) {
			/*** Indexed access ***/
	    for (col = 0; col < dstmgr.data_width; col += 2)
		buffer[offset++] =
		    (colourmap[GETJSAMPLE(data_ptr[col])] & 0xffff0000) |
		    (colourmap[GETJSAMPLE(data_ptr[col + 1])] & 0xffff);

	    if (dstmgr.padding)
		buffer[offset] =
		    (colourmap[GETJSAMPLE(data_ptr[col])] & 0xffff0000);

	} else {
			/*** RGB Triples ***/
	    for (col = 0; col < dstmgr.data_width; col += 6) {
		buffer[offset++] =
		    CvtRGB(GETJSAMPLE(data_ptr[col]),
			   GETJSAMPLE(data_ptr[col + 1]),
			   GETJSAMPLE(data_ptr[col + 2]),
			   GETJSAMPLE(data_ptr[col + 3]),
			   GETJSAMPLE(data_ptr[col + 4]),
			   GETJSAMPLE(data_ptr[col + 5]));
	    }

	    if (dstmgr.padding)
		buffer[offset] =
		    CvtRGB(GETJSAMPLE(data_ptr[col]),
			   GETJSAMPLE(data_ptr[col + 1]),
			   GETJSAMPLE(data_ptr[col + 2]), 0, 0, 0);
	}

    }

}

/****************************************************************************
* JPEG_ImageInfo
*
* Set internal information
****************************************************************************/
static void
JPEG_ImageInfo(struct jpeg_decompress_struct *cinfo)
{
	/*** Clear dstmgr ***/
    memset(&dstmgr, 0, sizeof(DSTMEM));

	/*** Get dimensions ***/
    jpeg_calc_output_dimensions(cinfo);

	/*** Determine colour output ***/
    if (cinfo->out_color_space == JCS_GRAYSCALE) {
	dstmgr.decoder = JPG_GREY;
    } else {
	if (cinfo->out_color_space == JCS_RGB) {
	    if (cinfo->quantize_colors)
		dstmgr.decoder = JPG_GREY;
	    else
		dstmgr.decoder = JPG_RGB;
	} else
	    dstmgr.decoder = JPG_NULL;
    }

	/*** Precompute palette ***/
    if (dstmgr.decoder == JPG_GREY)
	JPEG_Colourmap(cinfo);

	/*** Calculate row stride ***/
    dstmgr.row_width = dstmgr.data_width =
	cinfo->output_width * cinfo->output_components;

    dstmgr.screen_width = cinfo->output_width;

	/*** Need to align even ***/
    if (dstmgr.row_width & 1) {
	dstmgr.row_width++;
	dstmgr.screen_width++;
    }

    dstmgr.padding = dstmgr.row_width - dstmgr.data_width;

	/*** Allocate decompression buffer ***/
    dstmgr.dstbuffer = (*cinfo->mem->alloc_sarray)
	((j_common_ptr) cinfo, JPOOL_IMAGE, dstmgr.row_width,
	 (JDIMENSION) 1);

	/*** Allocate whole image buffer ***/
    dstmgr.whole_image = (*cinfo->mem->request_virt_sarray)
	((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
	 dstmgr.row_width, cinfo->output_height, (JDIMENSION) 1);

	/*** Allocate output buffer ***/
    dstmgr.imgbuffer =
	malloc(dstmgr.screen_width * cinfo->output_height * 2);
    memset(dstmgr.imgbuffer, 0,
	   dstmgr.screen_width * cinfo->output_height * 2);

}

/****************************************************************************
* JPEG_Decompress
*
* This is the only function call required. It expects a JPEGIMG structure
* to be prepared as follows:
*
* Inputs:
*	JPEGIMG.inbuffer		-	Pointer to source image
*	JPEGIMG.inbufferlength 	-	Length in bytes of source image
* Outputs:
*	JPEGIMG.outbuffer		-	NULL - Returned from function
*	JPEGIMG.outbufferlength	-	NULL - Length in bytes of outbuffer
*	JPEGIMG.width		-	NULL - Width in pixels of outbuffer
*	JPEGIMG.height		-	NULL - Height in pixels of outbuffer
****************************************************************************/
int
JPEG_Decompress(JPEGIMG * jpegimg)
{
    /*
     * Required jpeg structures 
     */
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JDIMENSION      num_scanlines;
//    JSAMPARRAY      dstbuffer;

    if ((jpegimg->inbuffer == NULL) || (jpegimg->inbufferlength <= 0))
	return 0;

	/*** Initialise the decompressor ***/
    jpeg_create_decompress(&cinfo);

	/*** Set standard error handler ***/
    cinfo.err = jpeg_std_error(&jerr);

	/*** No progress handler required ***/
    cinfo.progress = NULL;

	/*** Set source data ***/
    jpeg_memory_src(&cinfo, (const JOCTET *)jpegimg->inbuffer, jpegimg->inbufferlength);

	/*** Get default information ***/
    jpeg_read_header(&cinfo, TRUE);

	/*** Update with user specified parameters ***/
    if (jpegimg->num_colours) {
	cinfo.desired_number_of_colors = jpegimg->num_colours;
	cinfo.quantize_colors = TRUE;
    }

    if (jpegimg->dct_method)
	cinfo.dct_method = (J_DCT_METHOD)jpegimg->dct_method;

    if (jpegimg->dither_mode)
	cinfo.dither_mode = (J_DITHER_MODE)jpegimg->dither_mode;

    if (jpegimg->greyscale)
	cinfo.out_color_space = JCS_GRAYSCALE;

	/*** Set for decompression ***/
    JPEG_ImageInfo(&cinfo);

	/*** Update user information ***/
    jpegimg->width = dstmgr.screen_width;
    jpegimg->height = cinfo.output_height;
    jpegimg->outbufferlength =
	(cinfo.output_width * cinfo.output_height * 2);
    jpegimg->outbuffer = dstmgr.imgbuffer;

	/*** Start decompressor ***/
    jpeg_start_decompress(&cinfo);

    while (cinfo.output_scanline < cinfo.output_height) {
	num_scanlines = jpeg_read_scanlines(&cinfo, dstmgr.dstbuffer, 1);
		/*** Decode here ***/
	JPEG_DecodeLine(&cinfo, num_scanlines);
    }

	/*** Create XFB compatible output ***/
    JPEG_CopyToBuffer(&cinfo);

	/*** Terminate decompression ***/
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return 1;

}
