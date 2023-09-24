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


// Free memory functions, passing any kind of allocator implementation as the first param
#define ALLOC(allocator, size, ...)                 Alloc( allocator, size, __FILE__, __LINE__, ##__VA_ARGS__ )
#define ALLOC_STRUCT(allocator, type, ...)          (type *)Alloc( allocator, SIZEOF(type), __FILE__, __LINE__, ##__VA_ARGS__ )
#define ALLOC_ARRAY(allocator, type, count, ...)    (type *)Alloc( allocator, (count)*SIZEOF(type), __FILE__, __LINE__, ##__VA_ARGS__ )
#define FREE(allocator, mem, ...)                   Free( allocator, (void*)(mem), ##__VA_ARGS__ )

#undef DELETE
#define NEW(allocator, type, ...)                   new ( ALLOC( allocator, sizeof(type), ##__VA_ARGS__ ) ) type
#define DELETE(allocator, p, T, ...)                { if( p ) { p->~T(); FREE( allocator, p, ##__VA_ARGS__ ); p = nullptr; } }


namespace Memory
{
    enum Tags : u8
    {
        Unknown = 0,
    };

    enum Flags : u8
    {
        None = 0,
        MF_NoClear = 0x1,              // Don't zero memory upon allocation
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

    INLINE Params Default() { return Params(); }
    INLINE Params NoClear() { return Params( MF_NoClear ); }
    INLINE Params Aligned( u16 alignment )
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

// This guy casts an opaque data pointer to the appropriate type
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
// TODO Can we do this simpler plz
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



struct LazyAllocator
{
};

ALLOC_FUNC( LazyAllocator )
{
    ASSERT( !params.alignment );
    
    void* result = malloc( SizeT( sizeBytes ) );

    if( !params.IsSet( Memory::MF_NoClear ) )
        ZEROP( result, sizeBytes );

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

static const sz DefaultArenaPageSize = MEGABYTES( 16 );
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

    // FIXME This doesnt work for non-dynamic arenas
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
    if( !(params.flags & Memory::MF_NoClear) )
        ZEROP( result, size );

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



///// GENERIC HEAP
// General memory heap of a fixed initial size, can allocate any object type or size
// Merges free contiguous blocks and searches for empty blocks linearly, continuing where it last left off
// in circular buffer fashion, so it's appropriate for continuously allocating transient data with a limited lifetime.
// Could be easily extended to grow on request, by just allocating new chunks from the OS and adding them to the chain.
// 
// NOT THREAD SAFE atm

struct GenericHeap
{
    enum BlockFlags : u32
    {
        None    = 0,
        Used    = 0x01,
        Deleted = 0x02,
    };

    struct Block
    {
        Block* prev;
        Block* next;

        i32 size;
        u32 flags;
    };

    Block sentinel;
    Block* lastAllocated;
    sz allocatedBlocks;

    GenericHeap()
        : lastAllocated( &sentinel )
        , allocatedBlocks( 0 )
    {
        // Initialize empty sentinel
        sentinel.prev = &sentinel;
        sentinel.next = &sentinel;
        sentinel.size = 0;
        sentinel.flags = 0;

        // Insert empty block with the whole memory chunk
        static constexpr sz c_initialHeapSize = 256 * 1024 * 1024LL;
        InsertBlock( &sentinel, (u8*)globalPlatform.Alloc( c_initialHeapSize, 0 ), c_initialHeapSize );
    }

    void* Alloc( sz size_bytes )
    {
        void* result = nullptr;

        Block* block = FindBlockForSize( size_bytes );
        if( block )
        {
            result = UseBlock( block, size_bytes );
            lastAllocated = block;
            allocatedBlocks++;
        }
        else
        {
            // TODO Allocate a new block with size Max( size_bytes, c_initialHeapSize ) and add it to the chain
            // (keep track of all blocks in a vector for stats/freeing)
            ASSERT( false, "FULL" );
        }

        return result;
    }

    void Free( void* memory )
    {
        Block* block = (Block*)memory - 1;
        ASSERT( block->flags & BlockFlags::Used, "Can't free a free block!" );
        block->flags &= ~BlockFlags::Used;

        bool merged_next = MergeIfPossible( block, block->next );
        bool merged_prev = MergeIfPossible( block->prev, block );

        // In case we merged around the lastAllocated pointer, search backwards for a new used block
        if( lastAllocated->flags & BlockFlags::Deleted || !(lastAllocated->flags & BlockFlags::Used) )
        {
            lastAllocated = nullptr;
            Block* start_block = block->prev;
            for( Block* b = start_block->prev; b != start_block; b = b->prev )
            {
                if( b->flags & BlockFlags::Used )
                {
                    lastAllocated = b;
                    break;
                }
            }
            if( !lastAllocated )
                lastAllocated = &sentinel;
        }
        allocatedBlocks--;
    }

    private:
    Block* InsertBlock( Block* prev, void* block_start, sz available_size )
    {
        ASSERT( available_size > sizeof(Block), "Need more space" );
        Block* block = (Block*)block_start;
        block->size = I32(available_size - sizeof(Block));
        block->flags = 0;
        block->prev = prev;
        block->next = prev->next;
        block->prev->next = block;
        block->next->prev = block;

        return block;
    }

    Block* FindBlockForSize( sz size )
    {
        Block* result = nullptr;
        for( Block* block = lastAllocated->next; block != lastAllocated; block = block->next )
        {
            // TODO Keep a separate list of free blocks?
            if( !(block->flags & BlockFlags::Used) && block->size >= size )
            {
                result = block;
                break;
            }
        }

        return result;
    }

    void* UseBlock( Block* block, sz useBytes )
    {
        ASSERT( useBytes <= block->size, "Block can't hold %llu bytes", useBytes );

        block->flags |= BlockFlags::Used;
        void* result = (block + 1);

        sz remainingSize = block->size - useBytes;
        // TODO Tune this based on the smallest size we will typically have
        static constexpr sz c_splitThreshold = sizeof(Block) + 16;
        if( remainingSize > c_splitThreshold )
        {
            // Chop given block in two
            block->size = I32(useBytes);
            InsertBlock( block, (u8*)result + useBytes, remainingSize );
        }
        else
        {
            // TODO Record the unused portion so that it can be merged with neighbours
        }

        return result;
    }

    bool MergeIfPossible( Block* first, Block* second )
    {
        ASSERT( first->next == second, "Blocks are not linked" );
        ASSERT( second->prev == first, "Blocks are not linked" );

        bool result = false;

        if( first != &sentinel && second != &sentinel &&
            !(first->flags & BlockFlags::Used) &&
            !(second->flags & BlockFlags::Used) )
        {
            // This check only needed so we can support discontiguous memory pools if needed
            void* expectedOffset = (u8*)first + sizeof(Block) + first->size;
            ASSERT( second == expectedOffset, "Block is not at the expected location" );
            if( second == expectedOffset )
            {
                second->next->prev = first;
                first->next = second->next;
                first->size += sizeof(Block) + second->size;
                // Mark the merged block as deleted so we can correctly reposition lastAllocated pointer
                // (and to help with debugging)
                second->flags |= BlockFlags::Deleted;

                result = true;
            }
        }

        return result;
    }
};
