e3-ecmcPlugin_FFT
======
ESS Site-specific EPICS module : ecmcPlugin_FFT

A shared library with FFT functionalities loadable into ecmc:
https://github.com/epics-modules/ecmc (or local ess fork https://github.com/icshwi/ecmc).
Configuration is made through ecmccfg:
https://github.com/paulscherrerinstitute/ecmccfg (ot local ess fork https://github.com/icshwi/ecmccfg)

# Introduction

The main functionality of this plugin is to make FFT analysis of ecmc data. Most data in ecmc is accessible:
1. EtherCAT data  (ec0.s1.AI_1)
2. Motion data    (ax1.actpos)
3. PLC data       (plcs.plc0.static.testvalue)

Precautions have been taken in order to disturb ecmc real time thread as little as possible:
1. All CPU intensive calculations are handled in a low prio work thread (not ecmc thread).
2. Communication to epics is made via a dedicated asynPortDriver (not the same that ecmc uses).

See below for configuration options and the different modes supported by this ecmc plugin.

## Loading of FFT plugin in ecmc: 
A plugin is loaded by the ecmccfg command loadPlugin:
https://github.com/anderssandstrom/ecmccfg/blob/master/scripts/loadPlugin.cmd

Example:
```
 epicsEnvSet(ECMC_PLUGIN_FILNAME,"/epics/base-7.0.3.1/require/3.1.2/siteMods/ecmcPlugin_FFT/master/lib/linux-arm/libecmcPlugin_FFT.so")
 epicsEnvSet(ECMC_PLUGIN_CONFIG,"DBG_PRINT=1;SOURCE=ax1.actpos;")
 ${SCRIPTEXEC} ${ecmccfg_DIR}loadPlugin.cmd, "PLUGIN_ID=0,FILE=${ECMC_PLUGIN_FILNAME},CONFIG='${ECMC_PLUGIN_CONFIG}', REPORT=1
 epicsEnvUnset(ECMC_PLUGIN_FILNAME)
 epicsEnvUnset(ECMC_PLUGIN_CONFIG)

 dbLoadRecords(ecmcPluginFFT.template,"P=$(IOC):,INDEX=0, NELM=${FFT_NELM}")

```
This plugin supports multiple loading. For each load of the plugin a new FFT object will be created. In order to access these plugins, from plc:s or EPICS records, they can be accessed by an index. The first FFT plugin will have index 0. The next loaded FFT plugin will have index 1...

Note: If another plugin is loaded in between this will have no affect on these FFT indexes (so the FFT index is _not_ the same as plugin index).

## Configuration

The different available configuration settings:
* SOURCE=<source>    : Sets source variable for FFT (example: ec0.s1.AI_1). This config is mandatory.
* DBG_PRINT=<1/0>    : Enables/disables printouts from plugin, default = disabled.
* NFFT=<nfft>        : Data points to collect, default = 4096.
* APPLY_SCALE=<1/0>  : Apply scale, default = enabled.
* DC_REMOVE=<1/0>    : Remove DC offset of input data (SOURCE), default = disabled.
* ENABLE=<1/0>       : Enable data acq. and calcs (can be controlled over asyn), default = disabled.
* MODE=<CONT/TRIGG>  : Continious or triggered mode, defaults to TRIGG
* RATE=<rate in hz>  : fft data sample rate in hz (must be lower than ecmc rate and (ecmc_rate/fft_rate)=integer), default = ecmc rate.

Example configuration string:
```
"SOURCE=ax1.actpos;MODE=TRIGG;DBG_PRINT=1;ENABLE=1;"
```

### SOURCE
The data source is defined by setting the SOURCE option in the plugin configuration string.
This configuration is mandatory.

Example: Axis 1 actpos
```
"DBG_PRINT=1;SOURCE=ax1.actpos;"

```
Example: Ethercat slave 1 analog input ch1
```
"DBG_PRINT=1;SOURCE=ec0.s1.AI_1;"
```
### DBG_PRINT
Enable/disable printouts from plugin can be made bu setting the "DBG_PRINT" option. The default setting is disabled.

Exmaple: Disable
```
"DBG_PRINT=0;SOURCE=ax1.actpos;"
```

