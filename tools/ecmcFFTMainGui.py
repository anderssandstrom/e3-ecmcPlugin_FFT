#*************************************************************************
# Copyright (c) 2020 European Spallation Source ERIC
# ecmc is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution. 
#
#   ecmcFFTMainGui.py
#
#  Created on: October 6, 2020
#      Author: Anders Sandström
#    
#  Plots two waveforms (x vs y) updates for each callback on the y-pv
#  
#*************************************************************************

import sys
import epics
from PyQt5.QtWidgets import *
from PyQt5 import QtWidgets
from PyQt5.QtCore import *
from PyQt5.QtGui import *
import numpy as np
import matplotlib
matplotlib.use("Qt5Agg")
from matplotlib.figure import Figure
from matplotlib.animation import TimedAnimation
from matplotlib.lines import Line2D
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
import matplotlib.pyplot as plt 
import threading

# FFT object pvs <prefix>Plugin-FFT<fftPluginId>-<suffixname>
# IOC_TEST:Plugin-FFT0-stat
# IOC_TEST:Plugin-FFT0-NFFT x
# IOC_TEST:Plugin-FFT0-Mode-RB
# IOC_TEST:Plugin-FFT0-SampleRate-Act x
# IOC_TEST:Plugin-FFT0-Enable x
# IOC_TEST:Plugin-FFT0-Trigg x
# IOC_TEST:Plugin-FFT0-Source x
# IOC_TEST:Plugin-FFT0-Raw-Data-Act x
# IOC_TEST:Plugin-FFT0-PreProc-Data-Act 
# IOC_TEST:Plugin-FFT0-Spectrum-Amp-Act x
# IOC_TEST:Plugin-FFT0-Spectrum-X-Axis-Act x


class comSignal(QObject):
    data_signal = pyqtSignal(object)

