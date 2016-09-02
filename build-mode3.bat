copy template-cia-3.rsf template-cia.rsf
cd resources
copy AppInfo-3 AppInfo
cd ..
make clean

make

copy output\*.cia release\
pause