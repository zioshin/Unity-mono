#ifdef G_OS_WIN32

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#include <winsock2.h>

struct _GDir {
    HANDLE handle;
    gchar* current;
    gchar* next;
};

GDir *
g_dir_open (const gchar *path, guint flags, GError **error)
{
    GDir *dir;
    gunichar2* path_utf16;
    gunichar2* path_utf16_search;
    WIN32_FIND_DATAW find_data;

    g_return_val_if_fail (path != NULL, NULL);
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

    dir = g_new0 (GDir, 1);
    path_utf16 = u8to16 (path);
    path_utf16_search = g_malloc ((wcslen((wchar_t *) path_utf16) + 3)*sizeof(gunichar2));
    wcscpy (path_utf16_search, path_utf16);
    wcscat (path_utf16_search, L"\\*");

    dir->handle = FindFirstFileW (path_utf16_search, &find_data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        if (error) {
            gint err = errno;
            *error = g_error_new (G_LOG_DOMAIN, g_file_error_from_errno (err), strerror (err));
        }
        g_free (path_utf16_search);
        g_free (path_utf16);
        g_free (dir);
        return NULL;
    }
    g_free (path_utf16_search);
    g_free (path_utf16);

    while ((wcscmp ((wchar_t *) find_data.cFileName, L".") == 0) || (wcscmp ((wchar_t *) find_data.cFileName, L"..") == 0)) {
        if (!FindNextFileW (dir->handle, &find_data)) {
            if (error) {
                gint err = errno;
                *error = g_error_new (G_LOG_DOMAIN, g_file_error_from_errno (err), strerror (err));
            }
            g_free (dir);
            return NULL;
        }
    }

    dir->current = NULL;
    dir->next = u16to8 (find_data.cFileName);
    return dir;
}

const gchar *
g_dir_read_name (GDir *dir)
{
    WIN32_FIND_DATAW find_data;

    g_return_val_if_fail (dir != NULL && dir->handle != 0, NULL);

    if (dir->current)
        g_free (dir->current);
    dir->current = NULL;

    dir->current = dir->next;

    if (!dir->current)
        return NULL;

    dir->next = NULL;

    do {
        if (!FindNextFileW (dir->handle, &find_data)) {
            dir->next = NULL;
            return dir->current;
        }
    } while ((wcscmp ((wchar_t *) find_data.cFileName, L".") == 0) || (wcscmp ((wchar_t *) find_data.cFileName, L"..") == 0));

    dir->next = u16to8 (find_data.cFileName);
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
    FindClose (dir->handle);
    dir->handle = 0;
    g_free (dir);
}

#else

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

struct _GDir {
    DIR *dir;
#ifndef HAVE_REWINDDIR
    char *path;
#endif
};

GDir *
g_dir_open (const gchar *path, guint flags, GError **error)
{
    GDir *dir;

    g_return_val_if_fail (path != NULL, NULL);
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

    (void) flags; /* this is not used */
    dir = g_new (GDir, 1);
    dir->dir = opendir (path);
    if (dir->dir == NULL) {
        if (error) {
            gint err = errno;
            *error = g_error_new (G_LOG_DOMAIN, g_file_error_from_errno (err), strerror (err));
        }
        g_free (dir);
        return NULL;
    }
#ifndef HAVE_REWINDDIR
    dir->path = g_strdup (path);
#endif
    return dir;
}

const gchar *
g_dir_read_name (GDir *dir)
{
    struct dirent *entry;

    g_return_val_if_fail (dir != NULL && dir->dir != NULL, NULL);
    do {
        entry = readdir (dir->dir);
        if (entry == NULL)
            return NULL;
    } while ((strcmp (entry->d_name, ".") == 0) || (strcmp (entry->d_name, "..") == 0));

    return entry->d_name;
}

void
g_dir_rewind (GDir *dir)
{
    g_return_if_fail (dir != NULL && dir->dir != NULL);
#ifndef HAVE_REWINDDIR
    closedir (dir->dir);
    dir->dir = opendir (dir->path);
#else
    rewinddir (dir->dir);
#endif
}

void
g_dir_close (GDir *dir)
{
    g_return_if_fail (dir != NULL && dir->dir != 0);
    closedir (dir->dir);
#ifndef HAVE_REWINDDIR
    g_free (dir->path);
#endif
    dir->dir = NULL;
    g_free (dir);
}

int
g_mkdir_with_parents (const gchar *pathname, int mode)
{
    char *path, *d;
    int rv;
    
    if (!pathname || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }
    
    d = path = g_strdup (pathname);
    if (*d == '/')
        d++;
    
    while (TRUE) {
        if (*d == '/' || *d == '\0') {
          char orig = *d;
          *d = '\0';
          rv = mkdir (path, mode);
          if (rv == -1 && errno != EEXIST) {
            g_free (path);
            return -1;
          }

          *d++ = orig;
          while (orig == '/' && *d == '/')
            d++;
          if (orig == '\0')
            break;
        } else {
            d++;
        }
    }
    
    g_free (path);
    
    return 0;
}

#endif
