#!/usr/bin/env python3
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GdkPixbuf, Gio, GLib
import os
from datetime import datetime
import cairo

class FilePanel:
    def __init__(self, parent):
        self.parent = parent
        self.current_path = os.path.expanduser("~")
        self.selected_item = None
        self.history = [self.current_path]
        self.history_position = 0

        self.panel = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)

        self.toolbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        self.toolbar.set_margin_start(10)
        self.toolbar.set_margin_end(10)
        self.toolbar.set_margin_top(10)
        self.toolbar.set_margin_bottom(5)

        self.back_button = self.create_tool_button("go-previous-symbolic", "Back", self.on_back_clicked)
        self.toolbar.pack_start(self.back_button, False, False, 0)

        self.forward_button = self.create_tool_button("go-next-symbolic", "Forward", self.on_forward_clicked)
        self.toolbar.pack_start(self.forward_button, False, False, 0)

        self.up_button = self.create_tool_button("go-up-symbolic", "Up", self.on_up_clicked)
        self.toolbar.pack_start(self.up_button, False, False, 0)

        self.home_button = self.create_tool_button("go-home-symbolic", "Home", self.on_home_clicked)
        self.toolbar.pack_start(self.home_button, False, False, 0)

        self.refresh_button = self.create_tool_button("view-refresh-symbolic", "Refresh", self.on_refresh_clicked)
        self.toolbar.pack_start(self.refresh_button, False, False, 0)

        self.path_entry = Gtk.Entry()
        self.path_entry.set_text(self.current_path)
        self.path_entry.connect("activate", self.on_path_activated)
        self.toolbar.pack_start(self.path_entry, True, True, 0)

        self.search_entry = Gtk.SearchEntry()
        self.search_entry.set_placeholder_text("Search files...")
        self.search_entry.connect("search-changed", self.on_search_changed)
        self.toolbar.pack_start(self.search_entry, False, False, 0)

        self.panel.pack_start(self.toolbar, False, False, 0)

        self.create_file_view()

        self.statusbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        self.statusbar.set_margin_start(10)
        self.statusbar.set_margin_end(10)
        self.statusbar.set_margin_top(5)
        self.statusbar.set_margin_bottom(10)

        self.item_count_label = Gtk.Label("0 items")
        self.item_count_label.set_halign(Gtk.Align.START)
        self.statusbar.pack_start(self.item_count_label, True, True, 0)

        self.selected_info_label = Gtk.Label("")
        self.selected_info_label.set_halign(Gtk.Align.END)
        self.statusbar.pack_end(self.selected_info_label, False, False, 0)

        self.panel.pack_end(self.statusbar, False, False, 0)

        self.apply_styling()

        self.load_directory()

    def apply_styling(self):
        css_provider = Gtk.CssProvider()
        css = """
            .file-panel {
                background-color: rgba(0, 10, 30, 0.8);
                border-radius: 5px;
                border: 1px solid rgba(0, 191, 255, 0.3);
            }
            .file-panel-toolbar {
                background-color: rgba(0, 30, 60, 0.8);
                border-bottom: 1px solid rgba(0, 191, 255, 0.3);
            }
            .file-panel-statusbar {
                background-color: rgba(0, 30, 60, 0.8);
                border-top: 1px solid rgba(0, 191, 255, 0.3);
                font-size: 10px;
                color: #88DDFF;
            }
            .file-view {
                background-color: rgba(0, 15, 40, 0.7);
                color: white;
            }
            .file-view:selected {
                background-color: rgba(0, 120, 215, 0.8);
            }
            .tool-button {
                background-color: rgba(0, 40, 80, 0.7);
                border-radius: 4px;
                border: 1px solid rgba(0, 191, 255, 0.5);
                padding: 2px;
                transition: all 0.2s ease;
            }
            .tool-button:hover {
                background-color: rgba(0, 70, 130, 0.8);
                border-color: rgba(0, 255, 255, 0.8);
            }
            entry {
                background-color: rgba(0, 20, 50, 0.8);
                color: #00BFFF;
                border: 1px solid rgba(0, 191, 255, 0.5);
                border-radius: 4px;
                padding: 4px 8px;
            }
            entry:focus {
                border-color: rgba(0, 255, 255, 0.8);
            }
        """
        css_provider.load_from_data(css.encode())

        self.panel.get_style_context().add_class("file-panel")
        self.panel.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        self.toolbar.get_style_context().add_class("file-panel-toolbar")
        self.toolbar.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        self.statusbar.get_style_context().add_class("file-panel-statusbar")
        self.statusbar.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        self.path_entry.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        self.search_entry.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

    def create_tool_button(self, icon_name, tooltip, callback):
        button = Gtk.Button()
        button.set_tooltip_text(tooltip)
        button.set_relief(Gtk.ReliefStyle.NONE)
        icon = Gtk.Image.new_from_icon_name(icon_name, Gtk.IconSize.SMALL_TOOLBAR)
        button.add(icon)
        button.connect("clicked", callback)
        button.get_style_context().add_class("tool-button")
        return button

    def create_file_view(self):
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)

        self.store = Gtk.ListStore(GdkPixbuf.Pixbuf, str, str, str, str, str)

        self.treeview = Gtk.TreeView(model=self.store)
        self.treeview.set_headers_visible(True)
        self.treeview.set_enable_search(True)
        self.treeview.set_search_column(1)
        self.treeview.get_style_context().add_class("file-view")

        self.treeview.connect("row-activated", self.on_item_activated)
        selection = self.treeview.get_selection()
        selection.connect("changed", self.on_selection_changed)

        renderer_icon = Gtk.CellRendererPixbuf()
        column_icon = Gtk.TreeViewColumn("", renderer_icon, pixbuf=0)
        column_icon.set_fixed_width(30)
        column_icon.set_sizing(Gtk.TreeViewColumnSizing.FIXED)
        self.treeview.append_column(column_icon)

        renderer_name = Gtk.CellRendererText()
        column_name = Gtk.TreeViewColumn("Name", renderer_name, text=1)
        column_name.set_expand(True)
        column_name.set_sort_column_id(1)
        self.treeview.append_column(column_name)

        renderer_size = Gtk.CellRendererText()
        renderer_size.set_alignment(1.0, 0.5)
        column_size = Gtk.TreeViewColumn("Size", renderer_size, text=2)
        column_size.set_fixed_width(80)
        column_size.set_sizing(Gtk.TreeViewColumnSizing.FIXED)
        column_size.set_alignment(1.0)
        column_size.set_sort_column_id(2)
        self.treeview.append_column(column_size)

        renderer_modified = Gtk.CellRendererText()
        column_modified = Gtk.TreeViewColumn("Modified", renderer_modified, text=3)
        column_modified.set_fixed_width(150)
        column_modified.set_sizing(Gtk.TreeViewColumnSizing.FIXED)
        column_modified.set_sort_column_id(3)
        self.treeview.append_column(column_modified)

        renderer_type = Gtk.CellRendererText()
        column_type = Gtk.TreeViewColumn("Type", renderer_type, text=4)
        column_type.set_fixed_width(100)
        column_type.set_sizing(Gtk.TreeViewColumnSizing.FIXED)
        column_type.set_sort_column_id(4)
        self.treeview.append_column(column_type)

        scrolled.add(self.treeview)
        self.panel.pack_start(scrolled, True, True, 0)

    def load_directory(self, directory=None):
        if directory is not None:
            self.current_path = directory

        self.store.clear()
        self.path_entry.set_text(self.current_path)

        try:
            self.back_button.set_sensitive(self.history_position > 0)
            self.forward_button.set_sensitive(self.history_position < len(self.history) - 1)
            self.up_button.set_sensitive(self.current_path != '/')

            contents = []
            if self.current_path != '/':
                parent_path = os.path.dirname(self.current_path)
                parent_icon = self.get_icon_for_path(parent_path, is_dir=True)
                contents.append((parent_icon, "..", "", "", "Directory", parent_path))

            entries = os.listdir(self.current_path)
            for entry in sorted(entries, key=lambda x: (not os.path.isdir(os.path.join(self.current_path, x)), x.lower())):
                full_path = os.path.join(self.current_path, entry)
                try:
                    stat_info = os.stat(full_path)
                    size = self.format_size(stat_info.st_size) if not os.path.isdir(full_path) else ""
                    modified = datetime.fromtimestamp(stat_info.st_mtime).strftime('%Y-%m-%d %H:%M')
                    if os.path.isdir(full_path):
                        icon = self.get_icon_for_path(full_path, is_dir=True)
                        file_type = "Directory"
                    else:
                        icon = self.get_icon_for_path(full_path)
                        file_type = self.get_file_type(full_path)
                    if not entry.startswith('.'):
                        contents.append((icon, entry, size, modified, file_type, full_path))
                except (PermissionError, FileNotFoundError):
                    pass

            for item in contents:
                self.store.append(item)

            file_count = len(contents) - (1 if self.current_path != '/' else 0)
            self.item_count_label.set_text(f"{file_count} items")

        except (PermissionError, FileNotFoundError) as e:
            error_dialog = Gtk.MessageDialog(
                transient_for=self.parent,
                message_type=Gtk.MessageType.ERROR,
                buttons=Gtk.ButtonsType.OK,
                text="Error accessing directory"
            )
            error_dialog.format_secondary_text(str(e))
            error_dialog.run()
            error_dialog.destroy()
            if self.history_position > 0:
                self.history_position -= 1
                self.current_path = self.history[self.history_position]
                self.load_directory()

    def navigate_to(self, path):
        path = os.path.normpath(path)
        if not os.path.exists(path) or not os.path.isdir(path):
            return False
        if self.current_path != path:
            if self.history_position < len(self.history) - 1:
                self.history = self.history[:self.history_position + 1]
            self.history.append(path)
            self.history_position = len(self.history) - 1
        self.current_path = path
        self.load_directory()
        return True

    def get_icon_for_path(self, path, is_dir=None):
        theme = Gtk.IconTheme.get_default()
        if is_dir is None:
            is_dir = os.path.isdir(path)
        if is_dir:
            if path == os.path.expanduser("~"):
                icon_name = "user-home"
            elif path == "/":
                icon_name = "drive-harddisk"
            else:
                icon_name = "folder"
        else:
            mime_type = Gio.content_type_guess(path, None)[0]
            if mime_type:
                icon_name = Gio.content_type_get_icon(mime_type).to_string().split()[-1]
            else:
                icon_name = "text-x-generic"
        try:
            icon = theme.load_icon(icon_name, 24, 0)
        except:
            try:
                icon = theme.load_icon("text-x-generic", 24, 0)
            except:
                surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 24, 24)
                ctx = cairo.Context(surface)
                ctx.set_source_rgb(0.5, 0.5, 0.5)
                ctx.rectangle(0, 0, 24, 24)
                ctx.fill()
                pixbuf = Gdk.pixbuf_get_from_surface(surface, 0, 0, 24, 24)
                return pixbuf
        return icon

    def get_file_type(self, path):
        mime_type = Gio.content_type_guess(path, None)[0]
        return Gio.content_type_get_description(mime_type) if mime_type else "Unknown"

    def format_size(self, size_bytes):
        if size_bytes < 1024:
            return f"{size_bytes} B"
        elif size_bytes < 1024 * 1024:
            return f"{size_bytes / 1024:.1f} KB"
        elif size_bytes < 1024 * 1024 * 1024:
            return f"{size_bytes / (1024 * 1024):.1f} MB"
        else:
            return f"{size_bytes / (1024 * 1024 * 1024):.1f} GB"

    def on_back_clicked(self, button):
        if self.history_position > 0:
            self.history_position -= 1
            self.current_path = self.history[self.history_position]
            self.load_directory()

    def on_forward_clicked(self, button):
        if self.history_position < len(self.history) - 1:
            self.history_position += 1
            self.current_path = self.history[self.history_position]
            self.load_directory()

    def on_up_clicked(self, button):
        if self.current_path != '/':
            parent_path = os.path.dirname(self.current_path)
            self.navigate_to(parent_path)

    def on_home_clicked(self, button):
        home_path = os.path.expanduser("~")
        self.navigate_to(home_path)

    def on_refresh_clicked(self, button):
        self.load_directory()

    def on_path_activated(self, entry):
        path = entry.get_text()
        if not self.navigate_to(path):
            entry.set_text(self.current_path)

    def on_item_activated(self, treeview, path, column):
        model = treeview.get_model()
        full_path = model[path][5]
        if os.path.isdir(full_path):
            self.navigate_to(full_path)
        else:
            try:
                Gtk.show_uri_on_window(None, f"file://{full_path}", Gdk.CURRENT_TIME)
            except GLib.Error as e:
                print(f"Error opening file: {e}")

    def on_search_changed(self, entry):
        search_text = entry.get_text().lower()
        if not search_text:
            self.load_directory()
            return
        self.store.clear()
        try:
            entries = os.listdir(self.current_path)
            for entry in entries:
                if search_text in entry.lower():
                    full_path = os.path.join(self.current_path, entry)
                    try:
                        stat_info = os.stat(full_path)
                        size = self.format_size(stat_info.st_size) if not os.path.isdir(full_path) else ""
                        modified = datetime.fromtimestamp(stat_info.st_mtime).strftime('%Y-%m-%d %H:%M')
                        if os.path.isdir(full_path):
                            icon = self.get_icon_for_path(full_path, is_dir=True)
                            file_type = "Directory"
                        else:
                            icon = self.get_icon_for_path(full_path)
                            file_type = self.get_file_type(full_path)
                        self.store.append((icon, entry, size, modified, file_type, full_path))
                    except (PermissionError, FileNotFoundError):
                        pass
            result_count = len(self.store)
            self.item_count_label.set_text(f"{result_count} results")
        except (PermissionError, FileNotFoundError) as e:
            print(f"Error searching directory: {e}")
    def on_selection_changed(self, selection):
        model, treeiter = selection.get_selected()
        if treeiter is not None:
            self.selected_item = model[treeiter][5]  # Full path is in column 5
            name = model[treeiter][1]
            size = model[treeiter][2]
            modified = model[treeiter][3]
            file_type = model[treeiter][4]
            
            if file_type == "Directory":
                self.selected_info_label.set_text(f"Directory: {name}")
            else:
                self.selected_info_label.set_text(f"{name} - {size}")
        else:
            self.selected_item = None
            self.selected_info_label.set_text("")