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
int fftConstruct(char *configStr)
{
  //This module is allowed to load several times so no need to check if loaded

  // create FFT object and register data callback
  lastConfStr = strdup(configStr);
  return createFFT(configStr);
}

/** Optional function.
 *  Will be called once at unload.
 **/
void fftDestruct(void)
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
int fftRealtime(int ecmcError)
{  
  lastEcmcError = ecmcError;
  return 0;
}

/** Link to data source here since all sources should be availabe at this stage
 **/
int fftEnterRT(){
  return linkDataToFFTs();
}

/** Optional function.
 *  Will be called once just before leaving realtime mode
 *  Return value other than 0 will be considered error.
 **/
int fftExitRT(void){
  return 0;
}

// Plc function for clear of buffers
double fft_clear(double index) {
  return (double)clearFFT((int)index);
}

// Plc function for enable
double fft_enable(double index, double enable) {
  return (double)enableFFT((int)index, (int)enable);
}

// Plc function for trigg new measurement (will clear buffers)
double fft_trigg(double index) {
  return (double)triggFFT((int)index);
}

// Plc function for enable
double fft_mode(double index, double mode) {
  return (double)modeFFT((int)index, (FFT_MODE)((int)mode));
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
  .optionDesc = "\n    "ECMC_PLUGIN_DBG_OPTION_CMD"<1/0> : Enables/disables printouts from plugin.\n"
                "    "ECMC_PLUGIN_SOURCE_OPTION_CMD"<source> : Sets source variable for FFT (example: ec0.s1.AI_1).\n"
                "    "ECMC_PLUGIN_NFFT_OPTION_CMD"<nfft> : Data points to collect.\n" 
                "    "ECMC_PLUGIN_APPLY_SCALE_OPTION_CMD"<1/0> : Apply scale.\n" 
                "    "ECMC_PLUGIN_DC_REMOVE_OPTION_CMD"<1/0> : Remove DC offset of input data (SOURCE).\n" 
                "    "ECMC_PLUGIN_ENABLE_OPTION_CMD"<1/0> : Enable data acq. and calcs (can be controlled over asyn)."
                "    "ECMC_PLUGIN_MODE_OPTION_CMD"<CONT/TRIGG> : Continious or triggered mode."
                , 
  // Plugin version
  .version = ECMC_EXAMPLE_PLUGIN_VERSION,
  // Optional construct func, called once at load. NULL if not definded.
  .constructFnc = fftConstruct,
  // Optional destruct func, called once at unload. NULL if not definded.
  .destructFnc = fftDestruct,
  // Optional func that will be called each rt cycle. NULL if not definded.
  .realtimeFnc = fftRealtime,
  // Optional func that will be called once just before enter realtime mode
  .realtimeEnterFnc = fftEnterRT,
  // Optional func that will be called once just before exit realtime mode
  .realtimeExitFnc = fftExitRT,
  // PLC funcs
  .funcs[0] =
      { /*----fft_clear----*/
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
      { /*----fft_enable----*/
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
    .funcs[2] =
      { /*----fft_trigg----*/
        // Function name (this is the name you use in ecmc plc-code)
        .funcName = "fft_trigg",
        // Function description
        .funcDesc = "double fft_trigg(index) : Trigg new measurement for fft[index]. Will clear buffers.",
        /**
        * 7 different prototypes allowed (only doubles since reg in plc).
        * Only funcArg${argCount} func shall be assigned the rest set to NULL.
        **/
        .funcArg0 = NULL,
        .funcArg1 = fft_trigg,
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
    .funcs[3] =
      { /*----fft_mode----*/
        // Function name (this is the name you use in ecmc plc-code)
        .funcName = "fft_mode",
        // Function description
        .funcDesc = "double fft_mode(index, mode) : Set mode Cont(1)/Trigg(2) for fft[index].",
        /**
        * 7 different prototypes allowed (only doubles since reg in plc).
        * Only funcArg${argCount} func shall be assigned the rest set to NULL.
        **/
        .funcArg0 = NULL,
        .funcArg1 = NULL,
        .funcArg2 = fft_mode,
        .funcArg3 = NULL,
        .funcArg4 = NULL,
        .funcArg5 = NULL,
        .funcArg6 = NULL,
        .funcArg7 = NULL,
        .funcArg8 = NULL,
        .funcArg9 = NULL,
        .funcArg10 = NULL,        
      },
  .funcs[4] = {0},  // last element set all to zero..
  // PLC consts
  /* CONTINIOUS MODE = 1 */
  .consts[0] = {
        .constName = "fft_CONT",
        .constDesc = "Continious mode",
        .constValue = CONT
      },
  /* TRIGGERED MODE = 2 */
  .consts[1] = {
        .constName = "fft_TRIGG",
        .constDesc = "Triggered mode",
        .constValue = TRIGG
      },
  .consts[2] = {0}, // last element set all to zero..
};

ecmc_plugin_register(pluginDataDef);

# ifdef __cplusplus
}
# endif  // ifdef __cplusplus
