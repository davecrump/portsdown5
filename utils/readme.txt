This directory contains scripts used during the testing and release process.

Scripts that may be useful to advanced users:
=============================================

stop.sh (bash alias "stop"): Stops all Portsdown processes including transmit, receive and touch

uguir.sh (bash alias "ugui"): Compiles the Portsdown app and then runs it using the scheduler

guir.sh (bash alias "gui"): Runs the Portsdown app using the scheduler


Scripts used during the software release process:
=================================================

prep_for_release.sh : Copies the user config to a safe place and restores the factory config

after_release.sh : Restores the user config from the safe space


Scripts used during the software development process:
=====================================================

ugui.sh : Compiles the portsdown app and runs it without the scheduler

ulbv.sh : Compiles the Lime bandviewer app and runs it without the scheduler

ulbvng.sh : Compiles the Lime NG bandviewer app and runs it without the scheduler

uldmm.sh : Compiles the DMM app and runs it without the scheduler

ulnf.sh : Compiles the Lime Noise Figure app and runs it without the scheduler

ulnm.sh : Compiles the Lime Noise Measurement app and runs it without the scheduler

ulpm.sh : Compiles the Power Meter and runs it without the scheduler

ulsng.sh : downloads the latest update to LimeSuiteNG and compiles it

ulst.sh : recompiles LimeSDR toolbox

ulstng.sh : recompiles LimeSDR toolbox for LimeSuiteNG

ulswp.sh : recompiles the LimeSDR sweeper and runs it without the scheduler

usgr.sh : recompiles the SigGen and runs it without the scheduler

uw2l.sh : recompiles wav2lime and runs it with /home/pi/hamtv_short.wav

uw2lng.sh : recompiles wav2limeng and runs it with /home/pi/hamtv_short.wav

uall.sh: recompiles each app in term but does not run them.  Use after changes in the common libraries


Scripts used during the software development process for the KeyLimePi:
=======================================================================

ulsabv.sh : Compiles the bandviewer on 323 MHz for narrow sweep and runs it

ulsaif.sh : Compiles the 323 MHz IF detector for wide sweep and runs it

ulsasdr.sh : Compiles the BandViewer for KeyLime Pi and runs it

ulsdh.sh : Compiles the Application scheduler for KeyLime Pi and runs it
