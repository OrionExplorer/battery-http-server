@echo off
echo APACHE:
"C:\var\ab.exe" -n 100000 -k -c 100 http://localhost/index.html
pause