/**
 * RetroArch - Playlist Download Feature Example Implementation
 * 
 * This file serves as a reference implementation of the background download
 * logic for missing ROMs / games from an LPL playlist.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mocked types and structures to simulate RetroArch environment */
#define PATH_MAX_LENGTH 4096

typedef struct
{
   char path[PATH_MAX_LENGTH];
   char *download;
} playlist_entry_t;

typedef struct
{
   playlist_entry_t entries[100];
   size_t size;
} playlist_t;

typedef struct
{
   void *user_data;
   char path[PATH_MAX_LENGTH];
} file_transfer_t;

typedef struct
{
   int dummy;
} retro_task_t;

/* Simulation of the global download state lock */
static bool g_playlist_download_in_progress = false;

/**
 * Task completion callback.
 * Invoked by RetroArch's internal task queue thread upon completion/failure.
 */
static void playlist_entry_download_callback(
      retro_task_t *task, void *task_data,
      void *user_data, const char *error)
{
   file_transfer_t *transf = (file_transfer_t*)user_data;

   /* Unlock the global state, allowing future downloads */
   g_playlist_download_in_progress = false;

   /* Log results */
   if (error)
      printf("Download failed with error: %s\n", error);
   else
      printf("Download finished successfully. File stored at: %s\n", transf ? transf->path : "unknown");

   /* Safety cleanup */
   if (transf)
      free(transf);
}

/**
 * Mocked functions to simulate RetroArch's utility API.
 */
static playlist_t* playlist_get_cached(void) { return NULL; }
static void playlist_get_index(playlist_t *pl, size_t idx, const playlist_entry_t **entry) {}
static void fill_pathname_parent_dir(char *dest, const char *path, size_t size) {}
static bool path_is_directory(const char *path) { return true; }
static void path_mkdir(const char *path) {}
static int menu_cbs_exit(void) { return 0; }
static const char* msg_hash_to_str(int enum_idx) { return "A download is already in progress."; }
static void runloop_msg_queue_push(const char *msg, size_t len, unsigned prio, unsigned dur, bool flush, void* title, int icon, int cat) {}
static void task_push_http_transfer_file(const char *url, bool mute, const char *method, void *cb, file_transfer_t *transf) {}

#define MENU_ENUM_LABEL_VALUE_QT_DOWNLOAD_ALREADY_IN_PROGRESS 9999
#define MESSAGE_QUEUE_ICON_DEFAULT 0
#define MESSAGE_QUEUE_CATEGORY_INFO 0

/**
 * OK Button press event handler.
 * Executed in RetroArch's main thread when clicking on "Download Game".
 */
static int action_ok_playlist_entry_download(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   playlist_t *playlist               = playlist_get_cached();
   const playlist_entry_t *entry      = NULL;
   char parent_dir[PATH_MAX_LENGTH];
   file_transfer_t *transf            = NULL;

   if (!playlist)
      return menu_cbs_exit();

   playlist_get_index(playlist, entry_idx, &entry);
   if (!entry || !entry->download || !*entry->download)
      return menu_cbs_exit();

   /* 1. Concurrency Check (Anti-Parallel-Download Lock) */
   if (g_playlist_download_in_progress)
   {
      const char *msg = msg_hash_to_str(MENU_ENUM_LABEL_VALUE_QT_DOWNLOAD_ALREADY_IN_PROGRESS);
      if (msg)
         runloop_msg_queue_push(msg, strlen(msg), 1, 100, false, NULL,
               MESSAGE_QUEUE_ICON_DEFAULT, MESSAGE_QUEUE_CATEGORY_INFO);
      return menu_cbs_exit();
   }

   /* 2. Resolve parent directory & recursively create if it does not exist */
   parent_dir[0] = '\0';
   fill_pathname_parent_dir(parent_dir, entry->path, sizeof(parent_dir));
   if (*parent_dir && !path_is_directory(parent_dir))
   {
      path_mkdir(parent_dir);
   }

   /* 3. Acquire Lock */
   g_playlist_download_in_progress = true;

   /* 4. Configure HTTP transfer parameter block */
   transf = (file_transfer_t*)calloc(1, sizeof(*transf));
   if (!transf)
   {
      g_playlist_download_in_progress = false;
      return menu_cbs_exit();
   }

   strncpy(transf->path, entry->path, sizeof(transf->path) - 1);

   /* 5. Push task to queue */
   task_push_http_transfer_file(
         entry->download,
         false,
         "GET",
         playlist_entry_download_callback,
         transf);

   return menu_cbs_exit();
}
