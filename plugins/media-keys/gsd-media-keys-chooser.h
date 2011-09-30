#ifndef GSD_MEDIA_KEYS_CHOOSER_H
#define GSD_MEDIA_KEYS_CHOOSER_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include "gsd-osd-window.h"

G_BEGIN_DECLS

#define GSD_TYPE_MEDIA_KEYS_CHOOSER            (gsd_media_keys_chooser_get_type ())
#define GSD_MEDIA_KEYS_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),  GSD_TYPE_MEDIA_KEYS_CHOOSER, GsdMediaKeysChooser))
#define GSD_MEDIA_KEYS_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),   GSD_TYPE_MEDIA_KEYS_CHOOSER, GsdMediaKeysChooserClass))
#define GSD_IS_MEDIA_KEYS_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  GSD_TYPE_MEDIA_KEYS_CHOOSER))
#define GSD_IS_MEDIA_KEYS_CHOOSER_CLASS(klass) (G_TYPE_INSTANCE_GET_CLASS ((klass), GSD_TYPE_MEDIA_KEYS_CHOOSER))

typedef struct GsdMediaKeysChooser                   GsdMediaKeysChooser;
typedef struct GsdMediaKeysChooserClass              GsdMediaKeysChooserClass;
typedef struct GsdMediaKeysChooserPrivate            GsdMediaKeysChooserPrivate;

struct GsdMediaKeysChooser {
        GsdOsdWindow parent;

        GsdMediaKeysChooserPrivate  *priv;
};

struct GsdMediaKeysChooserClass {
        GsdOsdWindowClass parent_class;
};

typedef enum {
        GSD_MEDIA_KEYS_CHOOSER_ACTION_RFKILL,
} GsdMediaKeysChooserAction;

GType                 gsd_media_keys_chooser_get_type          (void);

GtkWidget *           gsd_media_keys_chooser_new               (void);
gboolean              gsd_media_keys_chooser_is_valid          (GsdMediaKeysChooser       *chooser);

void                  gsd_media_keys_chooser_draw_icons        (GsdMediaKeysChooser       *chooser,
                                                                GsdMediaKeysChooserAction  action);
G_END_DECLS

#endif
