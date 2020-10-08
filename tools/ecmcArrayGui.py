#*************************************************************************
# Copyright (c) 2020 European Spallation Source ERIC
# ecmc is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution. 
#
#   ecmcArrayGui.py
#
#  Created on: October 8, 2020
#      Author: Anders Sandström
#    
#  Plots two waveforms (y vs time) updates for each callback on the y-pv
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

class comSignal(QObject):
    data_signal = pyqtSignal(object)

class ecmcArrayGui(QtWidgets.QDialog):
    def __init__(self,yname=None):        
        super(ecmcArrayGui, self).__init__()        
        self.comSignalY = comSignal()        
        self.comSignalY.data_signal.connect(self.callbackFuncY)
        self.pause = 0
        self.spectY = None
        self.figure = plt.figure()                            
        self.plotted_line = None
        self.ax = None
        self.canvas = FigureCanvas(self.figure)   
        self.toolbar = NavigationToolbar(self.canvas, self) 
        self.pauseBtn = QPushButton(text = 'pause')
        self.pauseBtn.setFixedSize(100, 50)
        self.pauseBtn.clicked.connect(self.pauseBtnAction)        
        self.pauseBtn.setStyleSheet("background-color: green")
        self.pvNameY = yname # "IOC_TEST:Plugin-FFT0-Raw-Data-Act"        
        self.connectPvs() # Epics
        self.setGeometry(300, 300, 900, 700)
        self.setWindowTitle("ecmc Array plot: " + self.pvNameY)
        layout = QVBoxLayout() 
        layout.addWidget(self.toolbar) 
        layout.addWidget(self.canvas) 
        layout.addWidget(self.pauseBtn) 
        self.setLayout(layout)                
        return

    def connectPvs(self):        
        if self.pvNameY is None:            
            raise RuntimeError("pvname y must not be 'None'")
        if len(self.pvNameY)==0:
            raise RuntimeError("pvname  y must not be ''")

        self.pvY = epics.PV(self.pvNameY)        
        #print('self.pvY: ' + self.pvY.info)
        
        self.pvY.add_callback(self.onChangePvY)        
        QCoreApplication.processEvents()
        
    def onChangePvY(self,pvname=None, value=None, char_value=None,timestamp=None, **kw):
        self.comSignalY.data_signal.emit(value)        

    def pauseBtnAction(self):        
        self.pause = not self.pause
        if self.pause:
            self.pauseBtn.setStyleSheet("background-color: red");
        else:
            self.pauseBtn.setStyleSheet("background-color: green");
            self.comSignalY.data_signal.emit(self.spectY)
        return

    def callbackFuncY(self, value):
        if(np.size(value)) > 0:
            self.spectY = value
            self.plotSpect()
        return

    def plotSpect(self):
        if self.pause:            
            return

        if self.spectY is None:
            return
        
        # create an axis 
        if self.ax is None:
           self.ax = self.figure.add_subplot(111)
   
        # plot data 
        if self.plotted_line is not None:
            self.plotted_line.remove()

        self.plotted_line, = self.ax.plot(self.spectY, 'b*-') 
        self.ax.grid(True)

        plt.xlabel('Time []')
        plt.ylabel(self.pvNameY +' [' + self.pvY.units + ']')        
        # refresh canvas 
        self.canvas.draw()

        self.ax.autoscale(enable=True)

def printOutHelp():
  print("ecmcArrayGui: Plots waveforms data (updates on data callback). ")
  print("python ecmcArrayGui.py  <y.pv>")
  print("example: python ecmcArrayGui.py IOC_TEST:Plugin-FFT0-Raw-Data-Act")

if __name__ == "__main__":
    import sys    
    if len(sys.argv)!=2:
        printOutHelp()
        sys.exit()    
    yname=sys.argv[1]
    app = QtWidgets.QApplication(sys.argv)
    window=ecmcArrayGui(yname=yname)
    window.show()
    sys.exit(app.exec_())
