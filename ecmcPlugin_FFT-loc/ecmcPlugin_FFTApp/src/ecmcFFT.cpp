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

#define ECMC_PLUGIN_ASYN_PREFIX   "plugin.fft"
#define ECMC_PLUGIN_ASYN_ENABLE   "enable"
#define ECMC_PLUGIN_ASYN_RAWDATA  "rawdata"
#define ECMC_PLUGIN_ASYN_FFT_AMP  "fftamplitude"
#define ECMC_PLUGIN_ASYN_FFT_MODE "mode"
#define ECMC_PLUGIN_ASYN_FFT_STAT "status"

#include <sstream>
#include "ecmcFFT.h"
#include "ecmcPluginClient.h"
#include "ecmcAsynPortDriver.h"

#define PRINT_IF_DBG_MODE(fmt, ...)        \
   {                                       \
     if(cfgDbgMode_){                         \
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
ecmcFFT::ecmcFFT(int   fftIndex,       // index of this object (if several is created)
                 char* configStr) {
  cfgDataSourceStr_ = NULL;
  dataBuffer_       = NULL;
  dataItem_         = NULL;
  fftDouble_        = NULL;
  asynPort_         = NULL;
  asynEnable_       = NULL;  // Enable
  asynRawData_      = NULL;  // Input data
  asynFFTAmp_       = NULL;  // Result
  asynFFTMode_      = NULL;  // Mode
  asynFFTStat_      = NULL;  // Status
  status_           = NO_STAT;
  elementsInBuffer_ = 0;
  fftCalcDone_      = 0;
  callbackHandle_   = -1;
  objectId_         = fftIndex;
  scale_            = 1.0;
  triggOnce_        = 0;
  // Config defaults
  cfgDbgMode_       = 0;
  cfgNfft_          = ECMC_PLUGIN_DEFAULT_NFFT; // samples in fft (must be n^2)
  cfgDcRemove_      = 0;
  cfgApplyScale_    = 1;   // Scale as default to get correct amplitude in fft
  cfgEnable_        = 0;   // start disabled (enable over asyn)
  cfgMode_          = TRIGG;

  asynPort_         = (ecmcAsynPortDriver*) getEcmcAsynPortDriver();
  if(!asynPort_) {
    throw std::runtime_error("Asyn port NULL");
  }

  parseConfigStr(configStr); // Assigns all configs
  // Check valid nfft
  if(cfgNfft_ <= 0) {
    throw std::out_of_range("NFFT must be > 0 and even N^2.");
  }
  // set scale factor
  scale_ = 1.0 / cfgNfft_; // sqrt((double)cfgNfft_);

  // Allocate buffers
  dataBuffer_      = new double[cfgNfft_];               // Raw input data (real)
  fftBuffer_       = new std::complex<double>[cfgNfft_]; // FFT result (complex)
  fftBufferAmp_    = new double[cfgNfft_];               // FFT result amplitude (real)
  clearBuffers();

  // Allocate KissFFT
  fftDouble_ = new kissfft<double>(cfgNfft_,false);

  initAsyn();
}

ecmcFFT::~ecmcFFT() {
  if(dataBuffer_) {
    delete[] dataBuffer_;
  }
  // De regeister callback when unload
  if(callbackHandle_ >= 0) {
    dataItem_->deregDataUpdatedCallback(callbackHandle_);
  }
  if(cfgDataSourceStr_) {
    free(cfgDataSourceStr_);
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
      
      // ECMC_PLUGIN_DBG_OPTION_CMD (1/0)
      if (!strncmp(pThisOption, ECMC_PLUGIN_DBG_OPTION_CMD, strlen(ECMC_PLUGIN_DBG_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_DBG_OPTION_CMD);
        cfgDbgMode_ = atoi(pThisOption);
      } 
      
      // ECMC_PLUGIN_SOURCE_OPTION_CMD (Source string)
      else if (!strncmp(pThisOption, ECMC_PLUGIN_SOURCE_OPTION_CMD, strlen(ECMC_PLUGIN_SOURCE_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_SOURCE_OPTION_CMD);
        cfgDataSourceStr_=strdup(pThisOption);
      }

      // ECMC_PLUGIN_NFFT_OPTION_CMD (1/0)
      else if (!strncmp(pThisOption, ECMC_PLUGIN_NFFT_OPTION_CMD, strlen(ECMC_PLUGIN_NFFT_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_NFFT_OPTION_CMD);
        cfgNfft_ = atoi(pThisOption);
      }

      // ECMC_PLUGIN_APPLY_SCALE_OPTION_CMD (1/0)
      else if (!strncmp(pThisOption, ECMC_PLUGIN_APPLY_SCALE_OPTION_CMD, strlen(ECMC_PLUGIN_APPLY_SCALE_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_APPLY_SCALE_OPTION_CMD);
        cfgApplyScale_ = atoi(pThisOption);
      }

      // ECMC_PLUGIN_DC_REMOVE_OPTION_CMD (1/0)
      else if (!strncmp(pThisOption, ECMC_PLUGIN_DC_REMOVE_OPTION_CMD, strlen(ECMC_PLUGIN_DC_REMOVE_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_DC_REMOVE_OPTION_CMD);
        cfgDcRemove_ = atoi(pThisOption);
      }

      // ECMC_PLUGIN_ENABLE_OPTION_CMD (1/0)
      else if (!strncmp(pThisOption, ECMC_PLUGIN_ENABLE_OPTION_CMD, strlen(ECMC_PLUGIN_ENABLE_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_ENABLE_OPTION_CMD);
        cfgEnable_ = atoi(pThisOption);
      }

      // ECMC_PLUGIN_MODE_OPTION_CMD CONT/TRIGG
      else if (!strncmp(pThisOption, ECMC_PLUGIN_MODE_OPTION_CMD, strlen(ECMC_PLUGIN_MODE_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_MODE_OPTION_CMD);
        if(!strncmp(pThisOption, ECMC_PLUGIN_MODE_CONT_OPTION,strlen(ECMC_PLUGIN_MODE_CONT_OPTION))){
          cfgMode_ = CONT;
        }
        if(!strncmp(pThisOption, ECMC_PLUGIN_MODE_TRIGG_OPTION,strlen(ECMC_PLUGIN_MODE_TRIGG_OPTION))){
          cfgMode_ = TRIGG;
        }
      }

      pThisOption = pNextOption;
    }    
    free(pOptions);
  }

  // Data source must be defined...
  if(!cfgDataSourceStr_) { 
    throw std::invalid_argument( "Data source not defined.");
  }
}

void ecmcFFT::connectToDataSource() {
  // Get dataItem
  dataItem_        = (ecmcDataItem*) getEcmcDataItem(cfgDataSourceStr_);
  if(!dataItem_) {
    throw std::runtime_error( "Data item NULL." );
  }

  // Register data callback
  callbackHandle_ = dataItem_->regDataUpdatedCallback(f_dataUpdatedCallback, this);
  if (callbackHandle_ < 0) {
    throw std::runtime_error( "Failed to register data source callback.");
  }

  // Check data source
  if( !dataTypeSupported(dataItem_->getEcmcDataType()) ) {
    throw std::invalid_argument( "Data type not supported." );
  }
  status_ = IDLE;
}

void ecmcFFT::dataUpdatedCallback(uint8_t*       data, 
                                  size_t         size,
                                  ecmcEcDataType dt) {
  // No buffer or full or not enabled
  if(!dataBuffer_ || !cfgEnable_) {
    return;
  }
  if (cfgMode_ == TRIGG && !triggOnce_ ) {
    status_ = IDLE;
    return; // Wait for trigger from plc or asyn
  }

  if(cfgDbgMode_) {
    printEcDataArray(data, size, dt, objectId_);

    if(elementsInBuffer_ == cfgNfft_) {
      printf("Buffer full (%zu elements appended).\n",elementsInBuffer_);
    }
  }
  
  if(elementsInBuffer_ >= cfgNfft_) {
    //Buffer full
    if(!fftCalcDone_){
      // Perform calcs
      status_ = CALC;

      // **** Breakout to sperate low prio work thread below
      calcFFT();      // FFT cacluation
      scaleFFT();     // Scale FFT
      calcFFTAmp();   // Calculate amplitude from complex 
      // **** Breakout to thread above

      triggOnce_ = 0; // Wait for nex trigger if in trigg mode
  
      // Update asyn with both input and result
      asynRawData_->refreshParamRT(1); // Forced update (do not consider record rate)
      asynFFTAmp_->refreshParamRT(1);  // Forced update (do not consider record rate)

      if(cfgDbgMode_){
        printComplexArray(fftBuffer_,
                    cfgNfft_,
                    objectId_);
        printEcDataArray((uint8_t*)dataBuffer_,
                          cfgNfft_*sizeof(double),
                          ECMC_EC_F64,
                          objectId_);
      }

      // If mode continious then start over
      if(cfgMode_ == CONT) { 
        clearBuffers();
      }
    }
    return;
  }

  status_ = ACQ;

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
        addDataToBuffer((double)getUint64(pData));
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
  if(dataBuffer_ && (elementsInBuffer_ < cfgNfft_) ) {
    dataBuffer_[elementsInBuffer_] = data;
  }
  elementsInBuffer_ ++;
}

void ecmcFFT::clearBuffers() {
  memset(dataBuffer_,   0, cfgNfft_ * sizeof(double));
  memset(fftBufferAmp_, 0, cfgNfft_ * sizeof(double));
  for(unsigned int i = 0; i < cfgNfft_; ++i) {
    fftBuffer_[i].real(0);
    fftBuffer_[i].imag(0);
  }
  elementsInBuffer_ = 0;
  fftCalcDone_      = 0;
}

void ecmcFFT::calcFFT() {
  fftDouble_->transform_real(dataBuffer_, fftBuffer_);
  fftCalcDone_ = 1;
}

void ecmcFFT::scaleFFT() {
  if(!cfgApplyScale_) {
    return;
  }

  for(unsigned int i = 0 ; i < cfgNfft_ ; ++i ) {
    fftBuffer_[i] = fftBuffer_[i] * scale_;
  }
}

void ecmcFFT::calcFFTAmp() {  
  for(unsigned int i = 0 ; i < cfgNfft_ ; ++i ) {
    fftBufferAmp_[i] = std::abs(fftBuffer_[i]);
  }
}

void ecmcFFT::printEcDataArray(uint8_t*       data, 
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

void ecmcFFT::printComplexArray(std::complex<double>* fftBuff,
                                size_t elements,
                                int objId) {
  printf("fft id: %d, results: \n",objId);
  for(unsigned int i = 0 ; i < elements ; ++i ) {
    printf("%d: %lf\n", i, std::abs(fftBuff[i]));
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

// register a dummy asyn parameter "plugin.adv.counter"
void ecmcFFT::initAsyn() {
  
  if(!asynPort_) {
    throw std::runtime_error("Asyn port NULL");
  }
  
  // Add enable "plugin.fft%d.enable"
  std::string paramName =ECMC_PLUGIN_ASYN_PREFIX + to_string(objectId_) + 
             "." + ECMC_PLUGIN_ASYN_ENABLE;
  asynEnable_ = asynPort_->addNewAvailParam(paramName.c_str(),  // name
                                            asynParamInt32,       // asyn type 
                                            (uint8_t *)&(cfgEnable_),// pointer to data
                                            sizeof(cfgEnable_),      // size of data
                                            ECMC_EC_S32,          // ecmc data type
                                            0);                   // die if fail

  if(!asynEnable_) {
    throw std::runtime_error("Failed to create asyn parameter \"" + paramName +"\".\n");
  }
  asynEnable_->setAllowWriteToEcmc(true);
  asynEnable_->refreshParam(1); // read once into asyn param lib

  // Add rawdata "plugin.fft%d.rawdata"
  paramName =ECMC_PLUGIN_ASYN_PREFIX + to_string(objectId_) + 
             "." + ECMC_PLUGIN_ASYN_RAWDATA;
  
  asynRawData_ = asynPort_->addNewAvailParam(paramName.c_str(),      // name
                                             asynParamFloat64Array,   // asyn type 
                                             (uint8_t *)dataBuffer_,  // pointer to data
                                             cfgNfft_*sizeof(double), // size of data
                                             ECMC_EC_F64,             // ecmc data type
                                             0);                      // die if fail

  if(!asynRawData_) {
    throw std::runtime_error("Failed to create asyn parameter \"" + paramName +"\".\n");
  }
  asynRawData_->setAllowWriteToEcmc(false);
  asynRawData_->refreshParam(1); // read once into asyn param lib
  
  // Add fft amplitude "plugin.fft%d.fftamplitude"
  paramName = ECMC_PLUGIN_ASYN_PREFIX + to_string(objectId_) + 
             "." + ECMC_PLUGIN_ASYN_FFT_AMP;
  
  asynFFTAmp_ = asynPort_->addNewAvailParam(paramName.c_str(),        // name
                                            asynParamFloat64Array,    // asyn type 
                                            (uint8_t *)fftBufferAmp_, // pointer to data
                                            cfgNfft_*sizeof(double),  // size of data
                                            ECMC_EC_F64,              // ecmc data type
                                            0);                       // die if fail

  if(!asynFFTAmp_) {
    throw std::runtime_error("Failed to create asyn parameter \"" + paramName +"\".\n");
  }

  asynFFTAmp_->setAllowWriteToEcmc(false);
  asynFFTAmp_->refreshParam(1); // read once into asyn param lib

  // Add fft mode "plugin.fft%d.mode"
  paramName = ECMC_PLUGIN_ASYN_PREFIX + to_string(objectId_) + 
             "." + ECMC_PLUGIN_ASYN_FFT_MODE;
  
  asynFFTMode_ = asynPort_->addNewAvailParam(paramName.c_str(),   // name
                                             asynParamInt32,      // asyn type 
                                             (uint8_t *)cfgMode_, // pointer to data
                                             sizeof(cfgMode_),    // size of data
                                             ECMC_EC_S32,         // ecmc data type
                                             0);                  // die if fail

  if(!asynFFTMode_) {
    throw std::runtime_error("Failed to create asyn parameter \"" + paramName +"\".\n");
  }

  asynFFTMode_->setAllowWriteToEcmc(true);
  asynFFTMode_->refreshParam(1); // read once into asyn param lib

// Add fft mode "plugin.fft%d.status"
  paramName = ECMC_PLUGIN_ASYN_PREFIX + to_string(objectId_) + 
             "." + ECMC_PLUGIN_ASYN_FFT_STAT;
  
  asynFFTStat_ = asynPort_->addNewAvailParam(paramName.c_str(),   // name
                                             asynParamInt32,      // asyn type 
                                             (uint8_t *)status_, // pointer to data
                                             sizeof(status_),    // size of data
                                             ECMC_EC_S32,         // ecmc data type
                                             0);                  // die if fail

  if(!asynFFTStat_) {
    throw std::runtime_error("Failed to create asyn parameter \"" + paramName +"\".\n");
  }

  asynFFTStat_->setAllowWriteToEcmc(false);
  asynFFTStat_->refreshParam(1); // read once into asyn param lib
}

// Avoid issues with std:to_string()
std::string ecmcFFT::to_string(int value) {
  std::ostringstream os;
  os << value;
  return os.str();
}

void ecmcFFT::setEnable(int enable) {
  cfgEnable_ = enable;
}
  
void ecmcFFT::triggFFT() {
  clearBuffers();
  triggOnce_ = 1;
}

void ecmcFFT::setModeFFT(FFT_MODE mode) {
  cfgMode_ = mode;
}

FFT_STATUS ecmcFFT::getStatusFFT() {
  return status_;
}
