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

// data callback 
void dataUpdatedCallback(uint8_t* data, size_t size, ecmcEcDataType dt, void* obj) {
  printf("Data updates\n");
}

ecmcFFT::ecmcFFT(ecmcDataItem* dataItem, ecmcAsynPortDriver* asynPort) {
  dataItem_ = dataItem;
  asynPort_ = asynPort;
}

ecmcFFT::~ecmcFFT() {

}

int ecmcFFT::ConnectToDataSource() {
  return dataItem_->regDataUpdatedCallback(dataUpdatedCallback, this);
}
