Pre-compiled Windows libraries for mbedtls-3.0.0.
Compiled from the mbedtls source distribution (downloaded from github) using Visual Studio 2019.
All compilation options used the defaults provided, except:

- a static runtime (/MT or /MTd) was specified.
- /Z7 (include debug info in the object files themselves)
- /Zl (omit C runtime declarations), should _in theory_ allow to link this no matter which runtime flavor is used in the final app
