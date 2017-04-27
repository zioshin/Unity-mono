#ifdef PLATFORM_UNITY

#include <stdlib.h>
#include <glib.h>

#include "Path-c-api.h"

static const char *tmp_dir;

const gchar *
g_get_tmp_dir(void)
{
    if (tmp_dir == NULL)
        tmp_dir = UnityPalGetTempPath();

    return tmp_dir;
}

#endif // PLATFORM_UNITY
