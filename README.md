e3-ecmcPlugin_FFT
======
ESS Site-specific EPICS module : ecmcPlugin_FFT

This module is a work in progress and is not functional yet.

A shared library with FFT functionalities loadable into ecmc:
https://github.com/epics-modules/ecmc (or local ess fork https://github.com/icshwi/ecmc).
Configuration is made through ecmccfg: 
https://github.com/paulscherrerinstitute/ecmccfg (ot local ess fork https://github.com/icshwi/ecmccfg)

## Load FFT plugin in ecmc: 
A plugin is loaded by the ecmccfg command loadPlugin:
https://github.com/anderssandstrom/ecmccfg/blob/master/scripts/loadPlugin.cmd

Example:
```
 epicsEnvSet(ECMC_PLUGIN_FILNAME,"/epics/base-7.0.3.1/require/3.1.2/siteMods/ecmcPlugin_FFT/master/lib/linux-arm/libecmcPlugin_FFT.so")
 epicsEnvSet(ECMC_PLUGIN_CONFIG,"DBG_PRINT=1;SOURCE=ax1.actpos;")
 ${SCRIPTEXEC} ${ecmccfg_DIR}loadPlugin.cmd, "PLUGIN_ID=0,FILE=${ECMC_PLUGIN_FILNAME},CONFIG='${ECMC_PLUGIN_CONFIG}', REPORT=1
 epicsEnvUnset(ECMC_PLUGIN_FILNAME)
 epicsEnvUnset(ECMC_PLUGIN_CONFIG)
```
### Define SOURCE
The data source is defined by setting  the SOURCE option in the plugin configuration string.
Example: Axis 1 actpos
```
epicsEnvSet(ECMC_PLUGIN_CONFIG,"DBG_PRINT=1;SOURCE=ax1.actpos;")

```
Example: Ethercat slave 1 analog input ch1
```
epicsEnvSet(ECMC_PLUGIN_CONFIG,"DBG_PRINT=1;SOURCE=ec0.s1.AI_1;")

```
### Enable disable printouts
Enable/disable printouts from plugin can be made bu setting the "DBG_PRINT" option.

Exmaple: Disable
```
epicsEnvSet(ECMC_PLUGIN_CONFIG,"DBG_PRINT=0;SOURCE=ax1.actpos;")

