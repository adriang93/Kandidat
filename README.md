# Kandidatarbete DATX02-15-35 2015

Kod som tillh�r projektet DATX02-15-35 2015

# Lakitu

All kod i mappen Lakitu �r skriven i C++. Lakitu �r beroende av biblioteken OpenCV, OculusRiftExamples samt Boost och kr�ver att f�ljande .lib-filer inkluderas:

winmm.lib
Ws2_32.lib
kernel32.lib
user32.lib
gdi32.lib
winspool.lib
shell32.lib
ole32.lib
oleaut32.lib
uuid.lib
comdlg32.lib
advapi32.lib
opencv_core2411.lib
opencv_highgui2411.lib
opencv_imgproc2411.lib
opencv_objdetect2411.lib
opencv_features2d2411.lib
ExampleCommon.lib
OculusVR.lib
opengl32.lib
glew.lib
glfw3.lib
OpenCTM.lib
ExampleResources.lib
setupapi.lib

F�r att kunna k�ra programmet kr�vs att Oculus Runtime version 0.4.4 eller senare �r installerat.

# Navigationsmodul

Filen server.py utg�r navigationsmodulen i systemet och �r skriven i Python. Filen k�rs fr�n Mission Planner som ett skript.