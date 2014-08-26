#include <iostream>
#include <stdlib.h>

#include "LibPNGImage.h"

// please note that this is a C++ and C-mix due to the reason that the LibPNG-Library
// is made for C on Linux

#define PNG_HEADER_SIZE 4   //8 is the maximum size that can be checked by LibPNG

void FreeRowPointers(png_bytep* rowPointers, png_uint_32 const height){
  for (size_t y = 0; y < height; y++){
    free(rowPointers[y]);
    rowPointers[y] = 0;
  }
  free(rowPointers);
  rowPointers = 0;
}

LibPNGImage::LibPNGImage()
{
  mRowPointers = 0; //TODO: CTOR does not be executed
  mIsExpanded = 0;
  mIsCropped = 0;
}

LibPNGImage::~LibPNGImage(){
  if (mRowPointers != 0){
    if (mIsCropped) FreeRowPointers(mRowPointers, moldHeight);
    else            FreeRowPointers(mRowPointers, mHeight);
  }  
}


//reads the image into the class-buffer from a .png-file
int LibPNGImage::ReadPNG(char* const fileName){
  if (fileName == 0){
    std::cerr << "[LibPNGImage::ReadPNG] - FileName: " << fileName <<
      "is a zero-pointer" << std::endl;
    return NOT_OK;
  }

  unsigned char header[PNG_HEADER_SIZE];

  /* open file and test it for being a png*/
  FILE *fp = fopen(fileName, "rb");
  if (!fp){
    std::cerr << "[LibPNGImage::ReadPNG] - File: " << fileName <<
      "could not be opened for reading" << std::endl;
    return NOT_OK;    
  }

  /* read in of the signature bytes */
  if (fread(header, 1, PNG_HEADER_SIZE, fp) != PNG_HEADER_SIZE){
    std::cerr << "[LibPNGImage::ReadPNG] - File: " << fileName <<
      "'fread' failed" << std::endl;
    fclose(fp);
    return NOT_OK;
  }

  /* Compare the first PNG_HEADER_SIZE-bytes of the signature.*/
  if (png_sig_cmp(header, 0, PNG_HEADER_SIZE)){
    std::cerr << "[LibPNGImage::ReadPNG] - File: " << fileName <<
      "is not recognized as a PNG file" << std::endl;
    fclose(fp);
    return NOT_OK;
  }

  /*initializing structs*/
  mpPNG = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!mpPNG){
    std::cerr << "[LibPNGImage::ReadPNG] - 'png_create_read_struct' failed" << std::endl;
    fclose(fp);
    return NOT_OK;
  }

  mpInfo = png_create_info_struct(mpPNG);
  if (!mpInfo){
    std::cerr << "[LibPNGImage::ReadPNG] - 'png_create_info_struct' failed" << std::endl;
    png_destroy_read_struct(&mpPNG, (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    return NOT_OK;
  }

  mpEndInfo = png_create_info_struct(mpPNG);
  if (!mpEndInfo){
    std::cerr << "[LibPNGImage::ReadPNG] - 'png_create_info_struct ( end )' failed" << std::endl;
    png_destroy_read_struct(&mpPNG, &mpInfo, (png_infopp)NULL);
    fclose(fp);
    return NOT_OK;
  }

  if (setjmp(png_jmpbuf(mpPNG))){
    std::cerr << "[LibPNGImage::ReadPNG] - init_io failed" << std::endl;
    png_destroy_read_struct(&mpPNG, &mpInfo, &mpEndInfo);
    fclose(fp);
    return NOT_OK;
  }
  png_init_io(mpPNG, fp);   //initializing input and output-methods from LibPNG
  png_set_sig_bytes(mpPNG, PNG_HEADER_SIZE);  //tells LibPNG that we actually have read the header before (for checking if it is really a PNG in this case)
  png_read_info(mpPNG, mpInfo); //low-level-route of reading all file information up to the actual image data
  // png_read_end(mpPNG, mpEndInfo); //commented out: chunks after image are broken - so it throws an error
  png_get_IHDR(mpPNG, mpInfo, &mWidth, &mHeight, &mBitDepth, &mColorType, &mInterlace, &mCompression, &mFilter);  //reading image-parameters into members

  if (mBitDepth < 8){
    png_set_expand_gray_1_2_4_to_8(mpPNG);  //expanding to a full byte per Pixel from 1 bit per Pixel
    mIsExpanded = 0xFF;                     //setting flag for identifying expansion later in the code
  }
  else if (mBitDepth % 8 != 0){             //error when wrong bitdepth
    std::cerr << "[LibPNGImage::ReadPNG] - bytes per pixel are bigger than 8 and not a multiple of 8" << std::endl;
    png_destroy_read_struct(&mpPNG, &mpInfo, &mpEndInfo);
    fclose(fp);
    return NOT_OK;
  }
  mNumOfPasses = png_set_interlace_handling(mpPNG);
  
  if (mIsExpanded){
    png_read_update_info(mpPNG, mpInfo);  // has to be done: Optional call to update the users info structure
    png_get_IHDR(mpPNG, mpInfo, &mWidth, &mHeight, &mBitDepth, &mColorType, &mInterlace, &mCompression, &mFilter);
    if (png_get_PLTE(mpPNG, mpInfo, &mpPalette, &mNumPalette) != PNG_INFO_PLTE){
      std::cerr << "[LibPNGImage::ReadPNG] - init_io failed" << std::endl;
      png_destroy_read_struct(&mpPNG, &mpInfo, &mpEndInfo);
      fclose(fp);
      return NOT_OK;
    }
  }

  mBytesPerRow = png_get_rowbytes(mpPNG, mpInfo);

  /*read the PNG into the row_pointers-structure*/
  if (setjmp(png_jmpbuf(mpPNG))){
    std::cerr << "[LibPNGImage::ReadPNG] - error during read_image" << std::endl;
    png_destroy_read_struct(&mpPNG, &mpInfo, &mpEndInfo);
    fclose(fp);
    return NOT_OK;
  }
    
  mRowPointers = (png_bytep*)malloc(sizeof(png_bytep)* mHeight);
  for (size_t y = 0; y < mHeight; y++){
    mRowPointers[y] = (png_byte*)malloc(mBytesPerRow);
  }
  if (!mRowPointers){
    std::cerr << "[LibPNGImage::ReadPNG] - error during allocating memory for the image data" << std::endl;
    png_destroy_read_struct(&mpPNG, &mpInfo, &mpEndInfo);
    fclose(fp);
    return NOT_OK;
  }
    
  png_read_image(mpPNG, mRowPointers);  //reading the data into the row_pointers

  png_destroy_read_struct(&mpPNG, &mpInfo, &mpEndInfo);
  fclose(fp);
  return OK;
}

//writes the image from the class-buffer to a .png-file
int LibPNGImage::WritePNG(char* const fileName)
{
  if (mRowPointers == 0){
    std::cerr << "[LibPNGImage::WritePNG] - mRowPointers are a zero-pointer - please use LibPNGImage::'ReadPNG' before "<< std::endl;
    return NOT_OK;
  }

  if (fileName == 0){
    std::cerr << "[LibPNGImage::WritePNG] - FileName: " << fileName <<
      "is a zero-pointer" << std::endl;
    return NOT_OK;
  }

  /* open file */
  FILE *fp = fopen(fileName, "wb");
  if (!fp){
    std::cerr << "[LibPNGImage::WritePNG] - File: " << fileName <<
      "could not be opened for writing" << std::endl;
    return NOT_OK;
  }

  mpPNG = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!mpPNG){
    std::cerr << "[LibPNGImage::WritePNG] - png_create_write_struct failed " << std::endl;
    return NOT_OK;
  }

  mpInfo = png_create_info_struct(mpPNG);
  if (!mpInfo){
    std::cerr << "[LibPNGImage::WritePNG] - png_create_info_struct failed " << std::endl;
    png_destroy_write_struct(&mpPNG, NULL);
    return NOT_OK;
  }

  if (setjmp(png_jmpbuf(mpPNG))){
    std::cerr << "[LibPNGImage::WritePNG] - error during png_init_io " << std::endl;
    png_destroy_write_struct(&mpPNG, &mpInfo);
    return NOT_OK;
  }
  png_init_io(mpPNG, fp);

  /* write header */
  if (setjmp(png_jmpbuf(mpPNG))){
    std::cerr << "[LibPNGImage::WritePNG] - error during writing header " << std::endl;
    png_destroy_write_struct(&mpPNG, &mpInfo);
    return NOT_OK;
  }

  if (mIsExpanded){
    png_set_PLTE(mpPNG, mpInfo, mpPalette, mNumPalette);
  }

  png_set_IHDR(mpPNG, mpInfo, mWidth, mHeight, mBitDepth, mColorType, mInterlace, mCompression, mFilter);

  png_write_info(mpPNG, mpInfo);
  /* //THIS CODE DOES NOT FUNCTION !!!!- it's just an awful try
  ////////////////////////
  //SHRINK BACK TO 1-BiT
  png_color_8 sig_bit;
  png_color_8p psig_bit = &sig_bit;
  png_get_sBIT(mpPNG, mpInfo, &psig_bit);
  mColorType = 3;
  // Set the true bit depth of the image data 
  if (mColorType == 2)
  {
    sig_bit.red = 0;
    sig_bit.green = 0;
    sig_bit.blue = 0;
  }
  else
  {
    sig_bit.gray = 1;
  }
  if (mColorType & PNG_COLOR_MASK_ALPHA)
  {
    sig_bit.alpha = 0;
  }
  png_set_sBIT(mpPNG, mpInfo, &sig_bit);

  png_read_update_info(mpPNG, mpInfo);  // has to be done: Optional call to update the users info structure

  png_set_PLTE(mpPNG, mpInfo, mpPalette, mNumPalette);
  png_set_IHDR(mpPNG, mpInfo, mWidth, mHeight, mBitDepth, mColorType, mInterlace,
    mCompression, mFilter);

  ///////////////////////*/

  /* start write bytes */
  if (setjmp(png_jmpbuf(mpPNG))){
    std::cerr << "[LibPNGImage::WritePNG] - error during writing bytes " << std::endl;
    png_destroy_write_struct(&mpPNG, &mpInfo);
    return NOT_OK;
  }
  png_write_image(mpPNG, mRowPointers);
  /* end write bytes*/

  if (setjmp(png_jmpbuf(mpPNG))){
    std::cerr << "[LibPNGImage::WritePNG] - error end of write " << std::endl;
    png_destroy_write_struct(&mpPNG, &mpInfo);
    return NOT_OK;
  }
  png_write_end(mpPNG, NULL);
  png_destroy_write_struct(&mpPNG, &mpInfo);

  return 0;
}

