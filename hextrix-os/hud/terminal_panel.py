#!/usr/bin/env python3
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Vte', '2.91')
from gi.repository import Gtk, Gdk, Pango, Vte, GLib
import os

class TerminalPanel:
    def __init__(self, parent):
        self.parent = parent
        self.panel = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)

        self.toolbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        self.toolbar.set_margin_start(10)
        self.toolbar.set_margin_end(10)
        self.toolbar.set_margin_top(10)
        self.toolbar.set_margin_bottom(5)

        self.new_tab_button = self.create_tool_button("tab-new-symbolic", "New Tab", self.on_new_tab)
        self.toolbar.pack_start(self.new_tab_button, False, False, 0)

        self.copy_button = self.create_tool_button("edit-copy-symbolic", "Copy", self.on_copy)
        self.toolbar.pack_start(self.copy_button, False, False, 0)

        self.paste_button = self.create_tool_button("edit-paste-symbolic", "Paste", self.on_paste)
        self.toolbar.pack_start(self.paste_button, False, False, 0)

        self.clear_button = self.create_tool_button("edit-clear-all-symbolic", "Clear", self.on_clear)
        self.toolbar.pack_start(self.clear_button, False, False, 0)

        self.font_smaller_button = self.create_tool_button("zoom-out-symbolic", "Decrease Font Size", self.on_font_smaller)
        self.toolbar.pack_start(self.font_smaller_button, False, False, 0)

        self.font_larger_button = self.create_tool_button("zoom-in-symbolic", "Increase Font Size", self.on_font_larger)
        self.toolbar.pack_start(self.font_larger_button, False, False, 0)

        self.cmd_entry = Gtk.Entry()
        self.cmd_entry.set_placeholder_text("Enter command...")
        self.cmd_entry.connect("activate", self.on_cmd_activated)
        self.toolbar.pack_start(self.cmd_entry, True, True, 0)

        self.run_button = self.create_tool_button("system-run-symbolic", "Run Command", self.on_run_command)
        self.toolbar.pack_end(self.run_button, False, False, 0)

        self.panel.pack_start(self.toolbar, False, False, 0)

        self.notebook = Gtk.Notebook()
        self.notebook.set_scrollable(True)
        self.notebook.set_show_border(False)
        self.notebook.connect("switch-page", self.on_tab_switched)
        self.panel.pack_start(self.notebook, True, True, 0)

        self.terminals = []
        self.create_terminal_tab("Terminal")

        self.apply_styling()

        self.font_size = 12
        self.update_font()

    def apply_styling(self):
        css_provider = Gtk.CssProvider()
        css = """
            .terminal-panel {
                background-color: rgba(0, 10, 30, 0.8);
                border-radius: 5px;
                border: 1px solid rgba(0, 191, 255, 0.3);
            }
            .terminal-panel-toolbar {
                background-color: rgba(0, 30, 60, 0.8);
                border-bottom: 1px solid rgba(0, 191, 255, 0.3);
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
            notebook tab {
                background-color: rgba(0, 20, 50, 0.7);
                padding: 2px 8px;
                border: 1px solid rgba(0, 191, 255, 0.3);
                color: #00BFFF;
            }
            notebook tab:checked {
                background-color: rgba(0, 40, 90, 0.8);
                border-bottom: 1px solid #00BFFF;
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

        self.panel.get_style_context().add_class("terminal-panel")
        self.panel.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        self.toolbar.get_style_context().add_class("terminal-panel-toolbar")
        self.toolbar.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        self.cmd_entry.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        self.notebook.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

    def create_tool_button(self, icon_name, tooltip, callback):
        button = Gtk.Button()
        button.set_tooltip_text(tooltip)
        button.set_relief(Gtk.ReliefStyle.NONE)
        icon = Gtk.Image.new_from_icon_name(icon_name, Gtk.IconSize.SMALL_TOOLBAR)
        button.add(icon)
        button.connect("clicked", callback)
        button.get_style_context().add_class("tool-button")
        return button

    def create_terminal_tab(self, tab_title="Terminal"):
        terminal = Vte.Terminal()
        terminal.set_cursor_blink_mode(Vte.CursorBlinkMode.ON)
        terminal.set_mouse_autohide(True)
        terminal.set_encoding("UTF-8")
        
        # Make sure terminal receives input events
        terminal.set_can_focus(True)
        
        # Add specific event mask to receive all input events
        terminal.add_events(Gdk.EventMask.ALL_EVENTS_MASK)
        
        # Set terminal colors as before
        terminal.set_color_background(Gdk.RGBA(0, 0.05, 0.1, 0.9))
        terminal.set_color_foreground(Gdk.RGBA(0.8, 0.9, 1, 1))
        
        # Create scrolled window with event propagation
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled.add(terminal)
        
        # Create an event box to ensure mouse events reach the terminal
        event_box = Gtk.EventBox()
        event_box.add(scrolled)
        
        # Rest of your code remains the same...
        tab_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        tab_label = Gtk.Label(tab_title)
        tab_box.pack_start(tab_label, True, True, 0)
        
        close_icon = Gtk.Image.new_from_icon_name("window-close-symbolic", Gtk.IconSize.MENU)
        close_button = Gtk.Button()
        close_button.set_relief(Gtk.ReliefStyle.NONE)
        close_button.set_focus_on_click(False)
        close_button.add(close_icon)
        close_button.connect("clicked", self.on_tab_close, event_box)  # Note: changed from scrolled to event_box
        tab_box.pack_end(close_button, False, False, 0)
        tab_box.show_all()
        
        # Add the event box to the notebook instead of the scrolled window directly
        page_index = self.notebook.append_page(event_box, tab_box)
        self.notebook.set_current_page(page_index)
        self.notebook.show_all()
    
        terminal.spawn_sync(
            Vte.PtyFlags.DEFAULT,
            os.environ['HOME'],
            ["/bin/bash"],
            [],
            GLib.SpawnFlags.DO_NOT_REAP_CHILD,
            None,
            None,
        )

        terminal.connect("child-exited", self.on_terminal_exit)

        self.terminals.append(terminal)
        return terminal

    def get_current_terminal(self):
        page_num = self.notebook.get_current_page()
        if page_num >= 0 and page_num < len(self.terminals):
            return self.terminals[page_num]
        return None

    def update_font(self):
        for terminal in self.terminals:
            font_desc = Pango.FontDescription(f"Monospace {self.font_size}")
            terminal.set_font(font_desc)

    def on_new_tab(self, button):
        self.create_terminal_tab(f"Terminal {len(self.terminals)}")

    def on_tab_close(self, button, widget):
        page_num = self.notebook.page_num(widget)
        if page_num >= 0:
            if len(self.terminals) > 1:
                del self.terminals[page_num]
                self.notebook.remove_page(page_num)
            else:
                self.terminals[0].reset(True, True)

    def on_tab_switched(self, notebook, page, page_num):
        pass

    def on_copy(self, button):
        term = self.get_current_terminal()
        if term and term.get_has_selection():
            term.copy_clipboard()

    def on_paste(self, button):
        term = self.get_current_terminal()
        if term:
            term.paste_clipboard()

    def on_clear(self, button):
        term = self.get_current_terminal()
        if term:
            term.reset(True, True)

    def on_font_smaller(self, button):
        if self.font_size > 6:
            self.font_size -= 1
            self.update_font()

    def on_font_larger(self, button):
        if self.font_size < 24:
            self.font_size += 1
            self.update_font()

    def on_cmd_activated(self, entry):
        self.run_command()

    def on_run_command(self, button):
        self.run_command()

    def run_command(self):
        command = self.cmd_entry.get_text().strip()
        if command:
            term = self.get_current_terminal()
            if term:
                self.cmd_entry.set_text("")
                command += "\n"
                term.feed_child(command.encode())
    def ensure_terminal_focus(self):
        term = self.get_current_terminal()
        if term:
            term.grab_focus()
    def on_terminal_exit(self, terminal, status):
        terminal.spawn_sync(
            Vte.PtyFlags.DEFAULT,
            os.environ['HOME'],
            ["/bin/bash"],
            [],
            GLib.SpawnFlags.DO_NOT_REAP_CHILD,
            None,
            None,
        )