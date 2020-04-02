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
#include "ecmcFFT.h"

#define ECMC_PLUGIN_DBG_OPTION_CMD "DBG_PRINT="
#define ECMC_PLUGIN_SOURCE_OPTION_CMD "SOURCE="

#define PRINT_IF_DBG_MODE(fmt, ...)       \
  {                                       \
    if(dbgModeOption){                    \
      printf(fmt, ## __VA_ARGS__);        \
    }                                     \
  }                                       \

static int    lastEcmcError  = 0;
static double ecmcSampleRate = 0;
static void*  ecmcAsynPort   = NULL;
static char*  confStr        = NULL;
static int    dbgModeOption  = 0;
static char*  source         = NULL;


/** Optional. 
 *  Will be called once after successfull load into ecmc.
 *  Return value other than 0 will be considered error.
 *  configStr can be used for configuration parameters.
 **/
int adv_exampleConstruct(char * configStr)
{
  PRINT_IF_DBG_MODE("%s/%s:%d: ConfigStr=\"%s\"...\n",__FILE__, __FUNCTION__, __LINE__,configStr);
  // check config parameters
  if (configStr && configStr[0]) {    
    char *pOptions = strdup(configStr);
    char *pThisOption = pOptions;
    char *pNextOption = pOptions;

    while (pNextOption && pNextOption[0]) {
      pNextOption = strchr(pNextOption, ';');
      if (pNextOption) {
        *pNextOption = '\0'; /* Terminate */
        pNextOption++;       /* Jump to (possible) next */
      }
      
      // ECMC_PLUGIN_DBG_OPTION_CMD
      if (!strncmp(pThisOption, ECMC_PLUGIN_DBG_OPTION_CMD, strlen(ECMC_PLUGIN_DBG_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_DBG_OPTION_CMD);
        dbgModeOption = atoi(pThisOption);
      } 
      
      // ECMC_PLUGIN_SOURCE_OPTION_CMD
      else if (!strncmp(pThisOption, ECMC_PLUGIN_SOURCE_OPTION_CMD, strlen(ECMC_PLUGIN_SOURCE_OPTION_CMD))) {
        pThisOption += strlen(ECMC_PLUGIN_SOURCE_OPTION_CMD);
        // get string to next ';'
         source=strdup(pThisOption);
      } 
      pThisOption = pNextOption;
    }    
    free(pOptions);
  }
  //printout options
  PRINT_IF_DBG_MODE("%s/%s:%d: %s%d, %s\"%s\"\n",__FILE__, 
                    __FUNCTION__, __LINE__,ECMC_PLUGIN_DBG_OPTION_CMD,
                    dbgModeOption, ECMC_PLUGIN_SOURCE_OPTION_CMD, source);
  
  // Determine ecmc sample rate (just for demo)
  ecmcSampleRate = getSampleRate();
  PRINT_IF_DBG_MODE("%s/%s:%d: Ecmc sample rate is: %lf ms\n",__FILE__, __FUNCTION__, __LINE__,ecmcSampleRate);

  // Use ecmcAsynPort (just for demo)
  ecmcAsynPort = getAsynPort();

  // init asyn param counter
  initAsyn(ecmcAsynPort);
  
  return 0;
}

/** Optional function.
 *  Will be called once at unload.
 **/
void adv_exampleDestruct(void)
{
  PRINT_IF_DBG_MODE("%s/%s:%d...\n",__FILE__, __FUNCTION__, __LINE__);
  
  if(source) {
    free(source);
  }

  if(confStr){
    free(confStr);
  }
}

/** Optional function.
 *  Will be called each realtime cycle if definded
 *  ecmcError: Error code of ecmc. Makes it posible for 
 *  this plugin to react on ecmc errors
 *  Return value other than 0 will be considered to be an error code in ecmc.
 **/
int adv_exampleRealtime(int ecmcError)
{  
  //Update asynparam counter
  increaseCounter();
  lastEcmcError = ecmcError;
  return 0;
}

/** Optional function.
 *  Will be called once just before going to realtime mode
 *  Return value other than 0 will be considered error.
 **/
int adv_exampleEnterRT(){
  return 0;
}

/** Optional function.
 *  Will be called once just before leaving realtime mode
 *  Return value other than 0 will be considered error.
 **/
int adv_exampleExitRT(void){
  PRINT_IF_DBG_MODE("%s/%s:%d...\n",__FILE__, __FUNCTION__, __LINE__);
  return 0;
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
  .optionDesc = "\n    "ECMC_PLUGIN_DBG_OPTION_CMD"1/0      : Enables/disables printouts from plugin.\n"
                "    "ECMC_PLUGIN_SOURCE_OPTION_CMD"<source>    : Sets source variable for FFT (example: ec0.s1.AI_1).",
  // Plugin version
  .version = ECMC_EXAMPLE_PLUGIN_VERSION,
  // Optional construct func, called once at load. NULL if not definded.
  .constructFnc = adv_exampleConstruct,
  // Optional destruct func, called once at unload. NULL if not definded.
  .destructFnc = adv_exampleDestruct,
  // Optional func that will be called each rt cycle. NULL if not definded.
  .realtimeFnc = adv_exampleRealtime,
  // Optional func that will be called once just before enter realtime mode
  .realtimeEnterFnc = adv_exampleEnterRT,
  // Optional func that will be called once just before exit realtime mode
  .realtimeExitFnc = adv_exampleExitRT,
  // PLC funcs
  .funcs[0] = {0},  // last element set all to zero..
  // PLC consts
  .consts[0] = {0}, // last element set all to zero..
};

ecmc_plugin_register(pluginDataDef);

# ifdef __cplusplus
}
# endif  // ifdef __cplusplus
