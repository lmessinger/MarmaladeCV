This project shows how to use OpenCV within Marmalade. It uses OpenCV 2.3.1 so you might need to re-import the OpenCV files. All license conditions are the same as in OpenCV.

This is what we did:

1. add all OpenCV source file to the mkb file. Add also package and subproject zlib to mkb file

2. add options {enable-exeptions } to mkb file

3. change cstdlib file (the one from marmalade installation!) to add abs definition for int and short
add the following (XXX) after
# if !defined ( _STLP_LABS )
inline long abs(long __x) { return _STLP_VENDOR_CSTD::labs(__x); }
//XXXXXXXXXXXX
inline int abs(int __x) { return _STLP_VENDOR_CSTD::labs((long)__x); }
inline long abs(short __x) { return _STLP_VENDOR_CSTD::labs((long)__x); }
//XXXXXXXXXXXX
we then copied cstdlib file into include folder

4. change operations.hpp so that GNU_C will be considered undefined. you can do this, for example, by changing the #ifdef _GNUC_ to #ifdef __GNUC_Lior_

5. enlarge stack (preferably memory, too) in the icf file: 
[S3E]
MemSize=50485760
SysStackSize=4000000 # almost 4mb stack size

6. for x86 compilations remark __cpuid at system.cpp (search for Lior)

that's it - enjoy!

--
Lior

www.lgorithms.com

