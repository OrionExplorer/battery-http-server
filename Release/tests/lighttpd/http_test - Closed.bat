@echo off
rem "D:\Program Files (x86)\Apache Software Foundation\Apache2.2\bin\ab.exe"

echo LIGHTTPD:
"D:\Program Files (x86)\Apache Software Foundation\Apache2.2\bin\ab.exe" -n 10000 -c 50 http://localhost:8181/index.html
pause