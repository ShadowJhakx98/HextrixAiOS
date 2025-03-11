#!/usr/bin/env python3
import subprocess
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Vte', '2.91')
from gi.repository import Gtk, Gdk, GLib, Pango, Vte, Gio, GdkPixbuf
import os
import cairo
import numpy as np
import math
import random
import time
import threading
from datetime import datetime
import base64
import io
import logging
import requests
from pathlib import Path
import json

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Load environment variables from .env if available
try:
    from dotenv import load_dotenv
    
    # Get the directory of the current script
    script_dir = Path(__file__).parent.absolute()
    
    # Construct the path to the .env file
    env_path = script_dir / ".env"
    
    # Load the environment variables from the .env file
    if env_path.exists():
        print(f"Loading environment variables from {env_path}")
        load_dotenv(env_path)
    else:
        print(f"Warning: .env file not found at {env_path}")
        
except ImportError:
    print("Warning: python-dotenv not installed. Environment variables will not be loaded from .env file.")

# Set default values for required environment variables
os.environ.setdefault('CLOUDFLARE_ACCOUNT_ID', 'dev_mode')
os.environ.setdefault('CLOUDFLARE_API_KEY', 'dev_key')

# Try to import AI libraries but handle gracefully if not available
try:
    import nltk
    from nltk.tokenize import word_tokenize
    from nltk.corpus import stopwords
    from nltk.sentiment import SentimentIntensityAnalyzer
    nltk_available = True
    # Download NLTK data if not already downloaded
    nltk.download('punkt', quiet=True)
    nltk.download('stopwords', quiet=True)
    nltk.download('vader_lexicon', quiet=True)
except ImportError:
    nltk_available = False
    print("NLTK not available. Some AI features will be limited.")

try:
    from transformers import pipeline
    transformers_available = True
except ImportError:
    transformers_available = False
    print("Transformers not available. Some AI features will be limited.")

