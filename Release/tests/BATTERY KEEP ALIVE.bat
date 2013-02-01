@echo off
echo BATTERY:
"C:\var\ab.exe" -n 10000 -k -c 100 http://localhost:8080/index.html
pause