### NFFT
Defines number of samples for each measurement. Default setting is 4096.

Exmaple: 1024
```
"NFFT=1024;DBG_PRINT=0;SOURCE=ax1.actpos;"
```
### APPLY_SCALE
Apply scaling in order to get correct amplitude of fft. Disabled as default (lower cpu usage).

Exmaple: Enable
```
"APPLY_SCALE=1;NFFT=1024;DBG_PRINT=0;SOURCE=ax1.actpos;"
```

### DC_REMOVE
Remove DC of input signal. Default is disabled.

Exmaple: Enable
```
"DC_REMOVE=1;APPLY_SCALE=1;NFFT=1024;DBG_PRINT=0;SOURCE=ax1.actpos;"
```
### ENABLE
Enable data acq. and FFT calcs. The default settings is disabled so needs to be enabled from plc or over asyn in order to start calculations.

Exmaple: Enable at startup
```
"ENABLE=1;DC_REMOVE=1;APPLY_SCALE=1;NFFT=1024;DBG_PRINT=0;SOURCE=ax1.actpos;"
```
### MODE
The FFT module can operate in two different modes:
* CONTINIOUS (CONT)
* TRIGGERED (TRIGG)

Continious mode:
1. Clear data buffers 
2. Wait for enable (from plc or asyn/epics record or configuration(see above))
3. Data acqusition starts 
4. When buffers are full a worker thread will be triggered to calculate the FFT.
5. FTT results are sent by callback over asyn to epics records
6. goto 1.

Note: data will be "lost" during calculation time.

Triggered mode:
1. Clear data buffers 
2. Wait for enable (from plc or asyn/epics record or configuration(see above))
3. Wait for trigger (from plc or asyn/epics record)
3. Data acqusition starts 
4. When buffers are full a worker thread will be triggered to calculate the FFT.
5. FTT results are sent by callback over asyn to epics records
6. goto 1.

Exmaple: Mode triggered
```
"MODE=TRIGG;ENABLE=1;DC_REMOVE=1;APPLY_SCALE=1;NFFT=1024;DBG_PRINT=0;SOURCE=ax1.actpos;"
```
### RATE
Sets the sample rate of the raw input data (from data source). The default value is the ecmc rate for that data source.
Note: only a lower and "integer" division of sample rate can be defined.
Exmaple: Rate = 100Hz
```
RATE=100;MODE=TRIGG;ENABLE=1;DC_REMOVE=1;APPLY_SCALE=1;NFFT=1024;DBG_PRINT=0;SOURCE=ax1.actpos;"
```

## EPICS records
Each FFT plugin object will create a new asynportdriver-port named "PLUGIN.FFT<index>" (index is explaine above).
The reason for a dedicated asynport is to influence ecmc as little as possible.
The plugin contains a template file that will make most information availbe from records:
* Rawdata array                    (ro)
* FFT amplitude array (result)     (ro)
* FFT x axis (frequencies)         (ro)
* Status                           (ro) 
* Mode                             (rw)
* Sample rate                      (ro)
* Enable cmd                       (rw)
* Trigger cmd                      (rw)
* NFFT                             (ro)

The available records from this template file can bMeasurese listed by the cmd: 
```
dbgrep *FFT*
```
Note: The FFT asynparameters will not be visible by the ecmcReport iocsh command since the FFT records belong to another port.

## PLC interface

### PLC Functions
1. "fft_clear(arg0);"         double fft_clear(index) : Clear/reset fft[index].
2. "fft_enable(arg0, arg1);"  double fft_enable(index, enable) : Set enable for fft[index].
3. "fft_trigg(arg0);"         double fft_trigg(index) : Trigg new measurement for fft[index]. Will clear buffers.
4. "fft_mode(arg0, arg1);"    double fft_mode(index, mode) : Set mode Cont(1)/Trigg(2) for fft[index].
5. "fft_stat(arg0);"          double fft_stat(index) : Get status of fft (NO_STAT, IDLE, ACQ, CALC) for fft[index].

### PLC Constants:

