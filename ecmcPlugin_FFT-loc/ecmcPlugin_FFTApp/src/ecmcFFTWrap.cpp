/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcFFTWrap.cpp
*
*  Created on: Mar 22, 2020
*      Author: anderssandstrom
*      Credits to  https://github.com/sgreg/dynamic-loading 
*
\*************************************************************************/

// Needed to get headers in ecmc right...
#define ECMC_IS_PLUGIN

#include <vector>
#include <stdexcept>
#include "ecmcFFTWrap.h"
#include "ecmcFFT.h"
#include "ecmcFFTDefs.h"

static std::vector<ecmcFFT*>  ffts;
static int                    fftObjCounter = 0;

int createFFT(char* configStr) {

  // create new ecmcFFT object
  ecmcFFT* fft = NULL;
  try {
    fft = new ecmcFFT(fftObjCounter, configStr);
  }
  catch(std::exception& e) {
    if(fft) {
      delete fft;
    }
    printf("Exception: %s.",e.what());
    return ECMC_PLUGIN_FFT_ERROR_CODE;
  }
  
  ffts.push_back(fft);
  fftObjCounter++;

  return 0;
}

void deleteAllFFTs() {
  for(std::vector<ecmcFFT*>::iterator pfft = ffts.begin(); pfft != ffts.end(); ++pfft) {
    if(*pfft) {
      delete *pfft;
    }
  }
}
