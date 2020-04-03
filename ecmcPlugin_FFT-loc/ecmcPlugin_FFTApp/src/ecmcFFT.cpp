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
#define ECMC_PLUGIN_ERROR_FFT_DATATYPE_NOT_SUPPORTED 102

#include "ecmcFFT.h"


// New data callback from ecmc
static int printMissingObjError = 1;

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


// ecmc FFT class
ecmcFFT::ecmcFFT(ecmcDataItem* dataItem, ecmcAsynPortDriver* asynPort, size_t nfft) {
  bufferSizeBytes_ = 0;
  bytesInBuffer_   = 0;
  dataItem_        = dataItem;
  asynPort_        = asynPort;
  nfft_            = nfft;
  dataBuffer_      = NULL;
  errorId_         = 0;

  // Allocate buffer
  bufferSizeBytes_ = nfft_ * dataItem_->getEcmcDataElementSize();
  dataBuffer_ = new uint8_t[bufferSizeBytes_];
  
  if(!dataBuffer_) {
    printf("%s/%s:%d: Error: Failed allocate dataBuffer of size %d (0x%x).\n",
              __FILE__, __FUNCTION__, __LINE__, bufferSizeBytes_, ECMC_PLUGIN_ERROR_FFT_ALLOC_FAIL);   
    errorId_ = ECMC_PLUGIN_ERROR_FFT_ALLOC_FAIL;
    return;
  }

  clearBuffer();
  
  if( !dataTypeSupported(dataItem_->getEcmcDataType()) ) {
    errorId_ = ECMC_PLUGIN_ERROR_FFT_DATATYPE_NOT_SUPPORTED;
    return;
  }

  errorId_ = connectToDataSource();
  if(errorId_) {
    return;
  }
}

ecmcFFT::~ecmcFFT() {
  if(dataBuffer_) {
    delete[] dataBuffer_;
  }
}

int ecmcFFT::connectToDataSource() {
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

  //printData(data,size,dt);

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

int ecmcFFT::getErrorId() {
  return errorId_;
}

void ecmcFFT::printData(uint8_t*       data, 
                        size_t         size,
                        ecmcEcDataType dt) {
  
  size_t dataElementSize = getEcDataTypeByteSize(dt);

  uint8_t *pData = data;

  for(unsigned int i = 0; i < size / dataElementSize; ++i) {    
    switch(dt) {
      case ECMC_EC_U8:        
        printf("%hhu\n",getUint8(pData));
        break;
      case ECMC_EC_S8:
        printf("%hhd\n",getInt8(pData));
        break;
      case ECMC_EC_U16:
        printf("%hu\n",getUint16(pData));
        break;
      case ECMC_EC_S16:
        printf("%hd\n",getInt16(pData));
        break;
      case ECMC_EC_U32:
        printf("%u\n",getUint32(pData));
        break;
      case ECMC_EC_S32:
        printf("%d\n",getInt32(pData));
        break;
      case ECMC_EC_U64:
        printf("%" PRIu64 "\n",getInt64(pData));
        break;
      case ECMC_EC_S64:
        printf("%" PRId64 "\n",getInt64(pData));
        break;
      case ECMC_EC_F32:
        printf("%f\n",getFloat32(pData));
        break;
      case ECMC_EC_F64:
        printf("%lf\n",getFloat64(pData));
        break;
      default:
        break;
    }
  }
  // go to next element
  pData += dataElementSize;
}

int ecmcFFT::dataTypeSupported(ecmcEcDataType dt) {

  switch(dt) {
    case ECMC_EC_NONE:      
      return 0;
      break;
    case ECMC_EC_B1:
      return 0;
      break;
    case ECMC_EC_B2:
      return 0;
      break;
    case ECMC_EC_B3:
      return 0;
      break;
    case ECMC_EC_B4:
      return 0;
      break;
    default:
      return 1;
      break;
  }
  return 1;
}

uint8_t   ecmcFFT::getUint8(uint8_t* data) {
  return *data;
}

int8_t    ecmcFFT::getInt8(uint8_t* data) {
  int8_t* p=(int8_t*)data;
  return *p;
}

uint16_t  ecmcFFT::getUint16(uint8_t* data) {
  uint16_t* p=(uint16_t*)data;
  return *p;
}

int16_t   ecmcFFT::getInt16(uint8_t* data) {
  int16_t* p=(int16_t*)data;
  return *p;
}

uint32_t  ecmcFFT::getUint32(uint8_t* data) {
  uint32_t* p=(uint32_t*)data;
  return *p;
}

int32_t   ecmcFFT::getInt32(uint8_t* data) {
  int32_t* p=(int32_t*)data;
  return *p;
}

uint64_t  ecmcFFT::getUint64(uint8_t* data) {
  uint64_t* p=(uint64_t*)data;
  return *p;
}

int64_t   ecmcFFT::getInt64(uint8_t* data) {
  int64_t* p=(int64_t*)data;
  return *p;
}

float     ecmcFFT::getFloat32(uint8_t* data) {
  float* p=(float*)data;
  return *p;
}

double    ecmcFFT::getFloat64(uint8_t* data) {
  double* p=(double*)data;
  return *p;
}

size_t ecmcFFT::getEcDataTypeByteSize(ecmcEcDataType dt){
  switch(dt) {
  case ECMC_EC_NONE:
    return 0;
    break;

  case ECMC_EC_B1:
    return 1;
    break;

  case ECMC_EC_B2:
    return 1;
    break;

  case ECMC_EC_B3:
    return 1;
    break;

  case ECMC_EC_B4:
    return 1;
    break;

  case ECMC_EC_U8:
    return 1;
    break;

  case ECMC_EC_S8:
    return 1;
    break;

  case ECMC_EC_U16:
    return 2;
    break;

  case ECMC_EC_S16:
    return 2;
    break;

  case ECMC_EC_U32:
    return 4;
    break;

  case ECMC_EC_S32:
    return 4;
    break;

  case ECMC_EC_U64:
    return 8;
    break;

  case ECMC_EC_S64:
    return 8;
    break;

  case ECMC_EC_F32:
    return 4;
    break;

  case ECMC_EC_F64:
    return 8;
    break;

  default:
    return 0;
    break;
  }

  return 0;
}
