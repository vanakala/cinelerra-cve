#ifndef BINARY_H
#define BINARY_H

#include "sizes.h"
#include <stdio.h>

inline int putfourswap(int32_t number, FILE *file){
  fputc(number & 0xff, file);
  fputc((number & 0xff00) >> 8, file);
  fputc((number & 0xff0000) >> 16, file);
  fputc((number & 0xff000000) >> 24, file);
}

inline int putfour(int32_t number, FILE *file){
  fputc((number & 0xff000000) >> 24, file);
  fputc((number & 0xff0000) >> 16, file);
  fputc((number & 0xff00) >> 8, file);
  fputc(number & 0xff, file);
}

inline int32_t getfour(FILE *in){
  static int32_t number=0;

  number = (int32_t)fgetc(in) << 24;
  number += (int32_t)fgetc(in) << 16;
  number += (int32_t)fgetc(in) << 8;
  number += fgetc(in);
	return number;
}

inline int32_t getfourswap(FILE *in){
  static int32_t number=0;

  number = (int32_t)fgetc(in);
  number += (int32_t)fgetc(in) << 8;
  number += (int32_t)fgetc(in) << 16;
  number += fgetc(in) << 24;
	return number;
}

inline int16_t gettwo(FILE *in){
  static int16_t number=0;

  number = (int32_t)fgetc(in) << 8;
  number += fgetc(in);
	return number;
}

inline int puttwo(int16_t number, FILE *file){
  fputc((number & 0xff00) >> 8, file);
  fputc(number & 0xff, file);
}

#endif
