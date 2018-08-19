import cv2
import numpy as np
import time

x0 = 400
y0 = 200
height = 200
width = 200
numOfSamples = 800
counter = 400
saveImg = True
skinkernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(5,5))
gestname = 'V'
path = 'data_5/'

def saveROIImg(img):
	global counter, saveImg
	if counter >= numOfSamples:
		saveImg = False
		return
    
	counter = counter + 1
	name = gestname + str(counter)
	print("Saving img:",name)
	cv2.imwrite(path + gestname + '/' + name + ".png", img)
	time.sleep(0.04)


def skinMask(frame, x0, y0, width, height ):
    global start, saveImg
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
    
    if saveImg == True and start == True:
        saveROIImg(res)

    return res

start = False
n = 0

if __name__ == "__main__":
	cap = cv2.VideoCapture(0)
	cv2.namedWindow('Original', cv2.WINDOW_NORMAL)

	# set rt size as 640x480
	ret = cap.set(3,640)
	ret = cap.set(4,480)

	while(saveImg):
		n = n+1
		if(n == 300):
			start = True

		ret, frame = cap.read()

		if ret == True:
			roi = skinMask(frame, x0, y0, width, height)
			cv2.imshow('Original',frame)
			cv2.imshow('ROI', roi)

