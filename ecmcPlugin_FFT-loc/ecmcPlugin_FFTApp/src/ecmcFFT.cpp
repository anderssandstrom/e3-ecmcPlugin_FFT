/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcFFT.cpp
*
*  Created on: Mar 22, 2020
*      Author: anderssandstrom
*      Credits to  https://github.com/sgreg/dynamic-loading 
*
\*************************************************************************/

// Needed to get headers in ecmc right...
#define ECMC_IS_PLUGIN

#define ECMC_PLUGIN_ERROR_FFT_BASE 100
#define ECMC_PLUGIN_ERROR_FFT_ALLOC_FAIL 101

#include "ecmcFFT.h"

static int printMissingObjError = 1;

// data callback 
void f_dataUpdatedCallback(uint8_t* data, size_t size, ecmcEcDataType dt, void* obj) {
  if(!obj) {
    if(printMissingObjError){
      printf("%s/%s:%d: Error: Callback object NULL.. Data will not be added to buffer.\n",
              __FILE__, __FUNCTION__, __LINE__);
      printMissingObjError = 0;
      return;
    }
  }
  ecmcFFT * fftObj = (ecmcFFT*)obj;

  // Fill object buffer with data
  fftObj->dataUpdatedCallback(data,size,dt);
}

ecmcFFT::ecmcFFT(ecmcDataItem* dataItem, ecmcAsynPortDriver* asynPort, size_t nfft) {
  bufferSizeBytes_ = 0;
  bytesInBuffer_   = 0;
  dataItem_        = dataItem;
  asynPort_        = asynPort;
  nfft_            = nfft;
  dataBuffer_      = NULL;

  // Allocate buffer
  bufferSizeBytes_ = nfft_ * dataItem_->getEcmcDataElementSize();
  dataBuffer_ = new uint8_t[bufferSizeBytes_];
  
  if(!dataBuffer_) {
    printf("%s/%s:%d: Error: Failed allocate dataBuffer of size %d (0x%x).\n",
              __FILE__, __FUNCTION__, __LINE__, bufferSizeBytes_, ECMC_PLUGIN_ERROR_FFT_ALLOC_FAIL);   
  }
  clearBuffer();
}

ecmcFFT::~ecmcFFT() {
  if(dataBuffer_) {
    delete[] dataBuffer_;
  }
}

int ecmcFFT::ConnectToDataSource() {
  //Register data callback
  return dataItem_->regDataUpdatedCallback(f_dataUpdatedCallback, this);  
}

void ecmcFFT::dataUpdatedCallback(uint8_t*       data, 
                                  size_t         size,
                                  ecmcEcDataType dt) {
  // No buffer or full
  if(!dataBuffer_) {
    return;
  }

  if(bytesInBuffer_ == bufferSizeBytes_) {
    printf("Buffer full (%d bytes appended).\n",bytesInBuffer_);
  }

  // Start to fill buffer
  size_t bytesToCp = size;
  if(bytesToCp > bufferSizeBytes_ - bytesInBuffer_) {
    bytesToCp = bufferSizeBytes_ - bytesInBuffer_;
  }
  memcpy(&dataBuffer_[bytesInBuffer_],data,bytesToCp);
  bytesInBuffer_+=bytesToCp;
}
  
void ecmcFFT::clearBuffer() {
  memset(dataBuffer_,0,bufferSizeBytes_);
  bytesInBuffer_ = 0;
}
