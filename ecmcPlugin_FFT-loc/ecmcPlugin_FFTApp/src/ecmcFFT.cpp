/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcAdvanced.cpp
*
*  Created on: Mar 22, 2020
*      Author: anderssandstrom
*      Credits to  https://github.com/sgreg/dynamic-loading 
*
\*************************************************************************/

// Needed to get headers in ecmc right...
#define ECMC_IS_PLUGIN

// Error Codes
#define ECMC_ERROR_ASYNPORT_NULL 1
#define ECMC_ERROR_ASYN_PARAM_FAIL 2

#include "ecmcAdvanced.h"
#include "ecmcPluginClient.h"
#include "ecmcAsynPortDriver.h"

// Vars
static int counter = 0;
static ecmcAsynDataItem *paramCount = NULL;

// Use ecmcPluginClient.h interface
double getSampleRate() {
    
  return getEcmcSampleRate();
}

// Use ecmcPluginClient.h interface
void* getAsynPort() {
  return getEcmcAsynPortDriver();
}

// register a dummy asyn parameter "plugin.adv.counter"
int initAsyn() {
  
  ecmcAsynPortDriver *ecmcAsynPort = (ecmcAsynPortDriver *)getEcmcAsynPortDriver();
  if(!ecmcAsynPort) {
    printf("Error: ecmcPlugin_Advanced: ecmcAsynPortDriver NULL.");
    return ECMC_ERROR_ASYNPORT_NULL;
  }

  // Add a dummy counter that incraeses one for each rt cycle
  paramCount = ecmcAsynPort->addNewAvailParam(
                                        "plugin.adv.counter",  // name
                                         asynParamInt32,       // asyn type 
                                         (uint8_t *)&(counter),// pointer to data
                                         sizeof(counter),      // size of data
                                         ECMC_EC_S32,          // ecmc data type
                                         0);                   // die if fail
  if(!paramCount) {
    printf("Error: ecmcPlugin_Advanced: Failed to create asyn param \"plugin.adv.counter\".");
    return ECMC_ERROR_ASYN_PARAM_FAIL;
  }
  paramCount->addSupportedAsynType(asynParamInt32);  // Only allw records of this type 
  paramCount->setAllowWriteToEcmc(false);  // read only
  paramCount->refreshParam(1); // read once into asyn param lib
  ecmcAsynPort->callParamCallbacks(ECMC_ASYN_DEFAULT_LIST, ECMC_ASYN_DEFAULT_ADDR);
  return 0;
}

// increase value of counter and refresh asyn param
void increaseCounter(){
  counter++;
  if(paramCount){
    paramCount->refreshParamRT(0);
    // "callParamCallbacks" are handled in ecmc rt thread so don't call
  }
}
