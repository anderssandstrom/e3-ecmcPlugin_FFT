e3-ecmcPlugin_Advanced
======
ESS Site-specific EPICS module : ecmcPlugin_Advanced

Example illustrating how to implement plugins for use with ecmc (https://github.com/icshwi/ecmc).

Shows how to implement:
* callbacks 
* custom ecmc plc-functions
* custom ecmc plc-constants 
* access to ecmcAsynPort object to add plugin specific asyn parameter.

# Interface
The interface is defined in the structure ecmcPluginData in ecmcPluginDefs.h:
```
struct ecmcPluginData {
  // Name 
  const char *name;
  // Description
  const char *desc;
  // Plugin version
  int version;
  // ECMC_PLUG_VERSION_MAGIC
  int ifVersion;
  // Optional construct func, called once at load
  int (*constructFnc)(void);
  // Optional destruct func, called once at unload
  void (*destructFnc)(void);
  // Optional func that will be called once just before enter realtime mode
  int (*realtimeEnterFnc)(void*);
  // Optional func that will be called once just before exit realtime mode
  int (*realtimeExitFnc)(void);
  // Optional func that will be called each realtime cycle
  int (*realtimeFnc)(int);
  // Allow max ECMC_PLUGIN_MAX_PLC_FUNC_COUNT custom plc functions
  struct ecmcOnePlcFunc  funcs[ECMC_PLUGIN_MAX_PLC_FUNC_COUNT];
  // Allow max ECMC_PLUGIN_MAX_PLC_CONST_COUNT custom plc constants
  struct ecmcOnePlcConst consts[ECMC_PLUGIN_MAX_PLC_CONST_COUNT];
};
```
## Callbacks:
All callbacks are optional. If the callbacks are not used then set the func pointer to NULL 
("ecmcPluginData.*Fnc=NULL").

Example:
```
ecmcPluginData.destructFnc=NULL
ecmcPluginData.constructFnc=NULL
...
```

### int  constructFnc(), optional
This callback is called once when the plugin is loaded into ecmc. This is a good place to put code for any initialization needed in the plugin module.

Return value: 0 for success or error code.

In this example plugin only a printout is made in this callback.

### void destructFnc(), optional
This callback is called once when the plugin is unloaded. This is a good place to put cleanup code needed by the plugin module.

In this example plugin only a printout is made in this callback.

### int realtimeFnc(int ecmcErrorId), optional
This callback is called once in each realtime loop (sync to ecmc). This is a good place to put any cyclic processing needed by the plugin module. 

NOTE: This callback is executed by ecmc realtime thread. Take measures to stay as short time as possible in this function. If lots of processing is needed a separate worker thread might be a solution.

Parameters: ecmcErrorId: reflects the current errorstate of ecmc.

Return value: 0 for success or error code.

In this example a counter value is increased for each call and the coresponding asyn parameter is updated.

### int realtimeEnterFnc(void* ecmcRefs), optional
This callback is called once just before ecmc enters realtime mode (starts rt-thread). This is a good place to make any prepartions needed before cyclic processing starts.

Parameters: ecmcRefs: ref to ecmcdata that can be cast to ecmcPluginDataRefs
```
struct ecmcPluginDataRefs {
  double sampleTimeMS;
  ecmcAsynPortDriver *ecmcAsynPort;
};
```
Return value: 0 for success or error code.

In this example a asyn parameter called "plugin.adv.counter" is registered. The ecmc realtime samplerate is also determined from the ecmcRefs

### int realtimeExitFnc(), optional
This callback is called once just before ecmc exits realtime mode (exits rt-thread).

Return value: 0 for success or error code.

In this example plugin only a printout is made in this callback.

### Example:
```
// Compile data for lib so ecmc now what to use
struct ecmcPluginData pluginDataDef = {
  // Name 
  .name = "ecmcExamplePlugin",
  // Description
  .desc = "Advanced example with use of asynport obj.",
  // Plugin version
  .version = ECMC_EXAMPLE_PLUGIN_VERSION,
  // ECMC_PLUG_VERSION_MAGIC
  .ifVersion = ECMC_PLUG_VERSION_MAGIC, 
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
...
...

```
## PLC functions:
Custom ecmc PLC-functions can be implemented in plugins. Currentlly the interface supports implementation of up to 64 plc functions. Each plc function needs to be defined by the struct "ecmcOnePlcFunc":
```
struct ecmcOnePlcFunc {
  // Function name (this is the name you use in ecmc plc-code)
  const char *funcName;
  // Function description
  const char *funcDesc;
  /**
   * 7 different prototypes allowed (only doubles since reg in plc).
   * Only one funcArg<argCount> func shall be assigned the rest set to NULL 
   **/
  double (*funcArg0)();
  double (*funcArg1)(double);
  double (*funcArg2)(double,double);
  double (*funcArg3)(double,double,double);
  double (*funcArg4)(double,double,double,double);
  double (*funcArg5)(double,double,double,double,double);
  double (*funcArg6)(double,double,double,double,double,double);
};

```
Example:
```
.funcs[0] =      
      { /*----customPlcFunc1----*/
        // Function name (this is the name you use in ecmc plc-code)
        .funcName = "adv_plugin_func_1",
        // Function description
        .funcDesc = "Multiply arg0 with arg1.",
        /**
        * 7 different prototypes allowed (only doubles since reg in plc).
        * Only funcArg<argCount> one func shall be assigned the rest set to NULL.
        **/
        .funcArg0 = NULL,
        .funcArg1 = NULL,
        .funcArg2 = adv_customPlcFunc1, // Func 1 has 2 args
        .funcArg3 = NULL,
        .funcArg4 = NULL,
        .funcArg5 = NULL,
        .funcArg6 = NULL
      },
```
Note: Only the first non NULL function will be used (starting from funcArg0...)

## PLC constants
Custom ecmc PLC-constants can be implemented in plugins. Currentlly the interface supports implementation of up to 64 plc constants. Each plc constant needs to be defined by the struct "ecmcOnePlcConst":
```
struct ecmcOnePlcConst{
  const char *constName;
  const char *constDesc;
  double      constValue;
};

```
Example:
```
.consts[0] = {
        .constName = "adv_CONST_1",
        .constDesc = "Test constant \"adv_CONST_1\" = 1.234567890",
        .constValue = 1.234567890,
      },
```
## Dependencies:

All needed headers are available in ecmc (https://github.com/icshwi/ecmc)
### Simple plugins 

Only the "ecmcPluginDefs.h" header is needed.

### Advanced plugins 
When using the "void* ecmcRefs" param (cast to ecmcPluginDataRefs)in the "realtimeEnterFnc()" these additional headers are needed:
* from ecmc:
  * ecmcAsynPortDriver.h
  * ecmcAsynDataItem.h
  * ecmcAsynPortDriverUtils.h
  * ecmcDefinitions.h
  * ecmcErrorsList.h
  * ecmcPluginDataRefs.h
* from asyn:
  * asynPortDriver.h
  
Note: This define is needed in the plugin sources:
```
#define ECMC_IS_PLUGIN
```
## Plugin info for ecmcPlugin_Advanced
```
 Plugin info: 
   Name                 = ecmcExamplePlugin
   Description          = Advanced example with use of asynport obj.
   Version              = 1
   Interface version    = 512 (ecmc = 512)
       max plc funcs    = 64
       max plc consts   = 64
   Construct func       = @0x7fac4353d190
   Enter realtime func  = @0x7fac4353d200
   Exit realtime func   = @0x7fac4353d1c0
   Realtime func        = @0x7fac4353d1e0
   Destruct func        = @0x7fac4353d1b0
   dlhandle             = @0x182e580
   Plc functions:
     funcs[00]:
       Name       = "adv_plugin_func_1(arg0, arg1);"
       Desc       = Multiply arg0 with arg1.
       Arg count  = 2  
       func       = @0x7fac4353d170
     funcs[01]:
       Name       = "adv_plugin_func_2(arg0, arg1, arg2);"
       Desc       = Multiply arg0, arg1 and arg2.
       Arg count  = 3
       func       = @0x7fac4353d180
   Plc constants:
     consts[00]:
       Name     = "adv_CONST_1" = 1.235
       Desc     = Test constant "adv_CONST_1" = 1.234567890
     consts[01]:
       Name     = "adv_CONST_2" = 9.877
       Desc     = Test constant "adv_CONST_2" = 9.876543210 
```

## Access to asyn parameter (linked to record IOC_TEST:Plugin-Adv-Counter)
```
camonitor IOC_TEST:Plugin-Adv-Counter
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:21.535547 23  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:21.630954 24  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:21.740799 25  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:21.838110 26  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:21.930354 27  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:22.034919 28  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:22.130358 29  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:22.234041 30  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:22.334133 31  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:22.434937 32  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:22.530212 33  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:22.634437 34  
IOC_TEST:Plugin-Adv-Counter    2020-03-26 15:45:22.734606 35  
```
