beerfridge
==========

Beerfridge is a process refrigerator. Its primary design is to use cgroups to free and/or slow down processes when their X windows are not on the current tag/workspace, but it should be extensible for other uses as well.

Beerfridge consists of two parts. The first, the Beerfridge Client or bfc, sends messages. This is designed to be called by your window manager to let beerfridge know when windows are no longer being used. The second, the Beerfridge Demon or bfd, runs in the background and modifies the cgroups of your running processes.

Beerfridge is still a work in progress and will probably break your computer. You've been warned.

<H5>Requirements<H5>
Beerfridge needs cgroups to run.
You will also likely want to have your window manager set up to call the bfc.

<H5>License<H5>
Beerfridge is licensed under the AGPL v3
