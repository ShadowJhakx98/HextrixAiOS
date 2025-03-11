#!/usr/bin/env python3
# /opt/hextrix/hud/hextrix-hud-fullscreen.py

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Vte', '2.91')
from gi.repository import Gtk, Gdk, GLib, Pango, Vte, Gio, GdkPixbuf
import cairo
import numpy as np
import math
import random
import time
import threading
import os
from datetime import datetime
import base64
import io
import logging
import requests
from pathlib import Path


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

class NeuralNetworkVisualization:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.nodes = []
        self.connections = []
        self.animation_offset = 0
        self.pulse_speed = 0.02
        self.node_movement_chance = 0.02
        self.movement_range = 5
        self.generate_network(200, 400)
    
    def generate_network(self, num_nodes, num_connections):
        """Generate a neural network with random nodes and connections"""
        self.nodes = []
        for i in range(num_nodes):
            angle = random.uniform(0, 2 * math.pi)
            distance = random.uniform(0.1, 0.8) * min(self.width, self.height) * 0.4
            center_x = self.width / 2
            center_y = self.height / 2
            x = center_x + math.cos(angle) * distance * random.uniform(0.8, 1.2)
            y = center_y + math.sin(angle) * distance * random.uniform(0.8, 1.2)
            size = random.uniform(1.5, 4)
            brightness = random.uniform(0.7, 1.0)
            activity = random.uniform(0, 1)
            pulse_offset = random.uniform(0, 2 * math.pi)
            oscillation_speed = random.uniform(0.5, 1.5)
            self.nodes.append({
                "id": i,
                "x": x, 
                "y": y, 
                "size": size,
                "brightness": brightness,
                "activity": activity,
                "pulse_offset": pulse_offset,
                "oscillation_speed": oscillation_speed
            })
        
        self.connections = []
        for _ in range(num_connections):
            start = random.randint(0, num_nodes - 1)
            end = random.randint(0, num_nodes - 1)
            if start != end:
                strength = random.uniform(0.2, 1.0)
                self.connections.append({
                    "source": start, 
                    "target": end, 
                    "strength": strength,
                    "pulse_offset": random.uniform(0, 2 * math.pi),
                    "pulse_speed": random.uniform(0.5, 1.5)
                })
    
    def update(self):
        """Update the animation state of the network"""
        self.animation_offset += self.pulse_speed
        if self.animation_offset > 2 * math.pi:
            self.animation_offset -= 2 * math.pi
        
        for node in self.nodes:
            if random.random() < self.node_movement_chance:
                node["x"] += random.uniform(-self.movement_range, self.movement_range)
                node["y"] += random.uniform(-self.movement_range, self.movement_range)
                margin = 50
                node["x"] = max(margin, min(self.width - margin, node["x"]))
                node["y"] = max(margin, min(self.height - margin, node["y"]))
            node["activity"] += random.uniform(-0.05, 0.05)
            node["activity"] = max(0.3, min(1.0, node["activity"]))
    
    def draw(self, cr, pulse_value):
        """Draw the neural network visualization"""
        pat = cairo.LinearGradient(0, 0, 0, self.height)
        pat.add_color_stop_rgba(0, 0.0, 0.02, 0.05, 0.9)
        pat.add_color_stop_rgba(1, 0.0, 0.0, 0.02, 0.9)
        cr.set_source(pat)
        cr.paint()
        
        for conn in self.connections:
            start_node = self.nodes[conn["source"]]
            end_node = self.nodes[conn["target"]]
            conn_phase = (conn["pulse_offset"] + self.animation_offset * conn["pulse_speed"]) % (2 * math.pi)
            pulse_factor = 0.5 + 0.5 * math.sin(conn_phase)
            dist = math.sqrt((end_node["x"] - start_node["x"])**2 + (end_node["y"] - start_node["y"])**2)
            max_dist = min(self.width, self.height) * 0.3
            if dist > max_dist:
                continue
            alpha = (1.0 - dist/max_dist) * conn["strength"] * pulse_factor
            alpha = max(0.1, min(0.6, alpha))
            avg_activity = (start_node["activity"] + end_node["activity"]) / 2
            cr.set_source_rgba(0, 0.7 + 0.3 * avg_activity, 1.0, alpha)
            cr.set_line_width(0.5 * conn["strength"] * pulse_factor)
            cr.move_to(start_node["x"], start_node["y"])
            cr.line_to(end_node["x"], end_node["y"])
            cr.stroke()
        
        for node in self.nodes:
            node_phase = (node["pulse_offset"] + self.animation_offset * node["oscillation_speed"]) % (2 * math.pi)
            pulse_intensity = 0.7 + 0.3 * math.sin(node_phase)
            for i in range(5, 0, -1):
                alpha = 0.05 * i * node["activity"]
                radius = node["size"] * (1 + 0.4 * i) * pulse_intensity
                cr.set_source_rgba(0, 0.8, 1.0, alpha)
                cr.arc(node["x"], node["y"], radius, 0, 2 * math.pi)
                cr.fill()
            intensity = node["brightness"] * pulse_intensity * node["activity"]
            cr.set_source_rgba(0.5 * intensity, 0.8 * intensity, 1.0, 0.8)
            cr.arc(node["x"], node["y"], node["size"] * pulse_intensity, 0, 2 * math.pi)
            cr.fill()
        return True

