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

#include "ecmcFFT.h"
#include "ecmcFFTDefs.h"
#include "ecmcPluginClient.h"
#include "ecmcAsynPortDriver.h"

#define PRINT_IF_DBG_MODE(fmt, ...)        \
   {                                       \
     if(dbgMode_){                         \
       printf(fmt, ## __VA_ARGS__);        \
     }                                     \
   }                                       \


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

  // Call the correct fft object with new data
  fftObj->dataUpdatedCallback(data,size,dt);
}


/** ecmc FFT class
 * This object can throw: 
 *    - bad_alloc
 *    - invalid_argument
 *    - runtime_error
*/
ecmcFFT::ecmcFFT(int   fftIndex,       // index of this object
                 char* configStr) {
  dataSourceStr_    = NULL;
  dataBuffer_       = NULL;
  dataItem_         = NULL;
  fftDouble_        = NULL;
  asynPort_         = NULL;
  elementsInBuffer_ = 0;
  dbgMode_          = 0;
  fftCalcDone_      = 0;
  callbackHandle_   = -1;
  objectId_         = fftIndex;
  nfft_             = ECMC_PLUGIN_DEFAULT_NFFT; // samples in fft (must be n^2)
  asynPort_         = (ecmcAsynPortDriver*) getEcmcAsynPortDriver();
  if(!asynPort_) {
    throw std::runtime_error("Asyn port NULL");
  }

  parseConfigStr(configStr); // Assigns Configs
  connectToDataSource();     // Also assigns dataItem_

  // Allocate buffers
  dataBufferSize_  = nfft_ * sizeof(double);
  dataBuffer_      = new double[dataBufferSize_];
  fftBufferSize_   = (nfft_ * sizeof(double)) / 2 + 1;
  fftBuffer_       = new std::complex<double>[fftBufferSize_];
  clearBuffers();

  // Allocate KissFFT
  fftDouble_ = new kissfft<double>(nfft_,false);
}

ecmcFFT::~ecmcFFT() {
  if(dataBuffer_) {
    delete[] dataBuffer_;
  }
  // De regeister callback when unload
  if(callbackHandle_ >= 0) {
    dataItem_->deregDataUpdatedCallback(callbackHandle_);
  }
  if(dataSourceStr_) {
    free(dataSourceStr_);
  }
  if(fftDouble_) {
    delete fftDouble_;
  }
}

