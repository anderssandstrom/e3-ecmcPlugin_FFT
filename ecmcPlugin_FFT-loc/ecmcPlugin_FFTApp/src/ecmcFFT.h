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

#include <stdexcept>
#include "ecmcDataItem.h"
#include "ecmcAsynPortDriver.h"
#include "inttypes.h"
#include "kissfft/kissfft.hh"

class ecmcFFT {
 public:

  /** ecmc FFT class
   * This object can throw: 
   *    - bad_alloc
   *    - invalid_argument
   *    - runtime_error
  */
  ecmcFFT(int   fftIndex,    // index of this object  
          char* configStr);
  ~ecmcFFT();  

  // Add data to buffer
  void dataUpdatedCallback(uint8_t* data, 
                           size_t size,
                           ecmcEcDataType dt);
 private:
  void  parseConfigStr(char *configStr);
  void  connectToDataSource();
  void  clearBuffers();
  void  calcFFT();
  void  addDataToBuffer(double data);

  ecmcDataItem         *dataItem_;
  ecmcAsynPortDriver   *asynPort_;
  double*               dataBuffer_;
  size_t                dataBufferSize_;
  std::complex<double>* fftBuffer_; // Result
  size_t                fftBufferSize_;
  size_t                nfft_;
  size_t                elementsInBuffer_;
  // ecmc callback handle for use when deregister at unload
  int                   callbackHandle_;
  int                   dbgMode_;  //Allow dbg printouts
  int                   fftCalcDone_;
  char*                 dataSourceStr_;
  // A unique object id for this fft (if plugin is more than once)
  int                   objectId_;
  static int            dataTypeSupported(ecmcEcDataType dt);
  kissfft<double>*      fftDouble_;

  // Some utility functions
  static uint8_t        getUint8(uint8_t* data);
  static int8_t         getInt8(uint8_t* data);
  static uint16_t       getUint16(uint8_t* data);
  static int16_t        getInt16(uint8_t* data);
  static uint32_t       getUint32(uint8_t* data);
  static int32_t        getInt32(uint8_t* data);
  static uint64_t       getUint64(uint8_t* data);
  static int64_t        getInt64(uint8_t* data);
  static float          getFloat32(uint8_t* data);
  static double         getFloat64(uint8_t* data);
  static size_t         getEcDataTypeByteSize(ecmcEcDataType dt);
  static void           printData(uint8_t*       data, 
                                  size_t         size,
                                  ecmcEcDataType dt,
                                  int objId);
  static void           printResult(std::complex<double>* fftBuff,
                                    size_t elements,
                                    int objId);
};

#endif  /* ECMC_FFT_H_ */
