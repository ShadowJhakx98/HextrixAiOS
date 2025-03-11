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
import json  # Added for dock config handling

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

# Define DockCategory and DockItem classes
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

# [Assuming NeuralNetworkVisualization, ChatPanel, TerminalPanel, FilePanel are defined as in the first version]
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
    
    def draw(self, cr, pulse_value=None):
        if pulse_value is None:
            pulse_value = self.animation_offset
        pat = cairo.LinearGradient(0, 0, 0, self.height)
        pat.add_color_stop_rgba(0, 0.0, 0.02, 0.05, 0.9)
        pat.add_color_stop_rgba(1, 0.0, 0.0, 0.02, 0.9)
        cr.set_source(pat)
        cr.paint()
        
        for conn in self.connections:
            start_node = self.nodes[conn["source"]]
            end_node = self.nodes[conn["target"]]
            conn_phase = (conn["pulse_offset"] + pulse_value * conn["pulse_speed"]) % (2 * math.pi)
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
            node_phase = (node["pulse_offset"] + pulse_value * node["oscillation_speed"]) % (2 * math.pi)
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
        
        self.header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.header.set_margin_top(10)
        self.header.set_margin_bottom(10)
        self.header.set_margin_start(15)
        self.header.set_margin_end(15)
        
        self.title = Gtk.Label()
        self.title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Hextrix AI Conversation</span>")
        self.header.pack_start(self.title, True, True, 0)
        
        self.actions_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        self.entry = Gtk.Entry()
        self.entry.set_placeholder_text("Type your message here...")
        
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
        
        self.apply_styling()
        
        self.actions_box.pack_start(self.clear_button, False, False, 0)
        self.actions_box.pack_start(self.export_button, False, False, 0)
        
        self.header.pack_end(self.actions_box, False, False, 0)
        self.panel.pack_start(self.header, False, False, 0)
        
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
        
        self.input_area = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        self.input_area.set_margin_start(15)
        self.input_area.set_margin_end(15)
        self.input_area.set_margin_top(10)
        self.input_area.set_margin_bottom(15)
        
        self.input_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        self.entry.connect("activate", self.on_send_clicked)
        
        self.send_button = Gtk.Button()
        send_icon = Gtk.Image.new_from_icon_name("send", Gtk.IconSize.BUTTON)
        self.send_button.add(send_icon)
        self.send_button.connect("clicked", self.on_send_clicked)
        
        self.input_box.pack_start(self.entry, True, True, 0)
        self.input_box.pack_end(self.send_button, False, False, 0)
        
        self.input_area.pack_start(self.input_box, False, False, 0)
        
        self.media_buttons = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.media_buttons.set_halign(Gtk.Align.CENTER)
        self.media_buttons.set_margin_top(10)
        
        self.media_buttons.pack_start(self.mic_button, False, False, 0)
        self.media_buttons.pack_start(self.camera_button, False, False, 0)
        self.media_buttons.pack_start(self.screen_button, False, False, 0)
        
        self.input_area.pack_start(self.media_buttons, False, False, 0)
        
        self.panel.pack_end(self.input_area, False, False, 0)
        
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
        
        self.panel.get_style_context().add_class("chat-panel")
        self.panel.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.header.get_style_context().add_class("chat-header")
        self.header.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.entry.get_style_context().add_class("chat-input")
        self.entry.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
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
            GLib.timeout_add(1000, self.simulate_ai_response, text)
    
    def simulate_ai_response(self, user_text):
        response = f"I received your message: '{user_text}'"
        self.add_message("Hextrix", response)
        return False
    
    def add_message(self, sender, text):
        timestamp = datetime.now().strftime("%I:%M %p")
        bubble_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        bubble_box.set_halign(Gtk.Align.START if sender == "Hextrix" else Gtk.Align.END)
        
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
        
        content_label = Gtk.Label(label=text)
        content_label.set_line_wrap(True)
        content_label.set_max_width_chars(60)
        content_label.set_xalign(0.0)
        content_label.set_margin_top(3)
        
        bubble = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        bubble.add(header_box)
        bubble.add(content_label)
        
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
        
        adj = self.scrolled_window.get_vadjustment()
        adj.set_value(adj.get_upper() - adj.get_page_size())
        
        self.messages.append({"sender": sender, "text": text, "timestamp": timestamp})
    
    def on_clear_clicked(self, widget):
        for child in self.chat_box.get_children():
            self.chat_box.remove(child)
        self.messages = []
    
    def on_export_clicked(self, widget):
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
import json  # Added for dock config handling

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