void ecmcFFT::parseConfigStr(char *configStr) {

  // check config parameters
  if (configStr && configStr[0]) {    
    char *pOptions = strdup(configStr);
    char *pThisOption = pOptions;
    char *pNextOption = pOptions;
    
    while (pNextOption && pNextOption[0]) {
      pNextOption = strchr(pNextOption, ';');
      if (pNextOption) {
        *pNextOption = '\0'; /* Terminate */
        pNextOption++;       /* Jump to (possible) next */
      }
      
      // ECMC_PLUGIN_DBG_OPTION_CMD
      if (!strncmp(pThisOption, ECMC_PLUGIN_DBG_OPTION_CMD, strlen(ECMC_PLUGIN_DBG_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_DBG_OPTION_CMD);
        dbgMode_ = atoi(pThisOption);
      } 
      
      // ECMC_PLUGIN_SOURCE_OPTION_CMD
      else if (!strncmp(pThisOption, ECMC_PLUGIN_SOURCE_OPTION_CMD, strlen(ECMC_PLUGIN_SOURCE_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_SOURCE_OPTION_CMD);
        // get string to next ';'
        dataSourceStr_=strdup(pThisOption);
      }

      // ECMC_PLUGIN_NFFT_OPTION_CMD
      else if (!strncmp(pThisOption, ECMC_PLUGIN_NFFT_OPTION_CMD, strlen(ECMC_PLUGIN_NFFT_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_NFFT_OPTION_CMD);
        // get string to next ';'
        nfft_ = atoi(pThisOption);
      }
      pThisOption = pNextOption;
    }    
    free(pOptions);
  }
  if(!dataSourceStr_) { 
    throw std::invalid_argument( "Data source not defined.");
  }
}

void ecmcFFT::connectToDataSource() {
  // Get dataItem
  dataItem_        = (ecmcDataItem*) getEcmcDataItem(dataSourceStr_);
  if(!dataItem_) {
    throw std::runtime_error("Data item NULL.");
  }

  // Register data callback
  callbackHandle_ = dataItem_->regDataUpdatedCallback(f_dataUpdatedCallback, this);
  if (callbackHandle_ < 0) {
    callbackHandle_ = -1;
    throw std::runtime_error( "Failed to register data source callback.");
  }

  // Check data source
  if( !dataTypeSupported(dataItem_->getEcmcDataType()) ) {
    throw std::invalid_argument( "Data type not supported.");
  }
}

void ecmcFFT::dataUpdatedCallback(uint8_t*       data, 
                                  size_t         size,
                                  ecmcEcDataType dt) {
  // No buffer or full
  if(!dataBuffer_) {
    return;
  }
  
  if(dbgMode_) {
    printData(data, size, dt, objectId_);

    if(elementsInBuffer_ == nfft_) {
      printf("Buffer full (%zu elements appended).\n",elementsInBuffer_);
    }
  }
  
  if(elementsInBuffer_ >= nfft_) {
    //Buffer full
    if(!fftCalcDone_){
      calcFFT();
      if(dbgMode_){
        printResult(fftBuffer_,
                    fftBufferSize_,
                    objectId_);
      }
      // Buffer new data
      clearBuffers();
    }
    return;
  }

  size_t dataElementSize = getEcDataTypeByteSize(dt);

  uint8_t *pData = data;
  for(unsigned int i = 0; i < size / dataElementSize; ++i) {    
    switch(dt) {
      case ECMC_EC_U8:        
        addDataToBuffer((double)getUint8(pData));
        break;
      case ECMC_EC_S8:
        addDataToBuffer((double)getInt8(pData));
        break;
      case ECMC_EC_U16:
        addDataToBuffer((double)getUint16(pData));
        break;
      case ECMC_EC_S16:
        addDataToBuffer((double)getInt16(pData));
        break;
      case ECMC_EC_U32:
        addDataToBuffer((double)getUint32(pData));
        break;
      case ECMC_EC_S32:
        addDataToBuffer((double)getInt32(pData));
        break;
      case ECMC_EC_U64:
        addDataToBuffer((double)getInt64(pData));
        break;
      case ECMC_EC_S64:
        addDataToBuffer((double)getInt64(pData));
        break;
      case ECMC_EC_F32:
        addDataToBuffer((double)getFloat32(pData));
        break;
      case ECMC_EC_F64:
        addDataToBuffer((double)getFloat64(pData));
        break;
      default:
        break;
    }
    
    pData += dataElementSize;
  }
}

void ecmcFFT::addDataToBuffer(double data) {
  if(dataBuffer_ && elementsInBuffer_ < nfft_) {
    dataBuffer_[elementsInBuffer_] = data;
  }
  elementsInBuffer_ ++;
}

void ecmcFFT::clearBuffers() {
  memset(dataBuffer_, 0, dataBufferSize_);
  elementsInBuffer_ = 0;
  memset(fftBuffer_, 0, fftBufferSize_);
  fftCalcDone_ = 0;
}

void ecmcFFT::calcFFT() {
  fftDouble_->transform_real(dataBuffer_,fftBuffer_);
  fftCalcDone_ = 1;
}

void ecmcFFT::printData(uint8_t*       data, 
                        size_t         size,
                        ecmcEcDataType dt,
                        int objId) {
  printf("fft id: %d, data: ",objId);

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
    
    pData += dataElementSize;
  }
}

void ecmcFFT::printResult(std::complex<double>* fftBuff,
                          size_t elements,
                          int objId) {
  printf("fft id: %d, results: \n",objId);
  for(unsigned int i = 0 ; i < elements ; ++i ) {
    printf("%lf\n", std::abs(fftBuff[i]));
  }
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