//mirrors the image in the class-buffer
int LibPNGImage::MirrorPNG()
{
  if (mRowPointers == 0){
    std::cerr << "[LibPNGImage::MirrorPNG] - mRowPointers is zero - please use 'ReadPNG' before" << std::endl;
    return NOT_OK;
  }

  for (size_t y = 0; y < mHeight; y++){
    for (size_t x = 0; x < mBytesPerRow/2; x++){
      png_byte tmp = mRowPointers[y][x];
      mRowPointers[y][x] = mRowPointers[y][mBytesPerRow - 1 - x];
      mRowPointers[y][mBytesPerRow - 1 - x] = tmp;
    }
  }
  return OK;
}

//crops the image in the class-buffer by seeking for the Maxima and Minima of the color black around it
int LibPNGImage::CropPNG()
{
  size_t MinX = mBytesPerRow;
  size_t MinY = mHeight;
  size_t MaxX = 0;
  size_t MaxY = 0;
  int color0;     //simulates the access to a RGB-Pixel by adjusting offset
  int color1;
  int color2;
  for (size_t y = 0; y < mHeight; y++){
    for (size_t x = 0; x < mBytesPerRow; x++){
      color0 = x + 0;
      color1 = x + 1;
      color2 = x + 2;
      if ((mRowPointers[y][color0] == 0x00) && (mRowPointers[y][color1] == 0x00)
        && (mRowPointers[y][color2]) == 0x00 && (x < MinX)){
        MinX = x;
      }
      if ((mRowPointers[y][color0] == 0x00) && (mRowPointers[y][color1] == 0x00)
        && (mRowPointers[y][color2]) == 0x00 && (y < MinY)){
        MinY = y;
      }
      if ((mRowPointers[y][color0] == 0x00) && (mRowPointers[y][color1] == 0x00)
        && (mRowPointers[y][color2]) == 0x00 && (x > MaxX)){
        MaxX = x;
      }
      if ((mRowPointers[y][color0] == 0x00) && (mRowPointers[y][color1] == 0x00)
        && (mRowPointers[y][color2]) == 0x00 && (y > MaxY)){
        MaxY = y;
      }
      x += 2;   //only plus two because one is standard-increment--> 1+2 = 3
    }
  }

  int colororg0;
  int colororg1;
  int colororg2;

  size_t x, y, orgx, orgy;
  for (orgy = 0, y = MinY; y <= MaxY; orgy++, y++){
    for (orgx = 0, x = MinX; x <= MaxX; orgx++, x++){
      color0 = x + 0;
      color1 = x + 1;
      color2 = x + 2;
      colororg0 = orgx + 0;
      colororg1 = orgx + 1;
      colororg2 = orgx + 2;

      mRowPointers[orgy][colororg0] = mRowPointers[y][color0];
      mRowPointers[orgy][colororg1] = mRowPointers[y][color1];
      mRowPointers[orgy][colororg2] = mRowPointers[y][color2];

      x += 2;   //only plus two because one is standard-increment --> 1+2 = 3
      orgx += 2;
    }
  }
  moldHeight = mHeight;
  mIsCropped = 0xFF;
  mWidth = MaxX/3 - MinX/3 + 1;
  mHeight = MaxY - MinY + 1;

  return OK;
}

