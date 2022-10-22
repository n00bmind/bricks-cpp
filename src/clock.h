#pragma once

namespace Clock
{
    INLINE f32 AppTimeMillis()
    {
        return (f32)globalPlatform.ElapsedTimeMillis();
    }
    INLINE f32 AppTimeSeconds()
    {
        return (f32)(globalPlatform.ElapsedTimeMillis() * 0.001);
    }
}
