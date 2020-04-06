/*************************************************************************\
* Copyright (c) 2019 European Spallation Source ERIC
* ecmc is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
*
*  ecmcPluginExample.cpp
*
*  Created on: Mar 21, 2020
*      Author: anderssandstrom
*
\*************************************************************************/

// Needed to get headers in ecmc right...
#define ECMC_IS_PLUGIN
#define ECMC_EXAMPLE_PLUGIN_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif  // ifdef __cplusplus

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecmcPluginDefs.h"
#include "ecmcFFTDefs.h"
#include "ecmcFFTWrap.h"

static int    lastEcmcError   = 0;
static char*  lastConfStr         = NULL;

/** Optional. 
 *  Will be called once after successfull load into ecmc.
 *  Return value other than 0 will be considered error.
 *  configStr can be used for configuration parameters.
 **/
int fft_exampleConstruct(char *configStr)
{
  //This module is allowed to load several times so no need to check if loaded

  // create FFT object and register data callback
  lastConfStr = strdup(configStr);
  return createFFT(configStr);
}

/** Optional function.
 *  Will be called once at unload.
 **/
void fft_exampleDestruct(void)
{
  deleteAllFFTs();
  if(lastConfStr){
    free(lastConfStr);
  }
}

/** Optional function.
 *  Will be called each realtime cycle if definded
 *  ecmcError: Error code of ecmc. Makes it posible for 
 *  this plugin to react on ecmc errors
 *  Return value other than 0 will be considered to be an error code in ecmc.
 **/
int fft_exampleRealtime(int ecmcError)
{  
  lastEcmcError = ecmcError;
  return 0;
}

/** Link to data source here since all sources should be availabe at this stage
 **/
int fft_exampleEnterRT(){
  return linkDataToFFTs();
}

/** Optional function.
 *  Will be called once just before leaving realtime mode
 *  Return value other than 0 will be considered error.
 **/
int fft_exampleExitRT(void){
  return 0;
}

// Plc function for clear
double fft_clear(double index) {
  return (double)clearFFT((int)index);
}

// Plc function for enable
double fft_enable(double index, double enable) {
  return (double)enableFFT((int)index, (int)enable);
}

// Register data for plugin so ecmc know what to use
struct ecmcPluginData pluginDataDef = {
  // Allways use ECMC_PLUG_VERSION_MAGIC
  .ifVersion = ECMC_PLUG_VERSION_MAGIC, 
  // Name 
  .name = "ecmcPlugin_FFT",
  // Description
  .desc = "FFT plugin for use with ecmc.",
  // Option description
  .optionDesc = "\n    "ECMC_PLUGIN_DBG_OPTION_CMD"1/0 : Enables/disables printouts from plugin.\n"
                "    "ECMC_PLUGIN_SOURCE_OPTION_CMD"<source> : Sets source variable for FFT (example: ec0.s1.AI_1).\n"
                "    "ECMC_PLUGIN_NFFT_OPTION_CMD"<nfft> : Data points to collect.\n" 
                "    "ECMC_PLUGIN_APPLY_SCALE_OPTION_CMD"<1/0> : Apply scale.\n" 
                "    "ECMC_PLUGIN_DC_REMOVE_OPTION_CMD"<1/0> : Remove DC offset of input data (SOURCE).\n" 
                "    "ECMC_PLUGIN_ENABLE_OPTION_CMD"<1/0> : Enable data acq. and calcs (can be controlled over asyn).", 
  // Plugin version
  .version = ECMC_EXAMPLE_PLUGIN_VERSION,
  // Optional construct func, called once at load. NULL if not definded.
  .constructFnc = fft_exampleConstruct,
  // Optional destruct func, called once at unload. NULL if not definded.
  .destructFnc = fft_exampleDestruct,
  // Optional func that will be called each rt cycle. NULL if not definded.
  .realtimeFnc = fft_exampleRealtime,
  // Optional func that will be called once just before enter realtime mode
  .realtimeEnterFnc = fft_exampleEnterRT,
  // Optional func that will be called once just before exit realtime mode
  .realtimeExitFnc = fft_exampleExitRT,
  // PLC funcs
  .funcs[0] =
      { /*----customPlcFunc2----*/
        // Function name (this is the name you use in ecmc plc-code)
        .funcName = "fft_clear",
        // Function description
        .funcDesc = "double fft_clear(index) : Clear/reset fft[index].",
        /**
        * 7 different prototypes allowed (only doubles since reg in plc).
        * Only funcArg${argCount} func shall be assigned the rest set to NULL.
        **/
        .funcArg0 = NULL,
        .funcArg1 = fft_clear,
        .funcArg2 = NULL,
        .funcArg3 = NULL,
        .funcArg4 = NULL,
        .funcArg5 = NULL,
        .funcArg6 = NULL,
        .funcArg7 = NULL,
        .funcArg8 = NULL,
        .funcArg9 = NULL,
        .funcArg10 = NULL,        
      },
  .funcs[1] =
      { /*----customPlcFunc2----*/
        // Function name (this is the name you use in ecmc plc-code)
        .funcName = "fft_enable",
        // Function description
        .funcDesc = "double fft_enable(index, enable) : Set enable for fft[index].",
        /**
        * 7 different prototypes allowed (only doubles since reg in plc).
        * Only funcArg${argCount} func shall be assigned the rest set to NULL.
        **/
        .funcArg0 = NULL,
        .funcArg1 = NULL,
        .funcArg2 = fft_enable,
        .funcArg3 = NULL,
        .funcArg4 = NULL,
        .funcArg5 = NULL,
        .funcArg6 = NULL,
        .funcArg7 = NULL,
        .funcArg8 = NULL,
        .funcArg9 = NULL,
        .funcArg10 = NULL,        
      },
  .funcs[2] = {0},  // last element set all to zero..
  // PLC consts
  .consts[0] = {0}, // last element set all to zero..
};

ecmc_plugin_register(pluginDataDef);

# ifdef __cplusplus
}
# endif  // ifdef __cplusplus
