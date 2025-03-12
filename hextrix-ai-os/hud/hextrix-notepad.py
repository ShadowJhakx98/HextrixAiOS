#!/usr/bin/env python3
# Hextrix AI OS - Advanced Notepad Application

import gi
import os
import re
import subprocess
import json
import math
import time
from pathlib import Path

gi.require_version('Gtk', '4.0')
from gi.repository import Gtk, Gdk, GLib, Pango, Gio, GObject


class HextrixNotepad(Gtk.ApplicationWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.set_title("Hextrix Notepad")
        self.set_default_size(1000, 700)
        
        # Set up CSS styling
        self.setup_css()
        
        # Main container
        self.main_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.set_child(self.main_container)
        
        # Create headerbar
        self.setup_headerbar()
        
        # Create main layout
        self.setup_main_layout()
        
        # Initialize file state
        self.current_file = None
        self.modified = False
        self.update_title()
        
        # Start the pulsing effect
        GLib.timeout_add(50, self.pulse_effect)
        
    def setup_css(self):
        """Set up custom CSS styling for the application"""
        css_provider = Gtk.CssProvider()
        css = """
            window {
                background-color: rgba(0, 10, 20, 0.95);
            }
            
            headerbar {
                background-color: rgba(0, 15, 30, 0.9);
                border-bottom: 1px solid rgba(0, 191, 255, 0.5);
            }
            
            button {
                background-color: rgba(0, 40, 80, 0.7);
                border-radius: 4px;
                border: 1px solid rgba(0, 191, 255, 0.5);
                color: #00BFFF;
                transition: all 200ms ease;
                min-height: 26px;
                min-width: 26px;
                padding: 4px;
                margin: 2px;
            }
            
            button:hover {
                background-color: rgba(0, 70, 130, 0.8);
                border-color: rgba(0, 255, 255, 0.8);
            }
            
            .sidebar {
                background-color: rgba(0, 10, 30, 0.85);
                border-right: 1px solid rgba(0, 191, 255, 0.3);
            }
            
            treeview {
                background-color: transparent;
                color: #00BFFF;
            }
            
            treeview:selected {
                background-color: rgba(0, 100, 200, 0.3);
            }
            
            textview {
                background-color: rgba(0, 5, 15, 0.9);
                color: #FFFFFF;
            }
            
            textview text {
                background-color: rgba(0, 5, 15, 0.9);
                color: #FFFFFF;
            }
            
            label.statusbar {
                background-color: rgba(0, 20, 40, 0.8);
                border-top: 1px solid rgba(0, 191, 255, 0.3);
                color: #00BFFF;
                padding: 2px 5px;
            }
            
            box.search-bar {
                background-color: rgba(0, 30, 60, 0.8);
                border: 1px solid rgba(0, 191, 255, 0.5);
                border-radius: 4px;
                padding: 5px;
                margin: 5px;
            }
            
            entry {
                background-color: rgba(0, 20, 50, 0.8);
                color: #00BFFF;
                border: 1px solid rgba(0, 191, 255, 0.5);
                border-radius: 4px;
                padding: 5px;
            }
            
            notebook {
                background-color: rgba(0, 15, 30, 0.8);
            }
            
            notebook tab {
                background-color: rgba(0, 20, 50, 0.7);
                padding: 4px 10px;
                border: 1px solid rgba(0, 191, 255, 0.3);
                color: #00BFFF;
            }
            
            notebook tab:checked {
                background-color: rgba(0, 40, 90, 0.8);
                border-bottom: 2px solid #00BFFF;
            }
            
            scrolledwindow {
                background-color: transparent;
            }
            
            scrollbar {
                background-color: rgba(0, 10, 30, 0.5);
            }
            
            scrollbar slider {
                background-color: rgba(0, 150, 255, 0.3);
                border-radius: 10px;
                min-width: 8px;
                min-height: 8px;
            }
            
            scrollbar slider:hover {
                background-color: rgba(0, 180, 255, 0.5);
            }
            
            label.explorer-title {
                color: #00BFFF;
                font-weight: bold;
            }
        """
        css_provider.load_from_data(css.encode())
        
        # In GTK4, the way to add CSS providers has changed
        Gtk.StyleContext.add_provider_for_display(
            Gdk.Display.get_default(),
            css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
    
    def setup_headerbar(self):
        """Create the application headerbar with controls"""
        self.headerbar = Gtk.HeaderBar()
        self.headerbar.set_show_title_buttons(True)
        self.set_titlebar(self.headerbar)
        
        # File operations buttons
        self.new_button = Gtk.Button()
        self.new_button.set_tooltip_text("New File")
        new_icon = Gtk.Image.new_from_icon_name("document-new-symbolic")
        self.new_button.set_child(new_icon)
        self.new_button.connect("clicked", self.on_new_clicked)
        self.headerbar.pack_start(self.new_button)
        
        self.open_button = Gtk.Button()
        self.open_button.set_tooltip_text("Open File")
        open_icon = Gtk.Image.new_from_icon_name("document-open-symbolic")
        self.open_button.set_child(open_icon)
        self.open_button.connect("clicked", self.on_open_clicked)
        self.headerbar.pack_start(self.open_button)
        
        self.save_button = Gtk.Button()
        self.save_button.set_tooltip_text("Save File")
        save_icon = Gtk.Image.new_from_icon_name("document-save-symbolic")
        self.save_button.set_child(save_icon)
        self.save_button.connect("clicked", self.on_save_clicked)
        self.headerbar.pack_start(self.save_button)
        
        self.save_as_button = Gtk.Button()
        self.save_as_button.set_tooltip_text("Save As")
        save_as_icon = Gtk.Image.new_from_icon_name("document-save-as-symbolic")
        self.save_as_button.set_child(save_as_icon)
        self.save_as_button.connect("clicked", self.on_save_as_clicked)
        self.headerbar.pack_start(self.save_as_button)
        
        # Separator
        separator = Gtk.Separator(orientation=Gtk.Orientation.VERTICAL)
        self.headerbar.pack_start(separator)
        
        # Edit operations
        self.find_button = Gtk.Button()
        self.find_button.set_tooltip_text("Find/Replace")
        find_icon = Gtk.Image.new_from_icon_name("edit-find-symbolic")
        self.find_button.set_child(find_icon)
        self.find_button.connect("clicked", self.on_find_clicked)
        self.headerbar.pack_start(self.find_button)
        
        # Right side controls
        self.run_button = Gtk.Button()
        self.run_button.set_tooltip_text("Run Code")
        run_icon = Gtk.Image.new_from_icon_name("media-playback-start-symbolic")
        self.run_button.set_child(run_icon)
        self.run_button.connect("clicked", self.on_run_clicked)
        self.headerbar.pack_end(self.run_button)
        
        self.theme_button = Gtk.Button()
        self.theme_button.set_tooltip_text("Toggle Theme")
        theme_icon = Gtk.Image.new_from_icon_name("preferences-desktop-theme-symbolic")
        self.theme_button.set_child(theme_icon)
        self.theme_button.connect("clicked", self.on_theme_clicked)
        self.headerbar.pack_end(self.theme_button)
    
    def setup_main_layout(self):
        """Set up the main application layout"""
        # Main horizontal box
        self.main_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.main_container.append(self.main_box)
        
        # Sidebar for file browser and tools
        self.sidebar = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.sidebar.set_size_request(200, -1)
        self.sidebar.get_style_context().add_class("sidebar")
        
        # Sidebar header
        self.sidebar_header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.sidebar_header.set_margin_top(10)
        self.sidebar_header.set_margin_bottom(10)
        self.sidebar_header.set_margin_start(10)
        self.sidebar_header.set_margin_end(10)
        
        self.sidebar_title = Gtk.Label(label="Explorer")
        self.sidebar_title.get_style_context().add_class("explorer-title")
        # Make the title expand to take up space
        self.sidebar_title.set_hexpand(True)
        self.sidebar_title.set_xalign(0)
        self.sidebar_header.append(self.sidebar_title)
        
        self.refresh_button = Gtk.Button()
        self.refresh_button.set_tooltip_text("Refresh")
        refresh_icon = Gtk.Image.new_from_icon_name("view-refresh-symbolic")
        self.refresh_button.set_child(refresh_icon)
        self.refresh_button.connect("clicked", self.on_refresh_clicked)
        self.sidebar_header.append(self.refresh_button)
        
        self.sidebar.append(self.sidebar_header)
        
        # File browser
        self.file_store = Gtk.TreeStore()
        self.file_store.set_column_types([str, str, str])  # name, path, icon
        self.file_view = Gtk.TreeView(model=self.file_store)
        self.file_view.set_headers_visible(False)
        self.file_view.connect("row-activated", self.on_file_activated)
        
        renderer = Gtk.CellRendererPixbuf()
        column = Gtk.TreeViewColumn("Icon", renderer, icon_name=2)
        self.file_view.append_column(column)
        
        renderer = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn("Name", renderer, text=0)
        self.file_view.append_column(column)
        
        scrolled_file_view = Gtk.ScrolledWindow()
        scrolled_file_view.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled_file_view.set_child(self.file_view)
        
        self.sidebar.append(scrolled_file_view)
        
        # Add sidebar to main box
        self.main_box.append(self.sidebar)
        
        # Editor area with notebook for tabs
        self.editor_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.editor_box.set_hexpand(True)
        self.editor_box.set_vexpand(True)
        self.main_box.append(self.editor_box)
        
        # Notebook for multiple tabs
        self.notebook = Gtk.Notebook()
        self.notebook.set_scrollable(True)
        self.notebook.connect("switch-page", self.on_tab_switched)
        self.notebook.set_hexpand(True)
        self.notebook.set_vexpand(True)
        self.editor_box.append(self.notebook)
        
        # Create initial editor tab
        self.create_editor_tab("Untitled")
        
        # Search bar (initially hidden)
        self.search_bar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        self.search_bar.set_visible(False)
        self.search_bar.get_style_context().add_class("search-bar")
        
        self.search_entry = Gtk.Entry()
        self.search_entry.set_placeholder_text("Search...")
        self.search_entry.connect("activate", self.on_search_activated)
        self.search_entry.set_hexpand(True)
        self.search_bar.append(self.search_entry)
        
        self.replace_entry = Gtk.Entry()
        self.replace_entry.set_placeholder_text("Replace with...")
        self.replace_entry.set_hexpand(True)
        self.search_bar.append(self.replace_entry)
        
        self.search_next_button = Gtk.Button(label="Next")
        self.search_next_button.connect("clicked", self.on_search_next)
        self.search_bar.append(self.search_next_button)
        
        self.search_prev_button = Gtk.Button(label="Previous")
        self.search_prev_button.connect("clicked", self.on_search_prev)
        self.search_bar.append(self.search_prev_button)
        
        self.replace_button = Gtk.Button(label="Replace")
        self.replace_button.connect("clicked", self.on_replace_clicked)
        self.search_bar.append(self.replace_button)
        
        self.close_search_button = Gtk.Button()
        close_icon = Gtk.Image.new_from_icon_name("window-close-symbolic")
        self.close_search_button.set_child(close_icon)
        self.close_search_button.connect("clicked", self.on_close_search_clicked)
        self.search_bar.append(self.close_search_button)
        
        self.editor_box.append(self.search_bar)
        
        # Status bar
        self.statusbar = Gtk.Label(label="Ready")
        self.statusbar.get_style_context().add_class("statusbar")
        self.statusbar.set_xalign(0)
        self.editor_box.append(self.statusbar)
        
        # Initialize file browser
        self.populate_file_browser()
        
    def create_editor_tab(self, title):
        """Create a new editor tab with text view"""
        # Create text view
        self.source_view = Gtk.TextView()
        self.source_view.set_monospace(True)
        self.source_view.set_wrap_mode(Gtk.WrapMode.WORD)
        self.source_view.set_hexpand(True)
        self.source_view.set_vexpand(True)
        
        # Create buffer
        self.buffer = self.source_view.get_buffer()
        self.buffer.connect("changed", self.on_buffer_changed)
        
        # Create scrolled window
        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled_window.set_child(self.source_view)
        
        # Create tab label
        label_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        label = Gtk.Label(label=title)
        close_button = Gtk.Button()
        close_icon = Gtk.Image.new_from_icon_name("window-close-symbolic")
        close_button.set_child(close_icon)
        close_button.connect("clicked", self.on_close_tab_clicked)
        label_box.append(label)
        label_box.append(close_button)
        
        # Add tab to notebook
        self.notebook.append_page(scrolled_window, label_box)
        self.notebook.set_current_page(self.notebook.get_n_pages() - 1)
    
    def on_find_clicked(self, widget):
        """Handle find button click"""
        self.search_bar.set_visible(True)
        self.search_entry.grab_focus()
    
    def on_search_activated(self, widget):
        """Handle search entry activation"""
        self.on_search_next(widget)
        
    def on_search_next(self, widget):
        """Handle search next button click"""
        search_text = self.search_entry.get_text()
        if search_text:
            # Simple search implementation
            buffer = self.buffer
            start_iter = buffer.get_start_iter()
            
            # Find text
            search_flags = Gtk.TextSearchFlags.VISIBLE_ONLY | Gtk.TextSearchFlags.TEXT_ONLY
            found = start_iter.forward_search(search_text, search_flags, None)
            
            if found:
                match_start, match_end = found
                buffer.select_range(match_start, match_end)
                self.source_view.scroll_to_mark(buffer.get_insert(), 0.0, True, 0.0, 0.5)
        
    def on_search_prev(self, widget):
        """Handle search previous button click"""
        search_text = self.search_entry.get_text()
        if search_text:
            # Simple search implementation
            buffer = self.buffer
            end_iter = buffer.get_end_iter()
            
            # Find text
            search_flags = Gtk.TextSearchFlags.VISIBLE_ONLY | Gtk.TextSearchFlags.TEXT_ONLY
            found = end_iter.backward_search(search_text, search_flags, None)
            
            if found:
                match_start, match_end = found
                buffer.select_range(match_start, match_end)
                self.source_view.scroll_to_mark(buffer.get_insert(), 0.0, True, 0.0, 0.5)
        
    def on_replace_clicked(self, widget):
        """Handle replace button click"""
        search_text = self.search_entry.get_text()
        replace_text = self.replace_entry.get_text()
        if search_text and replace_text:
            buffer = self.buffer
            
            # Check if there's a selection that matches the search text
            if buffer.get_has_selection():
                start, end = buffer.get_selection_bounds()
                selected_text = buffer.get_text(start, end, True)
                
                if selected_text == search_text:
                    buffer.delete(start, end)
                    buffer.insert(start, replace_text)
                    
                    # Find next match
                    self.on_search_next(widget)
        
    def on_close_search_clicked(self, widget):
        """Close the search bar"""
        self.search_bar.set_visible(False)
        
    def on_close_tab_clicked(self, widget):
        """Handle tab close button click"""
        # In GTK4, we need to find which page the button belongs to
        for i in range(self.notebook.get_n_pages()):
            page = self.notebook.get_nth_page(i)
            tab_label = self.notebook.get_tab_label(page)
            
            # Check if the button is in this tab label
            if widget in [child for child in tab_label]:
                self.notebook.remove_page(i)
                break
        
    def on_tab_switched(self, notebook, page, page_num):
        """Handle tab switching"""
        self.update_title()
        
    def on_buffer_changed(self, buffer):
        """Handle buffer content changes"""
        self.modified = True
        self.update_title()
        
    def update_title(self):
        """Update window title with current file and modified status"""
        title = "Hextrix Notepad"
        if self.current_file:
            title += f" - {os.path.basename(self.current_file)}"
        if self.modified:
            title += " *"
        self.set_title(title)
        
    def populate_file_browser(self):
        """Populate the file browser with current directory contents"""
        self.file_store.clear()
        current_dir = os.getcwd()
        self.add_directory_to_store(current_dir, None)
        
    def add_directory_to_store(self, path, parent_iter):
        """Add directory contents to file browser"""
        try:
            for entry in os.scandir(path):
                if entry.is_dir():
                    iter = self.file_store.append(parent_iter)
                    self.file_store.set(iter,
                        0, entry.name,
                        1, entry.path,
                        2, "folder"
                    )
                    self.add_directory_to_store(entry.path, iter)
                else:
                    iter = self.file_store.append(parent_iter)
                    self.file_store.set(iter,
                        0, entry.name,
                        1, entry.path,
                        2, "text-x-generic"
                    )
        except PermissionError:
            pass
            
    def on_file_activated(self, tree_view, path, column):
        """Handle file selection in browser"""
        model = tree_view.get_model()
        iter = model.get_iter(path)
        file_path = model.get_value(iter, 1)
        if os.path.isfile(file_path):
            self.open_file(file_path)
            
    def open_file(self, file_path):
        """Open a file in a new tab"""
        try:
            with open(file_path, 'r') as f:
                content = f.read()
                
            self.create_editor_tab(os.path.basename(file_path))
            self.buffer.set_text(content)
            self.current_file = file_path
            self.modified = False
            self.update_title()
        except Exception as e:
            self.show_error_message(f"Failed to open file: {str(e)}")
            
    def show_error_message(self, message):
        """Show error message dialog"""
        dialog = Gtk.AlertDialog.new(message)
        dialog.set_modal(True)
        dialog.set_buttons(["OK"])
        dialog.show()
        
    def on_new_clicked(self, widget):
        """Handle new file creation"""
        self.create_editor_tab("Untitled")
        self.current_file = None
        self.modified = False
        self.update_title()
        
    def on_open_clicked(self, widget):
        """Handle file open dialog"""
        dialog = Gtk.FileDialog()
        dialog.set_title("Open File")
        
        # Set up file filter for text files
        filter = Gtk.FileFilter()
        filter.set_name("Text files")
        filter.add_mime_type("text/plain")
        filter.add_pattern("*.txt")
        filter.add_pattern("*.py")
        filter.add_pattern("*.js")
        filter.add_pattern("*.html")
        filter.add_pattern("*.css")
        dialog.set_default_filter(filter)
        
        dialog.open(self, None, self.on_open_dialog_response)
    
    def on_open_dialog_response(self, dialog, result):
        """Handle open dialog response"""
        try:
            file = dialog.open_finish(result)
            if file:
                self.open_file(file.get_path())
        except Exception as e:
            self.show_error_message(f"Error opening file: {str(e)}")
        
    def on_save_clicked(self, widget):
        """Handle file save"""
        if self.current_file:
            self.save_file(self.current_file)
        else:
            self.on_save_as_clicked(widget)
            
    def on_save_as_clicked(self, widget):
        """Handle save as dialog"""
        dialog = Gtk.FileDialog()
        dialog.set_title("Save File")
        
        # Set up file filter for text files
        filter = Gtk.FileFilter()
        filter.set_name("Text files")
        filter.add_mime_type("text/plain")
        filter.add_pattern("*.txt")
        filter.add_pattern("*.py")
        filter.add_pattern("*.js")
        filter.add_pattern("*.html")
        filter.add_pattern("*.css")
        dialog.set_default_filter(filter)
        
        dialog.save(self, None, self.on_save_dialog_response)
    
    def on_save_dialog_response(self, dialog, result):
        """Handle save dialog response"""
        try:
            file = dialog.save_finish(result)
            if file:
                self.save_file(file.get_path())
        except Exception as e:
            self.show_error_message(f"Error saving file: {str(e)}")
        
    def save_file(self, file_path):
        """Save current buffer to file"""
        try:
            buffer = self.source_view.get_buffer()
            text = buffer.get_text(buffer.get_start_iter(), buffer.get_end_iter(), True)
            
            with open(file_path, 'w') as f:
                f.write(text)
                
            self.current_file = file_path
            self.modified = False
            self.update_title()
            self.statusbar.set_text(f"File saved: {file_path}")
        except Exception as e:
            self.show_error_message(f"Failed to save file: {str(e)}")
            
    def on_run_clicked(self, widget):
        """Handle code execution"""
        buffer = self.source_view.get_buffer()
        code = buffer.get_text(buffer.get_start_iter(), buffer.get_end_iter(), True)
        
        try:
            # Create a new process to run the code
            with open("temp_script.py", "w") as f:
                f.write(code)
            
            subprocess.Popen(["python3", "temp_script.py"])
            self.statusbar.set_text("Code is running...")
        except Exception as e:
            self.show_error_message(f"Execution error: {str(e)}")
            
    def on_theme_clicked(self, widget):
        """Toggle between light and dark themes"""
        # Implement theme switching logic here
        self.statusbar.set_text("Theme switching not implemented yet")
        
    def on_refresh_clicked(self, widget):
        """Refresh file browser"""
        self.populate_file_browser()
    
    def pulse_effect(self):
        """Create pulsing effect for UI elements"""
        # Get current time for animation
        current_time = time.time()
        pulse_value = 0.5 + 0.5 * math.sin(current_time * 2)
        
        # Apply pulse effect to various UI elements
        widgets = [self.headerbar, self.sidebar, self.statusbar]
        for widget in widgets:
            context = widget.get_style_context()
            if pulse_value > 0.8:
                context.add_class("pulse-effect")
            else:
                context.remove_class("pulse-effect")
        
        # Continue the animation
        return True


class HextrixNotepadApp(Gtk.Application):
    def __init__(self):
        super().__init__(application_id="com.hextrix.notepad")
        self.window = None

    def do_activate(self):
        if not self.window:
            self.window = HextrixNotepad(application=self)
        self.window.present()


if __name__ == "__main__":
    app = HextrixNotepadApp()
    app.run()