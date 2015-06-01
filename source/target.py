import picamera
import picamera.array
import cv2
import numpy as np
import math
import time
import struct
from serial import Serial

# Find Serial device on Raspberry with ~ls /dev/tty*
arduino = Serial('/dev/ttyACM0', 9600)
time.sleep(2)

##def nothing(x):
##    pass
##
### Starting with 100's to prevent error while masking
##h,s,v = 100,100,100
##
##cv2.namedWindow('result')
##
### Creating track bar
##cv2.createTrackbar('h', 'result',0,179,nothing)
##cv2.createTrackbar('s', 'result',0,255,nothing)
##cv2.createTrackbar('v', 'result',0,255,nothing)


CAMERA_W = 480 
CAMERA_H = 270
PIX_SIZE = 13.0/CAMERA_W

CAMERA_CENTER_X = CAMERA_W/2
CAMERA_CENTER_Y = CAMERA_H/2

with picamera.PiCamera() as camera:
    with picamera.array.PiRGBArray(camera) as stream:
        camera.resolution = (CAMERA_W, CAMERA_H)
	camera.framerate = 24
	camera.hflip = True
	camera.vflip = True
	
	# define range of green color in HSV
        lower_green = np.array([40,140,70])
        upper_green = np.array([90,255,255])

        # define range of red color in HSV
        
        lower_red = np.array([130,90,90])
        upper_red = np.array([180,255,255])

        target_locked = 0
        shooting = 0
        diff_sum = 0
        diff_counter = 0
        stop_shooting = 0
        diff = 0

	for arr in camera.capture_continuous(stream, format='bgr', use_video_port=True):
            # Convert BGR to HSV
            stream.array = cv2.GaussianBlur(stream.array, (9, 9), 2)
            hsv = cv2.cvtColor(stream.array, cv2.COLOR_BGR2HSV) 

            # Threshold the HSV image to get only one color

##            h = cv2.getTrackbarPos('h','result')
##            s = cv2.getTrackbarPos('s','result')
##            v = cv2.getTrackbarPos('v','result')
##            lower_red = np.array([h,s,v])
            
            mask = cv2.inRange(hsv, lower_red, upper_red)

            # Bitwise-AND mask and original image
            #res = cv2.bitwise_and(stream.array, stream.array, mask= mask)          
            contours, hierarchy = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            if contours:
                max_perimeter = 0
                max_perimeter_box = 0            
                
                for i in range(len(contours)):
                    cnt = contours[i] 
                    if len(cnt) > 1:
                        
                        rect = cv2.minAreaRect(cnt)
                        box = cv2.cv.BoxPoints(rect)
                        box = np.int0(box)
                        perimeter = int(cv2.arcLength(cnt, True))
                        if perimeter > max_perimeter:
                            max_perimeter = perimeter
                            max_perimeter_box = box
                print max_perimeter           
                if max_perimeter > 40:
                    box = max_perimeter_box
                    centerX = int((box[0][0] + box[1][0] + box[2][0] + box[3][0])/4.0)
                    centerY = int((box[0][1] + box[1][1] + box[2][1] + box[3][1])/4.0)
                    
                    diff = centerX - CAMERA_CENTER_X
                    diff_sum += diff
                    diff_counter += 1

                    if diff_counter > 10 and abs(diff_sum) <= 15:
                        diff = 0
                        target_locked = 5
                    if abs(diff) > 70: # fast aiming
                        target_locked = 0
                        stop_shooting = 0
                        if diff > 0:
                            arduino.write('9')
                        else:
                            arduino.write('7')
                        arduino.flush()    
                    else: 
                        if abs(diff) > 40: # slow aiming
                            target_locked = 0
                            stop_shooting = 0
                            if diff > 0:
                                arduino.write('6')
                            else:
                                arduino.write('4')
                            arduino.flush()
                        else: # target in center
                            target_locked += 1
                            if target_locked >= 5 and shooting < time.clock() and stop_shooting == 0:
                                shooting = time.clock()+4
                                arduino.write('5') # shooting
                                target_locked = 0
                                diff_counter = 0
                                diff_sum = 0
                                stop_shooting = 1
                    #cv2.drawContours(stream.array,[box],0,(0,255,255),2)        
                else:
                  print "target lost"
                  arduino.write('c')
                  target_locked = 0 # target lost
            else:
                if diff > 0:
                 arduino.write('c')
                 target_locked = 0
                 diff = 0
            #cv2.imshow('result',stream.array)
            if cv2.waitKey(5) & 0xFF == ord('q'):
                break
            # reset the stream before the next capture
            stream.seek(0)
            stream.truncate()
            
        camera.close()
        cv2.destroyAllWindows()
