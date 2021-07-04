/*
The MIT License

Copyright (c) 2021 Oscar Peñas Pariente <n00bmindr0b0t@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#if NON_UNITY_BUILD
#include <string.h>
#include "common.h"
#include "intrinsics.h"
#endif


namespace Memory
{
    enum Tags : u8
    {
        Unknown = 0,
    };

    enum Flags : u8
    {
        None = 0,
        NoClear = 0x1,              // Don't zero memory upon allocation
    };

    struct Params
    {
        u8 flags;
        u8 tag;
        u16 alignment;

        Params( u8 flags = 0 )
            : flags( flags )
            , tag( Unknown )
            , alignment( 0 )
        {}

        bool IsSet( Flags flag ) const { return (flags & flag) == flag; }
    };

    inline Params
    Default()
    {
        Params result;
        return result;
    }

    inline Params
    Aligned( u16 alignment )
    {
        Params result;
        result.alignment = alignment;
        return result;
    }
}

using MemoryParams = Memory::Params;


#define ALLOC_FUNC(cls) void* Alloc( cls* data, sz sizeBytes, char const* filename, int line, MemoryParams params = {} )
#define ALLOC_METHOD void* Alloc( sz sizeBytes, char const* filename, int line, MemoryParams params = {} )
typedef void* (*AllocFunc)( void* impl, sz sizeBytes, char const* filename, int line, MemoryParams params );
#define FREE_FUNC(cls)  void Free( cls* data, void* memoryBlock, MemoryParams params = {} )
#define FREE_METHOD  void Free( void* memoryBlock, MemoryParams params = {} )
typedef void (*FreeFunc)( void* impl, void* memoryBlock, MemoryParams params );

// This guy casts the data pointer to the appropriate type
// and relies on overloading to call the correct pair of Alloc & Free functions accepting that as a first argument
template <typename Class>
struct AllocatorImpl
{
    static INLINE ALLOC_FUNC( void )
    {
        Class* obj = (Class*)data;
        return ::Alloc( obj, sizeBytes, filename, line, params );
    }

    static INLINE FREE_FUNC( void )
    {
        Class* obj = (Class*)data;
        ::Free( obj, memoryBlock, params );
    }
};


// This guy is just a generic non-templated wrapper to any kind of allocator whatsoever
// and two function pointers to its corresponding pair of free Alloc & Free functions
struct Allocator
{
    Allocator()
        : allocPtr( nullptr )
        , freePtr( nullptr )
        , impl( nullptr )
    {}

    template <typename Class>
    Allocator( Class* obj )
        : allocPtr( &AllocatorImpl<Class>::Alloc )
        , freePtr( &AllocatorImpl<Class>::Free )
        , impl( obj )
    {}

    // Pass-through for abstract allocators
    template <>
    Allocator( Allocator* obj )
        : allocPtr( obj->allocPtr )
        , freePtr( obj->freePtr )
        , impl( obj->impl )
    {}

    template <typename Class>
    static INLINE Allocator CreateFrom( Class* obj )
    {
        Allocator result( obj );
        return result;
    }

    INLINE ALLOC_METHOD
    {
        return allocPtr( impl, sizeBytes, filename, line, params );
    }

    INLINE FREE_METHOD
    {
        freePtr( impl, memoryBlock, params );
    }

    friend void* Alloc( Allocator* data, sz sizeBytes, char const* filename, int line, MemoryParams params );
    friend void Free( Allocator* data, void* memoryBlock, MemoryParams params );

private:
    AllocFunc allocPtr;
    FreeFunc freePtr;
    void* impl;
};

INLINE ALLOC_FUNC( Allocator )
{
    return data->allocPtr( data->impl, sizeBytes, filename, line, params );
}

INLINE FREE_FUNC( Allocator )
{
    data->freePtr( data->impl, memoryBlock, params );
}


// Free-function versions, passing the allocator implementation
#define ALLOC(allocator, size, ...)                 Alloc( allocator, size, __FILE__, __LINE__, ##__VA_ARGS__ )
#define ALLOC_STRUCT(allocator, type, ...)          (type *)Alloc( allocator, SIZEOF(type), __FILE__, __LINE__, ##__VA_ARGS__ )
#define ALLOC_ARRAY(allocator, type, count, ...)    (type *)Alloc( allocator, (count)*SIZEOF(type), __FILE__, __LINE__, ##__VA_ARGS__ )
#define FREE(allocator, mem, ...)                   Free( allocator, (void*)(mem), ##__VA_ARGS__ )

#undef DELETE
#define NEW(allocator, type, ...)                   new ( ALLOC( allocator, sizeof(type), ##__VA_ARGS__ ) ) type
#define DELETE(allocator, p, T, ...)                { if( p ) { p->~T(); FREE( allocator, p, ##__VA_ARGS__ ); p = nullptr; } }


// Global context to use across the entire application
// NOTE We wanna make sure everything inside here is not generally synchronized across threads to keep it fast!
// i.e. allocator, temp allocator, assert handler, logger (how do we collate and flush all logs?)
struct Context
{
    Allocator allocator;
    Allocator tmpAllocator;
    // ...
    // TODO Application-defined custom data
};

// NOTE TODO This is pretty weird to use in a non-unity build, as this pointer would get defined in each
// translation unit that includes this, but I have no idea how to forward-declare a thread_local variable!?
#if 0
thread_local Context   globalContextStack[64];
thread_local Context*  globalContextPtr;
#endif
thread_local Context** globalContext;
#define CTX (**globalContext)
#define CTX_ALLOC &CTX.allocator
#define CTX_TMPALLOC &CTX.tmpAllocator

#if 0
inline void InitContextStack( Context const& baseContext )
{
    globalContextPtr  = globalContextStack;
    *globalContextPtr = baseContext;
    globalContext     = &globalContextPtr;
}

inline PLATFORM_PUSH_CONTEXT( PushContext )
{
    globalContextPtr++;
    ASSERT( globalContextPtr < globalContextStack + ARRAYCOUNT(globalContextStack) );
    *globalContextPtr = newContext;
}

inline PLATFORM_POP_CONTEXT( PopContext )
{
    if( globalContextPtr > globalContextStack )
        globalContextPtr--;
}
#endif


struct ScopedContext
{
    ScopedContext( Context const& newContext )
    { globalPlatform.PushContext( newContext ); }
    ~ScopedContext()
    { globalPlatform.PopContext(); }
};
#define WITH_CONTEXT(context) \
    ScopedContext ctx_##__FUNC__##__LINE__(context)



struct LazyAllocator
{
};

ALLOC_FUNC( LazyAllocator )
{
    ASSERT( !params.alignment );
    
    void* result = malloc( Size( sizeBytes ) );

    if( !params.IsSet( Memory::NoClear ) )
        PZERO( result, sizeBytes );

    return result;
}

FREE_FUNC( LazyAllocator )
{
    free( memoryBlock );
}

///// MEMORY ARENA
// Linear memory arena that can grow in pages of a certain size
// Can be partitioned into sub arenas and supports "temporary blocks" (which can be nested, similar to a stack allocator)

#define PUSH_STRUCT(arena, type, ...) (type *)_PushSize( arena, SIZEOF(type), alignof(type), ## __VA_ARGS__ )
#define PUSH_ARRAY(arena, type, count, ...) (type *)_PushSize( arena, (count)*SIZEOF(type), alignof(type), ## __VA_ARGS__ )
#define PUSH_STRING(arena, count, ...) (char *)_PushSize( arena, (count)*SIZEOF(char), alignof(char), ## __VA_ARGS__ )
#define PUSH_SIZE(arena, size, ...) _PushSize( arena, size, DefaultMemoryAlignment, ## __VA_ARGS__ )

static const sz DefaultArenaPageSize = MEGABYTES( 1 );
static const sz DefaultMemoryAlignment = alignof(u64);

struct MemoryArenaHeader
{
    u8 *base;
    sz size;
    sz used;
    u8 _pad[40];
};

struct MemoryArena
{
    u8 *base;
    sz size;
    sz used;

    // This is always zero for static arenas
    sz pageSize;
    i32 pageCount;

    i32 tempCount;
};

// Initialize a static (fixed-size) arena on the given block of memory
inline void
InitArena( MemoryArena *arena, void *base, sz size )
{
    *arena = {};
    arena->base = (u8*)base;
    arena->size = size;
}

// Initialize an arena that grows dynamically in pages of the given size
inline void
InitArena( MemoryArena* arena, sz pageSize = DefaultArenaPageSize )
{
    ASSERT( pageSize );
    
    *arena = {};
    arena->pageSize = pageSize;
}

internal MemoryArenaHeader*
GetArenaHeader( MemoryArena* arena )
{
    MemoryArenaHeader* result = (MemoryArenaHeader*)arena->base - 1;
    return result;
}

internal void
FreeLastPage( MemoryArena* arena )
{
    // Restore previous page's info
    MemoryArenaHeader* header = GetArenaHeader( arena );
    void* freePage = header;

    arena->base = header->base;
    arena->size = header->size;
    arena->used = header->used;

    globalPlatform.Free( freePage );
    --arena->pageCount;
}

inline void
ClearArena( MemoryArena* arena )
{
    if( arena->base == nullptr )
        return;

    while( arena->pageCount > 0 )
        FreeLastPage( arena );

    sz pageSize = arena->pageSize;
    InitArena( arena, pageSize );
}

inline sz
Available( const MemoryArena& arena )
{
    return arena.size - arena.used;
}

inline bool
IsInitialized( const MemoryArena& arena )
{
    return arena.base && arena.size;
}

inline void *
_PushSize( MemoryArena *arena, sz size, sz minAlignment, MemoryParams params = {} )
{
#if 0
    if( !(params.flags & MemoryParams::Temporary) )
        // Need to pass temp memory flag if the arena has an ongoing temp memory block
        ASSERT( arena->tempCount == 0 );
#endif

    void* block = arena->base + arena->used;
    void* result = block;
    sz waste = 0;

    sz align = params.alignment;
    if( !align)
        align = minAlignment;
    if( align )
    {
        result = AlignUp( block, align );
        waste = (u8*)result - (u8*)block;
    }

    sz alignedSize = size + waste;
    if( arena->used + alignedSize > arena->size )
    {
        ASSERT( arena->pageSize, "Static arena overflow (size %llu)", arena->size );

        // NOTE We require headers to not change the cache alignment of a page
        ASSERT( SIZEOF(MemoryArenaHeader) == 64 );
        // Save current page info in next page's header
        MemoryArenaHeader headerInfo = {};
        headerInfo.base = arena->base;
        headerInfo.size = arena->size;
        headerInfo.used = arena->used;

        ASSERT( arena->pageSize > SIZEOF(MemoryArenaHeader) );
        sz pageSize = Max( size + SIZEOF(MemoryArenaHeader), arena->pageSize );
        arena->base = (u8*)globalPlatform.Alloc( pageSize, 0 ) + SIZEOF(MemoryArenaHeader);
        arena->size = pageSize - SIZEOF(MemoryArenaHeader);
        arena->used = 0;
        ++arena->pageCount;

        MemoryArenaHeader* header = GetArenaHeader( arena );
        *header = headerInfo;

        // Assume it's already aligned
        if( params.alignment )
            ASSERT( AlignUp( arena->base, params.alignment ) == arena->base );

        result = arena->base;
        alignedSize = size;
    }

    arena->used += alignedSize;

    // Have already moved up the block's pointer, so just clear the requested size
    if( !(params.flags & Memory::NoClear) )
        PZERO( result, size );

    return result;
}

inline MemoryArena
MakeSubArena( MemoryArena* arena, sz size, MemoryParams params = {} )
{
    ASSERT( arena->tempCount == 0 );

    // FIXME Do something so this gets invalidated if the parent arena gets cleared
    // Maybe just create a separate (inherited) struct containing a pointer to the parent and assert their pageCount is never less than ours?

    // Sub-arenas cannot grow
    MemoryArena result = {};
    result.base = (u8*)PUSH_SIZE( arena, size, params );
    result.size = size;
    // Register the pageCount of our parent
    result.pageCount = arena->pageCount;

    return result;
}

//#define ALLOC_FUNC(type) void* Alloc( type* allocator, sz sizeBytes, MemoryParams params = DefaultMemoryParams() )
ALLOC_FUNC( MemoryArena )
{
    void *result = _PushSize( data, sizeBytes, DefaultMemoryAlignment, params );
    return result;
}

FREE_FUNC( MemoryArena )
{
    // NOTE No-op
}

struct TemporaryMemory
{
    MemoryArena *arena;
    u8* baseRecord;
    sz usedRecord;
};

inline TemporaryMemory
BeginTemporaryMemory( MemoryArena *arena )
{
    TemporaryMemory result = {};

    result.arena = arena;
    result.baseRecord = arena->base;
    result.usedRecord = arena->used;

    ++arena->tempCount;

    return result;
}

inline void
EndTemporaryMemory( TemporaryMemory& tempMem )
{
    MemoryArena *arena = tempMem.arena;
    // Find the arena page where the temp memory block started
    while( arena->base != tempMem.baseRecord )
    {
        FreeLastPage( arena );
    }

    ASSERT( arena->used >= tempMem.usedRecord );
    arena->used = tempMem.usedRecord;

    ASSERT( arena->tempCount > 0 );
    --arena->tempCount;
}

struct ScopedTmpMemory
{
    TemporaryMemory mem;
    ScopedTmpMemory( MemoryArena* arena )
    {
        mem = BeginTemporaryMemory( arena );
    }
    ~ScopedTmpMemory()
    {
        EndTemporaryMemory( mem );
    }
};

inline void
CheckTemporaryBlocks( MemoryArena *arena )
{
    ASSERT( arena->tempCount == 0 );
}



///// STATIC MEMORY POOL
// General memory pool of a fixed initial size, can allocate any object type or size
// Merges free contiguous blocks and searches linearly


enum MemoryBlockFlags : u32
{
    None = 0,
    Used = 0x01,
};

struct MemoryBlock
{
    MemoryBlock* prev;
    MemoryBlock* next;

    sz size;
    u32 flags;
};

inline MemoryBlock*
InsertBlock( MemoryBlock* prev, sz size, void* memory )
{
    // TODO 'size' includes the MemoryBlock struct itself for now
    // Are we sure we wanna do this??
    ASSERT( size > SIZEOF(MemoryBlock) );
    MemoryBlock* block = (MemoryBlock*)memory;
    // TODO Are we sure this shouldn't be the other way around??
    block->size = size - SIZEOF(MemoryBlock);
    block->flags = MemoryBlockFlags::None;
    block->prev = prev;
    block->next = prev->next;
    block->prev->next = block;
    block->next->prev = block;

    return block;
}

inline MemoryBlock*
FindBlockForSize( MemoryBlock* sentinel, sz size )
{
    MemoryBlock* result = nullptr;

    // TODO Best match block! (find smallest that fits)
    // TODO Could also continue search where we last left it, for ring-type allocation patterns
    for( MemoryBlock* block = sentinel->next; block != sentinel; block = block->next )
    {
        if( block->size >= size && !(block->flags & MemoryBlockFlags::Used) )
        {
            result = block;
            break;
        }
    }

    return result;
}

inline void*
UseBlock( MemoryBlock* block, sz size, sz splitThreshold )
{
    ASSERT( size <= block->size );

    block->flags |= MemoryBlockFlags::Used;
    void* result = (block + 1);

    sz remainingSize = block->size - size;
    if( remainingSize > splitThreshold )
    {
        block->size -= remainingSize;
        InsertBlock( block, remainingSize, (u8*)result + size );
    }
    else
    {
        // TODO Record the unused portion so that it can be merged with neighbours
    }

    return result;
}

inline bool
MergeIfPossible( MemoryBlock* first, MemoryBlock* second, MemoryBlock* sentinel )
{
    bool result = false;

    if( first != sentinel && second != sentinel &&
        !(first->flags & MemoryBlockFlags::Used) &&
        !(second->flags & MemoryBlockFlags::Used) )
    {
        // This check only needed so we can support discontiguous memory pools if needed
        u8* expectedOffset = (u8*)first + SIZEOF(MemoryBlock) + first->size;
        if( (u8*)second == expectedOffset )
        {
            second->next->prev = second->prev;
            second->prev->next = second->next;

            first->size += SIZEOF(MemoryBlock) + second->size;

            result = true;
        }
    }

    return result;
}

// TODO Abstract the sentinel inside a 'MemoryPool' struct and put a pointer to that in the block
// so that we don't need to pass the sentinel, and we can remove the 'ownerPool' idea from the meshes
inline void
ReleaseBlockAt( void* memory, MemoryBlock* sentinel )
{
    MemoryBlock* block = (MemoryBlock*)memory - 1;
    block->flags &= ~MemoryBlockFlags::Used;

    MergeIfPossible( block, block->next, sentinel );
    MergeIfPossible( block->prev, block, sentinel );
}

