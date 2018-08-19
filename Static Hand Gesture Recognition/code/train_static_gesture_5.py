# -*- coding: utf-8 -*-

import cv2
import os
import sys
import math
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

# 配置utf-8输出环境
try:
    reload(sys)
    sys.setdefaultencoding('utf-8')
except:
    pass

# 读入数据
train = datasets.load_files("data_5")
# 返回值：Bunch Dictionary-like object.主要属性有
#   data:原始数据；
#   filenames:每个文件的名字；
#   target:类别标签（从0开始的整数索引）；
#   target_names:类别标签的具体含义（由子文件夹的名字`category_1_folder`等决定）

# 读入图片
# dimensions of our images.
img_width, img_height = 64, 64
input_shape = (img_width, img_height, 1)

l = len(train.filenames)
X = np.empty((l, img_width, img_height, 1), dtype="float32")  
for i in range(l):
    print 'reading image: ', i, train.filenames[i]
    img = Image.open(train.filenames[i])
    img = img.resize((img_width, img_height))
    arr = np.asarray(img, dtype="float32")
    X[i,:,:,0] = arr

# convert integers to dummy variables (one hot encoding)
dummy_y = np_utils.to_categorical(train.target, 5)

# define model structure
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


# compile the model
sgd = SGD(lr=0.005, decay=1e-6, momentum=0.9, nesterov=True)  
model.compile(loss='categorical_crossentropy', optimizer=sgd, metrics=['accuracy']) 


# splitting data into training set and test set. If random_state is set to an integer, the split datasets are fixed.
X_train, X_test, Y_train, Y_test = train_test_split(X, dummy_y, test_size=0.2, random_state=32)
print len(X_train),len(X_test)
nb_train_samples = len(X_train)
nb_validation_samples = len(X_test)

# 数据提升
# this is the augmentation configuration we will use for training
train_datagen = ImageDataGenerator(
    rescale=1. / 255,
    shear_range=0.2,
    zoom_range=0.2)

# this is the augmentation configuration we will use for testing:
# only rescaling
test_datagen = ImageDataGenerator(rescale=1. / 255)

batch_size = 32
train_generator = train_datagen.flow(X_train, Y_train, batch_size=batch_size)

validation_generator = test_datagen.flow(X_test, Y_test, batch_size=batch_size)

result = model.fit_generator(
    train_generator,
    steps_per_epoch=nb_train_samples // batch_size,
    epochs=20,
    validation_data=validation_generator,
    validation_steps=nb_validation_samples // batch_size)

model.save_weights('static_gesture_5.h5')

fig=plt.figure(figsize=(8,8))
plt.plot(result.epoch,result.history['acc'],label="acc")
plt.plot(result.epoch,result.history['val_acc'],label="val_acc")
plt.xlabel('num of Epochs')
plt.ylabel('accuracy')
plt.scatter(result.epoch,result.history['acc'],marker='*')
plt.scatter(result.epoch,result.history['val_acc'],marker='*')
plt.legend(loc='lower right')
fig.savefig('static_gesture_5_acc.jpg')  

fig=plt.figure(figsize=(8,8))
plt.plot(result.epoch,result.history['loss'],label="loss")
plt.plot(result.epoch,result.history['val_loss'],label="val_loss")
plt.xlabel('num of Epochs')
plt.ylabel('loss')
plt.scatter(result.epoch,result.history['loss'],marker='*')
plt.scatter(result.epoch,result.history['val_loss'],marker='*')
plt.legend(loc='upper right')
fig.savefig('static_gesture_5_loss.jpg')

