/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcAdvanced.h
*
*  Created on: Mar 22, 2020
*      Author: anderssandstrom
*
\*************************************************************************/
#ifndef ECMC_ADVANCED_H_
#define ECMC_ADVANCED_H_

#ifdef __cplusplus
extern "C" {
#endif  // ifdef __cplusplus

// get ecmc rt sample rate from ecmcPluginClient.h funcs
double getSampleRate();
// get ecmcAsynPort from ecmcPluginClient.h funcs
void*  getAsynPort();
// register a dummy asyn parameter "plugin.adv.counter"
int    initAsyn();
// increase value of counter and refresh asyn param
void   increaseCounter();

#ifdef __cplusplus
}
#endif  // ifdef __cplusplus

#endif  /* ECMC_ADVANCED_H_ */
