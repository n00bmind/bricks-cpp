Pre-compiled Windows libraries for benchmark-1.6.0.
Compiled from the source distribution (downloaded from https://github.com/google/benchmark/releases/tag/v1.6.0) using Visual Studio 2019.
All compilation options used the defaults provided, except:

- a static runtime (/MT or /MTd) was specified.
- /Z7 (include debug info in the object files themselves)
- /Zl (omit C runtime declarations), should _in theory_ allow to link this no matter which runtime flavor is used in the final app
