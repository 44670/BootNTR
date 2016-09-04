cd resources

copy AppInfo-0 AppInfo

cd ..

copy resources\green_icon.png resources\icon.png

make clean

mkdir build
copy resources\green_banner.bnr build\banner.bnr

make


copy output\*.cia release\

pause

