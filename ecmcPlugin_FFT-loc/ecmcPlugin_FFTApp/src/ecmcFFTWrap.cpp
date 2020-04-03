/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcFFTWrap.cpp
*
*  Created on: Mar 22, 2020
*      Author: anderssandstrom
*      Credits to  https://github.com/sgreg/dynamic-loading 
*
\*************************************************************************/

// Needed to get headers in ecmc right...
#define ECMC_IS_PLUGIN

#include "ecmcFFTWrap.h"
#include "ecmcFFT.h"
#include "ecmcAsynPortDriver.h"
#include "ecmcPluginClient.h"

static ecmcFFT*            fft           = NULL;
static ecmcDataItem*       dataItem      = NULL;
static ecmcAsynPortDriver* ecmcAsynPort  = NULL;
static int                 dbgModeOption = 0;

int createFFT(char* source, int dbgMode) {
  dbgModeOption = dbgMode;
  // Get ecmcDataItem for source
  dataItem = (ecmcDataItem*)getEcmcDataItem(source);
  if(!dataItem) {
    PRINT_IF_DBG_MODE("%s/%s:%d: Error: dataItem=NULL (source %s not found) (0x%x).\n",
                      __FILE__, __FUNCTION__, __LINE__, source,
                       ECMC_PLUGIN_ERROR_DATA_SOURCE_NULL);
    return ECMC_PLUGIN_ERROR_DATA_SOURCE_NULL;
  }

  // Get ecmcAsynPort
  ecmcAsynPort = (ecmcAsynPortDriver*)getEcmcAsynPortDriver();
  if(!ecmcAsynPort) {
    PRINT_IF_DBG_MODE("%s/%s:%d: Error: ecmcAsynPort NULL (0x%x).\n",
                      __FILE__, __FUNCTION__, __LINE__, ECMC_PLUGIN_ERROR_ASYNPORT_NULL);
    return ECMC_PLUGIN_ERROR_ASYNPORT_NULL;
  }

  // create new ecmcFFT object
  fft = new ecmcFFT(dataItem, ecmcAsynPort);
  if(!fft) {
    PRINT_IF_DBG_MODE("%s/%s:%d: Error: ecmcFFT NULL (0x%x).\n",
                      __FILE__, __FUNCTION__, __LINE__, ECMC_PLUGIN_ERROR_FFT_NULL);
    return ECMC_PLUGIN_ERROR_FFT_NULL;
  }  
  
  // Register callback
  return fft->getErrorId();
}
