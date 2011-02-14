/*
 *  display control panel plugin
 *  Copyright (C) 2011 Nicolai Hess
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
#include <gconf/gconf-client.h>
#include <gdk/gdkx.h>

#define GCONF_KEY_DISPLAY    "/system/osso/dsm/display/"
#define GCONF_KEY_POWER_SAVING           GCONF_KEY_DISPLAY "enable_power_saving"
#define GCONF_KEY_INHIBIT_BLANK_MODE  GCONF_KEY_DISPLAY "inhibit_blank_mode"
#define GCONF_KEY_DIM_TIMEOUTS GCONF_KEY_DISPLAY "possible_display_dim_timeouts"
#define GCONF_KEY_DIM_TIMEOUT GCONF_KEY_DISPLAY "display_dim_timeout"
#define GCONF_KEY_DISPLAY_BRIGHTNESS GCONF_KEY_DISPLAY "display_brightness"
#define GCONF_KEY_DISPLAY_BRIGHTNESS_MAX GCONF_KEY_DISPLAY "max_display_brightness_levels"
#define GCONF_KEY_TOUCHSCREEN_VIBRA   "/system/osso/dsm/vibra/touchscreen_vibra_enabled"
#define GCONF_KEY_TOUCHSCREEN_AUTOLCOK "/system/osso/dsm/locks/touchscreen_keypad_autolock_enabled"

#ifdef HILDON_DISABLE_DEPRECATED
#undef HILDON_DISABLE_DEPRECATED
#endif

static void
_brightness_level_changed(HildonControlbar* brightness_control_bar, gpointer user_data)
{
  GConfClient* gconf_client = GCONF_CLIENT(user_data);
  gconf_client_set_int(gconf_client, 
		       GCONF_KEY_DISPLAY_BRIGHTNESS, 
		       hildon_controlbar_get_value(brightness_control_bar), 
		       NULL);
}

static void
_size_changed(GdkScreen* screen,
	      gpointer user_data)
{
  GdkGeometry geometry;
  if(gdk_screen_get_width(gdk_display_get_default_screen(gdk_display_get_default())) < 800)
  {
    geometry.min_height = 680;
    geometry.min_width = 480; 
  }
  else
  {
    geometry.min_height = 360;
    geometry.min_width = 800; 
  }
  gtk_window_set_geometry_hints(GTK_WINDOW(user_data),
				GTK_WIDGET(user_data),
				&geometry,
				GDK_HINT_MIN_SIZE);
}

osso_return_t execute(osso_context_t* osso, gpointer user_data, gboolean user_activated)
{
  GtkWidget* dialog = gtk_dialog_new_with_buttons(dgettext("osso-display", "disp_ti_display"),
						  GTK_WINDOW(user_data),
						  GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
						  dgettext("hildon-libs", "wdgt_bd_save"), GTK_RESPONSE_ACCEPT,
						  NULL);
  GConfClient* gconf_client = gconf_client_get_default();
  g_assert(GCONF_IS_CLIENT(gconf_client));
  
  GtkWidget* pan = hildon_pannable_area_new();
  GtkWidget* box = gtk_vbox_new(FALSE, 0);
  GtkWidget* brightness_box = gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT);
  GtkWidget* brightness_label = gtk_label_new(dgettext("osso-display", "disp_fi_brightness"));
  GtkWidget* brightness_control_bar = hildon_controlbar_new();

  GtkWidget* display_dim_timeout_picker_button = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT,
									  HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  
  GtkWidget* touchscreen_keypad_autolock_check_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);  
  GtkWidget* inhibit_blank_mode_check_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);  
  GtkWidget* touchscreen_vibra_check_button  = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
  GtkWidget* power_saving_check_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
  GtkWidget* dim_timeout_selector = hildon_touch_selector_new_text();
  gint current_brightness = gconf_client_get_int(gconf_client, GCONF_KEY_DISPLAY_BRIGHTNESS, NULL);
  hildon_gtk_widget_set_theme_size(GTK_WIDGET(brightness_control_bar), 
				   HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);
  g_object_set(pan, "hscrollbar-policy", GTK_POLICY_NEVER, NULL);
  gtk_misc_set_alignment(GTK_MISC(brightness_label),
			 0, 0.5);
  
  hildon_button_set_title(HILDON_BUTTON(display_dim_timeout_picker_button), 
			  dgettext("osso-display", "disp_fi_backlight_period"));
  hildon_button_set_alignment(HILDON_BUTTON(display_dim_timeout_picker_button), 
			      0, 0.5, 1, 1);
  hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(display_dim_timeout_picker_button),
				    HILDON_TOUCH_SELECTOR(dim_timeout_selector));

  gtk_button_set_label(GTK_BUTTON(touchscreen_keypad_autolock_check_button), 
		       dgettext("osso-display", "disp_fi_lock_screen"));
  gtk_button_set_label(GTK_BUTTON(inhibit_blank_mode_check_button),
		       dgettext("osso-display", "disp_fi_display_stays_on"));
  gtk_button_set_label(GTK_BUTTON(touchscreen_vibra_check_button), 
		       dgettext("osso-display", "disp_fi_touchscreen_vibration"));
  gtk_button_set_label(GTK_BUTTON(power_saving_check_button),
		       dgettext("osso-display", "disp_fi_power_save_mode"));
  
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(touchscreen_keypad_autolock_check_button), 
				 gconf_client_get_bool(gconf_client, GCONF_KEY_TOUCHSCREEN_AUTOLCOK, NULL));
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(inhibit_blank_mode_check_button),
				 gconf_client_get_int(gconf_client, GCONF_KEY_INHIBIT_BLANK_MODE, NULL));
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(touchscreen_vibra_check_button),
				 gconf_client_get_bool(gconf_client, GCONF_KEY_TOUCHSCREEN_VIBRA, NULL));
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(power_saving_check_button),
				 gconf_client_get_bool(gconf_client, GCONF_KEY_POWER_SAVING, NULL));

  hildon_controlbar_set_range(HILDON_CONTROLBAR(brightness_control_bar), 
			      1, 
			      gconf_client_get_int(gconf_client, GCONF_KEY_DISPLAY_BRIGHTNESS_MAX, NULL));
  
  hildon_controlbar_set_value(HILDON_CONTROLBAR(brightness_control_bar), 
			      current_brightness);
  
  int timeout = gconf_client_get_int(gconf_client, GCONF_KEY_DIM_TIMEOUT, NULL);
  GSList* timeouts = gconf_client_get_list(gconf_client, GCONF_KEY_DIM_TIMEOUTS, GCONF_VALUE_INT, NULL);
  if(timeouts)
  {
    GSList* item = timeouts;
    int index = 0;
    while(item)
    {
      int value = GPOINTER_TO_INT(item->data);
      gchar* list_text = NULL;
      if(value < 60)
      {
	const gchar* text = dngettext("osso-display", "disp_va_gene_0", "disp_va_gene_1", value);
	list_text = g_strdup_printf(text, value);
      }
      else
      {
	int min_value = (int)(value/60);
	const gchar* text = dngettext("osso-display", "disp_va_do_2", "disp_va_gene_2", min_value);
	list_text = g_strdup_printf(text, min_value);
      }
      hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR(dim_timeout_selector),
					list_text);
      if(value == timeout)
	hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(dim_timeout_selector), 0, index);
      g_free(list_text);
      item = item->next;
      index++;
    }
  }

  gtk_box_pack_start(GTK_BOX(brightness_box), brightness_label, TRUE, TRUE, 2*HILDON_MARGIN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(brightness_box), GTK_WIDGET(brightness_control_bar), FALSE, FALSE, HILDON_MARGIN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(box), brightness_box, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), display_dim_timeout_picker_button, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), touchscreen_keypad_autolock_check_button, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), inhibit_blank_mode_check_button, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), touchscreen_vibra_check_button, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), power_saving_check_button, TRUE, FALSE, 0);

  // another box, so, the widgets arent scatterd over the
  // whole dialog  
  GtkWidget* box_box = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box_box), box, TRUE, FALSE, 0);
  hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pan), box_box);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), pan, TRUE, TRUE, 0);

  GdkGeometry geometry;
  if(gdk_screen_get_width(gdk_display_get_default_screen(gdk_display_get_default())) < 800) 
  { 
    geometry.min_height = 680; 
    geometry.min_width = 480; 
  } 
  else 
  { 
    geometry.min_height = 360;
    geometry.min_width = 800; 
  } 
  gtk_window_set_geometry_hints(GTK_WINDOW(dialog), 
				dialog, 
				&geometry, 
				GDK_HINT_MIN_SIZE); 
  g_signal_connect(gdk_display_get_default_screen(gdk_display_get_default()), 
		   "size-changed", 
		   G_CALLBACK(_size_changed), dialog); 
  g_signal_connect(brightness_control_bar, "value-changed", G_CALLBACK(_brightness_level_changed), gconf_client);
  gtk_widget_show_all(dialog);
  guint response = gtk_dialog_run(GTK_DIALOG(dialog));
  
  if(response == GTK_RESPONSE_ACCEPT)
  {
    gconf_client_set_bool(gconf_client,
			  GCONF_KEY_TOUCHSCREEN_AUTOLCOK,
			  hildon_check_button_get_active(HILDON_CHECK_BUTTON(touchscreen_keypad_autolock_check_button)),
			  NULL);
    gconf_client_set_int(gconf_client,
			 GCONF_KEY_INHIBIT_BLANK_MODE,
			 hildon_check_button_get_active(HILDON_CHECK_BUTTON(inhibit_blank_mode_check_button)),
			 NULL);
    gconf_client_set_bool(gconf_client,
			  GCONF_KEY_TOUCHSCREEN_VIBRA,
			  hildon_check_button_get_active(HILDON_CHECK_BUTTON(touchscreen_vibra_check_button)),
			  NULL);
    gconf_client_set_bool(gconf_client,
			  GCONF_KEY_POWER_SAVING,
			  hildon_check_button_get_active(HILDON_CHECK_BUTTON(power_saving_check_button)),
			  NULL);
    
    int active_dim_timeout_index = hildon_touch_selector_get_active(HILDON_TOUCH_SELECTOR(dim_timeout_selector), 0);
    GSList* active_item = g_slist_nth(timeouts, active_dim_timeout_index);
    if(active_item)
    {
      int value = GPOINTER_TO_INT(active_item->data);
      gconf_client_set_int(gconf_client,
			   GCONF_KEY_DIM_TIMEOUT,
			   value,
			   NULL);
			 
    }
  }
  else
  {
    if(hildon_controlbar_get_value(HILDON_CONTROLBAR(brightness_control_bar)) != current_brightness)
      gconf_client_set_int(gconf_client, 
			   GCONF_KEY_DISPLAY_BRIGHTNESS, 
			   current_brightness,
			   NULL);
  }
  if(timeouts)
    g_slist_free(timeouts);
  gtk_widget_destroy(dialog);
  g_object_unref(gconf_client);
  return OSSO_OK;
}

osso_return_t save_state(osso_context_t* osso, gpointer user_data)
{
  return OSSO_OK;
}
