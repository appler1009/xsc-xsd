XSC and XSD: Disk distribution software for MSX, Amiga, UNIX and PC
-------------------------------------------------------------------

Copyright (c) 1995 XelaSoft
Leiden, September 15th 1996

[![Build Status](https://travis-ci.org/appler1009/xsc-xsd.svg?branch=master)](https://travis-ci.org/appler1009/xsc-xsd)

The programs in this archive are designed to use disk-images created
with XelaSoft's D2F and F2D tools on non-MSX systems. The programs 
XSC and XSD are a general compression program and a general decompression
program. The compression format used is compatible with the compression
format used by D2F and F2D.

The program XSC can be used to convert a .XSA disk image into a .DSK disk
image, which then can be loaded into a MSX emulator supporting .DSK disk
images or which can be copied to a disk with some approriate tool.

The program XSD can be used to convert a .DSK disk image into a .XSA disk
image. Such a .XSA disk image will use less space on your harddisk and
require less transfer time when transferred electronically. Ofcourse, you
can also use some other compression utility like gzip or arj, but when you
do so, you will make life harder for the 'real' MSX users out there on this
planet.

When you have a real MSX computer or a PC running MS-DOS or MS-Windows 95, you
should also whatch out for the D2F and F2D tools, which can convert directly
from the disk to a .XSA file. In such case, you do not need a two step approach
to convert a disk into a .XSA file.

The programs D2F, F2D, XSC and XSD are public domain. I do hope that
these program's will become a defacto standard in the MSX (emulator) world.

When you like these programs, feel free to send a picture postcard of the
place where you live to:
A. Wulms
Oude Singel 206
2312 RJ Leiden (the Netherlands)

When you have any comments, suggestions, bug reports, etc. please send these
to the above mentioned address or to:
awulms@inter.nl.net

License
-------

Sterrebeek, 21-5-1998

This archive contains the source of the programs XSC and XSD.

These sources may be used by any one who wants to support the XSA format,
for example in an MSX emulator, provided the following conditions are met:
1) You have to put the following phrase in the documentation of the program
   in which you use the XSA code:
     <program reference> uses XSA code developed by Alex Wulms/XelaSoft
   Notice that you can replace <program reference> with the name of your
   program, for example 'The MSX emulator'. 
2) You do not make any changes to the XSA code that can impact the
   compatibility with the XSA format. Special attention should be paid to
   the compression routine. When the compression routine is changed, for
   example for performance reasons, then you must make sure that the newly
   compressed files can be uncompressed on an MSX system in a 64KB address
   space.

