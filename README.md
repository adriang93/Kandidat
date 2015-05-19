# Kandidatarbete DATX02-15-35 2015

Kod som tillhör projektet DATX02-15-35 2015

# Lakitu

All kod i mappen Lakitu är skriven i C++. Lakitu är beroende av biblioteken OpenCV, OculusRiftExamples samt Boost och kräver att följande .lib-filer inkluderas:

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

För att kunna köra programmet krävs att Oculus Runtime version 0.4.4 eller senare är installerat.

# Navigationsmodul

Filen server.py i grenen "navigationsmodul" utgör navigationsmodulen i systemet och är skriven i Python. Filen körs från Mission Planner som ett skript.
