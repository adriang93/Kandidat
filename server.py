import socket
import time

print 'start of program'
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
resolutionX = 720
resolutionY = 480

centerTolerance = resolutionX*0.05
heightTolerance = resolutionY*0.05
heightTolerancePercentage = 10 

maxRoll = 2000
minRoll = 1200

maxYaw = 2000
minYaw = 1200

maxPitch = 2000
minPitch = 1200

maxThrottle = 2000
minThrottle = 1200

targetDistance = 2
distanceTolerance = 0.1*targetDistance

ready = False

targetAltitude = 0.5
altitudeTolerance = 0.1*targetAltitude # allow targetAltitude +- altitudeTolerance

def regulateDistance(): # keep the drone at the correct distance from the user
	diff = cs.sonarrange - targetDistance
	print "diff: " + str(diff)

	percentage = diff/targetDistance

	if percentage < 0:
		percentage = -percentage
	if percentage > 100:
		percentage = 100

	elif diff > distanceTolerance: # drone is closer than desired
		print 'too far away'
		print 'sonarrange:' + str(cs.sonarrange)
		moveForward(percentage)
	elif diff < -distanceTolerance:
		print 'too close'
		print 'sonarrange:' + str(cs.sonarrange)
		moveBackward(percentage)
	else: 
		pitch(50) # set neutral pitch
		
def centerDrone(coordX):
	print 'centerDrone'
	if coordX > ((resolutionX/2) + centerTolerance):
                print "move left: " + str(coordX-resolutionX/2)
		moveLeft((coordX-resolutionX/2)/(resolutionX/2))

	elif coordX < ((resolutionX/2) - centerTolerance):
                print "move right: " + str(coordX-resolutionX/2)
		moveRight(((resolutionX/2)-coordX)/(resolutionX/2))

def regulateHeading(oculusHeading):
	print regulateHeading
	headingDiff = cs.yaw - oculusHeading
	print "headingDiff: "  + str(headingDiff)

	if headingDiff < 0: # make the headingDiff positive...
		headingDiff = headingDiff + 360

	if headingDiff < 180:
                print "turn left: " + str(headingDiff/180)
		percentage = 100*(headingDiff/180)
		turnLeft(percentage)

	elif headingDiff > 180: # just else or keep for clarity? 
                print "turn right: " + str(headingDiff/180)
		percentage = 100*((180-headingDiff)/180)
		turnRight(percentage)

def regulateHeight(coordY):
	percentage = coordY*100/resolutionY

	if coordY > (percentage + heightTolerancePercentage):
		throttle(percentage)

	elif coordX < (percentage - heightTolerancePercentage):
		throttle(percentage)

def moveForward(percentage): # low ->slow, high -> fast
	if validPercentage(percentage):
		pitch(50-(percentage/2))

def moveBackward(percentage): # low ->slow, high -> fast
	if validPercentage(percentage):
		pitch(50+(percentage/2))

def validPercentage(percentage):
	if percentage > 100 or percentage < 0:
		print 'invalid percentage'
		print percentage
		return False
	else:
		return True

def throttle(percentage): # in altitude hold, 40-60 hold current altitude, 
						   # >60 increases altitude, <40 lowers it
	if validPercentage(percentage):
		valueRange = maxThrottle-minThrottle
		value = percentage*valueRange
		Script.SendRC(3,minThrottle+value,True)

def land():
	print 'landing'
	# check if it is in altitude hold
	i = 60
	while i<0:
		i = i-1
		throttle(i)
		Script.Sleep(0.5)

def stop(): # only to be used when calm landing is not safe
	print 'stopping'
	throttle(0) #turn engines of completely

def start():
	print 'starting'
	# set mode to altitude hold
	# set target altitude
	# MAV.doARM(True)
        print "armed"
	# Has to be correct several times to stop peaks from messing shit up
	timesRight = 0
	while timesRight<10:
		if cs.alt < (targetAltitude-altitudeTolerance):
                        print "altitude < "
			throttle(70)
			timesRight = 0
		if cs.alt > (targetAltitude+altitudeTolerance):
                        print "altitiude > "
			throttle(30)
			timesRight = 0
		else:
                        print "ok"
			throttle(50)
			timesRight = timesRight + 1
			print timesRight
	ready = True


def moveLeft(percentage): # low ->slow, high -> fast
	if validPercentage(percentage):
		roll(50-(percentage/2))

def moveRight(percentage): # low ->slow, high -> fast
	if validPercentage(percentage):
		roll(50+(percentage/2))

def moveForward(percentage): # low ->slow, high -> fast
	if validPercentage(percentage):
		pitch(50-(percentage/2))

def moveBackward(percentage): # low ->slow, high -> fast
	if validPercentage(percentage):
		pitch(50+(percentage/2))
		
def yaw(percentage): # low -> counterclockwise, high -> clockwise
	if validPercentage(percentage):
		valueRange = maxYaw-minYaw
		value = percentage*valueRange
		Script.SendRC(4,minYaw+value,True)

def pitch(percentage): # low -> forward, high -> backward
	if validPercentage(percentage):
		valueRange = maxPitch-minPitch
		value = percentage*valueRange
		Script.SendRC(2,minPitch+value,True)

def roll(percentage): # low ->left, high -> right
	if validPercentage(percentage):
		valueRange = maxRoll-minRoll
		value = percentage*valueRange
		Script.SendRC(1,minRoll+value,True)

def turnRight(percentage): # low ->slow, high -> fast
	if validPercentage(percentage):
		yaw(50-(percentage/2))

def turnLeft(percentage): # low ->slow, high -> fast
	if validPercentage(percentage):
		yaw(50+(percentage/2))

print 'variables initiated'
try:
	print "binding"
	s.bind(("localhost", 6550))
except socket.error, msg:
	print "socket error"
	print str(msg[0]) + str(msg[1])
while True: 
	print "listening"
	s.listen(1)
	print "accepting"
	conn, addr = s.accept()

	while True:
                input = conn.recv(1024) # in format (heading,coordX,coordY,validCoord) alternatively (START) or (STOP)
                print "input: " + input
                if input == '':
                        print "Connection closed from other side"
                        break
                myList = input.split(',')
                print myList
                regulateDistance()

                if myList[0] == 'Land' or myList[0] == '':  
                # empty string could mean the program has crashed
                        print "landar"
                        land()

                elif myList[0] == 'STOP': # Stop all motors immediately
                        print "STOP!"
                        Script.SendRC(3,0,True)

                elif myList[0][:5] ==  'START':
                        print "startar"
                        start()

                elif myList[3] == "1" and ready == True: # valid data and has started
                        coordX = myList[1]		
                        centerDrone(int(coordX))

                        coordY = myList[2] 
                        regulateHeight(coordY)
                        # Could be used for additional robustness for height regulation

                        #oculusHeading = myList[0]
                        #regulateHeading(float(oculusHeading))

                        #regulateDistance()

