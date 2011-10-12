#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <liburfkill-glib/urfkill.h>

#include "gsd-media-keys-chooser.h"

#define GSD_MEDIA_KEYS_CHOOSER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_MEDIA_KEYS_CHOOSER, GsdMediaKeysChooserPrivate))

#define ICON_SCALE 0.50 /* The size of the icon compared to the whole OSD */
#define BOX_BG_ALPHA 0.50

enum GsdRfkillType
{
        GSD_RFKILL_WLAN = 0,
        GSD_RFKILL_WWAN,
        GSD_RFKILL_BLUETOOTH,
        GSD_NUM_RFKILL
};

struct GsdMediaKeysChooserPrivate
{
        GsdMediaKeysChooserAction  action;
        UrfKillswitch             *killswitch[GSD_NUM_RFKILL];
        GtkWidget                 *grid;
        GtkWidget                 *event_box[GSD_NUM_RFKILL];
        GtkWidget                 *icon[GSD_NUM_RFKILL];
        int                        icon_size;
};

G_DEFINE_TYPE (GsdMediaKeysChooser, gsd_media_keys_chooser, GSD_TYPE_OSD_WINDOW)

static gboolean
verify_killswitch (UrfKillswitch *killswitch)
{
        int state;

        g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), FALSE);

        g_object_get (killswitch, "state", &state, NULL);

        if (state == URFSWITCH_STATE_NO_ADAPTER)
                return FALSE;

        return TRUE;
}

static guint
count_vaild_killswitches (GsdMediaKeysChooser *chooser)
{
        guint i, count = 0;

        for (i = 0; i < GSD_NUM_RFKILL; i++) {
                if (verify_killswitch (chooser->priv->killswitch[i]))
                       count++;
        }

	return count;
}

static gint
get_gsd_rfkill_type (UrfKillswitch *killswitch)
{
        switch (urf_killswitch_get_switch_type (killswitch)) {
        case URFSWITCH_TYPE_WLAN:
                return GSD_RFKILL_WLAN;
        case URFSWITCH_TYPE_BLUETOOTH:
                return GSD_RFKILL_BLUETOOTH;
        case URFSWITCH_TYPE_WWAN:
                return GSD_RFKILL_WWAN;
        default:
                break;
        }

        return -1;
}

static const char *
get_killswitch_icon_name (UrfKillswitch *killswitch)
{
        UrfSwitchType type;
        gint state;

        type = urf_killswitch_get_switch_type (killswitch);
        g_object_get (killswitch, "state", &state, NULL);

        switch (type) {
        case URFSWITCH_TYPE_WLAN:
                if (state == URFSWITCH_STATE_UNBLOCKED)
                        return "wlan-unblocked";
                else if (state == URFSWITCH_STATE_SOFT_BLOCKED ||
                         state == URFSWITCH_STATE_HARD_BLOCKED)
                        return "wlan-blocked";
                break;
        case URFSWITCH_TYPE_BLUETOOTH:
                if (state == URFSWITCH_STATE_UNBLOCKED)
                        return "bt-unblocked";
                else if (state == URFSWITCH_STATE_SOFT_BLOCKED ||
                         state == URFSWITCH_STATE_HARD_BLOCKED)
                        return "bt-blocked";
                break;
        case URFSWITCH_TYPE_WWAN:
                if (state == URFSWITCH_STATE_UNBLOCKED)
                        return "wwan-unblocked";
                else if (state == URFSWITCH_STATE_SOFT_BLOCKED ||
                         state == URFSWITCH_STATE_HARD_BLOCKED)
                        return "wwan-blocked";
                break;
        default:
                break;
        }

        return NULL;
}

static GdkPixbuf *
load_pixbuf (GsdMediaKeysChooser *chooser,
             const char          *name,
             int                  icon_size)
{
        GtkIconTheme    *theme;
        GtkIconInfo     *info;
        GdkPixbuf       *pixbuf;
        GtkStyleContext *context;
        GdkRGBA          color;

        if (chooser != NULL && gtk_widget_has_screen (GTK_WIDGET (chooser))) {
                theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (chooser)));
        } else {
                theme = gtk_icon_theme_get_default ();
        }

        context = gtk_widget_get_style_context (GTK_WIDGET (chooser));
        gtk_style_context_get_background_color (context, GTK_STATE_NORMAL, &color);
        info = gtk_icon_theme_lookup_icon (theme,
                                           name,
                                           icon_size,
                                           GTK_ICON_LOOKUP_FORCE_SIZE | GTK_ICON_LOOKUP_GENERIC_FALLBACK);

        if (info == NULL) {
                g_warning ("Failed to load '%s'", name);
                return NULL;
        }

        pixbuf = gtk_icon_info_load_symbolic (info,
                                              &color,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);
        gtk_icon_info_free (info);

        return pixbuf;
}

