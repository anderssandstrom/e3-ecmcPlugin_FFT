#*************************************************************************
# Copyright (c) 2020 European Spallation Source ERIC
# ecmc is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution. 
#
#   ecmcFFTGui.py
#
#  Created on: October 6, 2020
#      Author: Anders Sandström
#    
#  Heavily inspired by: https://exceptionshub.com/real-time-plotting-in-while-loop-with-matplotlib.html
#
#  Extends the ecmcTrend class will epics pv callbacks
#  
#*************************************************************************

import sys
import os
import epics
from PyQt5.QtWidgets import *
from PyQt5 import QtWidgets
from PyQt5.QtCore import *
from PyQt5.QtGui import *
import functools
import numpy as np
import random as rd
import matplotlib
matplotlib.use("Qt5Agg")
from matplotlib.figure import Figure
from matplotlib.animation import TimedAnimation
from matplotlib.lines import Line2D
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
import matplotlib.pyplot as plt 

import time
import threading


class comSignal(QObject):
    data_signal = pyqtSignal(object)

class ecmcFFTGui(QtWidgets.QDialog):
    def __init__(self,pvName=None):        
        super(ecmcFFTGui, self).__init__()        
        self.comSignalX = comSignal()        
        self.comSignalX.data_signal.connect(self.callbackFuncX)
        self.comSignalY = comSignal()        
        self.comSignalY.data_signal.connect(self.callbackFuncY)
        self.pause = 0
        self.spectX = np.zeros(0)
        self.spectY = np.zeros(0)        
        self.figure = plt.figure()            
        # this is the Canvas Widget that  
        # displays the 'figure'it takes the 
        # 'figure' instance as a parameter to __init__ 
        self.canvas = FigureCanvas(self.figure) 
   
        # this is the Navigation widget 
        # it takes the Canvas widget and a parent 
        self.toolbar = NavigationToolbar(self.canvas, self) 

        # Pause
        self.pauseBtn = QPushButton(text = 'pause')
        self.pauseBtn.setFixedSize(100, 50)
        self.pauseBtn.clicked.connect(self.pauseBtnAction)        
        self.pauseBtn.setStyleSheet("background-color: green");

        self.startval = 0
        self.pvNameY = "IOC_TEST:Plugin-FFT1-Spectrum-Amp-Act"
        self.pvNameX = "IOC_TEST:Plugin-FFT1-Spectrum-X-Axis-Act"
        self.connectPvs() # Epics
        self.setTitle()

        # Define the geometry of the main window
        self.setGeometry(300, 300, 900, 700)
        self.setWindowTitle("ecmc plot")

        # creating a Vertical Box layout 
        layout = QVBoxLayout() 
           
        # adding tool bar to the layout 
        layout.addWidget(self.toolbar) 
           
        # adding canvas to the layout 
        layout.addWidget(self.canvas) 
           
        # adding push button to the layout 
        layout.addWidget(self.pauseBtn) 
           
        # setting layout to the main window 
        self.setLayout(layout)                
        return

    def connectPvs(self):        

        if self.pvNameX is None:            
            raise RuntimeError("pvname X must not be 'None'")
        if len(self.pvNameX)==0:
            raise RuntimeError("pvname  X must not be ''")

        if self.pvNameY is None:            
            raise RuntimeError("pvname y must not be 'None'")
        if len(self.pvNameY)==0:
            raise RuntimeError("pvname  y must not be ''")

        self.pvX = epics.PV(self.pvNameX)

        self.pvY = epics.PV(self.pvNameY)
        
        self.pvX.add_callback(self.onChangePvX)
        self.pvY.add_callback(self.onChangePvY)
        #self.myFig.addData(self.startval)        
        QCoreApplication.processEvents()
    
    def onChangePvX(self,pvname=None, value=None, char_value=None,timestamp=None, **kw):
        #print ('PV X Changed! ', pvname, char_value, time.ctime())
        self.comSignalX.data_signal.emit(value)

    def onChangePvY(self,pvname=None, value=None, char_value=None,timestamp=None, **kw):
        #print ('PV Y Changed! ', pvname, char_value, time.ctime())
        self.comSignalY.data_signal.emit(value)        

    def pauseBtnAction(self):        
        self.pause = not self.pause
        if self.pause:
            self.pauseBtn.setStyleSheet("background-color: red");
        else:
            self.pauseBtn.setStyleSheet("background-color: green");

        return


    def callbackFuncX(self, value):
        #print ('Callback X: ' + str(np.size(value)) )# + value )
        if(np.size(value)) > 0:
            self.spectX = value
        return

    def callbackFuncY(self, value):
        #print ('Callback y: ')# + value )
        if(np.size(value)) > 0:
            self.spectY = value
            self.plotSpect()
        return

    def setYLabel(self,label):
        #self.myFig.setYLabel(label)
        return

    def setTitle(self):
        #self.myFig.setTitle(label)
        self.setWindowTitle("ecmc plot: " + self.pvNameY + ' vs ' + self.pvNameX)
        return

    def plotSpect(self):
        if self.pause:
            print('paused!!!!')
            return

        print('plotSpect!!!!')
        
        # clearing old figure 
        self.figure.clear() 
   
        # create an axis 
        ax = self.figure.add_subplot(111)        

        # plot data 
        ax.plot(self.spectX,self.spectY , '*-') 
        ax.grid()
        # refresh canvas 
        self.canvas.draw() 


if __name__ == "__main__":
    import sys
    app = QtWidgets.QApplication(sys.argv)
    window=ecmcFFTGui();
    window.show()
    sys.exit(app.exec_())
