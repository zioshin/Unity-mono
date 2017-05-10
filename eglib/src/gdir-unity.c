#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#include "Directory-c-api.h"

struct _GDir {
    UnityPalFindHandle* handle;
    gchar* current;
    gchar* next;
};

GDir *
g_dir_open (const gchar *path, guint flags, GError **error)
{
    GDir *dir;
    gchar* path_search;
    char* result_file_name;
    gint unused_attributes;
    UnityPalErrorCode result;

    g_return_val_if_fail (path != NULL, NULL);
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

    dir = g_new0 (GDir, 1);
    path_search = g_malloc ((strlen(path) + 3)*sizeof(gchar));
    strcpy (path_search, path);
    strcat (path_search, "\\*");

    dir->handle = UnityPalDirectoryFindHandleNew(path_search);
    result = UnityPalDirectoryFindFirstFile(dir->handle, path_search, &result_file_name, &unused_attributes);
    if (!UnityPalSuccess(result)) {
        if (error) {
            gint err = errno;
            *error = g_error_new (G_LOG_DOMAIN, g_file_error_from_errno (err), strerror (err));
        }
        g_free (dir);
        return NULL;
    }

    while ((strcmp (result_file_name, ".") == 0) || (strcmp (result_file_name, "..") == 0)) {
        result = UnityPalDirectoryFindNextFile(dir->handle, &result_file_name, &unused_attributes);
        if (!UnityPalSuccess(result)) {
            if (error) {
                gint err = errno;
                *error = g_error_new (G_LOG_DOMAIN, g_file_error_from_errno (err), strerror (err));
            }
            g_free (dir);
            return NULL;
        }
    }

    dir->current = NULL;
    dir->next = result_file_name;
    return dir;
}

const gchar *
g_dir_read_name (GDir *dir)
{
    char* result_file_name;
    gint unused_attributes;
    UnityPalErrorCode result;

    g_return_val_if_fail (dir != NULL && dir->handle != 0, NULL);

    if (dir->current)
        g_free (dir->current);
    dir->current = NULL;

    dir->current = dir->next;

    if (!dir->current)
        return NULL;

    dir->next = NULL;

    do {
        result = UnityPalDirectoryFindNextFile(dir->handle, &result_file_name, &unused_attributes);
        if (!UnityPalSuccess(result)) {
            dir->next = NULL;
            return dir->current;
        }
    } while ((strcmp (result_file_name, ".") == 0) || (strcmp (result_file_name, "..") == 0));

    dir->next = result_file_name;
    return dir->current;
}

void
g_dir_rewind (GDir *dir)
{
}

void
g_dir_close (GDir *dir)
{
    g_return_if_fail (dir != NULL && dir->handle != 0);
    
    if (dir->current)
        g_free (dir->current);
    dir->current = NULL;
    if (dir->next)
        g_free (dir->next);
    dir->next = NULL;
    UnityPalDirectoryCloseOSHandle(dir->handle);
    UnityPalDirectoryFindHandleDelete(dir->handle);
    dir->handle = 0;
    g_free (dir);
}
