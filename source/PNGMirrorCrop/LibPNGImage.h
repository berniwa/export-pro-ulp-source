#ifndef LIBPNGIMAGE_H
#define LIBPNGIMAGE_H

#include <png.h>

#define PNGSIGSIZE 8    //size of PNG-signature (=header of the png-file), standard is 8

#define OK 0
#define NOT_OK 1

//Please do only use this class in the following order:

//1st: ReadPNG [reading image in]
//2st: MirrorPNG, CropPNG [edit image]
//3st: WritePNG [writing edited image back]

//a multiple use of an object is not supported!

//do not use Visual Leak-Detector - it produces errors in the read-process and edit-process

class LibPNGImage
{
public:
  LibPNGImage();
  ~LibPNGImage();
  int ReadPNG(char* const fileName);    //reads the image into the class-buffer from a .png-file
  int WritePNG(char* const fileName);   //writes the image from the class-buffer to a .png-file

  int MirrorPNG();                      //mirrors the image in the class-buffer
  int CropPNG();                        //crops the image in the class-buffer by seeking for the Maxima and Minima of the color black around it

private:

  png_structp mpPNG;          //PNG pointer - name in LibPNG-documentation: 'png_ptr'
  png_infop mpInfo;           //information pointer - name in LibPNG-documentation: 'info_ptr'
  png_infop mpEndInfo;        //end information pointer - name in LibPNG-documentation: 'end_ptr'
  png_bytep* mRowPointers;    //pointer to the image-data - name in LibPNG-documentation: 'row_pointers[height]'
  png_uint_32 mWidth;         //width of the image in Pixel     // !!! PLEASE DO NOT confuse mWidth with mBytesPerRow !!! //
  png_uint_32 mHeight;        //height of the image in Pixel
  png_size_t mBytesPerRow;    //byter per row of the image 
  int mColorType;             //colortype of the image - LibPNG-documentation: color_type is one of PNG_COLOR_TYPE_GRAY,
                              //PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB, or PNG_COLOR_TYPE_RGB_ALPHA.
  int mBitDepth;              //bitdepth of image - LibPNG-documentation: bit_depth is one of 1, 2, 4, 8, or 16, but valid values 
                              //also depend on the color_type selected.
  int mFilter;                //filter method of the image - LibPNGdocumentation: must be PNG_FILTER_TYPE_DEFAULT or, if you are 
                              //writing a PNG to be embedded in a MNG datastream, can also be PNG_INTRAPIXEL_DIFFERENCING
  int mInterlace;             //interlace_type supports to give a preview while image is loading - 
                              //LibPNGdocumentation: interlace is either PNG_INTERLACE_NONE or PNG_INTERLACE_ADAM7
  int mCompression;           //compression of the image - LibPNGdocumentation: compression_type and filter_type MUST currently
                              //be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE
  int mNumOfPasses;           //amount of passes due the the interlacing-image-loading technique
                              //more here: http://en.wikipedia.org/wiki/Interlacing_%28bitmaps%29
  
  int mNumPalette;            //amount of the colors in the color-palette of the image 
                              //(i. g.: Monochrome: Black & White -> num_plte_mono = 2)
  png_colorp mpPalette;       //pointer to the palette of colors (the array where the different colors are saved)

  unsigned char mIsExpanded;  //flag which is set (not from the library!) when the image is expanded from 1, 2, 4-Bitdepth to RGB
  unsigned char mIsCropped;   //flag which is sett (not from the library!) when the image is cropped due to memory-freeing-reasons
  png_uint_32 moldHeight;     //saves the old height of the image for freeing the memory correctly

};

#endif

