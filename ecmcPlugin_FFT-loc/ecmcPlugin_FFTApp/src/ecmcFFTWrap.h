/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcFFTWrap.h
*
*  Created on: Mar 22, 2020
*      Author: anderssandstrom
*
\*************************************************************************/
#ifndef ECMC_FFT_WRAP_H_
#define ECMC_FFT_WRAP_H_
#include "ecmcFFTDefs.h"

# ifdef __cplusplus
extern "C" {
# endif  // ifdef __cplusplus

int  createFFT(char *source);
int  linkDataToFFTs();
void deleteAllFFTs();
int  enableFFT(int fftIndex, int enable);
int  clearFFT(int fftIndex);
int  triggFFT(int fftIndex);
int  setModeFFT(int fftIndex, FFT_MODE mode);
# ifdef __cplusplus
}
# endif  // ifdef __cplusplus

#endif  /* ECMC_FFT_WRAP_H_ */
