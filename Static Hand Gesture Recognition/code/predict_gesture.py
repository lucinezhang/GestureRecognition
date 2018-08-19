# -*- coding: utf-8 -*-

import cv2
import numpy as np
import time
import os
import gestureCNN as myNN

x0 = 400
y0 = 200
height = 200
width = 200
lastgesture = -1
img_width, img_height = 64, 64

skinkernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(5,5))


def skinMask(frame, x0, y0, width, height ):
	global mod,lastgesture
	# HSV values
	low_range = np.array([0, 50, 80])
	upper_range = np.array([30, 200, 255])
	
	cv2.rectangle(frame, (x0,y0),(x0+width,y0+height),(0,255,0),1)
	roi = frame[y0:y0+height, x0:x0+width]
	
	hsv = cv2.cvtColor(roi, cv2.COLOR_BGR2HSV)
	
	#Apply skin color range
	mask = cv2.inRange(hsv, low_range, upper_range)
	
	mask = cv2.erode(mask, skinkernel, iterations = 1)
	mask = cv2.dilate(mask, skinkernel, iterations = 1)
	
	#blur
	mask = cv2.GaussianBlur(mask, (15,15), 1)
	#cv2.imshow("Blur", mask)
	
	#bitwise and mask original frame
	res = cv2.bitwise_and(roi, roi, mask = mask)
	# color to grayscale
	res = cv2.cvtColor(res, cv2.COLOR_BGR2GRAY)

	retgesture = myNN.guessGesture(mod, res)
	if lastgesture != retgesture:
		lastgesture = retgesture

		if lastgesture == 4:
			jump = ''' osascript -e 'tell application "System Events" to key code 49' '''
			#jump = ''' osascript -e 'tell application "System Events" to key down (49)' '''
			os.system(jump)
			print myNN.output[lastgesture] + "= Dino JUMP!"

		if lastgesture == -1:
			print "Nothing"
		else:
			print myNN.output[lastgesture]

		time.sleep(0.01)

	return res, lastgesture


if __name__ == "__main__":
	cap = cv2.VideoCapture(0)
	cv2.namedWindow('Original', cv2.WINDOW_NORMAL)

	# set rt size as 640x480
	ret = cap.set(3,640)
	ret = cap.set(4,480)

	print "Loading default weight file..."
	mod = myNN.loadCNN()

	while(True):

		ret, frame = cap.read()

		if ret == True:
			roi, gesture = skinMask(frame, x0, y0, width, height)
			cv2.imshow('Original',frame)
			cv2.imshow('ROI', roi)
			if gesture == -1:
				pic = cv2.imread("Nothing.png")
				cv2.imshow('gesture', pic)
			else:
				pic = cv2.imread(myNN.output[gesture]+'.png')
				cv2.imshow('gesture', pic)

			key = cv2.waitKey(10) & 0xff

			if key == 27:
				break

	cap.release()
	cv2.destroyAllWindows()




