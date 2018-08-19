#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import cv2
import os
import sys
import math
import json
import numpy as np
from PIL import Image
from keras.utils import np_utils
from keras.models import Sequential
from keras.layers import Conv2D, MaxPooling2D
from keras.layers import Activation, Dropout, Flatten, Dense
from keras.optimizers import SGD, Adadelta, Adagrad
from keras.wrappers.scikit_learn import KerasClassifier
from keras import initializers
from keras.callbacks import LearningRateScheduler

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

from sklearn import datasets
from sklearn import metrics
from sklearn.preprocessing import LabelEncoder
from sklearn.model_selection import train_test_split, KFold, cross_val_score
from keras.preprocessing.image import ImageDataGenerator, array_to_img, img_to_array, load_img


# outputs
output = ["A", "C", "D", "U", "V"]

img_width, img_height = 64, 64
input_shape = (img_width, img_height, 1)

# Load CNN model
def loadCNN():
    
    model = Sequential()
  
    # 第一个卷积层，4个卷积核，每个卷积核大小3*3。  
    model.add(Conv2D(4, (3, 3), padding='valid', input_shape=input_shape))
    model.add(Activation('relu'))   
    model.add(MaxPooling2D(pool_size=(2, 2)))

    # 第二个卷积层，8个卷积核，每个卷积核大小3*3。
    model.add(Conv2D(8, (3, 3), padding='valid'))  
    model.add(Activation('relu'))  
    model.add(MaxPooling2D(pool_size=(2, 2)))  
      
    # 第三个卷积层，16个卷积核，每个卷积核大小3*3   
    model.add(Conv2D(16, (3, 3), padding='valid'))  
    model.add(Activation('relu'))  
    model.add(MaxPooling2D(pool_size=(2, 2)))  
      
    # 全连接层 
    model.add(Flatten())  
    model.add(Dense(128, kernel_initializer='normal'))  
    model.add(Activation('relu'))  
    model.add(Dropout(0.5))

    # Softmax分类，输出是5类别  
    model.add(Dense(5, kernel_initializer='normal'))  
    model.add(Activation('softmax'))

    model.load_weights('static_gesture_5.h5')
    
    return model


# This function does the guessing work based on input images
def guessGesture(model, img):
    global output, get_output

    img = cv2.resize(img, (img_width,img_height))
    arr = np.asarray(img, dtype="float32")
    
    x = np.empty((1, img_width, img_height, 1), dtype="float32")
    x[0,:,:,0] = arr
    index = model.predict_classes(x,verbose = 0)
    prob_array = model.predict_proba(x,verbose = 0)
    
    #print prob_array
    
    d = {}
    i = 0
    for items in output:
        d[items] = prob_array[0][i] * 100
        i += 1
    
    # Get the output with maximum probability
    import operator
    
    guess = max(d.iteritems(), key=operator.itemgetter(1))[0]
    prob  = d[guess]

    if prob > 70.0:
        #print guess + "  Probability: ", prob

        #Enable this to save the predictions in a json file,
        #Which can be read by plotter app to plot bar graph
        #dump to the JSON contents to the file
        
        with open('gesturejson.txt', 'w') as outfile:
            json.dump(d, outfile)

        return output.index(guess)

    else:
        return -1

