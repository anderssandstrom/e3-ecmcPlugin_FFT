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

class ecmcFFT {
 public:
  ecmcFFT(ecmcDataItem *dataItem, 
          ecmcAsynPortDriver* asynPort,
          size_t nfft = ECMC_PLUGIN_DEFAULT_NFFT);

  ~ecmcFFT();  

  //Register callback
  int ConnectToDataSource();
  // Add data to buffer
  void dataUpdatedCallback(uint8_t* data, 
                           size_t size,
                           ecmcEcDataType dt);
 private:
  void clearBuffer();
  ecmcDataItem       *dataItem_;
  ecmcAsynPortDriver *asynPort_;
  uint8_t*            dataBuffer_;
  size_t              nfft_;
  size_t              bufferSizeBytes_;
  size_t              bytesInBuffer_;
};

#endif  /* ECMC_FFT_H_ */
