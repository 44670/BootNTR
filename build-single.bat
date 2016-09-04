cd resources

copy AppInfo-3 AppInfo

cd ..

copy resources\blue_icon.png resources\icon.png
copy resources\blue_banner.bnr banner.bnr

make clean


make


copy output\*.cia release\

pause
