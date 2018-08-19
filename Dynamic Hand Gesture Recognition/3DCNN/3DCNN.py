# -*- coding: utf-8 -*-
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

import os
import numpy as np
import math
from PIL import Image
from keras import layers
from keras.models import Sequential
from keras.layers import Dense, Dropout
from keras.layers import Conv3D, MaxPooling3D
from keras.layers import Activation, Dropout, Flatten, Dense
from keras.optimizers import SGD, Adadelta, Adagrad
from keras.utils.np_utils import to_categorical
from keras import initializers
import h5py

from sklearn.model_selection import train_test_split, KFold, cross_val_score
from sklearn.preprocessing import LabelEncoder
from keras.callbacks import LearningRateScheduler

# 读入图片
img_width = 80
img_height = 60
test_video_num = 180

HR_input_shape = (img_height, img_width, 50, 2)
LR_input_shape = (img_height/2, img_width/2, 50, 2)

HR_X_test = np.empty((test_video_num, img_height, img_width, 50, 2), dtype="float32")
LR_X_test = np.empty((test_video_num, img_height/2, img_width/2, 50, 2), dtype="float32")
target_test = np.empty(test_video_num)

video_no = 0
frame_no = 0

path = "SKIG_data/test/gradient" #文件夹目录  
dirs = os.listdir(path) #得到文件夹下的所有文件名称   
for dir in dirs: #遍历文件夹
	if dir[0] == '.':
		pass
	else:
		videos = os.listdir(path + '/' + dir)
		for video in videos:
			if video[0] == '.':
				pass
			else:
				target_test[video_no] = int(dir) - 1
				frame_no = 0
				frames = os.listdir(path + '/' + dir + '/' + video)
				for frame in frames:
					img = Image.open(path + '/' + dir + '/' + video + '/' + frame)
					img = img.convert("L")
					HR_img = img.resize((img_width, img_height))
					LR_img = img.resize((img_width/2, img_height/2))
					global arr
					HR_arr = np.array(HR_img, dtype="float32")
					LR_arr = np.array(LR_img, dtype="float32")
					if frame_no < 50:
						HR_X_test[video_no, :, :, frame_no, 0] = HR_arr
						LR_X_test[video_no, :, :, frame_no, 0] = LR_arr
						frame_no = frame_no + 1

				while frame_no < 50:
					HR_X_test[video_no, :, :, frame_no, 0] = HR_arr
					LR_X_test[video_no, :, :, frame_no, 0] = LR_arr
					frame_no = frame_no + 1
				video_no = video_no + 1

video_no = 0
frame_no = 0

path = "SKIG_data/test/depth" #文件夹目录  
dirs = os.listdir(path) #得到文件夹下的所有文件名称   
for dir in dirs: #遍历文件夹
	if dir[0] == '.':
		pass
	else:
		videos = os.listdir(path + '/' + dir)
		for video in videos:
			if video[0] == '.':
				pass
			else:
				frame_no = 0
				frames = os.listdir(path + '/' + dir + '/' + video)
				for frame in frames:
					if frame[0] == '.':
						pass
					else:
						img = Image.open(path + '/' + dir + '/' + video + '/' + frame)
						img = img.convert("L")
						HR_img = img.resize((img_width, img_height))
						LR_img = img.resize((img_width/2, img_height/2))
						global arr
						HR_arr = np.array(HR_img, dtype="float32")
						LR_arr = np.array(LR_img, dtype="float32")
						if frame_no < 50:
							HR_X_test[video_no, :, :, frame_no, 1] = HR_arr
							LR_X_test[video_no, :, :, frame_no, 1] = LR_arr
							frame_no = frame_no + 1

				while frame_no < 50:
					HR_X_test[video_no, :, :, frame_no, 1] = HR_arr
					LR_X_test[video_no, :, :, frame_no, 1] = LR_arr
					frame_no = frame_no + 1
				video_no = video_no + 1

# change the label into one-hot vector
Y_test = to_categorical(target_test, 10)

# The network structure
# High-resolution network
HR_model = Sequential()

