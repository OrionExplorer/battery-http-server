@echo off
rem "D:\Program Files\Apache Software Foundation\Apache2.2\bin\ab.exe"

"D:\Program Files\Apache Software Foundation\Apache2.2\bin\ab.exe" -n 10000 -k -c 50 http://localhost/batteryServer/index.html
"D:\Program Files\Apache Software Foundation\Apache2.2\bin\ab.exe" -n 10000 -k -c 50 http://localhost:8080/batteryServer/index.html
rem 95.108.104.151
pause