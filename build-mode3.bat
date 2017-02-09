copy template-cia-3.rsf template-cia.rsf
cd resources
copy AppInfo-3 AppInfo
copy icon-3.png icon.png
cd ..

echo #define APP_MEMTYPE (3) > source\gen.h

make clean

make

copy output\*.cia release\
pause