1. "fft_CONT"    = 1:  FFT Mode: Continious
2. "fft_TRIGG"   = 2:  FFT Mode :Triggered
3. "fft_NO_STAT" = 0:  FFT Status: Invalid state
4. "fft_IDLE"    = 1:  FFT Status: Idle state (waiting for trigger)
5. "fft_ACQ"     = 2:  FFT Status: Acquiring data
6. "fft_CALC"    = 3:  FFT Status: Calculating result

## Example script
An example script can be found in the iocsh directory of this repo.
This example load 2 FFT plugin objects:
1. SOURCE="plcs.plc0.static.sineval" which is a plc variable updated as a sinus with default freq 5 Hz.
2. SOURCE="ecmc.thread.latency.max". This is the execution latency of ecmc (in nanoseconds).

## Plugin info

```
Plugin info: 
  Index                = 1
  Name                 = ecmcPlugin_FFT
  Description          = FFT plugin for use with ecmc.
  Option description   = 
    DBG_PRINT=<1/0>    : Enables/disables printouts from plugin, default = disabled.
    SOURCE=<source>    : Sets source variable for FFT (example: ec0.s1.AI_1).
    NFFT=<nfft>        : Data points to collect, default = 4096.
    APPLY_SCALE=<1/0>  : Apply scale, default = disabled.
    DC_REMOVE=<1/0>    : Remove DC offset of input data (SOURCE), default = disabled.
    ENABLE=<1/0>       : Enable data acq. and calcs (can be controlled over asyn), default = disabled.
    MODE=<CONT/TRIGG>  : Continious or triggered mode, defaults to TRIGG
    RATE=<rate in hz>  : fft data sample rate in hz (must be lower than ecmc rate and (ecmc_rate/fft_rate)=integer), default = ecmc rate.
  Filename             = /epics/base-7.0.3.1/require/3.1.2/siteMods/ecmcPlugin_FFT/master/lib/linux-arm/libecmcPlugin_FFT.so
  Config string        = SOURCE=ecmc.thread.latency.max;DBG_PRINT=0;NFFT=1024;
  Version              = 1
  Interface version    = 512 (ecmc = 512)
     max plc funcs     = 64
     max plc func args = 10
     max plc consts    = 64
  Construct func       = @0xb5022044
  Enter realtime func  = @0xb5022090
  Exit realtime func   = @0xb502203c
  Realtime func        = @0xb5022034
  Destruct func        = @0xb502206c
  dlhandle             = @0x1d9de28
  Plc functions:
    funcs[00]:
      Name       = "fft_clear(arg0);"
      Desc       = double fft_clear(index) : Clear/reset fft[index].
      Arg count  = 1
      func       = @0xb5022094
    funcs[01]:
      Name       = "fft_enable(arg0, arg1);"
      Desc       = double fft_enable(index, enable) : Set enable for fft[index].
      Arg count  = 2
      func       = @0xb50220b0
    funcs[02]:
      Name       = "fft_trigg(arg0);"
      Desc       = double fft_trigg(index) : Trigg new measurement for fft[index]. Will clear buffers.
      Arg count  = 1
      func       = @0xb50220d4
    funcs[03]:
      Name       = "fft_mode(arg0, arg1);"
      Desc       = double fft_mode(index, mode) : Set mode Cont(1)/Trigg(2) for fft[index].
      Arg count  = 2
      func       = @0xb50220f0
    funcs[04]:
      Name       = "fft_stat(arg0);"
      Desc       = double fft_stat(index) : Get status of fft (NO_STAT, IDLE, ACQ, CALC) for fft[index].
      Arg count  = 1
      func       = @0xb5022114
  Plc constants:
    consts[00]:
      Name     = "fft_CONT" = 1.000
      Desc     = FFT Mode: Continious
    consts[01]:
      Name     = "fft_TRIGG" = 2.000
      Desc     = FFT Mode :Triggered
    consts[02]:
      Name     = "fft_NO_STAT" = 0.000
      Desc     = FFT Status: Invalid state
    consts[03]:
      Name     = "fft_IDLE" = 1.000
      Desc     = FFT Status: Idle state (waiting for trigger)
    consts[04]:
      Name     = "fft_ACQ" = 2.000
      Desc     = FFT Status: Acquiring data
    consts[05]:
      Name     = "fft_CALC" = 3.000
      Desc     = FFT Status: Calculating result

```
