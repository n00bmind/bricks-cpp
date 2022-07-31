#pragma once

#define DEBUG_IMGUI_REFLECTION 0

struct ImGuiReflector : public Reflector<true>
{
    static constexpr bool IsReading = true;
    static constexpr bool IsWriting = true;

    enum Flags
    {
        None = 0,
        ArrayItem       = 0x1,
        NodeCollapsed   = 0x2,
        Resizable       = 0x4,
    };

    struct StackEntry
    {
        char const* name;
        u32 flags;
    };

    BucketArray<StackEntry> stack;
    StackEntry* head;
    i32 objectDepth;

    ImGuiReflector( Allocator* allocator = CTX_TMPALLOC )
        : Reflector( allocator )
        , stack( 16, allocator )
        , head( nullptr )
        , objectDepth( 0 )
    {

    }

#if DEBUG_IMGUI_REFLECTION
#define DEBUG_TXT( msg, ... )   ImGui::TextColored( { 1.f, 1.f, 1.f, 0.5f }, msg, ##__VA_ARGS__ )
#else
#define DEBUG_TXT( msg, ... )   
#endif

    bool ObjectTreeNode()
    {
        while( objectDepth < stack.count )
        {
            ImGuiReflector::StackEntry& e = stack[ objectDepth ];

            int flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if( e.flags & ImGuiReflector::ArrayItem )
                flags &= ~ImGuiTreeNodeFlags_Framed;

            DEBUG_TXT( "--- Do TreeNode for depth %d / %d ('%s')", objectDepth, stack.count, e.name );

            bool nodeOpen = ImGui::TreeNodeEx( e.name, flags );
            objectDepth++;

            if( !nodeOpen )
            {
                e.flags |= NodeCollapsed;
                return false;
            }
        }

        return true;
    }

    bool PushObject( char const* name, u32 flags )
    {
        if( !ObjectTreeNode() )
        {
            DEBUG_TXT( "--- DONT Push '%s' (TreeNode is collapsed)", name );
            return false;
        }

        if( head )
        {
            // Parent node is collapsed
            if( head->flags & ImGuiReflector::NodeCollapsed )
            {
                DEBUG_TXT( "--- DONT Push '%s' (NodeCollapsed)", name );
                return false;
            }

            ImGui::Indent( 20 );
        }

        StackEntry* parent = head;

        // Add new node
        head = stack.Push( { name, flags } );
        ImGui::PushID( name );

#if DEBUG_IMGUI_REFLECTION
        StringBuilder nameBuffer;
        for( StackEntry const& e : stack )
        {
            nameBuffer.Append( e.name );
            if( &e != &stack.Last() )
                nameBuffer.Append( "/" );
        }
        DEBUG_TXT( "--- Push '%s' (stack depth is now %d)", nameBuffer.ToCStringTmp(), stack.count );
#endif

        return true;
    }

    void PopObject()
    {
        char const* name = head->name;

        ImGui::PopID();

        ASSERT( !stack.Empty(), "ImGuiReflector stack underrun" );
        stack.Pop();
        if( stack.Empty() )
            head = nullptr;
        else
        {
            head = &stack.Last();
            ImGui::Unindent( 20 );
        }

        objectDepth = stack.count;
        DEBUG_TXT( "--- Pop '%s' (back at depth %d)", name, objectDepth );
    }
};

INLINE sz ReflectFieldOffset( ImGuiReflector& r )
{
    return 0;
}

INLINE bool ReflectFieldStart( u32 fieldId, StaticString const& name, ReflectedTypeInfo<ImGuiReflector>* info, ImGuiReflector& r )
{
    return r.PushObject( name, 0u );
}

INLINE void ReflectFieldEnd( u32 fieldId, sz fieldStartOffset, ReflectedTypeInfo<ImGuiReflector<RW>>* info, ImGuiReflector& r )
{
    r.PopObject();
}


REFLECT_SPECIAL_R( ImGuiReflector, int )
{
    // TODO Ranges, notes, readonly, etc.
    ImGui::SliderInt( r.head->name, &d, -100, 100, "%d" );
    return ReflectOk;
}

REFLECT_SPECIAL_R( ImGuiReflector, String )
{
    // TODO Attribute for max length
    char strbuffer[128];
    d.CopyTo( strbuffer, sizeof(strbuffer) );

    if( ImGui::InputText( r.head->name, strbuffer, sizeof(strbuffer) ) )
        d = String( strbuffer );

    return ReflectOk;
}

REFLECT_SPECIAL_RT( ImGuiReflector, Array<T> )
{
    // Ensure we get a nice header
    if( !r.ObjectTreeNode() )
        return ReflectOk;

    if( r.head->flags & ImGuiReflector::Resizable )
    {
        int count = d.count;
        if( ImGui::InputInt( "Size", &count ) )
        {
            count = Max( count, 0 );
            d.Reset( count );
        }
    }

    char namebuffer[16];
    int remove_index = -1;
    for( int i = 0; i < d.count; ++i )
    {
        ImGui::PushID( i );

        // TODO 
        //if( ImGui::SmallButton( ICON_FA_TIMES_CIRCLE ) )
        if( ImGui::SmallButton( "X" ) )
            remove_index = i;
        if ( ImGui::IsItemHovered() )
            ImGui::SetTooltip( "Delete this element" );

        ImGui::SameLine();
        ImGui::Indent( 10 );

        ReflectResult ret = ReflectOk;
        snprintf( namebuffer, sizeof(namebuffer), "[%d]", i );
        if( r.PushObject( namebuffer, ImGuiReflector::ArrayItem ) )
        {
            ret = Reflect( r, d[i] );
            r.PopObject();
        }

        ImGui::Unindent( 10 );
        ImGui::PopID();

        if( !ret )
            return ret;
    }

    if( remove_index >= 0 )
        d.Remove( &d[remove_index] );

    return ReflectOk;
}

