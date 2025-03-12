#!/usr/bin/env python3
# app_drawer.py - Hextrix OS App Drawer Module
# Place this file in the same directory as hextrix-hud2.py or in your Python path

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango, GdkPixbuf, Gio
import os
import subprocess
import math
import cairo

class AppDrawer:
    """App Drawer component for Hextrix OS
    
    This class provides a searchable application drawer UI component
    that displays all available applications in a grid layout.
    
    Usage:
        # Create an instance with parent window reference and application items
        app_drawer = AppDrawer(parent_window, dock_items)
        
        # Add the drawer to a container
        container.pack_start(app_drawer.drawer, False, False, 0)
        
        # Use the toggle method to show/hide the drawer
        app_drawer.toggle()
    """
    
    def __init__(self, parent, dock_items, keybind=None):
        """Initialize the App Drawer with parent reference and application items
        
        Args:
            parent: Parent window or widget reference
            dock_items: List of application objects with name, command, and icon properties
            keybind: Optional key to use for launching apps (default: None - double-click only)
        """
        self.parent = parent
        self.dock_items = dock_items
        self.filtered_items = []
        self.search_text = ""
        self.keybind = keybind
        
        # Create the main drawer container
        self.drawer = Gtk.Revealer()
        self.drawer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_DOWN)
        self.drawer.set_transition_duration(300)
        
        # Create the content box with transparency
        self.content = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        
        # Apply styling
        self.content.get_style_context().add_class("app-drawer")
        self.drawer.add(self.content)
        
        # Header with search
        self.header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.header.set_margin_start(20)
        self.header.set_margin_end(20)
        self.header.set_margin_top(15)
        self.header.set_margin_bottom(10)
        
        # Title
        self.title = Gtk.Label("Applications")
        self.title.set_markup("<span font='16' weight='bold' color='#00BFFF'>Applications</span>")
        self.title.set_halign(Gtk.Align.START)
        self.header.pack_start(self.title, True, True, 0)
        
        # Search entry
        self.search_entry = Gtk.SearchEntry()
        self.search_entry.set_placeholder_text("Search applications...")
        self.search_entry.set_size_request(250, -1)
        self.search_entry.connect("search-changed", self.on_search_changed)
        self.header.pack_end(self.search_entry, False, False, 0)
        
        # Close button
        self.close_button = Gtk.Button()
        self.close_button.set_tooltip_text("Close App Drawer")
        close_icon = Gtk.Image.new_from_icon_name("window-close-symbolic", Gtk.IconSize.BUTTON)
        self.close_button.add(close_icon)
        self.close_button.connect("clicked", self.hide)
        self.close_button.get_style_context().add_class("drawer-close-button")
        self.header.pack_end(self.close_button, False, False, 0)
        
        self.content.pack_start(self.header, False, False, 0)
        
        # Create scrolled window for apps
        self.scrolled = Gtk.ScrolledWindow()
        self.scrolled.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        self.scrolled.set_min_content_height(400)
        self.scrolled.set_max_content_height(500)
        
        # Apps container (FlowBox)
        self.apps_container = Gtk.FlowBox()
        self.apps_container.set_valign(Gtk.Align.START)
        self.apps_container.set_max_children_per_line(8)
        self.apps_container.set_min_children_per_line(3)
        self.apps_container.set_selection_mode(Gtk.SelectionMode.NONE)
        self.apps_container.set_homogeneous(True)
        self.apps_container.set_row_spacing(20)
        self.apps_container.set_column_spacing(10)
        self.apps_container.set_margin_start(20)
        self.apps_container.set_margin_end(20)
        self.apps_container.set_margin_top(10)
        self.apps_container.set_margin_bottom(20)
        
        self.scrolled.add(self.apps_container)
        self.content.pack_start(self.scrolled, True, True, 0)
        
        # Footer
        self.footer = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.footer.set_margin_start(20)
        self.footer.set_margin_end(20)
        self.footer.set_margin_top(10)
        self.footer.set_margin_bottom(15)
        
        # App count
        self.app_count_label = Gtk.Label()
        self.footer.pack_start(self.app_count_label, True, True, 0)
        
        # Settings button
        self.settings_button = Gtk.Button(label="Settings")
        settings_icon = Gtk.Image.new_from_icon_name("preferences-system-symbolic", Gtk.IconSize.BUTTON)
        self.settings_button.set_image(settings_icon)
        self.settings_button.set_always_show_image(True)
        self.settings_button.connect("clicked", self.on_settings_clicked)
        self.footer.pack_end(self.settings_button, False, False, 0)
        
        # Add to favorites button
        self.favorites_button = Gtk.Button(label="Add to Dock")
        favorites_icon = Gtk.Image.new_from_icon_name("list-add-symbolic", Gtk.IconSize.BUTTON)
        self.favorites_button.set_image(favorites_icon)
        self.favorites_button.set_always_show_image(True)
        self.favorites_button.set_sensitive(False)  # Initially disabled
        self.favorites_button.connect("clicked", self.on_add_to_favorites)
        self.footer.pack_end(self.favorites_button, False, False, 10)
        
        self.content.pack_end(self.footer, False, False, 0)
        
        # Currently selected app (for favorites)
        self.selected_app = None
        
        # Apply styling
        self.apply_styling()
        
        # Populate apps
        self.populate_apps()
        
        # Connect key events
        self.content.connect("key-press-event", self.on_key_press)
        
        # Register in the parent's app widgets list
        if hasattr(parent, 'app_widgets'):
            parent.app_widgets.append(self.drawer)

    def apply_styling(self):
        """Apply CSS styling to the app drawer components"""
        css_provider = Gtk.CssProvider()
        css = """
            .app-drawer {
                background-color: rgba(0, 15, 40, 0.95);
                border-radius: 15px;
                border: 1px solid rgba(0, 150, 255, 0.5);
                box-shadow: 0 10px 30px rgba(0, 0, 0, 0.8);
                margin: 50px;
            }
            
            .app-item {
                background-color: rgba(0, 30, 60, 0.7);
                border-radius: 10px;
                border: 1px solid rgba(0, 150, 255, 0.4);
                padding: 10px;
                transition: all 250ms ease-in-out;
            }
            
            .app-item:hover {
                background-color: rgba(0, 50, 90, 0.8);
                border-color: rgba(0, 180, 255, 0.6);
                box-shadow: 0 0 15px rgba(0, 150, 255, 0.4);
            }
            
            .app-item.selected {
                background-color: rgba(0, 80, 120, 0.9);
                border-color: rgba(0, 220, 255, 0.8);
                box-shadow: 0 0 20px rgba(0, 180, 255, 0.6);
            }
            
            .app-label {
                color: #FFFFFF;
                font-size: 12px;
                margin-top: 5px;
            }
            
            .drawer-close-button {
                background-color: rgba(60, 60, 80, 0.7);
                border-radius: 50%;
                border: 1px solid rgba(150, 150, 200, 0.3);
                padding: 2px;
            }
            
            .drawer-close-button:hover {
                background-color: rgba(80, 80, 100, 0.8);
                border-color: rgba(180, 180, 220, 0.5);
            }
            
            entry {
                background-color: rgba(0, 20, 50, 0.8);
                color: #00BFFF;
                border: 1px solid rgba(0, 150, 255, 0.5);
                border-radius: 20px;
                padding: 6px 12px;
            }
            
            entry:focus {
                border-color: rgba(0, 200, 255, 0.8);
                box-shadow: 0 0 10px rgba(0, 150, 255, 0.4);
            }
            
            button {
                background-color: rgba(0, 40, 80, 0.7);
                color: #FFFFFF;
                border-radius: 5px;
                border: 1px solid rgba(0, 150, 255, 0.4);
                padding: 5px 10px;
                transition: all 250ms ease-in-out;
            }
            
            button:hover {
                background-color: rgba(0, 60, 110, 0.8);
                border-color: rgba(0, 180, 255, 0.6);
            }
            
            button:disabled {
                background-color: rgba(30, 30, 50, 0.5);
                color: rgba(150, 150, 150, 0.5);
                border-color: rgba(80, 80, 100, 0.3);
            }
            
            scrollbar {
                background-color: rgba(0, 10, 30, 0.5);
                border-radius: 10px;
                border: none;
            }
            
            scrollbar slider {
                background-color: rgba(0, 150, 255, 0.3);
                border-radius: 10px;
                min-width: 6px;
                min-height: 6px;
            }
            
            scrollbar slider:hover {
                background-color: rgba(0, 180, 255, 0.5);
            }
        """
        css_provider.load_from_data(css.encode())
        
        self.content.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        self.search_entry.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        self.close_button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        self.settings_button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        self.favorites_button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
    
    def create_app_button(self, app_item):
        """Create a button for an app in the drawer
        
        Args:
            app_item: Application item object with name, command, and icon
            
        Returns:
            Gtk.Box: Container with app button and label
        """
        button_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        button_box.set_halign(Gtk.Align.CENTER)
        button_box.get_style_context().add_class("app-item")
        
        # Store the app item as data for later reference
        button_box.app_item = app_item
        
        # Make the entire box clickable
        event_box = Gtk.EventBox()
        event_box.add(button_box)
        event_box.connect("button-press-event", self.on_app_box_clicked, button_box)
        
        # Button with icon
        button = Gtk.Button()
        button.set_relief(Gtk.ReliefStyle.NONE)
        button.set_tooltip_text(app_item.name)
        
        if hasattr(app_item, 'pixbuf') and app_item.pixbuf:
            # Create a scaled version of the pixbuf for consistency
            scaled_pixbuf = app_item.pixbuf.scale_simple(48, 48, GdkPixbuf.InterpType.BILINEAR)
            image = Gtk.Image.new_from_pixbuf(scaled_pixbuf)
        else:
            image = Gtk.Image.new_from_icon_name("application-x-executable", Gtk.IconSize.DIALOG)
        
        button.add(image)
        button.connect("clicked", self.on_app_clicked, app_item, button_box)
        
        # Label with app name
        label = Gtk.Label(app_item.name)
        label.set_ellipsize(Pango.EllipsizeMode.END)
        label.set_max_width_chars(12)
        label.set_lines(2)
        label.set_line_wrap(True)
        label.set_justify(Gtk.Justification.CENTER)
        label.get_style_context().add_class("app-label")
        
        button_box.pack_start(button, False, False, 0)
        button_box.pack_start(label, False, False, 0)
        
        return event_box
    
    def populate_apps(self):
        """Fill the app drawer with application icons"""
        # Clear existing items
        for child in self.apps_container.get_children():
            self.apps_container.remove(child)
        
        # Sort items by name
        items_to_show = self.filtered_items if self.search_text else self.dock_items
        sorted_items = sorted(items_to_show, key=lambda x: x.name.lower())
        
        # Add items to the container
        for item in sorted_items:
            app_button = self.create_app_button(item)
            self.apps_container.add(app_button)
        
        # Update app count
        app_count = len(sorted_items)
        count_text = f"{app_count} application{'s' if app_count != 1 else ''} available"
        self.app_count_label.set_markup(f"<span color='#88CCFF'>{count_text}</span>")
        
        # Show everything
        self.apps_container.show_all()
        
        # Reset selected app
        self.selected_app = None
        self.favorites_button.set_sensitive(False)
    
    def on_search_changed(self, entry):
        """Filter apps based on search text
        
        Args:
            entry: The search entry widget
        """
        self.search_text = entry.get_text().lower()
        if self.search_text:
            self.filtered_items = [
                item for item in self.dock_items 
                if self.search_text in item.name.lower() or 
                   (hasattr(item, 'command') and item.command and 
                    self.search_text in item.command.lower())
            ]
        else:
            self.filtered_items = []
        
        self.populate_apps()
    
    def on_app_box_clicked(self, event_box, event, button_box):
        """Handle click on app item to select it
        
        Args:
            event_box: The event box that received the click
            event: The event data
            button_box: The app's container box
        """
        # Clear previous selection
        for child in self.apps_container.get_children():
            box = child.get_child()
            box.get_style_context().remove_class("selected")
        
        # Set new selection
        button_box.get_style_context().add_class("selected")
        self.selected_app = button_box.app_item
        self.favorites_button.set_sensitive(True)
        
        # Double-click to launch
        if event.type == Gdk.EventType._2BUTTON_PRESS:
            self.launch_app(self.selected_app)
    
    def on_app_clicked(self, button, app_item, button_box):
        """Launch the selected application
        
        Args:
            button: The button widget
            app_item: The application item
            button_box: The app's container box
        """
        # Update selection
        for child in self.apps_container.get_children():
            box = child.get_child()
            box.get_style_context().remove_class("selected")
        
        button_box.get_style_context().add_class("selected")
        self.selected_app = app_item
        self.favorites_button.set_sensitive(True)
        
        # Launch the app
        self.launch_app(app_item)
    
    def launch_app(self, app_item):
        """Launch an application
        
        Args:
            app_item: The application item to launch
        """
        print(f"Launching: {app_item.name} ({app_item.command})")
        try:
            subprocess.Popen(app_item.command.split())
            # Optionally hide the drawer after launching an app
            GLib.timeout_add(300, self.hide)
        except Exception as e:
            print(f"Error launching {app_item.command}: {e}")
            # Could show an error message to the user
    
    def on_settings_clicked(self, button):
        """Open system settings
        
        Args:
            button: The button widget
        """
        try:
            subprocess.Popen(["gnome-control-center"])
        except Exception as e:
            print(f"Error opening settings: {e}")
    
    def on_add_to_favorites(self, button):
        """Add selected app to dock favorites
        
        Args:
            button: The button widget
        """
        if not self.selected_app:
            return
            
        if hasattr(self.parent, 'add_to_favorites') and callable(getattr(self.parent, 'add_to_favorites')):
            # Call the parent's add_to_favorites method if available
            self.parent.add_to_favorites(self.selected_app)
        elif hasattr(self.parent, 'categories') and 'Favorites' in self.parent.categories:
            # Direct access to parent's categories
            for category in self.parent.categories.values():
                if self.selected_app in category.items:
                    category.remove_item(self.selected_app)
            
            self.parent.categories['Favorites'].add_item(self.selected_app)
            
            # Refresh the dock if possible
            if hasattr(self.parent, 'populate_dock') and callable(getattr(self.parent, 'populate_dock')):
                self.parent.populate_dock()
            
            # Save config if possible
            if hasattr(self.parent, 'save_dock_config') and callable(getattr(self.parent, 'save_dock_config')):
                self.parent.save_dock_config()
        
        # Show feedback and reset selection
        self.app_count_label.set_markup(f"<span color='#00FF88'>Added {self.selected_app.name} to Dock</span>")
        GLib.timeout_add(2000, self.reset_status_message)
    
    def reset_status_message(self):
        """Reset the status message in the footer"""
        items_to_show = self.filtered_items if self.search_text else self.dock_items
        app_count = len(items_to_show)
        count_text = f"{app_count} application{'s' if app_count != 1 else ''} available"
        self.app_count_label.set_markup(f"<span color='#88CCFF'>{count_text}</span>")
        return False
    
    def on_key_press(self, widget, event):
        """Handle keyboard events
        
        Args:
            widget: The widget that received the event
            event: The key event
            
        Returns:
            bool: True if the event was handled
        """
        keyval = event.keyval
        
        if keyval == Gdk.KEY_Escape:
            self.hide()
            return True
            
        # Only use custom keybind if configured
        if self.keybind is not None and keyval == self.keybind and self.selected_app:
            self.launch_app(self.selected_app)
            return True
            
        return False
    
    def show(self, widget=None):
        """Show the app drawer"""
        self.drawer.set_reveal_child(True)
        # Clear any previous search and selection
        self.search_entry.set_text("")
        self.search_text = ""
        self.filtered_items = []
        self.selected_app = None
        self.favorites_button.set_sensitive(False)
        self.populate_apps()
        # Set focus to search
        self.search_entry.grab_focus()
    
    def hide(self, widget=None):
        """Hide the app drawer"""
        self.drawer.set_reveal_child(False)
    
    def toggle(self, widget=None):
        """Toggle the visibility of the app drawer"""
        if self.drawer.get_reveal_child():
            self.hide()
        else:
            self.show()
            
    def get_drawer(self):
        """Get the drawer widget for adding to containers
        
        Returns:
            Gtk.Revealer: The drawer widget
        """
        return self.drawer