# Import local modules
from dock import DockCategory, DockItem
from neural_network_visualization import NeuralNetworkVisualization
from chat_panel import ChatPanel
from terminal_panel import TerminalPanel
from file_panel import FilePanel
class HextrixHUD(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Hextrix OS Neural Interface")
        self.set_default_size(1920, 1080)
        self.set_position(Gtk.WindowPosition.CENTER)
        self.terminal_revealer = Gtk.Revealer()
        self.terminal_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_RIGHT)
        self.terminal_revealer.set_transition_duration(300)
        # Make window transparent and borderless
        self.set_app_paintable(True)
        self.set_decorated(False)

        # Support for transparency
        screen = self.get_screen()
        visual = screen.get_rgba_visual()
        if visual and screen.is_composited():
            self.set_visual(visual)
        
        # Start in fullscreen mode
        self.fullscreen()
        
        # === LAYOUT CHANGES: Use single overlay as main container ===
        # Main overlay container - everything is placed on top of this
        self.main_overlay = Gtk.Overlay()
        self.add(self.main_overlay)
        
        # Neural network visualization as the base layer
        self.visualization = Gtk.DrawingArea()
        self.visualization.connect("draw", self.on_draw)
        self.main_overlay.add(self.visualization)
        
        # Initialize neural network
        self.neural_network = NeuralNetworkVisualization(1920, 1080)
        # === PANEL SETUP ===
        # Terminal panel - now positioned directly in the overlay
        self.terminal_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.terminal_container.set_size_request(500, -1)
        self.terminal_container.set_halign(Gtk.Align.START)
        self.terminal_container.set_valign(Gtk.Align.FILL)
        
        self.terminal_revealer = Gtk.Revealer()
        self.terminal_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_RIGHT)
        self.terminal_revealer.set_transition_duration(300)
        self.terminal_revealer.add(self.terminal_container)
        
        # Terminal panel header
        self.terminal_panel_header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.terminal_panel_header.set_margin_top(10)
        self.terminal_panel_header.set_margin_start(10)
        self.terminal_panel_header.set_margin_end(10)
        self.terminal_panel_header.set_margin_bottom(5)
        self.terminal_panel_title = Gtk.Label("Terminal")
        self.terminal_panel_title.set_markup("<span foreground='#00BFFF' font='14'>Terminal</span>")
        self.terminal_panel_title.set_halign(Gtk.Align.START)
        self.terminal_panel_header.pack_start(self.terminal_panel_title, True, True, 0)
        self.terminal_panel_close = self.create_tool_button("window-close-symbolic", "Close", self.on_terminal_panel_close)
        self.terminal_panel_header.pack_end(self.terminal_panel_close, False, False, 0)
        self.terminal_container.pack_start(self.terminal_panel_header, False, False, 0)
        
        # Terminal panel content
        self.terminal_panel_content = TerminalPanel(self)
        self.terminal_container.pack_start(self.terminal_panel_content.panel, True, True, 0)
        
        # File browser panel - now positioned directly in the overlay
        self.files_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.files_container.set_size_request(400, -1)
        self.files_container.set_halign(Gtk.Align.END)
        self.files_container.set_valign(Gtk.Align.FILL)
        
        self.files_revealer = Gtk.Revealer()
        self.files_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_LEFT)
        self.files_revealer.set_transition_duration(300)
        self.files_revealer.add(self.files_container)
        
        # File panel header
        self.files_panel_header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.files_panel_header.set_margin_top(10)
        self.files_panel_header.set_margin_start(10)
        self.files_panel_header.set_margin_end(10)
        self.files_panel_header.set_margin_bottom(5)
        self.files_panel_title = Gtk.Label("File Browser")
        self.files_panel_title.set_markup("<span foreground='#00BFFF' font='14'>File Browser</span>")
        self.files_panel_title.set_halign(Gtk.Align.START)
        self.files_panel_header.pack_start(self.files_panel_title, True, True, 0)
        self.files_panel_close = self.create_tool_button("window-close-symbolic", "Close", self.on_files_panel_close)
        self.files_panel_header.pack_end(self.files_panel_close, False, False, 0)
        self.files_container.pack_start(self.files_panel_header, False, False, 0)
        
        # File panel content
        self.file_panel_content = FilePanel(self)
        self.files_container.pack_start(self.file_panel_content.panel, True, True, 0)
        
        # Chat panel setup
        self.chat_panel = ChatPanel(self)
        self.chat_panel.panel.set_halign(Gtk.Align.CENTER)
        self.chat_panel.panel.set_valign(Gtk.Align.CENTER)
        self.chat_panel.panel.set_no_show_all(True)
        self.chat_panel.panel.hide()
        # Dock setup - positioned at the bottom of the overlay
        self.dock_revealer = Gtk.Revealer()
        self.dock_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_UP)
        self.dock_revealer.set_transition_duration(300)
        self.dock_revealer.set_valign(Gtk.Align.END)
        self.dock_revealer.set_halign(Gtk.Align.CENTER)
        
        self.dock_scrolled = Gtk.ScrolledWindow()
        self.dock_scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.NEVER)
        self.dock_scrolled.set_min_content_height(150)
        self.dock_scrolled.set_min_content_width(800)
        self.dock_scrolled.set_max_content_height(150)
        
        self.dock = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.dock.set_margin_start(15)
        self.dock.set_margin_end(15)
        self.dock.set_margin_bottom(15)
        self.dock.set_margin_top(15)
        
        self.dock_scrolled.add(self.dock)
        self.dock_revealer.add(self.dock_scrolled)
        
        # Button panels
        # Top buttons (UI and HUD)
        self.top_buttons = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.top_buttons.set_halign(Gtk.Align.START)
        self.top_buttons.set_valign(Gtk.Align.START)
        self.top_buttons.set_margin_start(20)
        self.top_buttons.set_margin_top(20)
        
        self.ui_button = self.create_circle_button("UI", self.on_ui_clicked)
        self.top_buttons.pack_start(self.ui_button, False, False, 0)
        
        self.hud_button = self.create_circle_button("HUD", self.on_hud_clicked)
        self.top_buttons.pack_start(self.hud_button, False, False, 0)
        
        # Bottom buttons (Term, Files, Apps)
        self.bottom_buttons = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.bottom_buttons.set_halign(Gtk.Align.START)
        self.bottom_buttons.set_valign(Gtk.Align.END)
        self.bottom_buttons.set_margin_start(20)
        self.bottom_buttons.set_margin_bottom(20)
        
        self.term_button = self.create_circle_button("Term", self.on_term_clicked)
        self.bottom_buttons.pack_start(self.term_button, False, False, 0)
        
        self.files_button = self.create_circle_button("Files", self.on_files_clicked)
        self.bottom_buttons.pack_start(self.files_button, False, False, 0)
        
        self.dock_toggle = self.create_circle_button("Apps", self.on_dock_toggle)
        self.bottom_buttons.pack_start(self.dock_toggle, False, False, 0)
        
        # Full button (bottom right)
        self.full_button = self.create_circle_button("Full", self.on_full_clicked)
        self.full_button.set_halign(Gtk.Align.END)
        self.full_button.set_valign(Gtk.Align.END)
        self.full_button.set_margin_end(20)
        self.full_button.set_margin_bottom(20)
        # === ADDING COMPONENTS TO OVERLAY ===
        # Add all components to the overlay
        self.main_overlay.add_overlay(self.terminal_revealer)
        self.main_overlay.add_overlay(self.files_revealer)
        self.main_overlay.add_overlay(self.chat_panel.panel)
        self.main_overlay.add_overlay(self.dock_revealer)
        self.main_overlay.add_overlay(self.top_buttons)
        self.main_overlay.add_overlay(self.bottom_buttons)
        self.main_overlay.add_overlay(self.full_button)
        
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
        
        # Setup drag and drop
        self.setup_dock_dnd()
        
        # User config directory
        self.config_dir = os.path.expanduser("~/.config/hextrix")
        os.makedirs(self.config_dir, exist_ok=True)
        self.dock_config_file = os.path.join(self.config_dir, "dock_config.json")
        
        # Load saved dock configuration or discover apps
        self.dock_items = []
        if os.path.exists(self.dock_config_file):
            self.load_dock_config()
        else:
            self.discover_applications()
            self.save_dock_config()
        
        # Create dock categories
        self.populate_dock()
        
        # Apply styling to panels
        self.apply_panel_styling()
        
        # Hide panels initially
        self.terminal_revealer.set_reveal_child(False)
        self.files_revealer.set_reveal_child(False)
        self.dock_revealer.set_reveal_child(False)
        
        # Set up animation timer
        GLib.timeout_add(16, self.update_animation)
        
        # Connect key events
        self.connect("key-press-event", self.on_key_press)
    def apply_panel_styling(self):
        terminal_css_provider = Gtk.CssProvider()
        terminal_css = """
            .terminal-panel {
                background-color: rgba(0, 10, 30, 0.8);
                border-right: 1px solid rgba(0, 191, 255, 0.5);
            }
        """
        terminal_css_provider.load_from_data(terminal_css.encode())
        self.terminal_container.get_style_context().add_class("terminal-panel")
        self.terminal_container.get_style_context().add_provider(
            terminal_css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        files_css_provider = Gtk.CssProvider()
        files_css = """
            .files-panel {
                background-color: rgba(0, 10, 30, 0.8);
                border-left: 1px solid rgba(0, 191, 255, 0.5);
            }
        """
        files_css_provider.load_from_data(files_css.encode())
        self.files_container.get_style_context().add_class("files-panel")
        self.files_container.get_style_context().add_provider(
            files_css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

    def create_circle_button(self, label, callback):
        button = Gtk.Button(label=label)
        button.set_size_request(50, 50)
        button.connect("clicked", callback)
        
        css_provider = Gtk.CssProvider()
        css = """
            .circle-button {
                border-radius: 25px;
                background-color: rgba(0, 40, 80, 0.7);
                color: #00BFFF;
                border: 1px solid rgba(0, 150, 255, 0.5);
                padding: 5px;
                transition: all 0.2s ease;
            }
            .circle-button:hover {
                background-color: rgba(0, 70, 130, 0.7);
                border-color: rgba(0, 200, 255, 0.8);
            }
            .circle-button:active {
                background-color: rgba(0, 100, 180, 0.8);
            }
        """
        css_provider.load_from_data(css.encode())
        button.get_style_context().add_class("circle-button")
        button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        return button

    def create_tool_button(self, icon_name, tooltip, callback):
        button = Gtk.Button()
        button.set_tooltip_text(tooltip)
        button.set_relief(Gtk.ReliefStyle.NONE)
        icon = Gtk.Image.new_from_icon_name(icon_name, Gtk.IconSize.SMALL_TOOLBAR)
        button.add(icon)
        button.connect("clicked", callback)
        button.get_style_context().add_class("tool-button")
        
        css_provider = Gtk.CssProvider()
        css = """
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
        """
        css_provider.load_from_data(css.encode())
        button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        return button
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
        for child in self.dock.get_children():
            self.dock.remove(child)
        
        for category_id, category in self.categories.items():
            if category.items:
                category_box, revealer, items_box = self.create_category_section(category)
                self.dock.pack_start(category_box, False, False, 5)
        
        self.dock.show_all()
    def on_draw(self, widget, cr):
        self.neural_network.width = widget.get_allocated_width()
        self.neural_network.height = widget.get_allocated_height()
        self.neural_network.draw(cr, 0)
        return True
    
    def update_animation(self):
        self.neural_network.update()
        self.visualization.queue_draw()
        return True
    
    def on_ui_clicked(self, button):
        visible = self.chat_panel.panel.get_visible()
        self.chat_panel.panel.set_visible(not visible)
    
    def on_hud_clicked(self, button):
        pass
    
    def on_term_clicked(self, button):
        currently_visible = self.terminal_revealer.get_reveal_child()
        self.terminal_revealer.set_reveal_child(not currently_visible)
        
        # Focus the terminal when showing it
        if not currently_visible:
            # Wait for revealer animation to complete before focusing
            GLib.timeout_add(350, self.focus_current_terminal)
    def focus_current_terminal(self):
        """Focus the current terminal after it's shown"""
        if hasattr(self, 'terminal_panel_content') and self.terminal_panel_content:
            term = self.terminal_panel_content.get_current_terminal()
            if term:
                term.grab_focus()
        return False  # Don't repeat this timeout

    def on_files_clicked(self, button):
        currently_visible = self.files_revealer.get_reveal_child()
        self.files_revealer.set_reveal_child(not currently_visible)
    
    def on_dock_toggle(self, button):
        currently_visible = self.dock_revealer.get_reveal_child()
        self.dock_revealer.set_reveal_child(not currently_visible)
    
    def on_full_clicked(self, button):
        window_state = self.get_window().get_state()
        if window_state & Gdk.WindowState.FULLSCREEN:
            self.unfullscreen()
        else:
            self.fullscreen()
    
    def on_terminal_panel_close(self, button):
        self.terminal_revealer.set_reveal_child(False)
    
    def on_files_panel_close(self, button):
        self.files_revealer.set_reveal_child(False)
    def on_key_press(self, widget, event):
        keyval = event.keyval
        if keyval == Gdk.KEY_Escape:
            self.chat_panel.panel.hide()
            self.terminal_revealer.set_reveal_child(False)
            self.files_revealer.set_reveal_child(False)
            self.dock_revealer.set_reveal_child(False)
            return True
        if keyval in (Gdk.KEY_F11, Gdk.KEY_f, Gdk.KEY_F):
            self.on_full_clicked(None)
            return True
        if keyval == Gdk.KEY_Tab and event.state & Gdk.ModifierType.CONTROL_MASK:
            if self.chat_panel.panel.get_visible():
                self.chat_panel.panel.hide()
                self.terminal_revealer.set_reveal_child(True)
                self.files_revealer.set_reveal_child(False)
                self.dock_revealer.set_reveal_child(False)
            elif self.terminal_revealer.get_reveal_child():
                self.terminal_revealer.set_reveal_child(False)
                self.files_revealer.set_reveal_child(True)
                self.chat_panel.panel.hide()
                self.dock_revealer.set_reveal_child(False)
            elif self.files_revealer.get_reveal_child():
                self.files_revealer.set_reveal_child(False)
                self.dock_revealer.set_reveal_child(True)
                self.chat_panel.panel.hide()
                self.terminal_revealer.set_reveal_child(False)
            elif self.dock_revealer.get_reveal_child():
                self.dock_revealer.set_reveal_child(False)
                self.chat_panel.panel.show()
                self.terminal_revealer.set_reveal_child(False)
                self.files_revealer.set_reveal_child(False)
            else:
                self.chat_panel.panel.show()
            return True
        return False
    
    def setup_dock_dnd(self):
        self.dock.drag_dest_set(
            Gtk.DestDefaults.ALL,
            [],
            Gdk.DragAction.MOVE
        )
        target_entry = Gtk.TargetEntry.new("application/x-dock-item", Gtk.TargetFlags.SAME_APP, 0)
        self.dock.drag_dest_set_target_list([target_entry])
        self.dock.connect("drag-data-received", self.on_dock_drag_data_received)
    def on_dock_drag_begin(self, button, context, dock_item):
        if dock_item.pixbuf:
            Gtk.drag_set_icon_pixbuf(context, dock_item.pixbuf, 0, 0)
        else:
            Gtk.drag_set_icon_name(context, "application-x-executable", 0, 0)

    def on_dock_drag_data_get(self, button, context, selection, target_id, time, dock_item):
        index = self.dock_items.index(dock_item)
        selection.set(selection.get_target(), 8, str(index).encode())

    def on_dock_drag_data_received(self, widget, context, x, y, selection, target_id, time):
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
        index = int(selection.get_data().decode())
        if 0 <= index < len(self.dock_items):
            item = self.dock_items[index]
            for cat in self.categories.values():
                if item in cat.items:
                    cat.remove_item(item)
            category.add_item(item)
            self.populate_dock()
            self.save_dock_config()
            
    def on_category_expanded(self, button, category, icon):
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
    def discover_applications(self):
        self.dock_items = []
        
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
            
            duplicate = False
            for item in self.dock_items:
                if item.command == command:
                    duplicate = True
                    break
            
            if duplicate:
                continue
                
            item = DockItem(name, icon_path, command, desktop_id, category)
            
            in_favorites = False
            for fav in self.categories["Favorites"].items:
                if fav.command == command:
                    in_favorites = True
                    break
                    
            if not in_favorites:
                self.dock_items.append(item)
                if category in self.categories:
                    self.categories[category].add_item(item)
        
        self.categories["Favorites"].expanded = True
    def save_dock_config(self):
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
        print(f"Launching: {dock_item.name} ({dock_item.command})")
        try:
            subprocess.Popen(dock_item.command.split())
        except Exception as e:
            print(f"Error launching {dock_item.command}: {e}")


if __name__ == "__main__":
    css_provider = Gtk.CssProvider()
    css = """
        window {
            background-color: rgba(0, 0, 0, 0);
        }
    """
    css_provider.load_from_data(css.encode())
    
    screen = Gdk.Screen.get_default()
    style_context = Gtk.StyleContext()
    style_context.add_provider_for_screen(screen, css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
    
    win = HextrixHUD()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    
    win.chat_panel.panel.hide()
    
    Gtk.main()