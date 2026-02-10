#include "gdk/gdk.h"
#include "gdk/gdkkeysyms.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <string.h>

typedef struct _AppData {
	GtkWindow *window;
	GList *list;
	GtkWidget *list_box;
	GtkWidget *row;
	GtkWidget *entry;
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
	GtkListBoxRow *selected_row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app_data->list_box));
	if (keyval == GDK_KEY_Escape) {
		gtk_window_close(GTK_WINDOW(window));
		return true;
	} else if (keyval == GDK_KEY_Down) {
		GtkWidget *next_row = gtk_widget_get_next_sibling(GTK_WIDGET(selected_row));
		if (next_row != NULL) {
			gtk_list_box_select_row(GTK_LIST_BOX(app_data->list_box), GTK_LIST_BOX_ROW(next_row));
			gtk_widget_grab_focus(next_row);
			gtk_widget_grab_focus(app_data->entry);
		}
		return true;
	} else if (keyval == GDK_KEY_Up) {
		GtkWidget *prev_row = gtk_widget_get_prev_sibling(GTK_WIDGET(selected_row));
		if (prev_row != NULL) {
			gtk_list_box_select_row(GTK_LIST_BOX(app_data->list_box), GTK_LIST_BOX_ROW(prev_row));
			gtk_widget_grab_focus(prev_row);
			gtk_widget_grab_focus(app_data->entry);
		}
		return true;
	} else if (keyval == GDK_KEY_Return) {
		printf("dawdawdawdawd\n");
		GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app_data->list_box));
		if (row != NULL) {
			GAppInfo *info = g_object_get_data(G_OBJECT(row), "app-info");
			if (info != NULL) {
				GError *error = NULL;
				GAppLaunchContext *context = g_app_launch_context_new();
				if (g_app_info_launch(info, NULL, context, &error)) {
					gtk_window_close(app_data->window);
				}
			}
		}
		return true;
	}
	return false;
}

static gboolean calculate_score(GtkWidget *row, const char *user_input) {
	if (user_input == NULL || strlen(user_input) == 0) {
		g_object_set_data(G_OBJECT(row), "score", GINT_TO_POINTER(100));
		return true;
	}
	GAppInfo *info = g_object_get_data(G_OBJECT(row), "app-info");
	const char *display_name = g_app_info_get_display_name(info);
	char *n_display_name = g_utf8_strdown(display_name, -1);
	char *n_user_input = g_utf8_strdown(user_input, -1);
	gint score = 0;

	const char *match = strstr(n_display_name, n_user_input);
	if (match != NULL) {
		score = 100;
		gint match_index = g_utf8_pointer_to_offset(n_display_name, match);
		if (match_index == 0) {
			score += 100;
		} else if (*(match - 1) == ' ' || *(match - 1) == '-') {
			score += 50;
		}
		score -= match_index;
		score -= strlen(n_display_name);
	}
	g_free(n_display_name);
	g_free(n_user_input);
	g_object_set_data(G_OBJECT(row), "score", GINT_TO_POINTER(score));
	return (score > 0);
}

static void on_entry_change(GtkEditable *entry, gpointer window) {
	AppData *app_data = g_object_get_data(window, "app-data");
	const char *input = gtk_editable_get_text(entry);
	GtkWidget *row;
	row = gtk_widget_get_first_child(app_data->list_box);
	while (row != NULL) {
		calculate_score(row, input);
		row = gtk_widget_get_next_sibling(row);
	}
	gtk_list_box_unselect_all(GTK_LIST_BOX(app_data->list_box));
	gtk_list_box_invalidate_sort(GTK_LIST_BOX(app_data->list_box));
	gtk_list_box_invalidate_filter(GTK_LIST_BOX(app_data->list_box));
	GtkWidget *first_row = gtk_widget_get_first_child(app_data->list_box);
	gtk_list_box_select_row(GTK_LIST_BOX(app_data->list_box), GTK_LIST_BOX_ROW(first_row));
}

static void fill_list(GtkEditable *editable, gpointer window) {
	AppData *app_data = g_object_get_data(window, "app-data");
	for (GList *l = app_data->list; l != NULL; l = l->next) {
		GAppInfo *info = l->data;
		const char *display_name = g_app_info_get_display_name(info);
		if (g_app_info_should_show(info)) {
			GtkWidget *label = gtk_label_new(display_name);
			gtk_widget_set_halign(label, GTK_ALIGN_START);
			GtkWidget *row = gtk_list_box_row_new();
			gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
			g_object_set_data(G_OBJECT(row), "app-info", info);
			g_object_set_data(G_OBJECT(row), "score", GINT_TO_POINTER(100));
			gtk_list_box_append(GTK_LIST_BOX(app_data->list_box), row);
		}
	}
	GtkWidget *first_row = gtk_widget_get_first_child(app_data->list_box);
	gtk_list_box_select_row(GTK_LIST_BOX(app_data->list_box), GTK_LIST_BOX_ROW(first_row));
}

static gboolean app_filter_func(GtkListBoxRow *row, gpointer data) {
	gint score = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "score"));
	if (score > 0) {
		return true;
	}
	return false;
}

static gint app_sort_func(GtkListBoxRow *a, GtkListBoxRow *b, gpointer data) {
	gint score_a = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(a), "score"));
	gint score_b = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "score"));
	if (score_a > score_b) {
		return -1;
	} else if (score_a < score_b) {
		return 1;
	} else {
		return 0;
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
	gtk_list_box_set_filter_func(GTK_LIST_BOX(list_box), app_filter_func, NULL, NULL);
	gtk_list_box_set_sort_func(GTK_LIST_BOX(list_box), app_sort_func, NULL, NULL);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);

	AppData *app_data = g_new(AppData, 1);
	app_data->window = window;
	app_data->list = g_app_info_get_all();
	app_data->list_box = list_box;
	app_data->entry = entry;

	GtkEventController *controller = gtk_event_controller_key_new();
	gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
	g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_pressed), window);
	gtk_widget_add_controller(GTK_WIDGET(window), controller);

	g_signal_connect(entry, "changed", G_CALLBACK(on_entry_change), window);

	g_object_set_data_full(G_OBJECT(window), "app-data", app_data, free_data);

	fill_list(GTK_EDITABLE(entry), window);
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
