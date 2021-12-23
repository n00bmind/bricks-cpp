#pragma once

namespace Core
{
    struct Time
    {
        f64 appStartTimeMillis;
    };


    void InitTime( Time* time )
    {
        *time = {};
        time->appStartTimeMillis = globalPlatform.CurrentTimeMillis();
    }

    INLINE f32 AppTimeMillis( Time* time )
    {
        return (f32)(globalPlatform.CurrentTimeMillis() - time->appStartTimeMillis);
    }
    INLINE f32 AppTimeSeconds( Time* time )
    {
        return (f32)((globalPlatform.CurrentTimeMillis() - time->appStartTimeMillis) * 0.001);
    }
}
