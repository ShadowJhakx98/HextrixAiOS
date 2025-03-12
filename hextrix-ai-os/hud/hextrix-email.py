#!/usr/bin/env python3
# Hextrix AI OS - Decentralized Email Client

import gi
import smtplib
import imaplib
import email
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
import datetime
import json
import os
import threading

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango
from hextrix_data_handler import HextrixDataHandler

class HextrixEmail(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Hextrix Email")
        self.set_default_size(1400, 900)
        
        # Initialize data
        self.accounts = self.load_accounts()
        self.current_account = None
        self.contacts = self.load_contacts()
        self.current_thread = None
        self.messages = []
        
        # Setup UI
        self.setup_css()
        self.setup_main_layout()
        self.show_all()
        
    def setup_css(self):
        """Set up custom CSS styling"""
        css_provider = Gtk.CssProvider()
        css = """
            window {
                background-color: rgba(0, 10, 20, 0.95);
            }
            
            .header {
                background-color: rgba(0, 15, 30, 0.9);
                border-bottom: 1px solid rgba(0, 191, 255, 0.5);
                padding: 10px;
            }
            
            .sidebar {
                background-color: rgba(0, 15, 30, 0.9);
                border-right: 1px solid rgba(0, 191, 255, 0.5);
                padding: 10px;
            }
            
            .message-bubble {
                background-color: rgba(0, 20, 40, 0.8);
                border: 1px solid rgba(0, 191, 255, 0.3);
                border-radius: 15px;
                padding: 10px;
                margin: 5px;
                color: #00BFFF;
            }
            
            .message-bubble.sent {
                background-color: rgba(0, 100, 200, 0.5);
            }
            
            .message-bubble.received {
                background-color: rgba(0, 20, 40, 0.8);
            }
            
            .contact-list {
                background-color: rgba(0, 10, 20, 0.85);
            }
            
            .contact-list row {
                padding: 8px;
                border-bottom: 1px solid rgba(0, 100, 200, 0.2);
                color: #00BFFF;
            }
            
            .contact-list row:selected {
                background-color: rgba(0, 100, 200, 0.3);
            }
        """
        css_provider.load_from_data(css.encode())
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(),
            css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        
    def setup_main_layout(self):
        """Set up the main application layout"""
        # Main container
        self.main_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.add(self.main_box)
        
        # Sidebar
        self.setup_sidebar()
        
        # Main content
        self.setup_content()
        
    def setup_sidebar(self):
        """Set up the sidebar with accounts and contacts"""
        sidebar_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        sidebar_box.set_size_request(300, -1)
        sidebar_box.get_style_context().add_class("sidebar")
        
        # Accounts list
        self.accounts_list = Gtk.ListBox()
        self.accounts_list.get_style_context().add_class("contact-list")
        for account in self.accounts:
            row = Gtk.ListBoxRow()
            label = Gtk.Label(label=account['email'])
            row.add(label)
            self.accounts_list.add(row)
        self.accounts_list.connect("row-activated", self.on_account_selected)
        
        # Contacts list
        self.contacts_list = Gtk.ListBox()
        self.contacts_list.get_style_context().add_class("contact-list")
        for contact in self.contacts:
            row = Gtk.ListBoxRow()
            box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
            avatar = Gtk.Label(label=contact['name'][0].upper())
            avatar.get_style_context().add_class("message-bubble")
            name = Gtk.Label(label=contact['name'])
            box.pack_start(avatar, False, False, 0)
            box.pack_start(name, True, True, 0)
            row.add(box)
            self.contacts_list.add(row)
        self.contacts_list.connect("row-activated", self.on_contact_selected)
        
        # Add to sidebar
        sidebar_box.pack_start(self.accounts_list, True, True, 0)
        sidebar_box.pack_start(self.contacts_list, True, True, 0)
        
        self.main_box.pack_start(sidebar_box, False, False, 0)
        
    def setup_content(self):
        """Set up the main content area"""
        content_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        
        # Message view
        self.setup_message_view()
        content_box.pack_start(self.message_box, True, True, 0)
        
        # Compose area
        self.setup_compose_area()
        content_box.pack_start(self.compose_box, False, False, 0)
        
        self.main_box.pack_start(content_box, True, True, 0)
        
    def setup_message_view(self):
        """Set up the message view"""
        self.message_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        
        self.message_scroll = Gtk.ScrolledWindow()
        self.message_list = Gtk.ListBox()
        self.message_list.set_selection_mode(Gtk.SelectionMode.NONE)
        
        self.message_scroll.add(self.message_list)
        self.message_box.pack_start(self.message_scroll, True, True, 0)
        
    def setup_compose_area(self):
        """Set up the compose area"""
        self.compose_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        
        # Subject
        subject_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        subject_label = Gtk.Label(label="Subject:")
        self.subject_entry = Gtk.Entry()
        subject_box.pack_start(subject_label, False, False, 0)
        subject_box.pack_start(self.subject_entry, True, True, 0)
        
        # Message
        self.message_text = Gtk.TextView()
        self.message_text.set_wrap_mode(Gtk.WrapMode.WORD)
        
        # Send button
        send_button = Gtk.Button(label="Send")
        send_button.connect("clicked", self.on_send_message)
        
        self.compose_box.pack_start(subject_box, False, False, 0)
        self.compose_box.pack_start(self.message_text, True, True, 0)
        self.compose_box.pack_start(send_button, False, False, 0)
        
    def on_account_selected(self, listbox, row):
        """Handle account selection"""
        self.current_account = self.accounts[row.get_index()]
        self.load_messages()
        
    def on_contact_selected(self, listbox, row):
        """Handle contact selection"""
        contact = self.contacts[row.get_index()]
        self.current_thread = f"{self.current_account['email']}-{contact['email']}"
        self.load_thread_messages()
        
    def load_messages(self):
        """Load messages for the selected account"""
        if not self.current_account:
            return
            
        # Clear existing messages
        self.message_list.foreach(lambda widget: self.message_list.remove(widget))
        
        # Connect to IMAP server
        try:
            mail = imaplib.IMAP4_SSL(self.current_account['imap_server'])
            mail.login(self.current_account['email'], self.current_account['password'])
            mail.select('inbox')
            
            # Search for messages
            status, messages = mail.search(None, 'ALL')
            if status == 'OK':
                for num in messages[0].split():
                    status, data = mail.fetch(num, '(RFC822)')
                    if status == 'OK':
                        email_message = email.message_from_bytes(data[0][1])
                        self.add_message_to_view(email_message, 'received')
                        
            mail.logout()
        except Exception as e:
            print(f"Error loading messages: {e}")
            
    def load_thread_messages(self):
        """Load messages for the current thread"""
        if not self.current_thread:
            return
            
        # Clear existing messages
        self.message_list.foreach(lambda widget: self.message_list.remove(widget))
        
        # Load messages from thread
        thread_file = os.path.join(os.path.dirname(__file__), "threads", f"{self.current_thread}.json")
        if os.path.exists(thread_file):
            with open(thread_file, 'r') as f:
                messages = json.load(f)
                for message in messages:
                    self.add_message_to_view(message)
                    
    def add_message_to_view(self, message, direction=None):
        """Add a message to the view"""
        row = Gtk.ListBoxRow()
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        box.get_style_context().add_class("message-bubble")
        
        if direction:
            box.get_style_context().add_class(direction)
            
        # Sender and date
        header_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        sender = Gtk.Label(label=message['from'])
        date = Gtk.Label(label=message['date'])
        header_box.pack_start(sender, True, True, 0)
        header_box.pack_start(date, False, False, 0)
        
        # Message content
        content = Gtk.Label(label=message['content'])
        content.set_line_wrap(True)
        content.set_xalign(0)
        
        box.pack_start(header_box, False, False, 0)
        box.pack_start(content, True, True, 0)
        row.add(box)
        self.message_list.add(row)
        self.message_list.show_all()
        
    def on_send_message(self, button):
        """Handle sending a message"""
        if not self.current_account or not self.current_thread:
            return
            
        # Get message content
        subject = self.subject_entry.get_text()
        buffer = self.message_text.get_buffer()
        start_iter = buffer.get_start_iter()
        end_iter = buffer.get_end_iter()
        content = buffer.get_text(start_iter, end_iter, True)
        
        # Create message
        msg = MIMEMultipart()
        msg['From'] = self.current_account['email']
        msg['To'] = self.current_thread.split('-')[1]
        msg['Subject'] = subject
        msg.attach(MIMEText(content, 'plain'))
        
        # Send message
        try:
            server = smtplib.SMTP(self.current_account['smtp_server'], 587)
            server.starttls()
            server.login(self.current_account['email'], self.current_account['password'])
            server.send_message(msg)
            server.quit()
            
            # Save to thread
            self.save_message_to_thread({
                'from': self.current_account['email'],
                'to': self.current_thread.split('-')[1],
                'date': datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                'subject': subject,
                'content': content
            })
            
            # Clear compose area
            self.subject_entry.set_text("")
            buffer.set_text("")
            
            # Reload messages
            self.load_thread_messages()
        except Exception as e:
            print(f"Error sending message: {e}")
            
    def save_message_to_thread(self, message):
        """Save message to thread file"""
        thread_file = os.path.join(os.path.dirname(__file__), "threads", f"{self.current_thread}.json")
        messages = []
        
        if os.path.exists(thread_file):
            with open(thread_file, 'r') as f:
                messages = json.load(f)
                
        messages.append(message)
        
        with open(thread_file, 'w') as f:
            json.dump(messages, f)
            
    def load_accounts(self):
        """Load email accounts"""
        accounts_file = os.path.join(os.path.dirname(__file__), "accounts.json")
        if os.path.exists(accounts_file):
            with open(accounts_file, 'r') as f:
                return json.load(f)
        return []
        
    def load_contacts(self):
        """Load contacts"""
        contacts_file = os.path.join(os.path.dirname(__file__), "contacts.json")
        if os.path.exists(contacts_file):
            with open(contacts_file, 'r') as f:
                return json.load(f)
        return []
        
    def check_for_shared_data(self):
        """Check for shared data from other apps"""
        # Check for contact export
        contact_export = self.data_handler.load_data("email_contact_export.json")
        if contact_export:
            self.import_contact(contact_export)
            os.remove(os.path.join(self.data_handler.data_dir, "email_contact_export.json"))
            
        # Check for event invites
        event_invite = self.data_handler.load_data("email_event_invite.json")
        if event_invite:
            self.create_email_from_event(event_invite)
            os.remove(os.path.join(self.data_handler.data_dir, "email_event_invite.json"))
            
    def import_contact(self, contact_data):
        """Import contact from shared data"""
        if not any(c['email'] == contact_data['email'] for c in self.contacts):
            self.contacts.append(contact_data)
            self.data_handler.save_data("shared_contacts.json", self.contacts)
            self.update_contacts_list()
            
    def create_email_from_event(self, event_data):
        """Create email from event data"""
        self.current_thread = f"{self.current_account['email']}-{event_data['recipients'][0]}"
        self.subject_entry.set_text(event_data['subject'])
        buffer = self.message_text.get_buffer()
        buffer.set_text(event_data['body'])
        
if __name__ == "__main__":
    app = HextrixEmail()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()