cd resources

copy AppInfo-0 AppInfo

cd ..

copy resources\green_icon.png resources\icon.png
copy resources\green_banner.bnr banner.bnr

make clean


make


copy output\*.cia release\

pause

