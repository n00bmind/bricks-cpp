#pragma once

namespace Core
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