class ChatPanel:
    def __init__(self, parent):
        self.parent = parent
        self.panel = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.panel.set_size_request(800, 600)
        
        # Create all UI elements first before styling
        # Chat header with title and controls
        self.header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.header.set_margin_top(10)
        self.header.set_margin_bottom(10)
        self.header.set_margin_start(15)
        self.header.set_margin_end(15)
        
        self.title = Gtk.Label()
        self.title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Hextrix AI Conversation</span>")
        self.header.pack_start(self.title, True, True, 0)
        
        self.actions_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        # Initialize entry for styling
        self.entry = Gtk.Entry()
        self.entry.set_placeholder_text("Type your message here...")
        
        # Create buttons that will be styled
        self.clear_button = Gtk.Button(label="Clear")
        self.clear_button.connect("clicked", self.on_clear_clicked)
        
        self.export_button = Gtk.Button(label="Export")
        self.export_button.connect("clicked", self.on_export_clicked)
        
        self.mic_button = Gtk.Button(label="Microphone")
        self.mic_button.connect("clicked", self.on_mic_clicked)
        
        self.camera_button = Gtk.Button(label="Camera")
        self.camera_button.connect("clicked", self.on_camera_clicked)
        
        self.screen_button = Gtk.Button(label="Screen")
        self.screen_button.connect("clicked", self.on_screen_clicked)
        
        # Now apply styling after all elements are created
        self.apply_styling()
        
        # Continue building the UI
        self.actions_box.pack_start(self.clear_button, False, False, 0)
        self.actions_box.pack_start(self.export_button, False, False, 0)
        
        self.header.pack_end(self.actions_box, False, False, 0)
        self.panel.pack_start(self.header, False, False, 0)
        
        # Chat content area
        self.scrolled_window = Gtk.ScrolledWindow()
        self.scrolled_window.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        self.scrolled_window.set_min_content_height(400)
        
        self.chat_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        self.chat_box.set_margin_start(15)
        self.chat_box.set_margin_end(15)
        self.chat_box.set_margin_top(5)
        self.chat_box.set_margin_bottom(5)
        
        self.scrolled_window.add(self.chat_box)
        self.panel.pack_start(self.scrolled_window, True, True, 0)
        
        # Input area
        self.input_area = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        self.input_area.set_margin_start(15)
        self.input_area.set_margin_end(15)
        self.input_area.set_margin_top(10)
        self.input_area.set_margin_bottom(15)
        
        # Chat input and send button
        self.input_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        self.entry.connect("activate", self.on_send_clicked)
        
        self.send_button = Gtk.Button()
        send_icon = Gtk.Image.new_from_icon_name("send", Gtk.IconSize.BUTTON)
        self.send_button.add(send_icon)
        self.send_button.connect("clicked", self.on_send_clicked)
        
        self.input_box.pack_start(self.entry, True, True, 0)
        self.input_box.pack_end(self.send_button, False, False, 0)
        
        self.input_area.pack_start(self.input_box, False, False, 0)
        
        # Media buttons
        self.media_buttons = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.media_buttons.set_halign(Gtk.Align.CENTER)
        self.media_buttons.set_margin_top(10)
        
        self.media_buttons.pack_start(self.mic_button, False, False, 0)
        self.media_buttons.pack_start(self.camera_button, False, False, 0)
        self.media_buttons.pack_start(self.screen_button, False, False, 0)
        
        self.input_area.pack_start(self.media_buttons, False, False, 0)
        
        self.panel.pack_end(self.input_area, False, False, 0)
        
        # Chat history
        self.messages = []
    
    def apply_styling(self):
        css_provider = Gtk.CssProvider()
        css = """
            .chat-panel {
                background-color: rgba(0, 10, 30, 0.85);
                border-radius: 8px;
                border: 1px solid rgba(0, 150, 255, 0.5);
            }
            .chat-header {
                border-bottom: 1px solid rgba(0, 150, 255, 0.3);
                background-color: rgba(0, 20, 40, 0.7);
                border-radius: 7px 7px 0 0;
            }
            .chat-input {
                background-color: rgba(0, 30, 60, 0.6);
                border-radius: 4px;
                border: 1px solid rgba(0, 150, 255, 0.5);
                color: #FFFFFF;
                padding: 8px;
            }
            .chat-input:focus {
                border-color: rgba(0, 200, 255, 0.8);
            }
            .send-button {
                background-color: rgba(0, 100, 200, 0.6);
                border-radius: 4px;
                border: 1px solid rgba(0, 150, 255, 0.5);
            }
            .user-bubble {
                background-color: rgba(0, 50, 100, 0.7);
                border-radius: 12px 12px 12px 0;
                border: 1px solid rgba(0, 150, 255, 0.5);
                padding: 10px;
                margin: 5px 50px 5px 0;
            }
            .ai-bubble {
                background-color: rgba(0, 30, 60, 0.7);
                border-radius: 12px 12px 0 12px;
                border: 1px solid rgba(0, 150, 255, 0.3);
                padding: 10px;
                margin: 5px 0 5px 50px;
            }
            .media-button {
                background-color: rgba(0, 40, 80, 0.6);
                border-radius: 20px;
                border: 1px solid rgba(0, 150, 255, 0.5);
                color: #00BFFF;
                padding: 5px 15px;
                transition: all 0.2s ease;
            }
            .media-button:hover {
                background-color: rgba(0, 70, 130, 0.7);
                border-color: rgba(0, 200, 255, 0.8);
            }
            .action-button {
                background-color: rgba(0, 40, 80, 0.6);
                border-radius: 4px;
                border: 1px solid rgba(0, 150, 255, 0.5);
                color: #00BFFF;
                padding: 3px 10px;
            }
            .action-button:hover {
                background-color: rgba(0, 70, 130, 0.7);
                border-color: rgba(0, 200, 255, 0.8);
            }
        """
        css_provider.load_from_data(css.encode())
        
        # Apply styles
        self.panel.get_style_context().add_class("chat-panel")
        self.panel.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.header.get_style_context().add_class("chat-header")
        self.header.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.entry.get_style_context().add_class("chat-input")
        self.entry.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        # Apply styles to buttons
        self.clear_button.get_style_context().add_class("action-button")
        self.clear_button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.export_button.get_style_context().add_class("action-button")
        self.export_button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.mic_button.get_style_context().add_class("media-button")
        self.mic_button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.camera_button.get_style_context().add_class("media-button")
        self.camera_button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.screen_button.get_style_context().add_class("media-button")
        self.screen_button.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
    
    def on_send_clicked(self, widget):
        text = self.entry.get_text().strip()
        if text:
            self.add_message("You", text)
            self.entry.set_text("")
            
            # Here you would normally process through AI
            # For now, we'll just echo back
            GLib.timeout_add(1000, self.simulate_ai_response, text)
    
    def simulate_ai_response(self, user_text):
        # In a real app, this would call your AI processing
        response = f"I received your message: '{user_text}'"
        self.add_message("Hextrix", response)
        return False
    
    def add_message(self, sender, text):
        from datetime import datetime
        timestamp = datetime.now().strftime("%I:%M %p")
        
        # Create message container
        bubble_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        bubble_box.set_halign(Gtk.Align.START if sender == "Hextrix" else Gtk.Align.END)
        
        # Header with sender name and timestamp
        header_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        
        sender_label = Gtk.Label()
        if sender == "You":
            sender_label.set_markup(f"<b>{sender}</b>")
        else:
            sender_label.set_markup(f"<b><span color='#00BFFF'>{sender}</span></b>")
        
        time_label = Gtk.Label()
        time_label.set_markup(f"<small>{timestamp}</small>")
        
        if sender == "You":
            header_box.pack_end(sender_label, False, False, 0)
            header_box.pack_start(time_label, False, False, 0)
        else:
            header_box.pack_start(sender_label, False, False, 0)
            header_box.pack_end(time_label, False, False, 0)
        
        # Message content
        content_label = Gtk.Label(label=text)
        content_label.set_line_wrap(True)
        content_label.set_max_width_chars(60)
        content_label.set_xalign(0.0)
        content_label.set_margin_top(3)
        
        # Add components to bubble
        bubble = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        bubble.add(header_box)
        bubble.add(content_label)
        
        # Style the bubble
        css_provider = Gtk.CssProvider()
        css = """
            .user-bubble {
                background-color: rgba(0, 50, 100, 0.7);
                border-radius: 12px 12px 12px 0;
                border: 1px solid rgba(0, 150, 255, 0.5);
                padding: 10px;
                margin: 5px 50px 5px 0;
            }
            .ai-bubble {
                background-color: rgba(0, 30, 60, 0.7);
                border-radius: 12px 12px 0 12px;
                border: 1px solid rgba(0, 150, 255, 0.3);
                padding: 10px;
                margin: 5px 0 5px 50px;
            }
        """
        css_provider.load_from_data(css.encode())
        bubble.get_style_context().add_class("user-bubble" if sender == "You" else "ai-bubble")
        bubble.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        bubble_box.add(bubble)
        self.chat_box.add(bubble_box)
        self.chat_box.show_all()
        
        # Scroll to the bottom
        adj = self.scrolled_window.get_vadjustment()
        adj.set_value(adj.get_upper() - adj.get_page_size())
        
        # Store message
        self.messages.append({"sender": sender, "text": text, "timestamp": timestamp})
    
    def on_clear_clicked(self, widget):
        for child in self.chat_box.get_children():
            self.chat_box.remove(child)
        self.messages = []
    
    def on_export_clicked(self, widget):
        from datetime import datetime
        filename = f"hextrix_conversation_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        
        try:
            with open(filename, 'w') as f:
                for message in self.messages:
                    f.write(f"{message['timestamp']} - {message['sender']}: {message['text']}\n")
            self.add_message("Hextrix", f"Conversation exported to {filename}")
        except Exception as e:
            self.add_message("Hextrix", f"Error exporting conversation: {e}")
    
    def on_mic_clicked(self, widget):
        self.add_message("Hextrix", "Voice input is not yet implemented.")
    
    def on_camera_clicked(self, widget):
        self.add_message("Hextrix", "Camera input is not yet implemented.")
    
    def on_screen_clicked(self, widget):
        self.add_message("Hextrix", "Screen sharing is not yet implemented.")

