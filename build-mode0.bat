copy template-cia-0.rsf template-cia.rsf
cd resources
copy AppInfo-0 AppInfo
copy icon-0.png icon.png
cd ..

echo #define APP_MEMTYPE (0) > source\gen.h

make clean

make

copy output\*.cia release\
pause
