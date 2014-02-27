@echo off
rem *--------------------------------------------------------------*
rem * Play file given as first argument possibly applying a effect
rem * given as a second argument. Example:
rem * playwav file.wav echo 1 0.7 100 0.4
rem *--------------------------------------------------------------*
sox %1 -traw -swc2 -r44100 - %2 %3 %4 %5 %6 %7 %8 %9|devaudio.exe -wc2 -r44100