class TerminalPanel:
    def __init__(self, parent):
        self.parent = parent
        
        # Create terminal panel container
        self.panel = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        
        # Create toolbar
        self.toolbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        self.toolbar.set_margin_start(10)
        self.toolbar.set_margin_end(10)
        self.toolbar.set_margin_top(10)
        self.toolbar.set_margin_bottom(5)
        
        # Terminal control buttons
        self.new_tab_button = self.create_tool_button("tab-new-symbolic", "New Tab", self.on_new_tab)
        self.toolbar.pack_start(self.new_tab_button, False, False, 0)
        
        self.copy_button = self.create_tool_button("edit-copy-symbolic", "Copy", self.on_copy)
        self.toolbar.pack_start(self.copy_button, False, False, 0)
        
        self.paste_button = self.create_tool_button("edit-paste-symbolic", "Paste", self.on_paste)
        self.toolbar.pack_start(self.paste_button, False, False, 0)
        
        self.clear_button = self.create_tool_button("edit-clear-all-symbolic", "Clear", self.on_clear)
        self.toolbar.pack_start(self.clear_button, False, False, 0)
        
        # Font size adjustment
        self.font_smaller_button = self.create_tool_button("zoom-out-symbolic", "Decrease Font Size", self.on_font_smaller)
        self.toolbar.pack_start(self.font_smaller_button, False, False, 0)
        
        self.font_larger_button = self.create_tool_button("zoom-in-symbolic", "Increase Font Size", self.on_font_larger)
        self.toolbar.pack_start(self.font_larger_button, False, False, 0)
        
        # Command entry
        self.cmd_entry = Gtk.Entry()
        self.cmd_entry.set_placeholder_text("Enter command...")
        self.cmd_entry.connect("activate", self.on_cmd_activated)
        self.toolbar.pack_start(self.cmd_entry, True, True, 0)
        
        # Run button
        self.run_button = self.create_tool_button("system-run-symbolic", "Run Command", self.on_run_command)
        self.toolbar.pack_end(self.run_button, False, False, 0)
        
        self.panel.pack_start(self.toolbar, False, False, 0)
        
        # Create terminal notebook for tabs
        self.notebook = Gtk.Notebook()
        self.notebook.set_scrollable(True)
        self.notebook.set_show_border(False)
        self.notebook.connect("switch-page", self.on_tab_switched)
        self.panel.pack_start(self.notebook, True, True, 0)
        
        # Create first terminal tab
        self.terminals = []
        self.create_terminal_tab("Terminal")
        
        # Apply styling
        self.apply_styling()
        
        # Current font size
        self.font_size = 12
        self.update_font()
    
    def apply_styling(self):
        # Apply custom CSS
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
        self.panel.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        self.toolbar.get_style_context().add_class("terminal-panel-toolbar")
        self.toolbar.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        # Apply to command entry
        self.cmd_entry.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        # Apply to notebook
        self.notebook.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
    
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
        # Create terminal
        terminal = Vte.Terminal()
        terminal.set_cursor_blink_mode(Vte.CursorBlinkMode.ON)
        terminal.set_mouse_autohide(True)
        terminal.set_encoding("UTF-8")
        
        # Configure colors
        terminal.set_color_background(Gdk.RGBA(0, 0.05, 0.1, 0.9))  # Very dark blue
        terminal.set_color_foreground(Gdk.RGBA(0.8, 0.9, 1, 1))    # Bright blue-white
        
        # Create terminal widget container
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled.add(terminal)
        
        # Create tab label with close button
        tab_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        tab_label = Gtk.Label(tab_title)
        tab_box.pack_start(tab_label, True, True, 0)
        
        close_icon = Gtk.Image.new_from_icon_name("window-close-symbolic", Gtk.IconSize.MENU)
        close_button = Gtk.Button()
        close_button.set_relief(Gtk.ReliefStyle.NONE)
        close_button.set_focus_on_click(False)
        close_button.add(close_icon)
        close_button.connect("clicked", self.on_tab_close, scrolled)
        tab_box.pack_end(close_button, False, False, 0)
        tab_box.show_all()
        
        # Add to notebook
        page_index = self.notebook.append_page(scrolled, tab_box)
        self.notebook.set_current_page(page_index)
        self.notebook.show_all()
        
        # Start shell
        terminal.spawn_sync(
            Vte.PtyFlags.DEFAULT,
            os.environ['HOME'],
            ["/bin/bash"],
            [],
            GLib.SpawnFlags.DO_NOT_REAP_CHILD,
            None,
            None,
        )
        
        # Connect terminal signals
        terminal.connect("child-exited", self.on_terminal_exit)
        
        # Store terminal
        self.terminals.append(terminal)
        
        # Return the terminal
        return terminal
    
    def get_current_terminal(self):
        page_num = self.notebook.get_current_page()
        if page_num >= 0 and page_num < len(self.terminals):
            return self.terminals[page_num]
        return None
    
    def update_font(self):
        # Set font for all terminals
        for terminal in self.terminals:
            font_desc = Pango.FontDescription(f"Monospace {self.font_size}")
            terminal.set_font(font_desc)
    
    def on_new_tab(self, button):
        self.create_terminal_tab(f"Terminal {len(self.terminals)}")
    
    def on_tab_close(self, button, widget):
        page_num = self.notebook.page_num(widget)
        
        if page_num >= 0:
            if len(self.terminals) > 1:  # Don't close the last tab
                # Remove from terminals list
                del self.terminals[page_num]
                
                # Remove tab
                self.notebook.remove_page(page_num)
            else:
                # Just clear the terminal
                self.terminals[0].reset(True, True)
    
    def on_tab_switched(self, notebook, page, page_num):
        # Update UI based on active tab if needed
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
        if self.font_size > 6:  # Minimum size
            self.font_size -= 1
            self.update_font()
    
    def on_font_larger(self, button):
        if self.font_size < 24:  # Maximum size
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
                # Clear entry
                self.cmd_entry.set_text("")
                
                # Add newline to command
                command += "\n"
                
                # Send command to terminal
                term.feed_child(command.encode())
    
    def on_terminal_exit(self, terminal, status):
        # Terminal process exited, start a new shell
        terminal.spawn_sync(
            Vte.PtyFlags.DEFAULT,
            os.environ['HOME'],
            ["/bin/bash"],
            [],
            GLib.SpawnFlags.DO_NOT_REAP_CHILD,
            None,
            None,
        )

