copy template-cia-0.rsf template-cia.rsf
cd resources
copy AppInfo-0 AppInfo
cd ..
make clean

make

copy output\*.cia release\
pause
