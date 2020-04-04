/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcFFT.h
*
*  Created on: Mar 22, 2020
*      Author: anderssandstrom
*
\*************************************************************************/
#ifndef ECMC_FFT_H_
#define ECMC_FFT_H_

#define ECMC_PLUGIN_DEFAULT_NFFT 8192

#include "ecmcDataItem.h"
#include "ecmcAsynPortDriver.h"
#include "inttypes.h"
#include <stdexcept>

class ecmcFFT {
 public:
  ecmcFFT(int   fftIndex,       // index of this object  
          char* configStr);

  ~ecmcFFT();  

  int getErrorId();

  // Add data to buffer
  void dataUpdatedCallback(uint8_t* data, 
                           size_t size,
                           ecmcEcDataType dt);
 private:
  int  parseConfigStr(char *configStr);
  int  connectToDataSource();
  void clearBuffer();  
  ecmcDataItem       *dataItem_;
  ecmcAsynPortDriver *asynPort_;
  uint8_t*            dataBuffer_;
  size_t              nfft_;
  size_t              bufferSizeBytes_;
  size_t              bytesInBuffer_;
  int                 errorId_;
  int                 callbackHandle_;
  int                 dbgMode_;  //Allow dbg printouts
  char*               dataSourceStr_;
  // A unique object id for this fft (if plugin is more than once)
  int                 objectId_;
  static int          dataTypeSupported(ecmcEcDataType dt);

  //Some utility functions
  static uint8_t      getUint8(uint8_t* data);
  static int8_t       getInt8(uint8_t* data);
  static uint16_t     getUint16(uint8_t* data);
  static int16_t      getInt16(uint8_t* data);
  static uint32_t     getUint32(uint8_t* data);
  static int32_t      getInt32(uint8_t* data);
  static uint64_t     getUint64(uint8_t* data);
  static int64_t      getInt64(uint8_t* data);
  static float        getFloat32(uint8_t* data);
  static double       getFloat64(uint8_t* data);

  static size_t       getEcDataTypeByteSize(ecmcEcDataType dt);

  static void      printData(uint8_t*       data, 
                             size_t         size,
                             ecmcEcDataType dt);

};

#endif  /* ECMC_FFT_H_ */
