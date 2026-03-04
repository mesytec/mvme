#ifndef A767D065_00E7_42EF_BC55_0E55962EA4D3
#define A767D065_00E7_42EF_BC55_0E55962EA4D3

#include <graphviz/graphviz_version.h>

#if defined(GRAPHVIZ_VERSION_MAJOR) && GRAPHVIZ_VERSION_MAJOR >= 13

// Compatible with the old agstrfree signature. Assumes the strings are html by
// default. No idea and no time to look for the info and motivation for the
// change.
inline int agstrfree(Agraph_t *g, const char *str) { return agstrfree(g, str, true); }

// The type of 'length' changed from unsigned int* to size_t*.
inline int gvRenderData(GVC_t *gvc, graph_t *g, const char *format, char **result,
                        unsigned int *length)
{
    size_t lengthSizeT = 0;
    int ret = gvRenderData(gvc, g, format, result, &lengthSizeT);
    if (length)
        *length = static_cast<unsigned int>(lengthSizeT);
    return ret;
}
#endif

#endif /* A767D065_00E7_42EF_BC55_0E55962EA4D3 */