class FilePanel:
    def __init__(self, parent):
        self.parent = parent
        self.current_path = os.path.expanduser("~")
        self.selected_item = None
        self.history = [self.current_path]
        self.history_position = 0
        
        # Create file panel container
        self.panel = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        
        # Create toolbar
        self.toolbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        self.toolbar.set_margin_start(10)
        self.toolbar.set_margin_end(10)
        self.toolbar.set_margin_top(10)
        self.toolbar.set_margin_bottom(5)
        
        # Navigation buttons
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
        
        # Path entry
        self.path_entry = Gtk.Entry()
        self.path_entry.set_text(self.current_path)
        self.path_entry.connect("activate", self.on_path_activated)
        self.toolbar.pack_start(self.path_entry, True, True, 0)
        
        # Search entry
        self.search_entry = Gtk.SearchEntry()
        self.search_entry.set_placeholder_text("Search files...")
        self.search_entry.connect("search-changed", self.on_search_changed)
        self.toolbar.pack_start(self.search_entry, False, False, 0)
        
        self.panel.pack_start(self.toolbar, False, False, 0)
        
        # Create file browser view
        self.create_file_view()
        
        # Status bar
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
        
        # Apply styling
        self.apply_styling()
        
        # Load initial directory
        self.load_directory()
    
    def apply_styling(self):
        # Apply custom CSS
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
        self.panel.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        self.toolbar.get_style_context().add_class("file-panel-toolbar")
        self.toolbar.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        self.statusbar.get_style_context().add_class("file-panel-statusbar")
        self.statusbar.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        # Apply to path entry
        self.path_entry.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        # Apply to search entry
        self.search_entry.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
    
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
        # Create scrolled window
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        
        # Create list store
        self.store = Gtk.ListStore(
            GdkPixbuf.Pixbuf,  # Icon
            str,               # Name
            str,               # Size
            str,               # Modified
            str,               # Type
            str                # Full path
        )
        
        # Create tree view
        self.treeview = Gtk.TreeView(model=self.store)
        self.treeview.set_headers_visible(True)
        self.treeview.set_enable_search(True)
        self.treeview.set_search_column(1)  # Search by name
        self.treeview.get_style_context().add_class("file-view")
        
        # Connect signals
        self.treeview.connect("row-activated", self.on_item_activated)
        selection = self.treeview.get_selection()
        selection.connect("changed", self.on_selection_changed)
        
        # Create columns
        # Icon column
        renderer_icon = Gtk.CellRendererPixbuf()
        column_icon = Gtk.TreeViewColumn("", renderer_icon, pixbuf=0)
        column_icon.set_fixed_width(30)
        column_icon.set_sizing(Gtk.TreeViewColumnSizing.FIXED)
        self.treeview.append_column(column_icon)
        
        # Name column
        renderer_name = Gtk.CellRendererText()
        column_name = Gtk.TreeViewColumn("Name", renderer_name, text=1)
        column_name.set_expand(True)
        column_name.set_sort_column_id(1)
        self.treeview.append_column(column_name)
        
        # Size column
        renderer_size = Gtk.CellRendererText()
        renderer_size.set_alignment(1.0, 0.5)  # Right-align
        column_size = Gtk.TreeViewColumn("Size", renderer_size, text=2)
        column_size.set_fixed_width(80)
        column_size.set_sizing(Gtk.TreeViewColumnSizing.FIXED)
        column_size.set_alignment(1.0)  # Right-align
        column_size.set_sort_column_id(2)
        self.treeview.append_column(column_size)
        
        # Modified column
        renderer_modified = Gtk.CellRendererText()
        column_modified = Gtk.TreeViewColumn("Modified", renderer_modified, text=3)
        column_modified.set_fixed_width(150)
        column_modified.set_sizing(Gtk.TreeViewColumnSizing.FIXED)
        column_modified.set_sort_column_id(3)
        self.treeview.append_column(column_modified)
        
        # Type column
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
        
        # Clear store
        self.store.clear()
        
        # Update path entry
        self.path_entry.set_text(self.current_path)
        
        try:
            # Enable/disable navigation buttons
            self.back_button.set_sensitive(self.history_position > 0)
            self.forward_button.set_sensitive(self.history_position < len(self.history) - 1)
            self.up_button.set_sensitive(self.current_path != '/')
            
            # Get directory contents
            contents = []
            
            # Add parent directory
            if self.current_path != '/':
                parent_path = os.path.dirname(self.current_path)
                parent_icon = self.get_icon_for_path(parent_path, is_dir=True)
                contents.append((parent_icon, "..", "", "", "Directory", parent_path))
            
            # Add directories and files
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
                    
                    # Skip hidden files (starting with .)
                    if not entry.startswith('.'):
                        contents.append((icon, entry, size, modified, file_type, full_path))
                except (PermissionError, FileNotFoundError):
                    pass
            
            # Add to store
            for item in contents:
                self.store.append(item)
            
            # Update status bar
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
            
            # Go back to previous directory
            if self.history_position > 0:
                self.history_position -= 1
                self.current_path = self.history[self.history_position]
                self.load_directory()
    
    def navigate_to(self, path):
        # Normalize path
        path = os.path.normpath(path)
        
        # Check if path exists
        if not os.path.exists(path):
            return False
        
        # Check if path is a directory
        if not os.path.isdir(path):
            return False
        
        # Update history
        if self.current_path != path:
            # If we navigated back and then to a new path, truncate history
            if self.history_position < len(self.history) - 1:
                self.history = self.history[:self.history_position + 1]
            
            self.history.append(path)
            self.history_position = len(self.history) - 1
        
        # Load directory
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
                # Create a default icon
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
        
        if mime_type:
            return Gio.content_type_get_description(mime_type)
        else:
            return "Unknown"
    
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
            # Open file with default application
            try:
                Gtk.show_uri_on_window(None, f"file://{full_path}", Gdk.CURRENT_TIME)
            except GLib.Error as e:
                print(f"Error opening file: {e}")
    
    def on_selection_changed(self, selection):
        model, tree_iter = selection.get_selected()
        
        if tree_iter is not None:
            full_path = model[tree_iter][5]
            self.selected_item = full_path
            
            # Get file/directory info
            try:
                stat_info = os.stat(full_path)
                size = self.format_size(stat_info.st_size)
                modified = datetime.fromtimestamp(stat_info.st_mtime).strftime('%Y-%m-%d %H:%M')
                
                if os.path.isdir(full_path):
                    info_text = f"Directory: {modified}"
                else:
                    info_text = f"{size} • {modified}"
                
                self.selected_info_label.set_text(info_text)
            except (PermissionError, FileNotFoundError):
                self.selected_info_label.set_text("")
        else:
            self.selected_item = None
            self.selected_info_label.set_text("")
    
    def on_search_changed(self, entry):
        search_text = entry.get_text().lower()
        
        if not search_text:
            # If search is empty, reload the directory
            self.load_directory()
            return
        
        # Clear store for search results
        self.store.clear()
        
        # Search in current directory
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
            
            # Update status bar
            result_count = len(self.store)
            self.item_count_label.set_text(f"{result_count} results")
            
        except (PermissionError, FileNotFoundError) as e:
            print(f"Error searching directory: {e}")
