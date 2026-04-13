#include "gdk/gdk.h"
#include "gdk/gdkkeysyms.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk/gtkcssprovider.h"

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <stdio.h>
#include <string.h>

typedef struct _AppData {
	GtkWindow *window;
	GList *list;
	GtkWidget *list_box;
	GtkWidget *row;
	GtkWidget *entry;
	gint current_index;
	gint visible_rows;
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
		gint next_index = app_data->current_index + 1;
		if (next_index >= app_data->visible_rows - 1) {
			next_index = 0;
		}
		GtkListBoxRow *next_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app_data->list_box), next_index);
		if (next_row == NULL) {
			next_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app_data->list_box), 0);
			app_data->current_index = 0;
		} else {
			app_data->current_index = next_index;
			if (!gtk_widget_get_visible(GTK_WIDGET(next_row))) {
				next_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app_data->list_box), 0);
				app_data->current_index = 0;
			}
		}
		gtk_list_box_select_row(GTK_LIST_BOX(app_data->list_box), GTK_LIST_BOX_ROW(next_row));
		gtk_widget_grab_focus(GTK_WIDGET(next_row));
		gtk_widget_grab_focus(app_data->entry);
		gtk_editable_set_position(GTK_EDITABLE(app_data->entry), -1);
		return true;
	} else if (keyval == GDK_KEY_Up) {
		gint prev_index = app_data->current_index - 1;
		if (prev_index <= 0) {
			prev_index = app_data->visible_rows - 1;
		}
		GtkListBoxRow *next_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app_data->list_box), prev_index);
		if (next_row == NULL) {
			next_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app_data->list_box), 0);
			app_data->current_index = 0;
		} else {
			app_data->current_index = prev_index;
			if (!gtk_widget_get_visible(GTK_WIDGET(next_row))) {
				next_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app_data->list_box), 0);
				app_data->current_index = 0;
			}
		}
		gtk_list_box_select_row(GTK_LIST_BOX(app_data->list_box), GTK_LIST_BOX_ROW(next_row));
		gtk_widget_grab_focus(GTK_WIDGET(next_row));
		gtk_widget_grab_focus(app_data->entry);
		gtk_editable_set_position(GTK_EDITABLE(app_data->entry), -1);
		return true;
	} else if (keyval == GDK_KEY_Return) {
		GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app_data->list_box));
		if (row != NULL) {
			GAppInfo *info = g_object_get_data(G_OBJECT(row), "app-info");
			if (info != NULL) {
				GError *error = NULL;
				GAppLaunchContext *context = g_app_launch_context_new();
				if (g_app_info_launch(info, NULL, context, &error)) {
					gtk_window_close(app_data->window);
				} else {
					g_error_free(error);
				}
				g_object_unref(context);
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
	char *n_display_name = g_object_get_data(G_OBJECT(row), "name-lower");
	gint score = 0;

	const char *match = strstr(n_display_name, user_input);
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
	g_object_set_data(G_OBJECT(row), "score", GINT_TO_POINTER(score));
	return (score > 0);
}

static void on_entry_change(GtkEditable *entry, gpointer window) {
	AppData *app_data = g_object_get_data(window, "app-data");
	app_data->visible_rows = 0;
	const char *input = gtk_editable_get_text(entry);
	input = g_utf8_strdown(input, -1);
	GtkWidget *row;
	row = gtk_widget_get_first_child(app_data->list_box);
	while (row != NULL) {
		if (calculate_score(row, input)) {
			app_data->visible_rows++;
		}
		row = gtk_widget_get_next_sibling(row);
	}
	app_data->current_index = 0;
	gtk_list_box_unselect_all(GTK_LIST_BOX(app_data->list_box));
	gtk_list_box_invalidate_sort(GTK_LIST_BOX(app_data->list_box));
	gtk_list_box_invalidate_filter(GTK_LIST_BOX(app_data->list_box));
	GtkListBoxRow *first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app_data->list_box), 0);
	gtk_list_box_select_row(GTK_LIST_BOX(app_data->list_box), first_row);
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

			if (display_name != NULL) {
				char *display_name_lower = g_utf8_strdown(display_name, -1);
				g_object_set_data_full(G_OBJECT(row), "name-lower", display_name_lower, g_free);
			}

			gtk_list_box_append(GTK_LIST_BOX(app_data->list_box), row);
			app_data->visible_rows++;
		}
	}
	app_data->current_index = 0;
	GtkListBoxRow *first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app_data->list_box), 0);
	gtk_list_box_select_row(GTK_LIST_BOX(app_data->list_box), first_row);
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

static void on_css_error(GtkCssProvider *provider, GtkCssSection *section, GError *error, gpointer user_data) {
	g_printerr("CSS error: %s\n", error->message);
}

static void load_css(void) {
	GtkCssProvider *provider = gtk_css_provider_new();

	char *config_dir = g_build_filename(g_get_user_config_dir(), "sal", NULL);
	char *css_path = g_build_filename(config_dir, "style.css", NULL);

	g_signal_connect(provider, "parsing-error", G_CALLBACK(on_css_error), NULL);
	gtk_css_provider_load_from_path(provider, css_path);

	gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
	                                           GTK_STYLE_PROVIDER_PRIORITY_USER);

	g_free(css_path);
	g_free(config_dir);
	g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {

	GtkWindow *window = GTK_WINDOW(gtk_application_window_new(app));

	gtk_layer_init_for_window(window);
	gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
	gtk_window_set_default_size(window, 300, 200);
	gtk_widget_set_name(GTK_WIDGET(window), "main-window");

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign(box, GTK_ALIGN_FILL);
	gtk_widget_set_valign(box, GTK_ALIGN_FILL);
	gtk_window_set_child(window, box);
	gtk_widget_set_name(GTK_WIDGET(box), "box");

	GtkWidget *entry = gtk_entry_new();
	gtk_widget_set_hexpand(entry, true);
	gtk_box_append(GTK_BOX(box), entry);
	gtk_widget_set_name(GTK_WIDGET(entry), "input");

	GtkWidget *scrolled = gtk_scrolled_window_new();
	gtk_widget_set_vexpand(scrolled, true);
	gtk_box_append(GTK_BOX(box), scrolled);
	gtk_widget_set_name(GTK_WIDGET(scrolled), "scroll");

	GtkWidget *list_box = gtk_list_box_new();
	gtk_list_box_set_filter_func(GTK_LIST_BOX(list_box), app_filter_func, NULL, NULL);
	gtk_list_box_set_sort_func(GTK_LIST_BOX(list_box), app_sort_func, NULL, NULL);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);
	gtk_widget_set_name(GTK_WIDGET(list_box), "app-list");

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
	load_css();
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
