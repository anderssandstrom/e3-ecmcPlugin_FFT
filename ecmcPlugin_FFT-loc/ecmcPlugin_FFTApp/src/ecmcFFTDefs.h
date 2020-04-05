/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcFFTDefs.h
*
*  Created on: Mar 22, 2020
*      Author: anderssandstrom
*      Credits to  https://github.com/sgreg/dynamic-loading 
*
\*************************************************************************/

#ifndef ECMC_FFT_DEFS_H_
#define ECMC_FFT_DEFS_H_

// Options
#define ECMC_PLUGIN_DBG_OPTION_CMD         "DBG_PRINT="
#define ECMC_PLUGIN_SOURCE_OPTION_CMD      "SOURCE="
#define ECMC_PLUGIN_NFFT_OPTION_CMD        "NFFT="
#define ECMC_PLUGIN_APPLY_SCALE_OPTION_CMD "APPLY_SCALE="
#define ECMC_PLUGIN_DC_REMOVE_OPTION_CMD   "DC_REMOVE="

/** Just one error code in "c" part of plugin 
(error handled with exceptions i c++ part) */
#define ECMC_PLUGIN_FFT_ERROR_CODE 1

// Default size (must be n²)
#define ECMC_PLUGIN_DEFAULT_NFFT 4096

#endif  /* ECMC_FFT_DEFS_H_ */
