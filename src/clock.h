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

    // TODO gmtime_s is ofc completely non-standard on windows, so this should all go to the platform layer too
    time_t GetUnixTimeUTC( tm* time_out = nullptr, char* str_out = nullptr, size_t* str_out_len = nullptr, char const* fmt = "%FT%TZ" )
    {
        time_t now = time( nullptr );

        tm* now_struct = nullptr;
        if( time_out )
        {
            errno_t err = gmtime_s( time_out, &now );
            // TODO Check err etc
            if( !err )
                now_struct = time_out;
        }
        else
            now_struct = gmtime( &now );

        // Ensure fields are in range, and fix DST value
        // TODO nullptr should be actually handled
        ASSERT( now_struct );
        now = mktime( now_struct );

        if( str_out )
        {
            ASSERT( str_out_len, "Need a pointer to the max. length of str_out buffer" );
            ASSERT( fmt, "Need a valid format string for strftime" );
            size_t count = strftime( str_out, *str_out_len, fmt, now_struct );
            *str_out_len = count;
        }

        return now;
    }
}
