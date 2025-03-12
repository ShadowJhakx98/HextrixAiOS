#!/usr/bin/env python3
# Hextrix AI OS - Advanced Contacts Application

import gi
import json
import os
from pathlib import Path

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango, Gio, GObject
from hextrix_data_handler import HextrixDataHandler

class HextrixContacts(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Hextrix Contacts")
        self.set_default_size(1200, 800)
        
        # Initialize data
        self.contacts_file = os.path.expanduser("~/.hextrix-contacts.json")
        self.data_handler = HextrixDataHandler()
        self.contacts = self.data_handler.get_shared_contacts() or self.load_contacts()
        
    def save_contacts(self):
        """Save contacts to shared storage"""
        self.data_handler.save_data("shared_contacts.json", self.contacts)
        
    def export_to_email(self, contact):
        """Export contact to email app"""
        email_data = {
            'name': contact['name'],
            'email': contact['email'],
            'phone': contact.get('phone', ''),
            'address': contact.get('address', '')
        }
        self.data_handler.save_data("email_contact_export.json", email_data)
        
    def setup_css(self):
        """Set up custom CSS styling"""
        css_provider = Gtk.CssProvider()
        css = """
            window {
                background-color: rgba(0, 10, 20, 0.95);
            }
            
            .sidebar {
                background-color: rgba(0, 15, 30, 0.9);
                border-right: 1px solid rgba(0, 191, 255, 0.5);
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
            
            .contact-details {
                background-color: rgba(0, 5, 15, 0.9);
                padding: 20px;
            }
            
            .contact-details label {
                color: #00BFFF;
            }
            
            .contact-details entry {
                background-color: rgba(0, 20, 50, 0.8);
                color: #00BFFF;
                border: 1px solid rgba(0, 191, 255, 0.5);
                border-radius: 4px;
                padding: 5px;
            }
            
            .headerbar {
                background-color: rgba(0, 15, 30, 0.9);
                border-bottom: 1px solid rgba(0, 191, 255, 0.5);
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
        
        # Sidebar with contact list
        self.setup_sidebar()
        
        # Main content area
        self.setup_contact_details()
        
    def setup_sidebar(self):
        """Set up the sidebar with contact list and search"""
        self.sidebar = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.sidebar.set_size_request(300, -1)
        self.sidebar.get_style_context().add_class("sidebar")
        
        # Search bar
        self.search_entry = Gtk.SearchEntry()
        self.search_entry.set_placeholder_text("Search contacts...")
        self.search_entry.connect("search-changed", self.on_search_changed)
        self.sidebar.pack_start(self.search_entry, False, False, 0)
        
        # Contact list
        self.contact_list = Gtk.ListBox()
        self.contact_list.set_selection_mode(Gtk.SelectionMode.SINGLE)
        self.contact_list.connect("row-activated", self.on_contact_selected)
        self.contact_list.get_style_context().add_class("contact-list")
        
        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        scrolled_window.add(self.contact_list)
        self.sidebar.pack_start(scrolled_window, True, True, 0)
        
        # Add sidebar to main box
        self.main_box.pack_start(self.sidebar, False, False, 0)
        
        # Populate contact list
        self.populate_contact_list()
        
    def setup_contact_details(self):
        """Set up the contact details area"""
        self.details_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        self.details_box.set_margin_start(20)
        self.details_box.set_margin_end(20)
        self.details_box.set_margin_top(20)
        self.details_box.set_margin_bottom(20)
        self.details_box.get_style_context().add_class("contact-details")
        
        # Contact photo
        self.photo_button = Gtk.Button()
        self.photo_button.set_size_request(150, 150)
        self.photo_button.connect("clicked", self.on_change_photo)
        self.details_box.pack_start(self.photo_button, False, False, 0)
        
        # Contact fields
        self.fields = {
            "name": self.create_field("Full Name"),
            "phone": self.create_field("Phone"),
            "email": self.create_field("Email"),
            "address": self.create_field("Address"),
            "company": self.create_field("Company"),
            "notes": self.create_field("Notes", multi_line=True)
        }
        
        # Action buttons
        button_box = Gtk.Box(spacing=10)
        self.save_button = Gtk.Button(label="Save")
        self.save_button.connect("clicked", self.on_save_contact)
        button_box.pack_start(self.save_button, True, True, 0)
        
        self.delete_button = Gtk.Button(label="Delete")
        self.delete_button.connect("clicked", self.on_delete_contact)
        button_box.pack_start(self.delete_button, True, True, 0)
        
        self.details_box.pack_end(button_box, False, False, 0)
        
        # Add to main box
        self.main_box.pack_start(self.details_box, True, True, 0)
        
    def create_field(self, label, multi_line=False):
        """Create a labeled input field"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        lbl = Gtk.Label(label=label)
        lbl.set_xalign(0)
        
        if multi_line:
            entry = Gtk.TextView()
            entry.set_wrap_mode(Gtk.WrapMode.WORD)
            entry.set_size_request(-1, 100)
            scrolled = Gtk.ScrolledWindow()
            scrolled.add(entry)
            box.pack_start(lbl, False, False, 0)
            box.pack_start(scrolled, True, True, 0)
        else:
            entry = Gtk.Entry()
            box.pack_start(lbl, False, False, 0)
            box.pack_start(entry, False, False, 0)
            
        self.details_box.pack_start(box, False, False, 0)
        return entry
        
    def populate_contact_list(self):
        """Populate the contact list with all contacts"""
        self.contact_list.foreach(lambda widget: self.contact_list.remove(widget))
        
        for contact in self.contacts:
            row = Gtk.ListBoxRow()
            box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
            
            # Contact photo
            photo = Gtk.Image.new_from_icon_name("avatar-default-symbolic", Gtk.IconSize.DND)
            box.pack_start(photo, False, False, 0)
            
            # Contact name
            name = Gtk.Label(label=contact.get("name", "Unnamed"))
            name.set_xalign(0)
            box.pack_start(name, True, True, 0)
            
            row.add(box)
            self.contact_list.add(row)
            
    def on_contact_selected(self, listbox, row):
        """Handle contact selection"""
        index = row.get_index()
        self.current_contact = self.contacts[index]
        self.display_contact_details()
        
    def display_contact_details(self):
        """Display details of the selected contact"""
        if not self.current_contact:
            return
            
        # Update fields
        self.fields["name"].set_text(self.current_contact.get("name", ""))
        self.fields["phone"].set_text(self.current_contact.get("phone", ""))
        self.fields["email"].set_text(self.current_contact.get("email", ""))
        self.fields["address"].set_text(self.current_contact.get("address", ""))
        self.fields["company"].set_text(self.current_contact.get("company", ""))
        self.fields["notes"].get_buffer().set_text(self.current_contact.get("notes", ""))
        
        # Update photo
        photo_path = self.current_contact.get("photo")
        if photo_path and os.path.exists(photo_path):
            self.photo_button.set_image(Gtk.Image.new_from_file(photo_path))
        else:
            self.photo_button.set_image(Gtk.Image.new_from_icon_name(
                "avatar-default-symbolic", Gtk.IconSize.DND))
                
    def on_save_contact(self, button):
        """Save the current contact"""
        if not self.current_contact:
            return
            
        # Update contact data
        self.current_contact.update({
            "name": self.fields["name"].get_text(),
            "phone": self.fields["phone"].get_text(),
            "email": self.fields["email"].get_text(),
            "address": self.fields["address"].get_text(),
            "company": self.fields["company"].get_text(),
            "notes": self.fields["notes"].get_buffer().get_text(
                self.fields["notes"].get_buffer().get_start_iter(),
                self.fields["notes"].get_buffer().get_end_iter(),
                True)
        })
        
        self.save_contacts()
        self.populate_contact_list()
        
    def on_delete_contact(self, button):
        """Delete the current contact"""
        if not self.current_contact:
            return
            
        self.contacts.remove(self.current_contact)
        self.current_contact = None
        self.save_contacts()
        self.populate_contact_list()
        self.clear_contact_details()
        
    def clear_contact_details(self):
        """Clear the contact details area"""
        for field in self.fields.values():
            if isinstance(field, Gtk.Entry):
                field.set_text("")
            else:
                field.get_buffer().set_text("")
                
        self.photo_button.set_image(Gtk.Image.new_from_icon_name(
            "avatar-default-symbolic", Gtk.IconSize.DND))
            
    def on_change_photo(self, button):
        """Handle changing the contact photo"""
        dialog = Gtk.FileChooserDialog(
            title="Select Photo",
            parent=self,
            action=Gtk.FileChooserAction.OPEN
        )
        dialog.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OPEN, Gtk.ResponseType.OK
        )
        
        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            photo_path = dialog.get_filename()
            self.current_contact["photo"] = photo_path
            self.photo_button.set_image(Gtk.Image.new_from_file(photo_path))
        dialog.destroy()
        
    def on_search_changed(self, entry):
        """Handle search text changes"""
        search_text = entry.get_text().lower()
        for row in self.contact_list.get_children():
            label = row.get_children()[0].get_children()[1]
            name = label.get_text().lower()
            row.set_visible(search_text in name)
            
    def load_contacts(self):
        """Load contacts from file"""
        if os.path.exists(self.contacts_file):
            with open(self.contacts_file, 'r') as f:
                return json.load(f)
        return []
        
    def save_contacts(self):
        """Save contacts to file"""
        with open(self.contacts_file, 'w') as f:
            json.dump(self.contacts, f, indent=2)
            
if __name__ == "__main__":
    app = HextrixContacts()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()