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
#include "ecmcFFTDefs.h"
#include "inttypes.h"
#include <string>
#include "kissfft/kissfft.hh"

class ecmcFFT {
 public:

  /** ecmc FFT class
   * This object can throw: 
   *    - bad_alloc
   *    - invalid_argument
   *    - runtime_error
   *    - out_of_range
  */
  ecmcFFT(int   fftIndex,    // index of this object  
          char* configStr);
  ~ecmcFFT();  

  // Add data to buffer (called from "external" callback)
  void                  dataUpdatedCallback(uint8_t* data, 
                                            size_t size,
                                            ecmcEcDataType dt);
  // Call just before realtime because then all data sources should be available
  void                  connectToDataSource();
  void                  setEnable(int enable);
  void                  setModeFFT(FFT_MODE mode);
  FFT_STATUS            getStatusFFT();
  void                  clearBuffers();
  void                  triggFFT();

 private:
  void                  parseConfigStr(char *configStr);
  void                  addDataToBuffer(double data);
  void                  calcFFT();
  void                  scaleFFT();
  void                  calcFFTAmp();
  void                  initAsyn();
  void                  updateStatus(FFT_STATUS status);  // Also updates asynparam
  static int            dataTypeSupported(ecmcEcDataType dt);

  ecmcDataItem         *dataItem_;
  ecmcAsynPortDriver   *asynPort_;
  kissfft<double>*      fftDouble_;
  double*               dataBuffer_;    // Input data (real)
  std::complex<double>* fftBuffer_;     // Result (complex)
  double*               fftBufferAmp_;  // Resulting amplitude (abs of fftBuffer_)
  size_t                elementsInBuffer_;
  // ecmc callback handle for use when deregister at unload
  int                   callbackHandle_;
  int                   fftCalcDone_;
  int                   objectId_;         // Unique object id
  int                   triggOnce_;
  double                scale_;            // Config: Data set size  
  FFT_STATUS            status_;           // Status/state  (NO_STAT, IDLE, ACQ, CALC)

  // Config options
  char*                 cfgDataSourceStr_; // Config: data source string
  int                   cfgDbgMode_;       // Config: allow dbg printouts
  int                   cfgApplyScale_;    // Config: apply scale 1/nfft
  int                   cfgDcRemove_;      // Config: remove dc (average) 
  size_t                cfgNfft_;          // Config: Data set size
  int                   cfgEnable_;        // Config: Enable data acq./calc.
  FFT_MODE              cfgMode_;          // Config: Mode continous or triggered.


  // Asyn
  ecmcAsynDataItem*     asynEnable_;       // Enable/disable acq./calcs
  ecmcAsynDataItem*     asynRawData_;      // Raw data (input) array (double)
  ecmcAsynDataItem*     asynFFTAmp_;       // FFT amplitude array (double)
  ecmcAsynDataItem*     asynFFTMode_;      // FFT mode (cont/trigg)
  ecmcAsynDataItem*     asynFFTStat_;      // FFT status (no_stat/idle/acq/calc)

  // Some generic utility functions
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
  static void           printEcDataArray(uint8_t*       data, 
                                         size_t         size,
                                         ecmcEcDataType dt,
                                         int objId);
  static void           printComplexArray(std::complex<double>* fftBuff,
                                          size_t elements,
                                          int objId);
  static std::string    to_string(int value);
};

#endif  /* ECMC_FFT_H_ */