static void
refresh_icon (GsdMediaKeysChooser *chooser,
              int                  type)
{
        GsdMediaKeysChooserPrivate *priv = chooser->priv;
        GdkPixbuf *pixbuf;
        const char *icon_name;

        if (priv->action == GSD_MEDIA_KEYS_CHOOSER_ACTION_RFKILL)
                icon_name = get_killswitch_icon_name (priv->killswitch[type]);

        pixbuf = load_pixbuf (chooser, icon_name, priv->icon_size);
        gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon[type]), pixbuf);
}

static gboolean
draw_box_background (GtkWidget *event_box,
                     cairo_t   *cr,
                     gpointer   user_data)
{
        GsdOsdWindow    *window = GSD_OSD_WINDOW (user_data);
        GtkStyleContext *context;
        GdkRGBA          acolor;
        int              width;
        int              height;
        gdouble          corner_radius;

        context = gtk_widget_get_style_context (event_box);

        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        width = gtk_widget_get_allocated_width (event_box);
        height = gtk_widget_get_allocated_height (event_box);

        if (gsd_osd_window_is_composited (GSD_OSD_WINDOW (window)))
                corner_radius = height / 10;
        else
                corner_radius = 0.0;
        gsd_osd_window_draw_rounded_rectangle (cr, 1.0, 0.0, 0.0, corner_radius, width-1, height-1);
        gtk_style_context_get_background_color (context, GTK_STATE_NORMAL, &acolor);
        acolor.alpha = BOX_BG_ALPHA;
        gdk_cairo_set_source_rgba (cr, &acolor);
        cairo_fill(cr);

        return FALSE;
}

static gboolean
enter_notify_callback (GtkWidget      *event_box,
                       GdkEventButton *event,
                       gpointer        data)
{
        GsdOsdWindow *window = GSD_OSD_WINDOW (data);

        gsd_osd_window_stop_hide_timeout (window);

        g_signal_connect (G_OBJECT (event_box),
                          "draw",
                          G_CALLBACK (draw_box_background),
                          window);
        gtk_widget_queue_draw (event_box);

        return FALSE;
}

static gboolean
leave_notify_callback (GtkWidget      *event_box,
                       GdkEventButton *event,
                       gpointer        data)
{
        GsdOsdWindow *window = GSD_OSD_WINDOW (data);

        gsd_osd_window_update_and_hide (window);

        g_signal_handlers_disconnect_by_func (G_OBJECT (event_box),
                                              G_CALLBACK (draw_box_background),
                                              window);
        gtk_widget_queue_draw (event_box);

        return FALSE;
}

static GtkWidget *
create_chooser_eventbox (GsdMediaKeysChooser *chooser)
{
        GtkWidget *event_box;

        event_box = gtk_event_box_new ();
        gtk_event_box_set_visible_window (GTK_EVENT_BOX (event_box), FALSE);
        gtk_widget_set_app_paintable(event_box, TRUE);

        g_signal_connect (G_OBJECT (event_box),
                          "enter-notify-event",
                          G_CALLBACK (enter_notify_callback),
                          chooser);

        g_signal_connect (G_OBJECT (event_box),
                          "leave-notify-event",
                          G_CALLBACK (leave_notify_callback),
                          chooser);

        return event_box;
}

static gboolean
rfkill_button_press_callback (GtkWidget      *event_box,
                              GdkEventButton *event,
                              gpointer        data)
{
        UrfKillswitch *killswitch = URF_KILLSWITCH (data);
        int state;

        g_object_get (killswitch, "state", &state, NULL);
        if (state == URFSWITCH_STATE_UNBLOCKED)
                g_object_set (killswitch, "state", URFSWITCH_STATE_SOFT_BLOCKED, NULL);
        else if (state == URFSWITCH_STATE_SOFT_BLOCKED)
                g_object_set (killswitch, "state", URFSWITCH_STATE_UNBLOCKED, NULL);

        return FALSE;
}

static void
rfkill_state_changed_callback (UrfKillswitch *killswitch,
                               int            state,
                               gpointer       data)
{
        gint type;

        type = get_gsd_rfkill_type (killswitch);
        if (type >= 0)
                refresh_icon (GSD_MEDIA_KEYS_CHOOSER (data), type);
}