# Define DockCategory and DockItem classes
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

# [Assuming NeuralNetworkVisualization, ChatPanel, TerminalPanel, FilePanel are defined as in the first version]
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
        
        self.header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.header.set_margin_top(10)
        self.header.set_margin_bottom(10)
        self.header.set_margin_start(15)
        self.header.set_margin_end(15)
        
        self.title = Gtk.Label()
        self.title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Hextrix AI Conversation</span>")
        self.header.pack_start(self.title, True, True, 0)
        
        self.actions_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        self.entry = Gtk.Entry()
        self.entry.set_placeholder_text("Type your message here...")
        
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
        
        self.apply_styling()
        
        self.actions_box.pack_start(self.clear_button, False, False, 0)
        self.actions_box.pack_start(self.export_button, False, False, 0)
        
        self.header.pack_end(self.actions_box, False, False, 0)
        self.panel.pack_start(self.header, False, False, 0)
        
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
        
        self.input_area = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        self.input_area.set_margin_start(15)
        self.input_area.set_margin_end(15)
        self.input_area.set_margin_top(10)
        self.input_area.set_margin_bottom(15)
        
        self.input_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        self.entry.connect("activate", self.on_send_clicked)
        
        self.send_button = Gtk.Button()
        send_icon = Gtk.Image.new_from_icon_name("send", Gtk.IconSize.BUTTON)
        self.send_button.add(send_icon)
        self.send_button.connect("clicked", self.on_send_clicked)
        
        self.input_box.pack_start(self.entry, True, True, 0)
        self.input_box.pack_end(self.send_button, False, False, 0)
        
        self.input_area.pack_start(self.input_box, False, False, 0)
        
        self.media_buttons = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.media_buttons.set_halign(Gtk.Align.CENTER)
        self.media_buttons.set_margin_top(10)
        
        self.media_buttons.pack_start(self.mic_button, False, False, 0)
        self.media_buttons.pack_start(self.camera_button, False, False, 0)
        self.media_buttons.pack_start(self.screen_button, False, False, 0)
        
        self.input_area.pack_start(self.media_buttons, False, False, 0)
        
        self.panel.pack_end(self.input_area, False, False, 0)
        
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
        
        self.panel.get_style_context().add_class("chat-panel")
        self.panel.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.header.get_style_context().add_class("chat-header")
        self.header.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.entry.get_style_context().add_class("chat-input")
        self.entry.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
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
            GLib.timeout_add(1000, self.simulate_ai_response, text)
    
    def simulate_ai_response(self, user_text):
        response = f"I received your message: '{user_text}'"
        self.add_message("Hextrix", response)
        return False
    
    def add_message(self, sender, text):
        timestamp = datetime.now().strftime("%I:%M %p")
        bubble_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        bubble_box.set_halign(Gtk.Align.START if sender == "Hextrix" else Gtk.Align.END)
        
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
        
        content_label = Gtk.Label(label=text)
        content_label.set_line_wrap(True)
        content_label.set_max_width_chars(60)
        content_label.set_xalign(0.0)
        content_label.set_margin_top(3)
        
        bubble = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        bubble.add(header_box)
        bubble.add(content_label)
        
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
        
        adj = self.scrolled_window.get_vadjustment()
        adj.set_value(adj.get_upper() - adj.get_page_size())
        
        self.messages.append({"sender": sender, "text": text, "timestamp": timestamp})
    
    def on_clear_clicked(self, widget):
        for child in self.chat_box.get_children():
            self.chat_box.remove(child)
        self.messages = []
    
    def on_export_clicked(self, widget):
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
        
        terminal.set_color_background(Gdk.RGBA(0, 0.05, 0.1, 0.9))
        terminal.set_color_foreground(Gdk.RGBA(0.8, 0.9, 1, 1))
        
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled.add(terminal)
        
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
        
        page_index = self.notebook.append_page(scrolled, tab_box)
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
import json  # Added for dock config handling

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

