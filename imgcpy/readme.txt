
Imgcpy.exe is a tool by Alexia Frounze (http://alexfru.narod.ru/) that will copy
files to/from images.

The first parameter is the source parameter, while the second is the target.

C:\>imgcpy readme.txt testfat.img=a:\readme.txt
  Will copy the local file readme.txt to the test image as readme.txt in the
  root directory of the image.  The 'a:' tells imgcpy that the image file (testfat.img)
  is a 1.44meg floppy.

C:\>imgcpy readme.txt testfat.img=d:\readme.txt
  Will copy the local file readme.txt to the test image as readme.txt in the
  root directory of the image.  The 'd:' tells imgcpy that the image file (testfat.img)
  is a hard drive and to copy the file to the second partition of the image.

C:\>imgcpy testfat.img=c:\a_dir
   Will create a director called 'a_dir' in the root of the first partition of the
   hard drive image named 'testfat.img'.

C:\>imgcpy testfat.img=d:\readme.txt readme.txt
  Will copy readme.txt from the second partition of the hard drive image (d:) to
  the local (host) directory and name it readme.txt.


Contents:
  imgcpy.c       C Source code
  imgcpy.exe     DOS executable compiled with DJGPP (needs DPMI)
  readme.txt     This file.
