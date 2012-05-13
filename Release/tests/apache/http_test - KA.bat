@echo off
rem "D:\Program Files (x86)\Apache Software Foundation\Apache2.2\bin\ab.exe"

echo APACHE:
"D:\Program Files (x86)\Apache Software Foundation\Apache2.2\bin\ab.exe" -n 10000 -k -c 50 http://localhost/index.html