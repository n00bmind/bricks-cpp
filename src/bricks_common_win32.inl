#include <atomic>
#include <mutex>
#include "win32.h"
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

#include "magic.h"
#include "common.h"
#include "platform.h"
#include "intrinsics.h"
#include "maths.h"
#include "memory.h"
#include "context.h"
#include "threading.h"
#include "datatypes.h"
#include "logging.h"
#include "clock.h"
#include "strings.h"

#include "common.cpp"
#include "logging.cpp"
#include "platform.cpp"
#include "win32_platform.cpp"
