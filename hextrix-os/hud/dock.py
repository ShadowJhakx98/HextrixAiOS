#!/usr/bin/env python3
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('GdkPixbuf', '2.0')
from gi.repository import Gtk, GdkPixbuf, Gdk, Gio, Pango
import os
import json
import subprocess

class DockCategory:
    def __init__(self, name, icon):
        self.name = name
        self.icon = icon
        self.expanded = False
        self.items = []

    def add_item(self, item):
        self.items.append(item)

    def remove_item(self, item):
        if item in self.items:
            self.items.remove(item)

class DockItem:
    def __init__(self, name, icon_path, command, desktop_id, category):
        self.name = name
        self.icon_path = icon_path
        self.command = command
        self.desktop_id = desktop_id
        self.category = category
        if icon_path and os.path.exists(icon_path):
            self.pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_size(icon_path, 48, 48)
        else:
            self.pixbuf = None

    def to_dict(self):
        return {
            "name": self.name,
            "icon_path": self.icon_path,
            "command": self.command,
            "desktop_id": self.desktop_id,
            "category": self.category
        }

    @classmethod
    def from_dict(cls, data):
        return cls(
            data["name"],
            data["icon_path"],
            data["command"],
            data["desktop_id"],
            data["category"]
        )

class DockManager:
    def __init__(self, parent):
        self.parent = parent
        self.dock_items = []
        
        # Set up dock categories
        self.categories = {
            "Favorites": DockCategory("Favorites", "starred"),
            "Internet": DockCategory("Internet", "web-browser"),
            "Development": DockCategory("Development", "applications-development"),
            "Office": DockCategory("Office", "applications-office"),
            "Multimedia": DockCategory("Multimedia", "applications-multimedia"),
            "System": DockCategory("System", "applications-system"),
            "Utilities": DockCategory("Utilities", "applications-utilities"),
            "Games": DockCategory("Games", "applications-games"),
            "Other": DockCategory("Other", "applications-other")
        }
        
        # Create UI elements
        self.dock_revealer = Gtk.Revealer()
        self.dock_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_UP)
        self.dock_revealer.set_transition_duration(300)
        
        self.dock_scrolled = Gtk.ScrolledWindow()
        self.dock_scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.NEVER)
        self.dock_scrolled.set_min_content_height(150)
        self.dock_scrolled.set_min_content_width(800)
        self.dock_scrolled.set_max_content_height(150)
        
        self.dock = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.dock.set_halign(Gtk.Align.CENTER)
        self.dock.set_margin_start(15)
        self.dock.set_margin_end(15)
        self.dock.set_margin_bottom(15)
        self.dock.set_margin_top(15)
        
        self.dock_scrolled.add(self.dock)
        self.dock_revealer.add(self.dock_scrolled)
        
        # Setup drag and drop
        self.setup_dock_dnd()
        
        # User config directory
        self.config_dir = os.path.expanduser("~/.config/hextrix")
        os.makedirs(self.config_dir, exist_ok=True)
        self.dock_config_file = os.path.join(self.config_dir, "dock_config.json")
        
        # Load saved dock configuration or discover apps
        if os.path.exists(self.dock_config_file):
            self.load_dock_config()
        else:
            self.discover_applications()
            self.save_dock_config()
        
        # Create dock categories
        self.populate_dock()

    def get_dock_widget(self):
        """Return the main dock widget to be added to an application"""
        return self.dock_revealer

    def show_dock(self, show=True):
        """Show or hide the dock"""
        self.dock_revealer.set_reveal_child(show)

    def toggle_dock(self):
        """Toggle dock visibility"""
        currently_visible = self.dock_revealer.get_reveal_child()
        self.dock_revealer.set_reveal_child(not currently_visible)

    def setup_dock_dnd(self):
        self.dock.drag_dest_set(
            Gtk.DestDefaults.ALL,
            [],
            Gdk.DragAction.MOVE
        )
        target_entry = Gtk.TargetEntry.new("application/x-dock-item", Gtk.TargetFlags.SAME_APP, 0)
        self.dock.drag_dest_set_target_list([target_entry])
        self.dock.connect("drag-data-received", self.on_dock_drag_data_received)

    def create_dock_button(self, dock_item, category=None):
        button_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        button_box.set_halign(Gtk.Align.CENTER)
        
        button = Gtk.Button()
        button.set_tooltip_text(dock_item.name)
        button.get_style_context().add_class("dock-button")
        
        if dock_item.pixbuf:
            image = Gtk.Image.new_from_pixbuf(dock_item.pixbuf)
        else:
            image = Gtk.Image.new_from_icon_name("application-x-executable", Gtk.IconSize.DIALOG)
        
        button.add(image)
        button.connect("clicked", self.on_dock_item_clicked, dock_item)
        
        button.drag_source_set(
            Gdk.ModifierType.BUTTON1_MASK,
            [],
            Gdk.DragAction.MOVE
        )
        target_entry = Gtk.TargetEntry.new("application/x-dock-item", Gtk.TargetFlags.SAME_APP, 0)
        button.drag_source_set_target_list([target_entry])
        button.connect("drag-begin", self.on_dock_drag_begin, dock_item)
        button.connect("drag-data-get", self.on_dock_drag_data_get, dock_item)
        
        label = Gtk.Label(dock_item.name)
        label.set_max_width_chars(10)
        label.set_ellipsize(Pango.EllipsizeMode.END)
        label.set_margin_top(2)
        label.get_style_context().add_class("dock-label")
        
        button_box.pack_start(button, False, False, 0)
        button_box.pack_start(label, False, False, 0)
        
        css_provider = Gtk.CssProvider()
        css = """
            .dock-button {
                border-radius: 10px;
                background-color: rgba(0, 30, 60, 0.7);
                border: 1px solid #00BFFF;
                padding: 5px;
                transition: all 0.3s ease;
            }
            .dock-button:hover {
                background-color: rgba(0, 70, 130, 0.9);
                border: 1px solid #00FFFF;
                box-shadow: 0 0 10px rgba(0, 191, 255, 0.5);
            }
            .dock-label {
                color: #00BFFF;
                font-size: 10px;
            }
        """
        css_provider.load_from_data(css.encode())
        button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        label.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        return button_box

    def create_category_section(self, category):
        category_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        category_box.set_margin_start(10)
        category_box.set_margin_end(10)
        
        header_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        header_box.get_style_context().add_class("category-header")
        
        expander = Gtk.Button()
        expander.get_style_context().add_class("expander-button")
        expander.set_relief(Gtk.ReliefStyle.NONE)
        
        expand_icon = Gtk.Image.new_from_icon_name(
            "pan-down-symbolic" if category.expanded else "pan-end-symbolic",
            Gtk.IconSize.SMALL_TOOLBAR
        )
        expander.add(expand_icon)
        expander.connect("clicked", self.on_category_expanded, category, expand_icon)
        
        icon = Gtk.Image.new_from_icon_name(category.icon, Gtk.IconSize.MENU)
        label = Gtk.Label(category.name)
        label.get_style_context().add_class("category-label")
        
        header_box.pack_start(expander, False, False, 0)
        header_box.pack_start(icon, False, False, 0)
        header_box.pack_start(label, False, False, 0)
        
        items_box = Gtk.FlowBox()
        items_box.set_selection_mode(Gtk.SelectionMode.NONE)
        items_box.set_max_children_per_line(10)
        items_box.set_min_children_per_line(1)
        items_box.set_homogeneous(True)
        
        items_box.drag_dest_set(
            Gtk.DestDefaults.ALL,
            [],
            Gdk.DragAction.MOVE
        )
        target_entry = Gtk.TargetEntry.new("application/x-dock-item", Gtk.TargetFlags.SAME_APP, 0)
        items_box.drag_dest_set_target_list([target_entry])
        items_box.connect("drag-data-received", self.on_category_drag_data_received, category)
        
        for item in category.items:
            button = self.create_dock_button(item, category)
            child = Gtk.FlowBoxChild()
            child.add(button)
            items_box.add(child)
        
        category_box.pack_start(header_box, False, False, 0)
        
        revealer = Gtk.Revealer()
        revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_DOWN)
        revealer.set_transition_duration(300)
        revealer.add(items_box)
        revealer.set_reveal_child(category.expanded)
        
        category_box.pack_start(revealer, False, False, 0)
        
        css_provider = Gtk.CssProvider()
        css = """
            .category-header {
                background-color: rgba(0, 40, 80, 0.8);
                border-radius: 5px;
                padding: 5px;
                margin: 2px;
            }
            .category-label {
                color: #00BFFF;
                font-weight: bold;
                font-size: 12px;
                margin-left: 2px;
            }
            .expander-button {
                padding: 0px;
                margin: 0px;
                min-height: 16px;
                min-width: 16px;
            }
        """
        css_provider.load_from_data(css.encode())
        header_box.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        label.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        return category_box, revealer, items_box

    def populate_dock(self):
        """Create and populate the dock UI with categories and applications"""
        for child in self.dock.get_children():
            self.dock.remove(child)
        
        for category_id, category in self.categories.items():
            if category.items:
                category_box, revealer, items_box = self.create_category_section(category)
                self.dock.pack_start(category_box, False, False, 5)
        
        self.dock.show_all()

    def discover_applications(self):
        """Find installed applications on the system"""
        self.dock_items = []
        
        # Add default favorites
        default_favorites = [
            ("Terminal", "utilities-terminal", "gnome-terminal", "Favorites"),
            ("Files", "system-file-manager", "nautilus", "Favorites"),
            ("Firefox", "firefox", "firefox", "Favorites")
        ]
        
        for name, icon, cmd, category in default_favorites:
            theme = Gtk.IconTheme.get_default()
            icon_path = ""
            try:
                icon_info = theme.lookup_icon(icon, 48, 0)
                if icon_info:
                    icon_path = icon_info.get_filename()
            except:
                icon_path = icon
            
            item = DockItem(name, icon_path, cmd, None, category)
            self.dock_items.append(item)
            if category in self.categories:
                self.categories[category].add_item(item)
        
        # Find installed applications
        app_system = Gio.AppInfo.get_all()
        
        for app_info in app_system:
            if not app_info.should_show():
                continue
            
            name = app_info.get_display_name()
            command = app_info.get_commandline()
            desktop_id = app_info.get_id()
            
            if not command:
                continue
                
            categories = app_info.get_categories()
            category = "Other"
            
            if categories:
                categories = categories.split(";")
                if "Network" in categories or "WebBrowser" in categories:
                    category = "Internet"
                elif "Development" in categories:
                    category = "Development"
                elif "Office" in categories:
                    category = "Office"
                elif "AudioVideo" in categories or "Audio" in categories or "Video" in categories:
                    category = "Multimedia"
                elif "System" in categories:
                    category = "System"
                elif "Utility" in categories:
                    category = "Utilities"
                elif "Game" in categories:
                    category = "Games"
            
            icon = app_info.get_icon()
            icon_path = ""
            
            if icon:
                theme = Gtk.IconTheme.get_default()
                try:
                    icon_info = theme.lookup_icon(icon.to_string(), 48, 0)
                    if icon_info:
                        icon_path = icon_info.get_filename()
                except:
                    try:
                        icon_path = icon.to_string()
                    except:
                        icon_path = "application-x-executable"
            
            # Check for duplicates
            duplicate = False
            for item in self.dock_items:
                if item.command == command:
                    duplicate = True
                    break
            
            if duplicate:
                continue
                
            item = DockItem(name, icon_path, command, desktop_id, category)
            
            # Don't add to favorites if already there
            in_favorites = False
            for fav in self.categories["Favorites"].items:
                if fav.command == command:
                    in_favorites = True
                    break
                    
            if not in_favorites:
                self.dock_items.append(item)
                if category in self.categories:
                    self.categories[category].add_item(item)
        
        # Expand favorites by default
        self.categories["Favorites"].expanded = True

    def save_dock_config(self):
        """Save dock configuration to file"""
        config = {
            "items": [item.to_dict() for item in self.dock_items],
            "categories": {
                name: {
                    "expanded": category.expanded,
                    "items": [self.dock_items.index(item) for item in category.items]
                } for name, category in self.categories.items()
            }
        }
        with open(self.dock_config_file, 'w') as f:
            json.dump(config, f, indent=2)

    def load_dock_config(self):
        """Load dock configuration from file"""
        try:
            with open(self.dock_config_file, 'r') as f:
                config = json.load(f)
            
            self.dock_items = [DockItem.from_dict(item_data) for item_data in config["items"]]
            
            for category in self.categories.values():
                category.items = []
            
            for name, category_data in config["categories"].items():
                if name in self.categories:
                    self.categories[name].expanded = category_data["expanded"]
                    for item_index in category_data["items"]:
                        if 0 <= item_index < len(self.dock_items):
                            self.categories[name].add_item(self.dock_items[item_index])
        except Exception as e:
            print(f"Error loading dock configuration: {e}")
            self.discover_applications()

    def on_dock_item_clicked(self, button, dock_item):
        """Launch application when dock item is clicked"""
        print(f"Launching: {dock_item.name} ({dock_item.command})")
        try:
            subprocess.Popen(dock_item.command.split())
        except Exception as e:
            print(f"Error launching {dock_item.command}: {e}")

    def on_category_expanded(self, button, category, icon):
        """Handle category expand/collapse"""
        category.expanded = not category.expanded
        icon.set_from_icon_name(
            "pan-down-symbolic" if category.expanded else "pan-end-symbolic",
            Gtk.IconSize.SMALL_TOOLBAR
        )
        parent = button.get_parent().get_parent()
        revealer = None
        for child in parent.get_children():
            if isinstance(child, Gtk.Revealer):
                revealer = child
                break
        if revealer:
            revealer.set_reveal_child(category.expanded)
        self.save_dock_config()

    def on_dock_drag_begin(self, button, context, dock_item):
        """Set drag icon when drag begins"""
        if dock_item.pixbuf:
            Gtk.drag_set_icon_pixbuf(context, dock_item.pixbuf, 0, 0)
        else:
            Gtk.drag_set_icon_name(context, "application-x-executable", 0, 0)

    def on_dock_drag_data_get(self, button, context, selection, target_id, time, dock_item):
        """Provide data for drag operation"""
        index = self.dock_items.index(dock_item)
        selection.set(selection.get_target(), 8, str(index).encode())

    def on_dock_drag_data_received(self, widget, context, x, y, selection, target_id, time):
        """Handle drop onto dock area (add to favorites)"""
        index = int(selection.get_data().decode())
        if 0 <= index < len(self.dock_items):
            item = self.dock_items[index]
            if item not in self.categories["Favorites"].items:
                for category in self.categories.values():
                    if item in category.items:
                        category.remove_item(item)
                self.categories["Favorites"].add_item(item)
                self.populate_dock()
                self.save_dock_config()

    def on_category_drag_data_received(self, widget, context, x, y, selection, target_id, time, category):
        """Handle drop onto category"""
        index = int(selection.get_data().decode())
        if 0 <= index < len(self.dock_items):
            item = self.dock_items[index]
            for cat in self.categories.values():
                if item in cat.items:
                    cat.remove_item(item)
            category.add_item(item)
            self.populate_dock()
            self.save_dock_config()