class HextrixHUD(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Hextrix OS Neural Interface")
        self.set_default_size(1920, 1080)
        self.set_position(Gtk.WindowPosition.CENTER)
        
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
        
        # Main container structure
        self.main_container = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.add(self.main_container)
        
        # Terminal panel revealer (left side)
        self.terminal_panel_revealer = Gtk.Revealer()
        self.terminal_panel_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_RIGHT)
        self.terminal_panel_revealer.set_transition_duration(300)
        self.terminal_panel = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.terminal_panel_revealer.add(self.terminal_panel)
        self.terminal_panel.set_size_request(500, -1)  # Width of 500px
        self.main_container.pack_start(self.terminal_panel_revealer, False, False, 0)
        
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
        self.terminal_panel.pack_start(self.terminal_panel_header, False, False, 0)
        
        # Central area with the overlay
        self.central_area = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.main_container.pack_start(self.central_area, True, True, 0)
        
        # Main overlay container
        self.overlay = Gtk.Overlay()
        self.central_area.pack_start(self.overlay, True, True, 0)
        
        # Neural network visualization (background)
        self.neural_network = NeuralNetworkVisualization(1920, 1080)
        self.visualization = Gtk.DrawingArea()
        self.visualization.connect("draw", self.on_draw)
        self.overlay.add(self.visualization)
        
        # Create modal panels
        self.chat_panel = ChatPanel(self)
        self.terminal_panel_content = TerminalPanel(self)
        self.file_panel_content = FilePanel(self)
        
        # Add terminal content to the terminal panel
        self.terminal_panel.pack_start(self.terminal_panel_content.panel, True, True, 0)
        
        # Set up panel positions and initial visibility
        self.chat_panel.panel.set_halign(Gtk.Align.CENTER)
        self.chat_panel.panel.set_valign(Gtk.Align.CENTER)
        self.chat_panel.panel.set_no_show_all(True)
        self.chat_panel.panel.hide()
        
        self.overlay.add_overlay(self.chat_panel.panel)
        
        # Desktop/File panel revealer (right side)
        self.desktop_panel_revealer = Gtk.Revealer()
        self.desktop_panel_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_LEFT)
        self.desktop_panel_revealer.set_transition_duration(300)
        self.desktop_panel = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.desktop_panel_revealer.add(self.desktop_panel)
        self.desktop_panel.set_size_request(400, -1)  # Width of 400px
        self.main_container.pack_end(self.desktop_panel_revealer, False, False, 0)
        
        # Desktop panel header
        self.desktop_panel_header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.desktop_panel_header.set_margin_top(10)
        self.desktop_panel_header.set_margin_start(10)
        self.desktop_panel_header.set_margin_end(10)
        self.desktop_panel_header.set_margin_bottom(5)
        self.desktop_panel_title = Gtk.Label("File Browser")
        self.desktop_panel_title.set_markup("<span foreground='#00BFFF' font='14'>File Browser</span>")
        self.desktop_panel_title.set_halign(Gtk.Align.START)
        self.desktop_panel_header.pack_start(self.desktop_panel_title, True, True, 0)
        self.desktop_panel_close = self.create_tool_button("window-close-symbolic", "Close", self.on_desktop_panel_close)
        self.desktop_panel_header.pack_end(self.desktop_panel_close, False, False, 0)
        self.desktop_panel.pack_start(self.desktop_panel_header, False, False, 0)
        
        # Add file content to desktop panel
        self.desktop_panel.pack_start(self.file_panel_content.panel, True, True, 0)
        
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
        
        self.overlay.add_overlay(self.top_buttons)
        
        # Bottom buttons (Term, Files, Full)
        self.bottom_buttons = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.bottom_buttons.set_halign(Gtk.Align.START)
        self.bottom_buttons.set_valign(Gtk.Align.END)
        self.bottom_buttons.set_margin_start(20)
        self.bottom_buttons.set_margin_bottom(20)
        
        self.term_button = self.create_circle_button("Term", self.on_term_clicked)
        self.bottom_buttons.pack_start(self.term_button, False, False, 0)
        
        self.files_button = self.create_circle_button("Files", self.on_files_clicked)
        self.bottom_buttons.pack_start(self.files_button, False, False, 0)
        
        self.overlay.add_overlay(self.bottom_buttons)
        
        # Full button (bottom right)
        self.full_button = self.create_circle_button("Full", self.on_full_clicked)
        self.full_button.set_halign(Gtk.Align.END)
        self.full_button.set_valign(Gtk.Align.END)
        self.full_button.set_margin_end(20)
        self.full_button.set_margin_bottom(20)
        
        self.overlay.add_overlay(self.full_button)
        
        # Apply styling to panels
        self.apply_panel_styling()
        
        # Hide panels initially
        self.terminal_panel_revealer.set_reveal_child(False)
        self.desktop_panel_revealer.set_reveal_child(False)
        
        # Set up animation timer
        GLib.timeout_add(16, self.update_animation)
        
        # Connect key events
        self.connect("key-press-event", self.on_key_press)
    
    def apply_panel_styling(self):
        # Terminal panel styling
        terminal_css_provider = Gtk.CssProvider()
        terminal_css = """
            .terminal-panel {
                background-color: rgba(0, 10, 30, 0.8);
                border-right: 1px solid rgba(0, 191, 255, 0.5);
            }
        """
        terminal_css_provider.load_from_data(terminal_css.encode())
        self.terminal_panel.get_style_context().add_class("terminal-panel")
        self.terminal_panel.get_style_context().add_provider(
            terminal_css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        # Desktop panel styling
        desktop_css_provider = Gtk.CssProvider()
        desktop_css = """
            .desktop-panel {
                background-color: rgba(0, 10, 30, 0.8);
                border-left: 1px solid rgba(0, 191, 255, 0.5);
            }
        """
        desktop_css_provider.load_from_data(desktop_css.encode())
        self.desktop_panel.get_style_context().add_class("desktop-panel")
        self.desktop_panel.get_style_context().add_provider(
            desktop_css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
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
        
        # Apply custom CSS
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
        button.get_style_context().add_provider(
            css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
        return button
    
    def on_draw(self, widget, cr):
        # Update dimensions
        self.neural_network.width = widget.get_allocated_width()
        self.neural_network.height = widget.get_allocated_height()
        
        # Draw the neural network
        self.neural_network.draw(cr, 0)
        return True
    
    def update_animation(self):
        # Update neural network and redraw
        self.neural_network.update()
        self.visualization.queue_draw()
        return True  # Continue the timer
    
    def on_ui_clicked(self, button):
        # Toggle the chat panel
        visible = self.chat_panel.panel.get_visible()
        self.chat_panel.panel.set_visible(not visible)
    
    def on_hud_clicked(self, button):
        # Toggle HUD elements (e.g., system info, performance monitors)
        pass
    
    def on_term_clicked(self, button):
        # Toggle the terminal panel using the revealer
        currently_visible = self.terminal_panel_revealer.get_reveal_child()
        self.terminal_panel_revealer.set_reveal_child(not currently_visible)
    
    def on_files_clicked(self, button):
        # Toggle the file panel using the revealer
        currently_visible = self.desktop_panel_revealer.get_reveal_child()
        self.desktop_panel_revealer.set_reveal_child(not currently_visible)
    
    def on_full_clicked(self, button):
        # Toggle fullscreen
        window_state = self.get_window().get_state()
        if window_state & Gdk.WindowState.FULLSCREEN:
            self.unfullscreen()
        else:
            self.fullscreen()
    
    def on_terminal_panel_close(self, button):
        # Hide terminal panel
        self.terminal_panel_revealer.set_reveal_child(False)
    
    def on_desktop_panel_close(self, button):
        # Hide desktop panel
        self.desktop_panel_revealer.set_reveal_child(False)
    
    def on_key_press(self, widget, event):
        # Handle keyboard shortcuts
        keyval = event.keyval
        
        # Escape key - hide all panels
        if keyval == Gdk.KEY_Escape:
            self.chat_panel.panel.hide()
            self.terminal_panel_revealer.set_reveal_child(False)
            self.desktop_panel_revealer.set_reveal_child(False)
            return True
        
        # F11 or F key - toggle fullscreen
        if keyval == Gdk.KEY_F11 or keyval == Gdk.KEY_f or keyval == Gdk.KEY_F:
            self.on_full_clicked(None)
            return True
        
        # Tab between panels
        if keyval == Gdk.KEY_Tab and event.state & Gdk.ModifierType.CONTROL_MASK:
            if self.chat_panel.panel.get_visible():
                self.chat_panel.panel.hide()
                self.terminal_panel_revealer.set_reveal_child(True)
                self.desktop_panel_revealer.set_reveal_child(False)
            elif self.terminal_panel_revealer.get_reveal_child():
                self.terminal_panel_revealer.set_reveal_child(False)
                self.desktop_panel_revealer.set_reveal_child(True)
                self.chat_panel.panel.hide()
            elif self.desktop_panel_revealer.get_reveal_child():
                self.desktop_panel_revealer.set_reveal_child(False)
                self.chat_panel.panel.show()
                self.terminal_panel_revealer.set_reveal_child(False)
            else:
                self.chat_panel.panel.show()
            return True
        
        return False

if __name__ == "__main__":
    # Set up styles for the application
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
    
    # Create and show the application window
    win = HextrixHUD()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    
    # Ensure panels are initially hidden
    win.chat_panel.panel.hide()
    
    Gtk.main()
