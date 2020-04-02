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

#include "ecmcDataItem.h"
#include "ecmcAsynPortDriver.h"

class ecmcFFT {
 public:
  ecmcFFT(ecmcDataItem *dataItem, ecmcAsynPortDriver* asynPort);
  ~ecmcFFT();  

  //Register callback
  int ConnectToDataSource();

 private:
  ecmcDataItem       *dataItem_;
  ecmcAsynPortDriver *asynPort_;
};

#endif  /* ECMC_FFT_H_ */
