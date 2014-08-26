#include <iostream>
#include <cstring>  //for Linux-Users

#include "LibPNGImage.h"

void ShowWrongParams(){
  std::cout << "wrong amount of input parameters" << std::endl;
  std::cout << "please use one of these input-methods:" << std::endl;
  std::cout << "1st Method: <name> <crop> <mirror> " << std::endl;
  std::cout << "2nd Method: <name> <crop&mirror> " << std::endl;
  std::cout << "Input: 0 = no action, anything other = action" << std::endl << std::endl;
}

void ShowWrongName(){
  std::cout << "please type in only .png files including the postfix" << std::endl;
  std::cout << "----------------------------------------------------" << std::endl;
}

int main(int argc, char** argv){
  if (argc < 3 || argc > 4 || argv[1] == ""){
    ShowWrongParams();
    ShowWrongName();
    getchar();
    return 1;
  }
  else{
    LibPNGImage ImgObj;
    if (ImgObj.ReadPNG(argv[1]) == OK){
      if (argc == 3){  //2nd Method: <name> <crop&mirror>
        if (strcmp(argv[2], "0") != 0){
          std::cout << "Cropping and Mirroring image: " << argv[1] << std::endl;
          if(ImgObj.CropPNG()){
            getchar();
            return 1;
          }
          if(ImgObj.MirrorPNG()){
            getchar();
            return 1;
          }
        }
        if(ImgObj.WritePNG(argv[1]) == NOT_OK){
          getchar();
          return 1;
        }
      }
      else{                             //  1st Method: <name> <crop> <mirror>
        if (strcmp(argv[3], "0") != 0){  //  mirror
          std::cout << "Mirroring image: " << argv[1] << std::endl;
          if(ImgObj.MirrorPNG()){
            getchar();
            return 1;
          }
        }
        if (strcmp(argv[2], "0") != 0){  //  crop
          std::cout << "Cropping image: " << argv[1] << std::endl;
          if(ImgObj.CropPNG()){
            getchar();
            return 1;
          }
        }
        if(ImgObj.WritePNG(argv[1]) == NOT_OK){
          getchar();
          return 1;
        }
      }
    }
    else{
      std::cout << "----------------------------------------------------" << std::endl;
      getchar();
      return 1;
    }
  }
  return 0;
}

