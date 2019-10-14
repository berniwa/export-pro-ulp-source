#include <iostream>
#include <stdlib.h>

#include "LibPNGImage.h"

// please note that this is a C++ and C-mix due to the reason that the LibPNG-Library
// is made for C on Linux

#define PNG_HEADER_SIZE       4       //8 is the maximum size that can be checked by LibPNG
#define PNG_BYTES_PER_PIXEL   4       //3 for RGB, 4 for RGBA


LibPNGImage::LibPNGImage()
{
  mRowPointers = 0; //TODO: CTOR does not be executed
  mIsExpanded = 0;
  mIsCropped = 0;
  mPngData = 0;
}

LibPNGImage::~LibPNGImage(){
  if(mPngData != NULL){free(mPngData);}
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
  
  //Add alpha channel
  png_set_tRNS_to_alpha(mpPNG);
  png_set_filler(mpPNG, 0xFF, PNG_FILLER_AFTER);
  png_read_update_info(mpPNG, mpInfo);

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


  //We will allocate quite a lot of memory
  //depending on the image, it may well be >1Gb
  //so memory allocation needs to be done carefully 
  //to speed up the process

  //Allocate an array of pointers for the vertical lines
  mRowPointers = (png_bytep*)malloc(sizeof(png_bytep*) * mHeight);

  //Allocate the entire memory block for the image in one go 
  size_t memsize = (size_t)mWidth*mHeight*PNG_BYTES_PER_PIXEL;
  mPngData = (char *)malloc(memsize);
  if(mPngData == NULL) {
    std::cerr << "[LibPNGImage::ReadPNG] - unable to allocate image memory: "<<(int)memsize << std::endl;
    return NOT_OK;
  }

  //Set the buffer pointers accordingly
  for (size_t y = 0; y < mHeight; y++){
    mRowPointers[y] = (png_byte*)(mPngData + (y * mBytesPerRow) );
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

  png_set_IHDR(mpPNG, mpInfo, mWidth, mHeight, mBitDepth, PNG_COLOR_TYPE_RGBA, mInterlace, mCompression, mFilter);

  png_write_info(mpPNG, mpInfo);

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
    for (size_t x = 0; x < mBytesPerRow/2; x+=PNG_BYTES_PER_PIXEL){
      //We need to modify 3 bytes at a time, in order to preserve the colors
      png_byte tmp[PNG_BYTES_PER_PIXEL];
      memcpy(tmp, &mRowPointers[y][x], PNG_BYTES_PER_PIXEL);
      memcpy(&mRowPointers[y][x], &mRowPointers[y][mBytesPerRow - PNG_BYTES_PER_PIXEL - x], PNG_BYTES_PER_PIXEL);
      memcpy(&mRowPointers[y][mBytesPerRow - PNG_BYTES_PER_PIXEL - x], tmp, PNG_BYTES_PER_PIXEL);
    }
  }
  return OK;
}

int LibPNGImage::CheckPixel(char *data){
    char *red   = (char *)&data[0];
    char *green = (char *)&data[1];
    char *blue  = (char *)&data[2];
    char *alpha = (char *)&data[3];

    if(*red == 0x00 && *green == 0x00 && *blue == 0x00){
      return 0;
    } else {
      *red =   0xFF;
      *green = 0xFF;
      *blue  = 0xFF;
      *alpha = 0x00;
      return 1;      
    }
}

//crops the image in the class-buffer by seeking for the Maxima and Minima of the color black around it
int LibPNGImage::CropPNG()
{

  size_t MinX = mBytesPerRow;
  size_t MinY = mHeight;
  size_t MaxX = 0;
  size_t MaxY = 0;

  for (size_t y = 0; y < mHeight; y++){
    //Run outline detection from top-left
    for (size_t x = 0; x < mBytesPerRow; x+=PNG_BYTES_PER_PIXEL){
      char *red   = (char *)&mRowPointers[y][x + 0];
      char *green = (char *)&mRowPointers[y][x + 1];
      char *blue  = (char *)&mRowPointers[y][x + 2];

      //If we detect a black outline
      if(*red == 0x00 && *green == 0x00 && *blue == 0x00){        
        if(x < MinX){
          MinX = x;
        }
        if(y < MinY){
          MinY = y;
        }
        if(x > MaxX){
          MaxX = x;
        }
        if(y > MaxY){
          MaxY = y;
        }
      }
    }
  }

  //Generate cutouts from left to right
  for (size_t y = MinY; y < MaxY; y++){
     for (size_t x = MinX; x < MaxX; x+=PNG_BYTES_PER_PIXEL){
      if(!this->CheckPixel((char *)&mRowPointers[y][x])){
        break;
      }
    }
  }
  //Generate cutouts from right to left
  for (size_t y = MinY; y < MaxY; y++){
     for (size_t x = MaxX; x >= MinX; x-=PNG_BYTES_PER_PIXEL){
      if(!this->CheckPixel((char *)&mRowPointers[y][x])){
        break;
      }
    }
  }
  //Generate cutouts from top to bottom
  for (size_t x = MaxX; x >= MinX; x-=PNG_BYTES_PER_PIXEL){
    for (size_t y = MinY; y < MaxY; y++){
      if(!this->CheckPixel((char *)&mRowPointers[y][x])){
        break;
      }
    }
  }
  //Generate cutouts from bottom to top
  for (size_t x = MaxX; x >= MinX; x-=PNG_BYTES_PER_PIXEL){
    for (size_t y = MaxY; y > MinY; y--){
      if(!this->CheckPixel((char *)&mRowPointers[y][x])){
        break;
      }
    }
  }

  //Transparent vias/drills
  for (size_t y = MinY; y < MaxY; y++){
    for (size_t x = MinX; x < MaxX; x+=PNG_BYTES_PER_PIXEL){
      char *red   = (char *)&mRowPointers[y][x + 0];
      char *green = (char *)&mRowPointers[y][x + 1];
      char *blue  = (char *)&mRowPointers[y][x + 2];
      char *alpha = (char *)&mRowPointers[y][x + 3];

      //If we detect a black outline
      if(*red == 0x00 && *green == 0x00 && *blue == 0x00){  
        *red =   0xFF;
        *green = 0xFF;
        *blue  = 0xFF;   
        *alpha = 0x00;      
      }      
    }
  }
  
  //Move to origin
  size_t x, y, orgx, orgy;
  for (orgy = 0, y = MinY; y <= MaxY; orgy++, y++){
    for (orgx = 0, x = MinX; x <= MaxX; orgx+=PNG_BYTES_PER_PIXEL, x+=PNG_BYTES_PER_PIXEL){
      memcpy(&mRowPointers[orgy][orgx], &mRowPointers[y][x], PNG_BYTES_PER_PIXEL);
    }
  }
  moldHeight = mHeight;
  mIsCropped = 0xFF;
  mWidth = MaxX/PNG_BYTES_PER_PIXEL - MinX/PNG_BYTES_PER_PIXEL + 1;
  mHeight = MaxY - MinY + 1;

  return OK;
}

