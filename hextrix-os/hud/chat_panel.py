#!/usr/bin/env python3
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib
from datetime import datetime

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