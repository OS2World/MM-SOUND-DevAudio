@echo off
rem *--------------------------------------------------------------*
rem * Play a tracker file given as first argument. Example:
rem * playmod file.mod
rem * NOTE: MikMod's raw output driver has a bug in it, so at current
rem * time it is unusable. I'll submit the required changes to MikMod's
rem * author, in the meantime I've included a binary of MikMod.
rem * NOTE: "6" in "-d6" directive below is the stdout driver. You
rem * should run "MikMod -l" and see if this corresponds to your
rem * MikMod setup (driver ordinal number depends of compilation
rem * options).
rem *--------------------------------------------------------------*
MikMod %1 -d6 %2 %3 %4 %5 %6 %7 %8 %9|devaudio.exe -wc2 -r44100
