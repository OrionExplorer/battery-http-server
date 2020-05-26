@echo off
echo BATTERY:
"C:\var\ab.exe" -n 90000 -k -c 100 http://localhost:9090/index.html
pause