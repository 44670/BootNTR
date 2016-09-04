cd resources

copy AppInfo-3 AppInfo

cd ..

copy resources\blue_icon.png resources\icon.png

make clean

mkdir build
copy resources\blue_banner.bnr build\banner.bnr

make


copy output\*.cia release\

pause