# Define DockCategory and DockItem classes
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

# [Assuming NeuralNetworkVisualization, ChatPanel, TerminalPanel, FilePanel are defined as in the first version]
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
        
        self.header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.header.set_margin_top(10)
        self.header.set_margin_bottom(10)
        self.header.set_margin_start(15)
        self.header.set_margin_end(15)
        
        self.title = Gtk.Label()
        self.title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Hextrix AI Conversation</span>")
        self.header.pack_start(self.title, True, True, 0)
        
        self.actions_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        self.entry = Gtk.Entry()
        self.entry.set_placeholder_text("Type your message here...")
        
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
        
        self.apply_styling()
        
        self.actions_box.pack_start(self.clear_button, False, False, 0)
        self.actions_box.pack_start(self.export_button, False, False, 0)
        
        self.header.pack_end(self.actions_box, False, False, 0)
        self.panel.pack_start(self.header, False, False, 0)
        
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
        
        self.input_area = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        self.input_area.set_margin_start(15)
        self.input_area.set_margin_end(15)
        self.input_area.set_margin_top(10)
        self.input_area.set_margin_bottom(15)
        
        self.input_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        self.entry.connect("activate", self.on_send_clicked)
        
        self.send_button = Gtk.Button()
        send_icon = Gtk.Image.new_from_icon_name("send", Gtk.IconSize.BUTTON)
        self.send_button.add(send_icon)
        self.send_button.connect("clicked", self.on_send_clicked)
        
        self.input_box.pack_start(self.entry, True, True, 0)
        self.input_box.pack_end(self.send_button, False, False, 0)
        
        self.input_area.pack_start(self.input_box, False, False, 0)
        
        self.media_buttons = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.media_buttons.set_halign(Gtk.Align.CENTER)
        self.media_buttons.set_margin_top(10)
        
        self.media_buttons.pack_start(self.mic_button, False, False, 0)
        self.media_buttons.pack_start(self.camera_button, False, False, 0)
        self.media_buttons.pack_start(self.screen_button, False, False, 0)
        
        self.input_area.pack_start(self.media_buttons, False, False, 0)
        
        self.panel.pack_end(self.input_area, False, False, 0)
        
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
        
        self.panel.get_style_context().add_class("chat-panel")
        self.panel.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.header.get_style_context().add_class("chat-header")
        self.header.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
        self.entry.get_style_context().add_class("chat-input")
        self.entry.get_style_context().add_provider(css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        
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
            GLib.timeout_add(1000, self.simulate_ai_response, text)
    
    def simulate_ai_response(self, user_text):
        response = f"I received your message: '{user_text}'"
        self.add_message("Hextrix", response)
        return False
    
    def add_message(self, sender, text):
        timestamp = datetime.now().strftime("%I:%M %p")
        bubble_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        bubble_box.set_halign(Gtk.Align.START if sender == "Hextrix" else Gtk.Align.END)
        
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
        
        content_label = Gtk.Label(label=text)
        content_label.set_line_wrap(True)
        content_label.set_max_width_chars(60)
        content_label.set_xalign(0.0)
        content_label.set_margin_top(3)
        
        bubble = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        bubble.add(header_box)
        bubble.add(content_label)
        
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
        
        adj = self.scrolled_window.get_vadjustment()
        adj.set_value(adj.get_upper() - adj.get_page_size())
        
        self.messages.append({"sender": sender, "text": text, "timestamp": timestamp})
    
    def on_clear_clicked(self, widget):
        for child in self.chat_box.get_children():
            self.chat_box.remove(child)
        self.messages = []
    
    def on_export_clicked(self, widget):
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
        # Note: set_encoding is deprecated in newer VTE versions; UTF-8 is default
        # terminal.set_encoding("UTF-8")
        
        terminal.set_color_background(Gdk.RGBA(0, 0.05, 0.1, 0.9))
        terminal.set_color_foreground(Gdk.RGBA(0.8, 0.9, 1, 1))
        
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled.add(terminal)
        
        tab_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        tab_label = Gtk.Label(label=tab_title)  # Updated to avoid deprecation
        tab_box.pack_start(tab_label, True, True, 0)
        
        close_icon = Gtk.Image.new_from_icon_name("window-close-symbolic", Gtk.IconSize.MENU)
        close_button = Gtk.Button()
        close_button.set_relief(Gtk.ReliefStyle.NONE)
        close_button.set_focus_on_click(False)
        close_button.add(close_icon)
        close_button.connect("clicked", self.on_tab_close, scrolled)
        tab_box.pack_end(close_button, False, False, 0)
        tab_box.show_all()
        
        page_index = self.notebook.append_page(scrolled, tab_box)
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
        """Handle changes in the file view selection."""
        model, treeiter = selection.get_selected()
        if treeiter is not None:
            name = model[treeiter][1]
            size = model[treeiter][2]
            modified = model[treeiter][3]
            file_type = model[treeiter][4]
            self.selected_info_label.set_text(f"{name} - {file_type} - {size} - {modified}")
        else:
            self.selected_info_label.set_text("")

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
        self.terminal_panel.set_size_request(500, -1)
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
        self.desktop_panel.set_size_request(400, -1)
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
        
        # Bottom buttons (Term, Files, Apps, Full)
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
        
        self.overlay.add_overlay(self.bottom_buttons)
        
        # Full button (bottom right)
        self.full_button = self.create_circle_button("Full", self.on_full_clicked)
        self.full_button.set_halign(Gtk.Align.END)
        self.full_button.set_valign(Gtk.Align.END)
        self.full_button.set_margin_end(20)
        self.full_button.set_margin_bottom(20)
        
        self.overlay.add_overlay(self.full_button)
        
        # Dock setup
        self.dock_revealer = Gtk.Revealer()
        self.dock_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_UP)
        self.dock_revealer.set_transition_duration(300)
        self.dock_scrolled = Gtk.ScrolledWindow()
        self.dock_scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.NEVER)
        self.dock_scrolled.set_min_content_height(150)
        self.dock_scrolled.set_min_content_width(800)
        self.dock_scrolled.set_max_content_height(150)
        self.dock_revealer.add(self.dock_scrolled)
        self.dock = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.dock.set_halign(Gtk.Align.CENTER)
        self.dock.set_margin_start(15)
        self.dock.set_margin_end(15)
        self.dock.set_margin_bottom(15)
        self.dock.set_margin_top(15)
        self.dock_scrolled.add(self.dock)
        self.central_area.pack_end(self.dock_revealer, False, False, 0)
        
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
        self.terminal_panel_revealer.set_reveal_child(False)
        self.desktop_panel_revealer.set_reveal_child(False)
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
        self.terminal_panel.get_style_context().add_class("terminal-panel")
        self.terminal_panel.get_style_context().add_provider(
            terminal_css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
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
        for child in self.dock.get_children():
            self.dock.remove(child)
        
        for category_id, category in self.categories.items():
            if category.items:
                category_box, revealer, items_box = self.create_category_section(category)
                self.dock.pack_start(category_box, False, False, 5)
        
        self.dock.show_all()

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
        import subprocess
        try:
            subprocess.Popen(dock_item.command.split())
        except Exception as e:
            print(f"Error launching {dock_item.command}: {e}")

    def on_dock_toggle(self, button):
        currently_visible = self.dock_revealer.get_reveal_child()
        self.dock_revealer.set_reveal_child(not currently_visible)

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

    def on_draw(self, widget, cr):
            self.neural_network.width = widget.get_allocated_width()
            self.neural_network.height = widget.get_allocated_height()
            self.neural_network.draw(cr, self.neural_network.animation_offset)
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
        currently_visible = self.terminal_panel_revealer.get_reveal_child()
        self.terminal_panel_revealer.set_reveal_child(not currently_visible)
    
    def on_files_clicked(self, button):
        currently_visible = self.desktop_panel_revealer.get_reveal_child()
        self.desktop_panel_revealer.set_reveal_child(not currently_visible)
    
    def on_full_clicked(self, button):
        window_state = self.get_window().get_state()
        if window_state & Gdk.WindowState.FULLSCREEN:
            self.unfullscreen()
        else:
            self.fullscreen()
    
    def on_terminal_panel_close(self, button):
        self.terminal_panel_revealer.set_reveal_child(False)
    
    def on_desktop_panel_close(self, button):
        self.desktop_panel_revealer.set_reveal_child(False)
    
    def on_key_press(self, widget, event):
        keyval = event.keyval
        if keyval == Gdk.KEY_Escape:
            self.chat_panel.panel.hide()
            self.terminal_panel_revealer.set_reveal_child(False)
            self.desktop_panel_revealer.set_reveal_child(False)
            self.dock_revealer.set_reveal_child(False)
            return True
        if keyval in (Gdk.KEY_F11, Gdk.KEY_f, Gdk.KEY_F):
            self.on_full_clicked(None)
            return True
        if keyval == Gdk.KEY_Tab and event.state & Gdk.ModifierType.CONTROL_MASK:
            if self.chat_panel.panel.get_visible():
                self.chat_panel.panel.hide()
                self.terminal_panel_revealer.set_reveal_child(True)
                self.desktop_panel_revealer.set_reveal_child(False)
                self.dock_revealer.set_reveal_child(False)
            elif self.terminal_panel_revealer.get_reveal_child():
                self.terminal_panel_revealer.set_reveal_child(False)
                self.desktop_panel_revealer.set_reveal_child(True)
                self.chat_panel.panel.hide()
                self.dock_revealer.set_reveal_child(False)
            elif self.desktop_panel_revealer.get_reveal_child():
                self.desktop_panel_revealer.set_reveal_child(False)
                self.dock_revealer.set_reveal_child(True)
                self.chat_panel.panel.hide()
                self.terminal_panel_revealer.set_reveal_child(False)
            elif self.dock_revealer.get_reveal_child():
                self.dock_revealer.set_reveal_child(False)
                self.chat_panel.panel.show()
                self.terminal_panel_revealer.set_reveal_child(False)
                self.desktop_panel_revealer.set_reveal_child(False)
            else:
                self.chat_panel.panel.show()
            return True
        return False

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
        self.terminal_panel.set_size_request(500, -1)
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
        self.desktop_panel.set_size_request(400, -1)
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
        
        # Bottom buttons (Term, Files, Apps, Full)
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
        
        self.overlay.add_overlay(self.bottom_buttons)
        
        # Full button (bottom right)
        self.full_button = self.create_circle_button("Full", self.on_full_clicked)
        self.full_button.set_halign(Gtk.Align.END)
        self.full_button.set_valign(Gtk.Align.END)
        self.full_button.set_margin_end(20)
        self.full_button.set_margin_bottom(20)
        
        self.overlay.add_overlay(self.full_button)
        
        # Dock setup
        self.dock_revealer = Gtk.Revealer()
        self.dock_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_UP)
        self.dock_revealer.set_transition_duration(300)
        self.dock_scrolled = Gtk.ScrolledWindow()
        self.dock_scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.NEVER)
        self.dock_scrolled.set_min_content_height(150)
        self.dock_scrolled.set_min_content_width(800)
        self.dock_scrolled.set_max_content_height(150)
        self.dock_revealer.add(self.dock_scrolled)
        self.dock = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.dock.set_halign(Gtk.Align.CENTER)
        self.dock.set_margin_start(15)
        self.dock.set_margin_end(15)
        self.dock.set_margin_bottom(15)
        self.dock.set_margin_top(15)
        self.dock_scrolled.add(self.dock)
        self.central_area.pack_end(self.dock_revealer, False, False, 0)
        
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
        self.terminal_panel_revealer.set_reveal_child(False)
        self.desktop_panel_revealer.set_reveal_child(False)
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
        self.terminal_panel.get_style_context().add_class("terminal-panel")
        self.terminal_panel.get_style_context().add_provider(
            terminal_css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
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
                transform: scale(1.1);
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
        import subprocess
        try:
            subprocess.Popen(dock_item.command.split())
        except Exception as e:
            print(f"Error launching {dock_item.command}: {e}")

    def on_dock_toggle(self, button):
        currently_visible = self.dock_revealer.get_reveal_child()
        self.dock_revealer.set_reveal_child(not currently_visible)

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
        currently_visible = self.terminal_panel_revealer.get_reveal_child()
        self.terminal_panel_revealer.set_reveal_child(not currently_visible)
    
    def on_files_clicked(self, button):
        currently_visible = self.desktop_panel_revealer.get_reveal_child()
        self.desktop_panel_revealer.set_reveal_child(not currently_visible)
    
    def on_full_clicked(self, button):
        window_state = self.get_window().get_state()
        if window_state & Gdk.WindowState.FULLSCREEN:
            self.unfullscreen()
        else:
            self.fullscreen()
    
    def on_terminal_panel_close(self, button):
        self.terminal_panel_revealer.set_reveal_child(False)
    
    def on_desktop_panel_close(self, button):
        self.desktop_panel_revealer.set_reveal_child(False)
    
    def on_key_press(self, widget, event):
        keyval = event.keyval
        if keyval == Gdk.KEY_Escape:
            self.chat_panel.panel.hide()
            self.terminal_panel_revealer.set_reveal_child(False)
            self.desktop_panel_revealer.set_reveal_child(False)
            self.dock_revealer.set_reveal_child(False)
            return True
        if keyval in (Gdk.KEY_F11, Gdk.KEY_f, Gdk.KEY_F):
            self.on_full_clicked(None)
            return True
        if keyval == Gdk.KEY_Tab and event.state & Gdk.ModifierType.CONTROL_MASK:
            if self.chat_panel.panel.get_visible():
                self.chat_panel.panel.hide()
                self.terminal_panel_revealer.set_reveal_child(True)
                self.desktop_panel_revealer.set_reveal_child(False)
                self.dock_revealer.set_reveal_child(False)
            elif self.terminal_panel_revealer.get_reveal_child():
                self.terminal_panel_revealer.set_reveal_child(False)
                self.desktop_panel_revealer.set_reveal_child(True)
                self.chat_panel.panel.hide()
                self.dock_revealer.set_reveal_child(False)
            elif self.desktop_panel_revealer.get_reveal_child():
                self.desktop_panel_revealer.set_reveal_child(False)
                self.dock_revealer.set_reveal_child(True)
                self.chat_panel.panel.hide()
                self.terminal_panel_revealer.set_reveal_child(False)
            elif self.dock_revealer.get_reveal_child():
                self.dock_revealer.set_reveal_child(False)
                self.chat_panel.panel.show()
                self.terminal_panel_revealer.set_reveal_child(False)
                self.desktop_panel_revealer.set_reveal_child(False)
            else:
                self.chat_panel.panel.show()
            return True
        return False

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
        self.terminal_panel.set_size_request(500, -1)
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
        self.desktop_panel.set_size_request(400, -1)
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
        
        # Bottom buttons (Term, Files, Apps, Full)
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
        
        self.overlay.add_overlay(self.bottom_buttons)
        
        # Full button (bottom right)
        self.full_button = self.create_circle_button("Full", self.on_full_clicked)
        self.full_button.set_halign(Gtk.Align.END)
        self.full_button.set_valign(Gtk.Align.END)
        self.full_button.set_margin_end(20)
        self.full_button.set_margin_bottom(20)
        
        self.overlay.add_overlay(self.full_button)
        
        # Dock setup
        self.dock_revealer = Gtk.Revealer()
        self.dock_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_UP)
        self.dock_revealer.set_transition_duration(300)
        self.dock_scrolled = Gtk.ScrolledWindow()
        self.dock_scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.NEVER)
        self.dock_scrolled.set_min_content_height(150)
        self.dock_scrolled.set_min_content_width(800)
        self.dock_scrolled.set_max_content_height(150)
        self.dock_revealer.add(self.dock_scrolled)
        self.dock = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.dock.set_halign(Gtk.Align.CENTER)
        self.dock.set_margin_start(15)
        self.dock.set_margin_end(15)
        self.dock.set_margin_bottom(15)
        self.dock.set_margin_top(15)
        self.dock_scrolled.add(self.dock)
        self.central_area.pack_end(self.dock_revealer, False, False, 0)
        
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
        self.terminal_panel_revealer.set_reveal_child(False)
        self.desktop_panel_revealer.set_reveal_child(False)
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
        self.terminal_panel.get_style_context().add_class("terminal-panel")
        self.terminal_panel.get_style_context().add_provider(
            terminal_css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
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
                transform: scale(1.1);
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
        import subprocess
        try:
            subprocess.Popen(dock_item.command.split())
        except Exception as e:
            print(f"Error launching {dock_item.command}: {e}")

    def on_dock_toggle(self, button):
        currently_visible = self.dock_revealer.get_reveal_child()
        self.dock_revealer.set_reveal_child(not currently_visible)

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
        currently_visible = self.terminal_panel_revealer.get_reveal_child()
        self.terminal_panel_revealer.set_reveal_child(not currently_visible)
    
    def on_files_clicked(self, button):
        currently_visible = self.desktop_panel_revealer.get_reveal_child()
        self.desktop_panel_revealer.set_reveal_child(not currently_visible)
    
    def on_full_clicked(self, button):
        window_state = self.get_window().get_state()
        if window_state & Gdk.WindowState.FULLSCREEN:
            self.unfullscreen()
        else:
            self.fullscreen()
    
    def on_terminal_panel_close(self, button):
        self.terminal_panel_revealer.set_reveal_child(False)
    
    def on_desktop_panel_close(self, button):
        self.desktop_panel_revealer.set_reveal_child(False)
    
    def on_key_press(self, widget, event):
        keyval = event.keyval
        if keyval == Gdk.KEY_Escape:
            self.chat_panel.panel.hide()
            self.terminal_panel_revealer.set_reveal_child(False)
            self.desktop_panel_revealer.set_reveal_child(False)
            self.dock_revealer.set_reveal_child(False)
            return True
        if keyval in (Gdk.KEY_F11, Gdk.KEY_f, Gdk.KEY_F):
            self.on_full_clicked(None)
            return True
        if keyval == Gdk.KEY_Tab and event.state & Gdk.ModifierType.CONTROL_MASK:
            if self.chat_panel.panel.get_visible():
                self.chat_panel.panel.hide()
                self.terminal_panel_revealer.set_reveal_child(True)
                self.desktop_panel_revealer.set_reveal_child(False)
                self.dock_revealer.set_reveal_child(False)
            elif self.terminal_panel_revealer.get_reveal_child():
                self.terminal_panel_revealer.set_reveal_child(False)
                self.desktop_panel_revealer.set_reveal_child(True)
                self.chat_panel.panel.hide()
                self.dock_revealer.set_reveal_child(False)
            elif self.desktop_panel_revealer.get_reveal_child():
                self.desktop_panel_revealer.set_reveal_child(False)
                self.dock_revealer.set_reveal_child(True)
                self.chat_panel.panel.hide()
                self.terminal_panel_revealer.set_reveal_child(False)
            elif self.dock_revealer.get_reveal_child():
                self.dock_revealer.set_reveal_child(False)
                self.chat_panel.panel.show()
                self.terminal_panel_revealer.set_reveal_child(False)
                self.desktop_panel_revealer.set_reveal_child(False)
            else:
                self.chat_panel.panel.show()
            return True
        return False

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
