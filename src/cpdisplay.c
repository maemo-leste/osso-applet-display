/*
 *  display control panel plugin
 *  Copyright (C) 2011 Nicolai Hess
 *  Copyright (C) 2021 Carl Klemm
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <hildon/hildon.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <string.h>
#include <locale.h>
#include <mce/dbus-names.h>
#include <gdk/gdkx.h>

#ifdef HILDON_DISABLE_DEPRECATED
#undef HILDON_DISABLE_DEPRECATED
#endif

static GDBusConnection *get_dbus_connection(void)
{
	GError *error = NULL;
	char *addr;
	
	GDBusConnection *s_bus_conn;

	addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (addr == NULL) {
		g_error("fail to get dbus addr: %s\n", error->message);
		g_free(error);
		return NULL;
	}

	s_bus_conn = g_dbus_connection_new_for_address_sync(addr,
			G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
			G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
			NULL, NULL, &error);

	if (s_bus_conn == NULL) {
		g_error("fail to create dbus connection: %s\n", error->message);
		g_free(error);
	}

	return s_bus_conn;
}

static gint32 mce_get_dbus_int(GDBusConnection *bus, const gchar *request, gint32 defval)
{
	GVariant *result;

	GError *error = NULL;

	result = g_dbus_connection_call_sync(bus, MCE_SERVICE, MCE_REQUEST_PATH,
		MCE_REQUEST_IF, request, NULL, NULL,
		G_DBUS_CALL_FLAGS_NONE, 2000, NULL, &error);
	
	if (error || !result) {
		g_critical("%s: Can not get value for %s", __func__, request);
		return defval;
	}

	GVariantType *int_variant = g_variant_type_new("(i)");

	if (!g_variant_is_of_type(result, int_variant)) {
		g_critical("%s: Can not get value for %s wrong type: %s instead of (i)",
				   __func__, request, g_variant_get_type_string(result));
		g_variant_unref(result);
		g_variant_type_free(int_variant);
		return defval;
	}

	g_variant_type_free(int_variant);

	gint32 value;
	g_variant_get(result, "(i)", &value);
	g_variant_unref(result);
	return value;
}

static gboolean mce_set_dbus_int(GDBusConnection *bus, const gchar *request, gint32 val)
{
	GVariant *result;
	GError *error = NULL;

	result = g_dbus_connection_call_sync(bus, MCE_SERVICE, MCE_REQUEST_PATH,
		MCE_REQUEST_IF, request, g_variant_new("(i)", val), NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if (error) {
		g_critical("%s: Failed to send %s to mce", __func__, request);
		return FALSE;
	}

	g_variant_unref(result);

	return TRUE;
}

static gboolean mce_get_dbus_bool(GDBusConnection *bus, const gchar *request, gboolean defval)
{
	GVariant *result;

	GError *error = NULL;

	result = g_dbus_connection_call_sync(bus, MCE_SERVICE, MCE_REQUEST_PATH,
		MCE_REQUEST_IF, request, NULL, NULL,
		G_DBUS_CALL_FLAGS_NONE, 2000, NULL, &error);
	
	if (error || !result) {
		g_critical("%s: Can not get value for %s", __func__, request);
		return defval;
	}

	GVariantType *int_variant = g_variant_type_new("(b)");

	if (!g_variant_is_of_type(result, int_variant)) {
		g_critical("%s: Can not get value for %s wrong type: %s instead of (b)",
				   __func__, request, g_variant_get_type_string(result));
		g_variant_unref(result);
		g_variant_type_free(int_variant);
		return defval;
	}

	g_variant_type_free(int_variant);

	gboolean value;
	g_variant_get(result, "(b)", &value);
	g_variant_unref(result);
	return value;
}

static gboolean mce_set_dbus_bool(GDBusConnection *bus, const gchar *request, gboolean val)
{
	GVariant *result;
	GError *error = NULL;

	result = g_dbus_connection_call_sync(bus, MCE_SERVICE, MCE_REQUEST_PATH,
		MCE_REQUEST_IF, request, g_variant_new("(b)", val), NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if (error) {
		g_critical("%s: Failed to send %s to mce", __func__, request);
		return FALSE;
	}

	g_variant_unref(result);

	return TRUE;
}

static void _brightness_level_changed(HildonControlbar* brightness_control_bar, gpointer user_data)
{
  GDBusConnection *bus = (GDBusConnection*)user_data;
  mce_set_dbus_int(bus, MCE_DISPLAY_BRIGTNESS_SET, hildon_controlbar_get_value(brightness_control_bar));
}

static void _size_changed(GdkScreen *screen, gpointer user_data)
{
	GdkGeometry geometry;
	if (gdk_screen_get_width(gdk_display_get_default_screen(gdk_display_get_default())) < 
		gdk_screen_get_height(gdk_display_get_default_screen(gdk_display_get_default()))) {
		geometry.min_height = 500;
		geometry.min_width = 480;
	} else {
		geometry.min_height = 320;
		geometry.min_width = 800;
	}
	gtk_window_set_geometry_hints(GTK_WINDOW(user_data), GTK_WIDGET(user_data), &geometry, GDK_HINT_MIN_SIZE);
}

osso_return_t execute(osso_context_t *osso, gpointer user_data, gboolean user_activated)
{
	GtkWidget *dialog = gtk_dialog_new_with_buttons(dgettext("osso-display", "disp_ti_display"),
							GTK_WINDOW(user_data),
							GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
							dgettext("hildon-libs", "wdgt_bd_save"), GTK_RESPONSE_ACCEPT,
							NULL);
	GDBusConnection *s_bus = get_dbus_connection();
	g_assert(s_bus);

	GtkWidget *pan = hildon_pannable_area_new();
	GtkWidget *box = gtk_vbox_new(FALSE, 0);
	GtkWidget *brightness_box = gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT);
	GtkWidget *brightness_label = gtk_label_new(dgettext("osso-display", "disp_fi_brightness"));
	GtkWidget *brightness_control_bar = hildon_controlbar_new();

	GtkWidget *display_dim_timeout_picker_button = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT,
										HILDON_BUTTON_ARRANGEMENT_VERTICAL);

	GtkWidget *touchscreen_keypad_autolock_check_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
	GtkWidget *inhibit_blank_mode_check_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    // TODO: reinstate with new mce interface when it lands
    // https://github.com/maemo-leste/mce/issues/27
	//GtkWidget *power_saving_check_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
	GtkWidget *dim_timeout_selector = hildon_touch_selector_new_text();
	gint current_brightness = mce_get_dbus_int(s_bus, MCE_DISPLAY_BRIGTNESS_GET, 1);
	hildon_gtk_widget_set_theme_size(GTK_WIDGET(brightness_control_bar),
					 HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);
	g_object_set(pan, "hscrollbar-policy", GTK_POLICY_NEVER, NULL);
	gtk_misc_set_alignment(GTK_MISC(brightness_label), 0, 0.5);

	hildon_button_set_title(HILDON_BUTTON(display_dim_timeout_picker_button),
				dgettext("osso-display", "disp_fi_backlight_period"));
	hildon_button_set_alignment(HILDON_BUTTON(display_dim_timeout_picker_button), 0, 0.5, 1, 1);
	hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(display_dim_timeout_picker_button),
					  HILDON_TOUCH_SELECTOR(dim_timeout_selector));

	gtk_button_set_label(GTK_BUTTON(touchscreen_keypad_autolock_check_button),
			     dgettext("osso-display", "disp_fi_lock_screen"));
	gtk_button_set_label(GTK_BUTTON(inhibit_blank_mode_check_button),
			     dgettext("osso-display", "disp_fi_display_stays_on"));
	//gtk_button_set_label(GTK_BUTTON(power_saving_check_button),
	//		     dgettext("osso-display", "disp_fi_power_save_mode"));

	hildon_check_button_set_active(HILDON_CHECK_BUTTON(touchscreen_keypad_autolock_check_button),
				       mce_get_dbus_bool(s_bus, MCE_AUTOLOCK_GET, FALSE));
	hildon_check_button_set_active(HILDON_CHECK_BUTTON(inhibit_blank_mode_check_button),
				       mce_get_dbus_int(s_bus, MCE_DISPLAY_TIMEOUT_MODE_GET, 0));
	//hildon_check_button_set_active(HILDON_CHECK_BUTTON(power_saving_check_button),
	//			       gconf_client_get_bool(gconf_client, GCONF_KEY_POWER_SAVING, NULL));

	hildon_controlbar_set_range(HILDON_CONTROLBAR(brightness_control_bar), 1, 5);

	hildon_controlbar_set_value(HILDON_CONTROLBAR(brightness_control_bar), current_brightness);

	int timeout =  mce_get_dbus_int(s_bus, MCE_DISPLAY_TIMEOUT_GET, 0);
	const int timeouts[] = MCE_DISPLAY_TIMEOUT_LEVELS;
	
	for(size_t index = 0; timeouts[index] >= 0; ++index) {
		int value = timeouts[index];
		gchar *list_text = NULL;
		if (value < 60) {
			const gchar *text =
				dngettext("osso-display", "disp_va_gene_0", "disp_va_gene_1", value);
			list_text = g_strdup_printf(text, value);
		} else {
			int min_value = (int)(value / 60);
			const gchar *text =
				dngettext("osso-display", "disp_va_do_2", "disp_va_gene_2", min_value);
			list_text = g_strdup_printf(text, min_value);
		}
		hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR(dim_timeout_selector), list_text);
		if (value == timeout)
			hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(dim_timeout_selector), 0, index);
		g_free(list_text);
	}

	gtk_box_pack_start(GTK_BOX(brightness_box), brightness_label, TRUE, TRUE, 2 * HILDON_MARGIN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(brightness_box), GTK_WIDGET(brightness_control_bar), FALSE, FALSE,
			   HILDON_MARGIN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(box), brightness_box, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), display_dim_timeout_picker_button, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), touchscreen_keypad_autolock_check_button, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), inhibit_blank_mode_check_button, TRUE, FALSE, 0);
	//gtk_box_pack_start(GTK_BOX(box), power_saving_check_button, TRUE, FALSE, 0);

	// another box, so, the widgets arent scatterd over the
	// whole dialog  
	GtkWidget *box_box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box_box), box, TRUE, FALSE, 0);
	hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pan), box_box);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), pan, TRUE, TRUE, 0);

	_size_changed(NULL, GTK_WINDOW(dialog));
	g_signal_connect(gdk_display_get_default_screen(gdk_display_get_default()),
			 "size-changed", G_CALLBACK(_size_changed), dialog);
	g_signal_connect(brightness_control_bar, "value-changed", G_CALLBACK(_brightness_level_changed), s_bus);
	gtk_widget_show_all(dialog);
	guint response = gtk_dialog_run(GTK_DIALOG(dialog));

	if (response == GTK_RESPONSE_ACCEPT) {
		mce_set_dbus_bool(s_bus,
				      MCE_AUTOLOCK_SET,
				      hildon_check_button_get_active(HILDON_CHECK_BUTTON(touchscreen_keypad_autolock_check_button)));
		mce_set_dbus_int(s_bus, MCE_DISPLAY_TIMEOUT_MODE_SET,
						 hildon_check_button_get_active(HILDON_CHECK_BUTTON(inhibit_blank_mode_check_button)));
		//gconf_client_set_bool(gconf_client, GCONF_KEY_POWER_SAVING,
		//		      hildon_check_button_get_active(HILDON_CHECK_BUTTON(power_saving_check_button)),
		//		      NULL);

		int active_dim_timeout_index =
		    hildon_touch_selector_get_active(HILDON_TOUCH_SELECTOR(dim_timeout_selector), 0);
		if (active_dim_timeout_index < sizeof(timeouts)/sizeof(*timeouts) && active_dim_timeout_index > 0)
			mce_set_dbus_int(s_bus, MCE_DISPLAY_TIMEOUT_SET, timeouts[active_dim_timeout_index]);
	} else {
		if (hildon_controlbar_get_value(HILDON_CONTROLBAR(brightness_control_bar)) != current_brightness)
			mce_set_dbus_int(s_bus, MCE_DISPLAY_BRIGTNESS_SET, current_brightness);
	}
	gtk_widget_destroy(dialog);
	g_object_unref(s_bus);
	return OSSO_OK;
}

osso_return_t save_state(osso_context_t *osso, gpointer user_data)
{
	return OSSO_OK;
}
