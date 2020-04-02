/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcFFTWrap.h
*
*  Created on: Mar 22, 2020
*      Author: anderssandstrom
*
\*************************************************************************/
#ifndef ECMC_FFT_WRAP_H_
#define ECMC_FFT_WRAP_H_

# ifdef __cplusplus
extern "C" {
# endif  // ifdef __cplusplus

//Error codes
#define ECMC_PLUGIN_ERROR_WRAP_BASE 10
#define ECMC_PLUGIN_ERROR_CONFIG_STR_PARSE_FAIL (ECMC_PLUGIN_ERROR_WRAP_BASE + 1)
#define ECMC_PLUGIN_ERROR_NO_SOURCE (ECMC_PLUGIN_ERROR_WRAP_BASE + 2)
#define ECMC_PLUGIN_ERROR_DATA_SOURCE_NULL (ECMC_PLUGIN_ERROR_WRAP_BASE + 3)
#define ECMC_PLUGIN_ERROR_ASYNPORT_NULL (ECMC_PLUGIN_ERROR_WRAP_BASE + 4)
#define ECMC_PLUGIN_ERROR_FFT_NULL (ECMC_PLUGIN_ERROR_WRAP_BASE + 5)

#define PRINT_IF_DBG_MODE(fmt, ...)       \
  {                                       \
    if(dbgModeOption){                    \
      printf(fmt, ## __VA_ARGS__);        \
    }                                     \
  }                                       \


int createFFT(char *source, int dbgMode);

# ifdef __cplusplus
}
# endif  // ifdef __cplusplus

#endif  /* ECMC_FFT_WRAP_H_ */