class ecmcFFTMainGui(QtWidgets.QDialog):
    def __init__(self,prefix=None,fftPluginId=None):        
        super(ecmcFFTMainGui, self).__init__()        
        self.pvPrefixStr = prefix
        self.fftPluginId = fftPluginId

        # Callbacks through signals
        self.comSignalSpectX = comSignal()        
        self.comSignalSpectX.data_signal.connect(self.callbackFuncSpectX)
        self.comSignalSpectY = comSignal()        
        self.comSignalSpectY.data_signal.connect(self.callbackFuncSpectY)
        self.comSignalRawData = comSignal()        
        self.comSignalRawData.data_signal.connect(self.callbackFuncrawData)

        self.pause = 0

        # Data Arrays
        self.spectX = None
        self.spectY = None
        self.rawdataY = None
        self.rawdataX = None

        self.enable = None

        self.figure = plt.figure()                            
        self.plottedLineSpect = None
        self.plottedLineRaw = None
        self.axSpect = None
        self.axRaw = None
        self.canvas = FigureCanvas(self.figure)   
        self.toolbar = NavigationToolbar(self.canvas, self) 
        self.pauseBtn = QPushButton(text = 'pause')
        self.pauseBtn.setFixedSize(100, 50)
        self.pauseBtn.clicked.connect(self.pauseBtnAction)        
        self.pauseBtn.setStyleSheet("background-color: green")

        self.enableBtn = QPushButton(text = 'enable FFT')
        self.enableBtn.setFixedSize(100, 50)
        self.enableBtn.clicked.connect(self.enableBtnAction)            

        self.triggBtn = QPushButton(text = 'trigg FFT')
        self.triggBtn.setFixedSize(100, 50)
        self.triggBtn.clicked.connect(self.triggBtnAction)            

        # Pv names based on structure:  <prefix>Plugin-FFT<fftPluginId>-<suffixname>
        self.pvNameSpectY = self.buildPvName('Spectrum-Amp-Act') # "IOC_TEST:Plugin-FFT1-Spectrum-Amp-Act"
        print("self.pvNameSpectY=" + self.pvNameSpectY)
        self.pvNameSpectX = self.buildPvName('Spectrum-X-Axis-Act') # "IOC_TEST:Plugin-FFT1-Spectrum-X-Axis-Act"
        print("self.pvNameSpectX=" + self.pvNameSpectX)
        self.pvNameRawDataY = self.buildPvName('Raw-Data-Act') # IOC_TEST:Plugin-FFT0-Raw-Data-Act
        print("self.pvNameRawDataY=" + self.pvNameRawDataY)
        self.pvnNameEnable = self.buildPvName('Enable') # IOC_TEST:Plugin-FFT0-Enable
        print("self.pvnNameEnable=" + self.pvnNameEnable)
        self.pvnNameTrigg = self.buildPvName('Trigg') # IOC_TEST:Plugin-FFT0-Trigg
        print("self.pvnNameTrigg=" + self.pvnNameTrigg)
        self.pvnNameSource = self.buildPvName('Source') # IOC_TEST:Plugin-FFT0-Source
        print("self.pvnNameSource=" + self.pvnNameSource)
        self.pvnNameSampleRate = self.buildPvName('SampleRate-Act') # IOC_TEST:Plugin-FFT0-SampleRate-Act
        print("self.pvnNameSampleRate=" + self.pvnNameSampleRate)
        self.pvnNameNFFT = self.buildPvName('NFFT') # IOC_TEST:Plugin-FFT0-NFFT
        print("self.pvnNameNFFT=" + self.pvnNameNFFT)
        self.pvnNameMode = self.buildPvName('Mode-RB') # IOC_TEST:Plugin-FFT0-Mode-RB
        print("self.pvnNameMode=" + self.pvnNameMode)

        self.connectPvs()
        
        # Check actual value of pvs
        if(self.pvEnable.get()>0):
          self.enableBtn.setStyleSheet("background-color: green")
          self.enable = True
        else:
          self.enableBtn.setStyleSheet("background-color: red")
          self.enable = False

        self.sourceStr = self.pvSource.get(as_string=True)
        self.sampleRate = self.pvSampleRate.get()
        self.NFFT = self.pvNFFT.get()
        
        self.mode = self.pvMode.get()        
        self.modeStr = "NO_MODE"
        self.triggBtn.setEnabled(False) # Only enable if mode = TRIGG = 2
        if self.mode == 1:
            self.modeStr = "CONT"           
        if self.mode == 2:
           self.modeStr = "TRIGG"
           self.triggBtn.setEnabled(True)
        
        # Fix layout
        self.setGeometry(300, 300, 900, 700)
        self.setWindowTitle("ecmc FFT Main plot: prefix=" + self.pvPrefixStr + " , fftId=" + str(self.fftPluginId) + 
                            ", source="  + self.sourceStr + ", rate=" + str(self.sampleRate) + 
                            ", nfft=" + str(self.NFFT) + ", mode=" + self.modeStr)
        layoutVert = QVBoxLayout()
        layoutVert.addWidget(self.toolbar) 
        layoutVert.addWidget(self.canvas) 

        layoutControl = QHBoxLayout() 
        layoutControl.addWidget(self.pauseBtn)
        layoutControl.addWidget(self.enableBtn)
        layoutControl.addWidget(self.triggBtn)
    
        frameControl = QFrame(self)
        frameControl.setFixedHeight(70)
        frameControl.setLayout(layoutControl)

        layoutVert.addWidget(frameControl) 

        self.setLayout(layoutVert)                
        return

    def buildPvName(self, suffixname):
        return self.pvPrefixStr + 'Plugin-FFT' + str(self.fftPluginId) + '-' + suffixname 

    def connectPvs(self):        

        if self.pvNameSpectX is None:
            raise RuntimeError("pvname X spect must not be 'None'")
        if len(self.pvNameSpectX)==0:
            raise RuntimeError("pvname  X spect must not be ''")

        if self.pvNameSpectY is None:
            raise RuntimeError("pvname y spect must not be 'None'")
        if len(self.pvNameSpectY)==0:
            raise RuntimeError("pvname  y spect must not be ''")

        if self.pvNameRawDataY is None:
            raise RuntimeError("pvname raw data must not be 'None'")
        if len(self.pvNameRawDataY)==0:
            raise RuntimeError("pvname raw data must not be ''")

        if self.pvnNameEnable is None:
            raise RuntimeError("pvname enable must not be 'None'")
        if len(self.pvnNameEnable)==0:
            raise RuntimeError("pvname enable must not be ''")

        if self.pvnNameTrigg is None:
            raise RuntimeError("pvname trigg must not be 'None'")
        if len(self.pvnNameTrigg)==0:
            raise RuntimeError("pvname trigg must not be ''")

        if self.pvnNameSource is None:
            raise RuntimeError("pvname source must not be 'None'")
        if len(self.pvnNameSource)==0:
            raise RuntimeError("pvname source must not be ''")

        if self.pvnNameSampleRate is None:
            raise RuntimeError("pvname sample rate must not be 'None'")
        if len(self.pvnNameSampleRate)==0:
            raise RuntimeError("pvname sample rate must not be ''")
        
        if self.pvnNameNFFT is None:
            raise RuntimeError("pvname NFFT must not be 'None'")
        if len(self.pvnNameNFFT)==0:
            raise RuntimeError("pvname NFFT must not be ''")
        
        if self.pvnNameMode is None:
            raise RuntimeError("pvname mode must not be 'None'")
        if len(self.pvnNameMode)==0:
            raise RuntimeError("pvname mode must not be ''")
        
        self.pvSpectX = epics.PV(self.pvNameSpectX)
        #print('self.pvSpectX: ' + self.pvSpectX.info)

        self.pvSpectY = epics.PV(self.pvNameSpectY)
        #print('self.pvSpectY: ' + self.pvSpectY.info)

        self.pvRawData = epics.PV(self.pvNameRawDataY)
        #print('self.pvRawData: ' + self.pvSpectY.info)

        self.pvEnable = epics.PV(self.pvnNameEnable)
        #print('self.pvEnable: ' + self.pvEnable.info)
        
        self.pvTrigg = epics.PV(self.pvnNameTrigg)
        #print('self.pvTrigg: ' + self.pvTrigg.info)

        self.pvSource = epics.PV(self.pvnNameSource)
        #print('self.pvSource: ' + self.pvSource.info)

        self.pvSampleRate = epics.PV(self.pvnNameSampleRate)
        #print('self.pvSampleRate: ' + self.pvSampleRate.info)

        self.pvNFFT = epics.PV(self.pvnNameNFFT)
        #print('self.pvNFFT: ' + self.pvNFFT.info)

        self.pvMode = epics.PV(self.pvnNameMode)
        #print('self.pvMode: ' + self.pvMode.info)        

        self.pvSpectX.add_callback(self.onChangePvSpectX)
        self.pvSpectY.add_callback(self.onChangePvSpectY)
        self.pvRawData.add_callback(self.onChangePvrawData)

        QCoreApplication.processEvents()
    
    def onChangePvSpectX(self,pvname=None, value=None, char_value=None,timestamp=None, **kw):
        self.comSignalSpectX.data_signal.emit(value)

    def onChangePvSpectY(self,pvname=None, value=None, char_value=None,timestamp=None, **kw):
        self.comSignalSpectY.data_signal.emit(value)        

    def onChangePvrawData(self,pvname=None, value=None, char_value=None,timestamp=None, **kw):
        self.comSignalRawData.data_signal.emit(value)        

    def pauseBtnAction(self):   
        self.pause = not self.pause
        if self.pause:
            self.pauseBtn.setStyleSheet("background-color: red")
        else:
            self.pauseBtn.setStyleSheet("background-color: green")
            # Retrigger plots with newest values
            self.comSignalSpectY.data_signal.emit(self.spectY)
            self.comSignalRawData.data_signal.emit(self.rawdataY)
        return

    def enableBtnAction(self):
        self.enable = not self.enable
        self.pvEnable.put(self.enable)
        if self.enable:
          self.enableBtn.setStyleSheet("background-color: green")
        else:
          self.enableBtn.setStyleSheet("background-color: red")
        return

    def triggBtnAction(self):
        self.pvTrigg.put(True)
        return        

    def callbackFuncSpectX(self, value):
        if(np.size(value)) > 0:
            self.spectX = value
            self.xDataValid = 1
        return

    def callbackFuncSpectY(self, value):
        if(np.size(value)) > 0:
            self.spectY = value
            self.plotSpect()            
        return

    def callbackFuncrawData(self, value):
        if(np.size(value)) > 0:
            if self.rawdataX is None or np.size(value) != np.size(self.rawdataY):
                self.rawdataX = np.arange(-np.size(value)/self.sampleRate, 0, 1/self.sampleRate)

            self.rawdataY = value
            # print("Size X,Y: " + str(np.size(self.rawdataX))+ ", " +str(np.size(self.rawdataY)))
            self.plotRaw()
        return

    def plotSpect(self):
        if self.pause:            
            return
        if self.spectX is None:
            return
        if self.spectY is None:
            return
        
        # print("plotSpect")
        # create an axis for spectrum
        if self.axSpect is None:
           self.axSpect = self.figure.add_subplot(212)

        # plot data 
        if self.plottedLineSpect is not None:
            self.plottedLineSpect.remove()

        self.plottedLineSpect, = self.axSpect.plot(self.spectX,self.spectY, 'b*-') 
        self.axSpect.grid(True)

        self.axSpect.set_xlabel(self.pvNameSpectX +' [' + self.pvSpectX.units + ']')
        self.axSpect.set_ylabel(self.pvNameSpectY +' [' + self.pvSpectY.units + ']')

        # refresh canvas 
        self.canvas.draw()

        self.axSpect.autoscale(enable=False)

    def plotRaw(self):
        if self.pause:            
            return
        if self.rawdataY is None:
            return
        
        # create an axis for spectrum
        if self.axRaw is None:
           self.axRaw = self.figure.add_subplot(211)

        # plot data 
        if self.plottedLineRaw is not None:
            self.plottedLineRaw.remove()

        self.plottedLineRaw, = self.axRaw.plot(self.rawdataX,self.rawdataY, 'b*-') 
        self.axRaw.grid(True)

        self.axRaw.set_xlabel('Time [s]')
        self.axRaw.set_ylabel(self.pvNameRawDataY +' [' + self.pvRawData.units + ']') 

        # refresh canvas 
        self.canvas.draw()

        self.axRaw.autoscale(enable=True)

def printOutHelp():
  print("ecmcFFTMainGui: Plots waveforms of FFT data (updates on Y data callback). ")
  print("python ecmcFFTMainGui.py <prefix> <fftId>")
  print("<prefix>:  Ioc prefix ('IOC_TEST:')")
  print("<fftId> :  Id of fft plugin ('0')")
  print("example : python ecmcFFTMainGui.py 'IOC_TEST:' '0'")
  print("Will connect to Pvs: <prefix>Plugin-FFT<fftId>-*")

if __name__ == "__main__":
    import sys    
    if len(sys.argv)!=3:
        printOutHelp()
        sys.exit()
    prefix=sys.argv[1]
    fftid=int(sys.argv[2])
    app = QtWidgets.QApplication(sys.argv)
    window=ecmcFFTMainGui(prefix=prefix,fftPluginId=fftid)
    window.show()
    sys.exit(app.exec_())
