# -*- coding: utf-8 -*-
import matplotlib
matplotlib.use('Agg')
import os
import numpy as np
import math
from PIL import Image
from keras.models import Sequential
from keras.layers import Dense, Dropout
from keras.layers import Conv3D, MaxPooling3D
from keras.layers import Activation, Dropout, Flatten, Dense
from keras.optimizers import SGD, Adadelta, Adagrad
from keras.utils.np_utils import to_categorical
from keras import initializers

from sklearn.model_selection import train_test_split, KFold, cross_val_score
from sklearn.preprocessing import LabelEncoder
from keras.callbacks import LearningRateScheduler

import matplotlib.pyplot as plt

# learning rate schedule
def step_decay(epoch):
	initial_lrate = 0.001
	drop = 0.9
	epochs_drop = 10.0
	lrate = initial_lrate * math.pow(drop, math.floor((1+epoch)/epochs_drop))
	return lrate

# 读入图片
img_width = 80
img_height = 60
train_video_num = 900
test_video_num = 180

input_shape = (img_height, img_width, 50, 2)

X_train = np.empty((train_video_num, img_height, img_width, 50, 2), dtype="float32")
X_test = np.empty((test_video_num, img_height, img_width, 50, 2), dtype="float32")
target_train = np.empty(train_video_num)
target_test = np.empty(test_video_num)

video_no = 0
frame_no = 0

path = "SKIG_data/train/gradient" #文件夹目录  
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
				target_train[video_no] = int(dir) - 1
				frame_no = 0
				frames = os.listdir(path + '/' + dir + '/' + video)
				for frame in frames:
					img = Image.open(path + '/' + dir + '/' + video + '/' + frame)
					img = img.resize((img_width, img_height))
					img = img.convert("L")
					global arr
					arr = np.array(img, dtype="float32")
					if frame_no < 50:
						X_train[video_no, :, :, frame_no, 0] = arr
						frame_no = frame_no + 1

				while frame_no < 50:
					X_train[video_no, :, :, frame_no, 0] = arr
					frame_no = frame_no + 1
				video_no = video_no + 1

video_no = 0
frame_no = 0

path = "SKIG_data/train/depth" #文件夹目录  
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
					img = Image.open(path + '/' + dir + '/' + video + '/' + frame)
					img = img.resize((img_width, img_height))
					img = img.convert("L")
					arr = np.array(img, dtype="float32")
					if frame_no < 50:
						X_train[video_no, :, :, frame_no, 1] = arr
						frame_no = frame_no + 1

				while frame_no < 50:
					X_train[video_no, :, :, frame_no, 1] = arr
					frame_no = frame_no + 1
				video_no = video_no + 1

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
					img = img.resize((img_width, img_height))
					img = img.convert("L")
					arr = np.array(img, dtype="float32")
					if frame_no < 50:
						X_test[video_no, :, :, frame_no, 0] = arr
						frame_no = frame_no + 1

				while frame_no < 50:
					X_test[video_no, :, :, frame_no, 0] = arr
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
						img = img.resize((img_width, img_height))
						img = img.convert("L")
						arr = np.array(img, dtype="float32")
						if frame_no < 50:
							X_test[video_no, :, :, frame_no, 1] = arr
							frame_no = frame_no + 1

				while frame_no < 50:
					X_test[video_no, :, :, frame_no, 1] = arr
					frame_no = frame_no + 1
				video_no = video_no + 1

# change the label into one-hot vector
Y_train = to_categorical(target_train, 10)
Y_test = to_categorical(target_test, 10)

# The network structure
# High-resolution network
HR_model = Sequential()

# 3D convolution and max-pooling
HR_model.add(Conv3D(4, (7, 7, 5), padding='valid', input_shape=input_shape, kernel_initializer='random_uniform', bias_initializer='ones'))
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

# compile the model
sgd = SGD(lr=0.0, decay=0.0, momentum=0.9, nesterov=True)
HR_model.compile(loss='categorical_crossentropy', optimizer=sgd, metrics=['accuracy'])

# learning schedule callback
lrate = LearningRateScheduler(step_decay)
callbacks_list = [lrate]

# train the models
result = HR_model.fit(X_train, Y_train, batch_size=20, epochs=50, verbose=1, callbacks=callbacks_list, validation_split=0.0, validation_data=(X_test, Y_test), shuffle=True, class_weight=None, sample_weight=None, initial_epoch=0)
# predict samples
H = HR_model.evaluate(X_test, Y_test)
print('Test loss:', H[0])
print('Test accuracy:', H[1])
HR_model.save_weights('50_epochs_HR.h5')

# plot the result
fig=plt.figure(figsize=(8,8))
plt.plot(result.epoch,result.history['acc'],label="acc")
plt.plot(result.epoch,result.history['val_acc'],label="val_acc")
plt.xlabel('num of Epochs')
plt.ylabel('accuracy')
plt.scatter(result.epoch,result.history['acc'],marker='*')
plt.scatter(result.epoch,result.history['val_acc'],marker='*')
plt.legend(loc='lower right')
fig.savefig('HR_acc.jpg')  

fig=plt.figure(figsize=(8,8))
plt.plot(result.epoch,result.history['loss'],label="loss")
plt.plot(result.epoch,result.history['val_loss'],label="val_loss")
plt.xlabel('num of Epochs')
plt.ylabel('loss')
plt.scatter(result.epoch,result.history['loss'],marker='*')
plt.scatter(result.epoch,result.history['val_loss'],marker='*')
plt.legend(loc='upper right')
fig.savefig('HR_loss.jpg') 





