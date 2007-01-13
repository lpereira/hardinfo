#ifndef __SYNCMANAGER_H__
#define __SYNCMANAGER_H__

#include <gtk/gtk.h>

typedef struct _SyncEntry	SyncEntry;

typedef enum {
  SYNC_RECEIVE,
  SYNC_SEND,
  SYNC_BOTH
} SyncDirection;

struct _SyncEntry {
  gchar			*name;
  gchar			*save_to;
  SyncDirection		 direction;
  
  gchar			*(*get_data)(void);
};

void sync_manager_add_entry(SyncEntry *entry);
void sync_manager_show(void);

#endif	/* __SYNCMANAGER_H__ */