# 3D convolution and max-pooling
HR_model.add(Conv3D(4, (7, 7, 5), padding='valid', input_shape=HR_input_shape, kernel_initializer='random_uniform', bias_initializer='ones'))
HR_model.add(Activation('relu'))   
HR_model.add(MaxPooling3D(pool_size=(2, 2, 2)))

# 3D convolution and max-pooling
HR_model.add(Conv3D(8, (5, 5, 3), padding='valid', kernel_initializer='random_uniform', bias_initializer='ones'))  
HR_model.add(Activation('relu'))  
HR_model.add(MaxPooling3D(pool_size=(2, 2, 2)))  
  
# 3D convolution and max-pooling
HR_model.add(Conv3D(32, (5, 5, 3), padding='valid', kernel_initializer='random_uniform', bias_initializer='ones'))  
HR_model.add(Activation('relu'))  
HR_model.add(MaxPooling3D(pool_size=(1, 2, 2)))  
  
# 3D convolution and max-pooling
HR_model.add(Conv3D(64, (3, 3, 3), padding='valid', kernel_initializer='random_uniform', bias_initializer='ones'))
HR_model.add(Activation('relu'))
HR_model.add(MaxPooling3D(pool_size=(2, 2, 1)))

HR_model.add(Flatten()) 

# FCL 1
HR_model.add(Dense(512, kernel_initializer=initializers.random_normal(stddev=0.01), bias_initializer='ones'))  
HR_model.add(Activation('relu'))  
HR_model.add(Dropout(0.25))

# FCL 2
HR_model.add(Dense(256, kernel_initializer=initializers.random_normal(stddev=0.01), bias_initializer='ones'))
HR_model.add(Activation('relu'))
HR_model.add(Dropout(0.25))

# SM 1
HR_model.add(Dense(10, kernel_initializer=initializers.random_normal(stddev=0.01), bias_initializer='zeros'))  
HR_model.add(Activation('softmax'))


# Low-resolution network
LR_model = Sequential()

# 3D convolution and max-pooling
LR_model.add(Conv3D(8, (5, 5, 5), padding='valid', input_shape=LR_input_shape, kernel_initializer='random_uniform', bias_initializer='ones'))  
LR_model.add(Activation('relu'))  
LR_model.add(MaxPooling3D(pool_size=(2, 2, 2)))  
  
# 3D convolution and max-pooling
LR_model.add(Conv3D(32, (5, 5, 3), padding='valid', kernel_initializer='random_uniform', bias_initializer='ones'))  
LR_model.add(Activation('relu'))  
LR_model.add(MaxPooling3D(pool_size=(2, 2, 2)))  
  
# 3D convolution and max-pooling
LR_model.add(Conv3D(64, (3, 3, 3), padding='valid', kernel_initializer='random_uniform', bias_initializer='ones'))
LR_model.add(Activation('relu'))
LR_model.add(MaxPooling3D(pool_size=(1, 2, 2)))

LR_model.add(Flatten()) 

# FCL 3
LR_model.add(Dense(512, kernel_initializer=initializers.random_normal(stddev=0.01), bias_initializer='ones'))  
LR_model.add(Activation('relu'))  
LR_model.add(Dropout(0.5))

# FCL 4
LR_model.add(Dense(256, kernel_initializer=initializers.random_normal(stddev=0.01), bias_initializer='ones'))
LR_model.add(Activation('relu'))
LR_model.add(Dropout(0.5))

# SM 2
LR_model.add(Dense(10, kernel_initializer=initializers.random_normal(stddev=0.01), bias_initializer='zeros'))  
LR_model.add(Activation('softmax'))


HR_model.load_weights('50_epochs_HR.h5')
LR_model.load_weights('50_epochs_LR.h5')


# predict samples and multiply the probabilities
H = HR_model.predict_proba(HR_X_test)
L = LR_model.predict_proba(LR_X_test)

proba = np.empty(10)
Label = np.empty(len(Y_test))
for i in range(len(Y_test)):
	max_proba = 0
	index = 0
	for j in range(10):
		proba[j] = H[i][j] * L[i][j]
		if max_proba < proba[j]:
			max_proba = proba[j]
			index = j
	Label[i] = index

# accuracy
print "test accuracy", np.mean(target_test == Label)






 
 
