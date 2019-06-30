@echo off
echo BATTERY:
"C:\var\ab.exe" -n 90000 -k -c 100 http://192.168.0.103:8081/mjpeg.html
pause