#include "gdk/gdk.h"
#include "gdk/gdkkeysyms.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>

typedef struct _AppData {
	GtkWindow *window;
	GList *list;
	GtkWidget *list_box;
} AppData;

static void free_data(gpointer data) {
	AppData *app_data = (AppData *)data;
	if (app_data->list != NULL) {
		g_list_free_full(app_data->list, g_object_unref);
	}
	g_free(data);
}

static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state,
                               gpointer window) {
	AppData *app_data = g_object_get_data(window, "app-data");
	if (keyval == GDK_KEY_Escape) {
		gtk_window_close(GTK_WINDOW(window));
		return true;
	}
	return false;
}

static void update_list(GtkEditable *editable, gpointer window) {
	AppData *app_data = g_object_get_data(window, "app-data");
	GtkWidget *list_entry;
	while ((list_entry = gtk_widget_get_first_child(app_data->list_box)) != NULL) {
		gtk_list_box_remove(GTK_LIST_BOX(app_data->list_box), list_entry);
	}
	for (GList *l = app_data->list; l->next != NULL; l = l->next) {
		GAppInfo *info = l->data;
		const char *display_name = g_app_info_get_display_name(info);
		if (g_app_info_should_show(info)) {
			GtkWidget *label = gtk_label_new(display_name);
			gtk_widget_set_halign(label, GTK_ALIGN_START);
			GtkWidget *row = gtk_list_box_row_new();
			gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
			g_object_set_data(G_OBJECT(row), "app-info", info);
			gtk_list_box_append(GTK_LIST_BOX(app_data->list_box), row);
		}
	}
}

static void activate(GtkApplication *app, gpointer user_data) {

	GtkWindow *window = GTK_WINDOW(gtk_application_window_new(app));

	gtk_layer_init_for_window(window);
	gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
	gtk_window_set_default_size(window, 300, 200);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign(box, GTK_ALIGN_FILL);
	gtk_widget_set_valign(box, GTK_ALIGN_FILL);
	gtk_window_set_child(window, box);

	GtkWidget *entry = gtk_entry_new();
	gtk_widget_set_hexpand(entry, true);
	gtk_box_append(GTK_BOX(box), entry);

	GtkWidget *scrolled = gtk_scrolled_window_new();
	gtk_widget_set_vexpand(scrolled, true);
	gtk_box_append(GTK_BOX(box), scrolled);

	GtkWidget *list_box = gtk_list_box_new();
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);

	AppData *app_data = g_new(AppData, 1);
	app_data->window = window;
	app_data->list = g_app_info_get_all();
	app_data->list_box = list_box;

	GtkEventController *controller = gtk_event_controller_key_new();
	g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_pressed), window);
	gtk_widget_add_controller(GTK_WIDGET(window), controller);

	g_signal_connect(entry, "changed", G_CALLBACK(update_list), window);

	g_object_set_data_full(G_OBJECT(window), "app-data", app_data, free_data);

	update_list(GTK_EDITABLE(entry), window);
	gtk_window_present(window);
}

int main(int argc, char **argv) {
	GtkApplication *app;
	int status;

	app = gtk_application_new("org.sal", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
