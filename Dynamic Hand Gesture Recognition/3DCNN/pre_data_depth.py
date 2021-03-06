from __future__ import division
import cv2
import os
import sys
import math
import numpy as np

path = 'SKIG/dep'
files= os.listdir(path) #get all files
sum = 0

os.mkdir('data/depth/1/')
os.mkdir('data/depth/2/')
os.mkdir('data/depth/3/')
os.mkdir('data/depth/4/')
os.mkdir('data/depth/5/')
os.mkdir('data/depth/6/')
os.mkdir('data/depth/7/')
os.mkdir('data/depth/8/')
os.mkdir('data/depth/9/')
os.mkdir('data/depth/10/')

for file in files:
    if len(file) == 61:
        out_path = 'data/depth/' + file[56] + '/'
    elif len(file) == 62:
        out_path = 'data/depth/' + file[56] + file[57] + '/'
    else:
        continue

    sum = sum + 1

    vid_path = path + '/' + file
    video = cv2.VideoCapture(vid_path)
    vid_name = vid_path.split('/')[-1].split('.')[0]
    out_full_path = os.path.join(out_path, vid_name)

    fcount = int(video.get(cv2.cv.CV_CAP_PROP_FRAME_COUNT))

    try:
        os.mkdir(out_full_path)
    except OSError:
        pass

    interval = int(math.floor(fcount / 50))
    j = 0

    for i in range(fcount):
        success, frame = video.read()
        assert success

        if i % interval == 0:
            j = j + 1
            if j>50:
                break
            frame = cv2.resize(frame, (160, 120))
            cv2.imwrite('{}/{:02d}.jpg'.format(out_full_path, j), frame)
        else:
            continue

    print '{} done'.format(vid_name), fcount

print sum
sys.stdout.flush()