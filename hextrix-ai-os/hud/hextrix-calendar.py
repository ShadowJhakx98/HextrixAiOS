#!/usr/bin/env python3
# Hextrix AI OS - Advanced Calendar Application

import gi
import datetime
from dateutil import rrule
import calendar
import json
import os

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango
from hextrix_data_handler import HextrixDataHandler

class HextrixCalendar(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Hextrix Calendar")
        self.set_default_size(1200, 800)
        
        # Initialize data
        self.current_date = datetime.date.today()
        self.events = self.load_events()
        self.selected_event = None
        self.view_mode = "month"  # Can be "month", "week", "day", "year"
        
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
            
            .calendar {
                background-color: rgba(0, 10, 20, 0.9);
                padding: 10px;
            }
            
            .calendar-day {
                background-color: rgba(0, 20, 40, 0.8);
                border: 1px solid rgba(0, 191, 255, 0.3);
                color: #00BFFF;
                padding: 10px;
                margin: 2px;
            }
            
            .calendar-day:hover {
                background-color: rgba(0, 100, 200, 0.5);
            }
            
            .calendar-day.today {
                background-color: rgba(0, 100, 200, 0.3);
                border: 2px solid #00BFFF;
            }
            
            .calendar-day.selected {
                background-color: rgba(0, 150, 255, 0.5);
            }
            
            .event-panel {
                background-color: rgba(0, 15, 30, 0.9);
                border-left: 1px solid rgba(0, 191, 255, 0.5);
                padding: 10px;
            }
            
            .event-list {
                background-color: rgba(0, 10, 20, 0.85);
            }
            
            .event-list row {
                padding: 8px;
                border-bottom: 1px solid rgba(0, 100, 200, 0.2);
                color: #00BFFF;
            }
            
            .event-list row:selected {
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
        self.main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.add(self.main_box)
        
        # Header
        self.setup_header()
        
        # Main content
        self.setup_content()
        
    def setup_header(self):
        """Set up the calendar header"""
        header_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        header_box.get_style_context().add_class("header")
        
        # Navigation buttons
        self.prev_button = Gtk.Button(label="<")
        self.prev_button.connect("clicked", self.on_prev)
        header_box.pack_start(self.prev_button, False, False, 0)
        
        self.next_button = Gtk.Button(label=">")
        self.next_button.connect("clicked", self.on_next)
        header_box.pack_start(self.next_button, False, False, 0)
        
        # Date display
        self.date_label = Gtk.Label()
        self.date_label.set_markup(f"<big>{self.current_date.strftime('%B %Y')}</big>")
        header_box.pack_start(self.date_label, True, True, 0)
        
        # View mode switcher
        view_mode_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        self.view_mode_buttons = {
            "year": Gtk.Button(label="Year"),
            "month": Gtk.Button(label="Month"),
            "week": Gtk.Button(label="Week"),
            "day": Gtk.Button(label="Day")
        }
        for mode, button in self.view_mode_buttons.items():
            button.connect("clicked", self.on_view_mode_changed, mode)
            view_mode_box.pack_start(button, False, False, 0)
        header_box.pack_start(view_mode_box, False, False, 0)
        
        # Add header to main box
        self.main_box.pack_start(header_box, False, False, 0)
        
    def setup_content(self):
        """Set up the main content area"""
        content_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        
        # Calendar view
        self.setup_calendar_view()
        content_box.pack_start(self.calendar_box, True, True, 0)
        
        # Event panel
        self.setup_event_panel()
        content_box.pack_start(self.event_box, False, False, 0)
        
        # Add to main box
        self.main_box.pack_start(content_box, True, True, 0)
        
    def setup_calendar_view(self):
        """Set up the calendar view"""
        self.calendar_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.calendar_box.get_style_context().add_class("calendar")
        
        # Weekday headers
        weekdays_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=2)
        for day in ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]:
            label = Gtk.Label(label=day)
            label.set_xalign(0.5)
            weekdays_box.pack_start(label, True, True, 0)
        self.calendar_box.pack_start(weekdays_box, False, False, 0)
        
        # Calendar grid
        self.calendar_grid = Gtk.Grid()
        self.calendar_grid.set_row_homogeneous(True)
        self.calendar_grid.set_column_homogeneous(True)
        self.calendar_grid.set_row_spacing(2)
        self.calendar_grid.set_column_spacing(2)
        
        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.add(self.calendar_grid)
        self.calendar_box.pack_start(scrolled_window, True, True, 0)
        
        self.update_calendar_view()
        
    def setup_event_panel(self):
        """Set up the event panel"""
        self.event_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.event_box.set_size_request(300, -1)
        self.event_box.get_style_context().add_class("event-panel")
        
        # Event list
        self.event_list = Gtk.ListBox()
        self.event_list.get_style_context().add_class("event-list")
        
        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.add(self.event_list)
        self.event_box.pack_start(scrolled_window, True, True, 0)
        
        # Event controls
        event_controls = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        
        self.add_event_button = Gtk.Button(label="Add Event")
        self.add_event_button.connect("clicked", self.on_add_event)
        event_controls.pack_start(self.add_event_button, True, True, 0)
        
        self.edit_event_button = Gtk.Button(label="Edit")
        self.edit_event_button.connect("clicked", self.on_edit_event)
        event_controls.pack_start(self.edit_event_button, True, True, 0)
        
        self.delete_event_button = Gtk.Button(label="Delete")
        self.delete_event_button.connect("clicked", self.on_delete_event)
        event_controls.pack_start(self.delete_event_button, True, True, 0)
        
        self.event_box.pack_start(event_controls, False, False, 0)
        
        self.update_event_list()
        
    def update_calendar_view(self):
        """Update the calendar view based on current date and view mode"""
        # Clear existing calendar
        for child in self.calendar_grid.get_children():
            self.calendar_grid.remove(child)
            
        if self.view_mode == "month":
            self.show_month_view()
        elif self.view_mode == "week":
            self.show_week_view()
        elif self.view_mode == "day":
            self.show_day_view()
        elif self.view_mode == "year":
            self.show_year_view()
            
        self.calendar_grid.show_all()
        
    def show_month_view(self):
        """Show the month view"""
        cal = calendar.Calendar()
        month_days = cal.monthdayscalendar(self.current_date.year, self.current_date.month)
        
        for week_num, week in enumerate(month_days):
            for day_num, day in enumerate(week):
                if day == 0:
                    continue
                    
                day_box = self.create_day_box(day)
                self.calendar_grid.attach(day_box, day_num, week_num, 1, 1)
                
    def create_day_box(self, day):
        """Create a day box with events"""
        day_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        day_box.get_style_context().add_class("calendar-day")
        
        # Highlight today
        if (day == self.current_date.day and
            self.current_date.month == datetime.date.today().month and
            self.current_date.year == datetime.date.today().year):
            day_box.get_style_context().add_class("today")
            
        # Day number
        day_label = Gtk.Label(label=str(day))
        day_box.pack_start(day_label, False, False, 0)
        
        # Events
        date = datetime.date(self.current_date.year, self.current_date.month, day)
        events = self.get_events_for_date(date)
        for event in events:
            event_label = Gtk.Label(label=event["title"])
            event_label.set_xalign(0)
            day_box.pack_start(event_label, False, False, 0)
            
        # Connect click handler
        day_box.connect("button-press-event", self.on_day_clicked, date)
        
        return day_box
        
    def on_day_clicked(self, widget, event, date):
        """Handle day click"""
        self.current_date = date
        self.update_event_list()
        self.update_date_label()
        
    def update_event_list(self):
        """Update the event list with events for the selected date"""
        self.event_list.foreach(lambda widget: self.event_list.remove(widget))
        
        events = self.get_events_for_date(self.current_date)
        for event in events:
            row = Gtk.ListBoxRow()
            box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
            
            # Event title
            title = Gtk.Label(label=event["title"])
            title.set_xalign(0)
            box.pack_start(title, True, True, 0)
            
            # Event time
            time = Gtk.Label(label=event["time"])
            box.pack_start(time, False, False, 0)
            
            row.add(box)
            self.event_list.add(row)
            
    def get_events_for_date(self, date):
        """Get events for a specific date"""
        return [event for event in self.events if event["date"] == date.isoformat()]
        
    def on_prev(self, button):
        """Handle previous button click"""
        if self.view_mode == "month":
            self.current_date = self.current_date.replace(day=1) - datetime.timedelta(days=1)
        elif self.view_mode == "week":
            self.current_date -= datetime.timedelta(weeks=1)
        elif self.view_mode == "day":
            self.current_date -= datetime.timedelta(days=1)
        elif self.view_mode == "year":
            self.current_date = self.current_date.replace(year=self.current_date.year - 1)
            
        self.update_calendar_view()
        self.update_date_label()
        
    def on_next(self, button):
        """Handle next button click"""
        if self.view_mode == "month":
            self.current_date = (self.current_date.replace(day=28) + 
                               datetime.timedelta(days=4)).replace(day=1)
        elif self.view_mode == "week":
            self.current_date += datetime.timedelta(weeks=1)
        elif self.view_mode == "day":
            self.current_date += datetime.timedelta(days=1)
        elif self.view_mode == "year":
            self.current_date = self.current_date.replace(year=self.current_date.year + 1)
            
        self.update_calendar_view()
        self.update_date_label()
        
    def on_view_mode_changed(self, button, mode):
        """Handle view mode change"""
        self.view_mode = mode
        self.update_calendar_view()
        
    def update_date_label(self):
        """Update the date label based on current view mode"""
        if self.view_mode == "month":
            self.date_label.set_markup(f"<big>{self.current_date.strftime('%B %Y')}</big>")
        elif self.view_mode == "week":
            self.date_label.set_markup(f"<big>Week {self.current_date.isocalendar()[1]}</big>")
        elif self.view_mode == "day":
            self.date_label.set_markup(f"<big>{self.current_date.strftime('%B %d, %Y')}</big>")
        elif self.view_mode == "year":
            self.date_label.set_markup(f"<big>{self.current_date.year}</big>")
            
    def on_add_event(self, button):
        """Handle add event button click"""
        dialog = Gtk.Dialog(title="Add Event", parent=self, flags=0)
        dialog.add_buttons(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                         Gtk.STOCK_OK, Gtk.ResponseType.OK)
        
        content_area = dialog.get_content_area()
        
        # Event title
        title_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        title_label = Gtk.Label(label="Title:")
        self.event_title = Gtk.Entry()
        title_box.pack_start(title_label, False, False, 0)
        title_box.pack_start(self.event_title, True, True, 0)
        content_area.pack_start(title_box, False, False, 0)
        
        # Event date
        date_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        date_label = Gtk.Label(label="Date:")
        self.event_date = Gtk.Entry()
        self.event_date.set_text(self.current_date.isoformat())
        date_box.pack_start(date_label, False, False, 0)
        date_box.pack_start(self.event_date, True, True, 0)
        content_area.pack_start(date_box, False, False, 0)
        
        # Event time
        time_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        time_label = Gtk.Label(label="Time:")
        self.event_time = Gtk.Entry()
        self.event_time.set_text(self.selected_event["time"])
        time_box.pack_start(time_label, False, False, 0)
        time_box.pack_start(self.event_time, True, True, 0)
        content_area.pack_start(time_box, False, False, 0)
        
        dialog.show_all()
        response = dialog.run()
        
        if response == Gtk.ResponseType.OK:
            self.events.append({
                "title": self.event_title.get_text(),
                "date": self.event_date.get_text(),
                "time": self.event_time.get_text()
            })
            self.save_events()
            self.update_event_list()
            self.update_calendar_view()
            
        dialog.destroy()
        
    def on_edit_event(self, button):
        """Handle edit event button click"""
        selected_row = self.event_list.get_selected_row()
        if not selected_row:
            return
            
        index = selected_row.get_index()
        self.selected_event = self.events[index]
        
        dialog = Gtk.Dialog(title="Edit Event", parent=self, flags=0)
        dialog.add_buttons(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                         Gtk.STOCK_OK, Gtk.ResponseType.OK)
        
        content_area = dialog.get_content_area()
        
        # Event title
        title_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        title_label = Gtk.Label(label="Title:")
        self.event_title = Gtk.Entry()
        self.event_title.set_text(self.selected_event["title"])
        title_box.pack_start(title_label, False, False, 0)
        title_box.pack_start(self.event_title, True, True, 0)
        content_area.pack_start(title_box, False, False, 0)
        
        # Event date
        date_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        date_label = Gtk.Label(label="Date:")
        self.event_date = Gtk.Entry()
        self.event_date.set_text(self.selected_event["date"])
        date_box.pack_start(date_label, False, False, 0)
        date_box.pack_start(self.event_date, True, True, 0)
        content_area.pack_start(date_box, False, False, 0)
        
        # Event time
        time_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
        time_label = Gtk.Label(label="Time:")
        self.event_time = Gtk.Entry()
        self.event_time.set_text(self.selected_event["time"])
        time_box.pack_start(time_label, False, False, 0)
        time_box.pack_start(self.event_time, True, True, 0)
        content_area.pack_start(time_box, False, False, 0)
        
        dialog.show_all()
        response = dialog.run()
        
        if response == Gtk.ResponseType.OK:
            self.selected_event["title"] = self.event_title.get_text()
            self.selected_event["date"] = self.event_date.get_text()
            self.selected_event["time"] = self.event_time.get_text()
            self.save_events()
            self.update_event_list()
            self.update_calendar_view()
            
        dialog.destroy()
        
    def on_delete_event(self, button):
        """Handle delete event button click"""
        selected_row = self.event_list.get_selected_row()
        if not selected_row:
            return
            
        index = selected_row.get_index()
        del self.events[index]
        self.save_events()
        self.update_event_list()
        self.update_calendar_view()
        
    def load_events(self):
        """Load events from file"""
        events_file = os.path.join(os.path.dirname(__file__), "events.json")
        if os.path.exists(events_file):
            with open(events_file, 'r') as f:
                return json.load(f)
        return []
        
    def save_events(self):
        """Save events to file"""
        events_file = os.path.join(os.path.dirname(__file__), "events.json")
        with open(events_file, 'w') as f:
            json.dump(self.events, f)
            
    def show_week_view(self):
        """Show the week view"""
        start_of_week = self.current_date - datetime.timedelta(days=self.current_date.weekday())
        
        for day_num in range(7):
            date = start_of_week + datetime.timedelta(days=day_num)
            day_box = self.create_day_box(date.day)
            self.calendar_grid.attach(day_box, day_num, 0, 1, 1)
            
    def show_day_view(self):
        """Show the day view"""
        day_box = self.create_day_box(self.current_date.day)
        self.calendar_grid.attach(day_box, 0, 0, 1, 1)
        
    def show_year_view(self):
        """Show the year view"""
        for month in range(1, 13):
            month_label = Gtk.Label(label=calendar.month_name[month])
            month_label.set_xalign(0.5)
            self.calendar_grid.attach(month_label, (month-1)%3, (month-1)//3, 1, 1)
            
    def create_event_with_contact(self, contact):
        """Create event linked to a contact"""
        event = {
            'title': f"Meeting with {contact['name']}",
            'date': datetime.date.today().isoformat(),
            'time': "10:00",
            'contact': contact['email'],
            'description': f"Meeting with {contact['name']} at {contact.get('address', '')}"
        }
        self.events.append(event)
        self.save_events()
        
    def send_event_invite(self, event):
        """Send event invite via email"""
        email_data = {
            'event': event,
            'recipients': [event['contact']],
            'subject': f"Event Invitation: {event['title']}",
            'body': f"You're invited to: {event['title']}\n\nDetails:\n{event['description']}"
        }
        self.data_handler.save_data("email_event_invite.json", email_data)
        
if __name__ == "__main__":
    app = HextrixCalendar()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()