static void
add_rfkill_icon_to_grid (GsdMediaKeysChooser *chooser,
                         int                  type)
{
        GsdMediaKeysChooserPrivate *priv = chooser->priv;

        if (priv->icon[type])
            goto done;

        priv->icon[type] = gtk_image_new ();
        gtk_widget_show (priv->icon[type]);

        priv->event_box[type] = create_chooser_eventbox (chooser);
        gtk_container_add (GTK_CONTAINER (priv->event_box[type]), priv->icon[type]);

        g_signal_connect (G_OBJECT (priv->event_box[type]),
                          "button-press-event",
                          G_CALLBACK (rfkill_button_press_callback),
                          priv->killswitch[type]);

        g_signal_connect (G_OBJECT (priv->killswitch[type]),
                          "state-changed",
                           G_CALLBACK (rfkill_state_changed_callback),
                           chooser);

        gtk_grid_attach (GTK_GRID (priv->grid), priv->event_box[type], type, 0, 1, 1);
done:
        refresh_icon (chooser, type);
        gtk_widget_show (priv->event_box[type]);
}

void
gsd_media_keys_chooser_draw_icons (GsdMediaKeysChooser       *chooser,
                                   GsdMediaKeysChooserAction  action)
{
        GsdMediaKeysChooserPrivate *priv = chooser->priv;
        int window_width;
        int window_height;
        guint icon_count = 1;

        if (priv->action != action) {
                int type;
                for (type = 0; type < GSD_NUM_RFKILL; type++) {
                        if (priv->event_box[type]) {
                                gtk_widget_destroy (priv->event_box[type]);
                                priv->event_box[type] = NULL;
                                priv->icon[type] = NULL;
                        }
                }
                priv->action = action;
        }

        gtk_window_get_size (GTK_WINDOW (chooser), &window_width, &window_height);

        priv->icon_size = (int)round (window_height * ICON_SCALE);

        if (action == GSD_MEDIA_KEYS_CHOOSER_ACTION_RFKILL) {
                int type;
                icon_count = count_vaild_killswitches (chooser);
                for (type = 0; type < GSD_NUM_RFKILL; type++) {
                        if (verify_killswitch (priv->killswitch[type]))
                                add_rfkill_icon_to_grid (chooser, type);
                        else if (priv->event_box[type])
                                gtk_widget_hide (priv->event_box[type]);
                }
        }

        gtk_window_set_default_size (GTK_WINDOW (chooser), window_height * icon_count, window_height);
}

static void
gsd_media_keys_chooser_dispose (GObject *object)
{
        GsdMediaKeysChooserPrivate *priv;
        int i;

        priv = GSD_MEDIA_KEYS_CHOOSER (object)->priv;

        for (i = 0; i < GSD_NUM_RFKILL; i++) {
                if (priv->killswitch[i]) {
                        g_signal_handlers_disconnect_by_func (G_OBJECT (priv->killswitch[i]),
                                                              rfkill_state_changed_callback,
                                                              GSD_MEDIA_KEYS_CHOOSER (object));
                        g_object_unref (priv->killswitch[i]);
                        priv->killswitch[i] = NULL;
                }
        }
}

static void
gsd_media_keys_chooser_class_init (GsdMediaKeysChooserClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = gsd_media_keys_chooser_dispose;

        g_type_class_add_private (klass, sizeof (GsdMediaKeysChooserPrivate));
}

static void
gsd_media_keys_chooser_init (GsdMediaKeysChooser *chooser)
{
        int i;

        chooser->priv = GSD_MEDIA_KEYS_CHOOSER_GET_PRIVATE (chooser);

        chooser->priv->action = GSD_MEDIA_KEYS_CHOOSER_ACTION_RFKILL;

        chooser->priv->killswitch[GSD_RFKILL_WLAN] = urf_killswitch_new (URFSWITCH_TYPE_WLAN);
        chooser->priv->killswitch[GSD_RFKILL_BLUETOOTH] = urf_killswitch_new (URFSWITCH_TYPE_BLUETOOTH);
        chooser->priv->killswitch[GSD_RFKILL_WWAN] = urf_killswitch_new (URFSWITCH_TYPE_WWAN);

        chooser->priv->grid = gtk_grid_new ();
        gtk_grid_set_column_homogeneous (GTK_GRID (chooser->priv->grid), TRUE);
        gtk_grid_set_row_homogeneous (GTK_GRID (chooser->priv->grid), TRUE);
        gtk_container_add (GTK_CONTAINER (chooser), chooser->priv->grid);
        gtk_widget_show (chooser->priv->grid);

        for (i = 0; i < GSD_NUM_RFKILL; i++) {
                chooser->priv->icon[i] = NULL;
                chooser->priv->event_box[i] = NULL;
        }

        gsd_osd_window_set_ignore_event (GSD_OSD_WINDOW (chooser), FALSE);
}

GtkWidget *
gsd_media_keys_chooser_new (void)
{
        return g_object_new (GSD_TYPE_MEDIA_KEYS_CHOOSER, NULL);
}
