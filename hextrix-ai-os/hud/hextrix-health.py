#!/usr/bin/env python3
# Hextrix AI OS - Comprehensive Health Tracking

import gi
import json
import os
import datetime
import matplotlib.pyplot as plt
from matplotlib.figure import Figure
from matplotlib.backends.backend_gtk3agg import FigureCanvasGTK3Agg as FigureCanvas
import numpy as np
from pathlib import Path

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango

class HextrixDataHandler:
    def __init__(self):
        self.data_dir = os.path.expanduser("~/.hextrix")
        Path(self.data_dir).mkdir(parents=True, exist_ok=True)
        
    def save_data(self, filename, data):
        path = os.path.join(self.data_dir, filename)
        with open(path, 'w') as f:
            json.dump(data, f)
            
    def load_data(self, filename):
        path = os.path.join(self.data_dir, filename)
        if os.path.exists(path):
            with open(path, 'r') as f:
                return json.load(f)
        return None

class HextrixHealth(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Hextrix Health")
        self.set_default_size(1400, 900)
        
        # Initialize data
        self.data_handler = HextrixDataHandler()
        self.health_data = self.load_health_data()
        self.current_view = "dashboard"
        self.settings = self.load_settings()
        
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
            
            .health-card {
                background-color: rgba(0, 20, 40, 0.8);
                border: 1px solid rgba(0, 191, 255, 0.3);
                border-radius: 15px;
                padding: 15px;
                margin: 10px;
                color: #00BFFF;
            }
            
            .health-card:hover {
                background-color: rgba(0, 100, 200, 0.5);
            }
            
            .tracker-list {
                background-color: rgba(0, 10, 20, 0.85);
            }
            
            .tracker-list row {
                padding: 8px;
                border-bottom: 1px solid rgba(0, 100, 200, 0.2);
                color: #00BFFF;
            }
            
            .tracker-list row:selected {
                background-color: rgba(0, 100, 200, 0.3);
            }
            
            .stats-container {
                background-color: rgba(0, 15, 30, 0.8);
                border: 1px solid rgba(0, 191, 255, 0.3);
                border-radius: 10px;
                padding: 10px;
                margin: 10px;
            }
            
            .settings-section {
                background-color: rgba(0, 20, 40, 0.8);
                border: 1px solid rgba(0, 191, 255, 0.3);
                border-radius: 10px;
                padding: 15px;
                margin: 10px;
            }
            
            .settings-label {
                color: #00BFFF;
                font-size: 14px;
                margin-bottom: 5px;
            }
            
            .settings-switch {
                margin: 5px;
            }
            
            .settings-entry {
                background-color: rgba(0, 30, 60, 0.7);
                color: #00BFFF;
                border: 1px solid rgba(0, 191, 255, 0.5);
                border-radius: 5px;
                padding: 5px;
            }
            
            .chart-label {
                color: #00BFFF;
                font-size: 16px;
                font-weight: bold;
                margin: 10px;
            }
            
            button {
                background-color: rgba(0, 40, 80, 0.7);
                color: #00BFFF;
                border: 1px solid rgba(0, 150, 255, 0.5);
                border-radius: 5px;
                padding: 8px 15px;
                transition: all 0.2s ease;
            }
            
            button:hover {
                background-color: rgba(0, 70, 130, 0.8);
                border-color: rgba(0, 200, 255, 0.8);
            }
            
            combobox {
                background-color: rgba(0, 30, 60, 0.7);
                color: #00BFFF;
                border: 1px solid rgba(0, 191, 255, 0.5);
                border-radius: 5px;
                padding: 5px;
            }
            
            entry {
                background-color: rgba(0, 30, 60, 0.7);
                color: #00BFFF;
                border: 1px solid rgba(0, 191, 255, 0.5);
                border-radius: 5px;
                padding: 8px;
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
        """Set up the sidebar with navigation"""
        sidebar_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        sidebar_box.set_size_request(300, -1)
        sidebar_box.get_style_context().add_class("sidebar")
        
        # Navigation buttons
        nav_items = [
            ("Dashboard", "dashboard"),
            ("Period Tracker", "period"),
            ("Sexual Health", "sexual"),
            ("Masturbation Log", "masturbation"),
            ("Statistics", "stats"),
            ("Settings", "settings")
        ]
        
        for label, view in nav_items:
            button = Gtk.Button(label=label)
            button.connect("clicked", self.on_nav_clicked, view)
            sidebar_box.pack_start(button, False, False, 5)
            
        self.main_box.pack_start(sidebar_box, False, False, 0)
        
    def setup_content(self):
        """Set up the main content area"""
        self.content_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.main_box.pack_start(self.content_box, True, True, 0)
        
        # Initialize dashboard
        self.setup_dashboard()
        
    def setup_dashboard(self):
        """Set up the dashboard view"""
        # Clear existing content
        for child in self.content_box.get_children():
            self.content_box.remove(child)
            
        # Create cards
        period_status = self.health_data.get('period', {}).get('current_status', 'N/A')
        sexual_count = len(self.health_data.get('sexual_activity', []))
        masturbation_count = len(self.health_data.get('masturbation', []))
        
        cards = [
            ("Period Cycle", period_status),
            ("Sexual Activity", f"{sexual_count} records"),
            ("Masturbation", f"{masturbation_count} records"),
            ("Health Stats", "View Insights")
        ]
        
        for title, value in cards:
            card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
            card.get_style_context().add_class("health-card")
            
            title_label = Gtk.Label()
            title_label.set_markup(f"<big>{title}</big>")
            value_label = Gtk.Label(label=value)
            value_label.set_markup(f"<span color='#00FFFF'>{value}</span>")
            
            card.pack_start(title_label, False, False, 0)
            card.pack_start(value_label, False, False, 0)
            self.content_box.pack_start(card, False, False, 10)
            
        self.content_box.show_all()
        
    def setup_period_tracker(self):
        """Set up period tracking view"""
        # Clear existing content
        for child in self.content_box.get_children():
            self.content_box.remove(child)
            
        # Period tracking form
        form_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=15)
        form_box.set_margin_top(20)
        form_box.set_margin_bottom(20)
        form_box.set_margin_start(20)
        form_box.set_margin_end(20)
        
        # Form title
        title_label = Gtk.Label()
        title_label.set_markup("<span font='16' weight='bold' color='#00BFFF'>Period Tracking</span>")
        form_box.pack_start(title_label, False, False, 10)
        
        # Cycle start
        start_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        start_label = Gtk.Label(label="Cycle Start:")
        start_label.set_markup("<span color='#00BFFF'>Cycle Start:</span>")
        start_label.set_size_request(120, -1)
        start_label.set_xalign(0)
        self.cycle_start = Gtk.Entry()
        self.cycle_start.set_placeholder_text("YYYY-MM-DD")
        start_box.pack_start(start_label, False, False, 0)
        start_box.pack_start(self.cycle_start, True, True, 0)
        
        # Symptoms
        symptoms_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        symptoms_label = Gtk.Label(label="Symptoms:")
        symptoms_label.set_markup("<span color='#00BFFF'>Symptoms:</span>")
        symptoms_label.set_xalign(0)
        
        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.set_min_content_height(200)
        
        self.symptoms_entry = Gtk.TextView()
        self.symptoms_entry.set_wrap_mode(Gtk.WrapMode.WORD)
        scrolled_window.add(self.symptoms_entry)
        
        symptoms_box.pack_start(symptoms_label, False, False, 0)
        symptoms_box.pack_start(scrolled_window, True, True, 0)
        
        # Save button
        save_button = Gtk.Button(label="Save")
        save_button.connect("clicked", self.on_save_period)
        save_button.set_margin_top(15)
        
        # Add everything to form
        form_box.pack_start(start_box, False, False, 0)
        form_box.pack_start(symptoms_box, True, True, 0)
        form_box.pack_start(save_button, False, False, 0)
        
        self.content_box.pack_start(form_box, True, True, 0)
        self.content_box.show_all()
        
    def setup_sexual_health(self):
        """Set up sexual health tracking"""
        # Clear existing content
        for child in self.content_box.get_children():
            self.content_box.remove(child)
            
        # Sexual activity form
        form_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=15)
        form_box.set_margin_top(20)
        form_box.set_margin_bottom(20)
        form_box.set_margin_start(20)
        form_box.set_margin_end(20)
        
        # Form title
        title_label = Gtk.Label()
        title_label.set_markup("<span font='16' weight='bold' color='#00BFFF'>Sexual Health Tracker</span>")
        form_box.pack_start(title_label, False, False, 10)
        
        # Date
        date_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        date_label = Gtk.Label()
        date_label.set_markup("<span color='#00BFFF'>Date:</span>")
        date_label.set_size_request(120, -1)
        date_label.set_xalign(0)
        self.sex_date = Gtk.Entry()
        self.sex_date.set_placeholder_text("YYYY-MM-DD")
        self.sex_date.set_text(datetime.date.today().isoformat())
        date_box.pack_start(date_label, False, False, 0)
        date_box.pack_start(self.sex_date, True, True, 0)
        
        # Partner
        partner_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        partner_label = Gtk.Label()
        partner_label.set_markup("<span color='#00BFFF'>Partner:</span>")
        partner_label.set_size_request(120, -1)
        partner_label.set_xalign(0)
        self.partner_entry = Gtk.Entry()
        self.partner_entry.set_placeholder_text("Partner name")
        partner_box.pack_start(partner_label, False, False, 0)
        partner_box.pack_start(self.partner_entry, True, True, 0)
        
        # Protection
        protection_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        protection_label = Gtk.Label()
        protection_label.set_markup("<span color='#00BFFF'>Protection:</span>")
        protection_label.set_size_request(120, -1)
        protection_label.set_xalign(0)
        
        self.protection_combo = Gtk.ComboBoxText()
        self.protection_combo.append_text("Yes")
        self.protection_combo.append_text("No")
        self.protection_combo.set_active(0)
        
        protection_box.pack_start(protection_label, False, False, 0)
        protection_box.pack_start(self.protection_combo, True, True, 0)
        
        # Save button
        save_button = Gtk.Button(label="Save")
        save_button.connect("clicked", self.on_save_sexual_activity)
        save_button.set_margin_top(15)
        
        # Add to form
        form_box.pack_start(date_box, False, False, 0)
        form_box.pack_start(partner_box, False, False, 0)
        form_box.pack_start(protection_box, False, False, 0)
        form_box.pack_start(save_button, False, False, 0)
        
        # Recent records
        records_label = Gtk.Label()
        records_label.set_markup("<span font='14' weight='bold' color='#00BFFF'>Recent Records</span>")
        records_label.set_margin_top(20)
        form_box.pack_start(records_label, False, False, 10)
        
        # Records list
        records_scroll = Gtk.ScrolledWindow()
        records_scroll.set_min_content_height(200)
        
        self.records_list = Gtk.ListBox()
        self.records_list.get_style_context().add_class("tracker-list")
        
        # Populate records
        sexual_records = self.health_data.get('sexual_activity', [])
        for record in reversed(sexual_records[:10]):  # Show last 10 records
            record_row = Gtk.ListBoxRow()
            record_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
            
            date_label = Gtk.Label()
            date_label.set_markup(f"<span color='#00FFFF'>{record.get('date', 'Unknown')}</span>")
            date_label.set_size_request(120, -1)
            
            partner_label = Gtk.Label(label=record.get('partner', 'Unknown'))
            protection_label = Gtk.Label()
            protection_label.set_markup(f"<span color='{'#00FF00' if record.get('protection') == 'Yes' else '#FF6666'}'>{record.get('protection', 'Unknown')}</span>")
            
            record_box.pack_start(date_label, False, False, 0)
            record_box.pack_start(partner_label, True, True, 0)
            record_box.pack_start(protection_label, False, False, 0)
            
            record_row.add(record_box)
            self.records_list.add(record_row)
        
        records_scroll.add(self.records_list)
        form_box.pack_start(records_scroll, True, True, 0)
        
        self.content_box.pack_start(form_box, True, True, 0)
        self.content_box.show_all()
        
    def setup_masturbation_tracker(self):
        """Set up masturbation tracking"""
        # Clear existing content
        for child in self.content_box.get_children():
            self.content_box.remove(child)
            
        # Masturbation form
        form_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=15)
        form_box.set_margin_top(20)
        form_box.set_margin_bottom(20)
        form_box.set_margin_start(20)
        form_box.set_margin_end(20)
        
        # Form title
        title_label = Gtk.Label()
        title_label.set_markup("<span font='16' weight='bold' color='#00BFFF'>Masturbation Log</span>")
        form_box.pack_start(title_label, False, False, 10)
        
        # Date
        date_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        date_label = Gtk.Label()
        date_label.set_markup("<span color='#00BFFF'>Date:</span>")
        date_label.set_size_request(120, -1)
        date_label.set_xalign(0)
        self.masturbation_date = Gtk.Entry()
        self.masturbation_date.set_placeholder_text("YYYY-MM-DD")
        self.masturbation_date.set_text(datetime.date.today().isoformat())
        date_box.pack_start(date_label, False, False, 0)
        date_box.pack_start(self.masturbation_date, True, True, 0)
        
        # Duration
        duration_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        duration_label = Gtk.Label()
        duration_label.set_markup("<span color='#00BFFF'>Duration (mins):</span>")
        duration_label.set_size_request(120, -1)
        duration_label.set_xalign(0)
        self.duration_entry = Gtk.Entry()
        self.duration_entry.set_placeholder_text("Duration in minutes")
        duration_box.pack_start(duration_label, False, False, 0)
        duration_box.pack_start(self.duration_entry, True, True, 0)
        
        # Notes
        notes_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        notes_label = Gtk.Label()
        notes_label.set_markup("<span color='#00BFFF'>Notes:</span>")
        notes_label.set_xalign(0)
        
        notes_scroll = Gtk.ScrolledWindow()
        notes_scroll.set_min_content_height(100)
        
        self.notes_entry = Gtk.TextView()
        self.notes_entry.set_wrap_mode(Gtk.WrapMode.WORD)
        notes_scroll.add(self.notes_entry)
        
        notes_box.pack_start(notes_label, False, False, 0)
        notes_box.pack_start(notes_scroll, True, True, 0)
        
        # Save button
        save_button = Gtk.Button(label="Save")
        save_button.connect("clicked", self.on_save_masturbation)
        save_button.set_margin_top(15)
        
        # Add to form
        form_box.pack_start(date_box, False, False, 0)
        form_box.pack_start(duration_box, False, False, 0)
        form_box.pack_start(notes_box, True, True, 0)
        form_box.pack_start(save_button, False, False, 0)
        
        # Recent records
        records_label = Gtk.Label()
        records_label.set_markup("<span font='14' weight='bold' color='#00BFFF'>Recent Records</span>")
        records_label.set_margin_top(20)
        form_box.pack_start(records_label, False, False, 10)
        
        # Records list
        records_scroll = Gtk.ScrolledWindow()
        records_scroll.set_min_content_height(200)
        
        self.m_records_list = Gtk.ListBox()
        self.m_records_list.get_style_context().add_class("tracker-list")
        
        # Populate records
        masturbation_records = self.health_data.get('masturbation', [])
        for record in reversed(masturbation_records[:10]):  # Show last 10 records
            record_row = Gtk.ListBoxRow()
            record_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
            
            date_label = Gtk.Label()
            date_label.set_markup(f"<span color='#00FFFF'>{record.get('date', 'Unknown')}</span>")
            date_label.set_size_request(120, -1)
            
            duration_label = Gtk.Label(label=f"{record.get('duration', '0')} mins")
            
            record_box.pack_start(date_label, False, False, 0)
            record_box.pack_start(duration_label, True, True, 0)
            
            record_row.add(record_box)
            self.m_records_list.add(record_row)
        
        records_scroll.add(self.m_records_list)
        form_box.pack_start(records_scroll, True, True, 0)
        
        self.content_box.pack_start(form_box, True, True, 0)
        self.content_box.show_all()
        
    def setup_statistics(self):
        """Set up statistics view"""
        # Clear existing content
        for child in self.content_box.get_children():
            self.content_box.remove(child)
            
        # Statistics container
        stats_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        stats_box.set_margin_top(20)
        stats_box.set_margin_bottom(20)
        stats_box.set_margin_start(20)
        stats_box.set_margin_end(20)
        
        # Title
        title_label = Gtk.Label()
        title_label.set_markup("<span font='18' weight='bold' color='#00BFFF'>Health Statistics</span>")
        stats_box.pack_start(title_label, False, False, 10)
        
        # Period cycle statistics
        if 'period' in self.health_data and 'history' in self.health_data['period']:
            period_stats = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
            period_stats.get_style_context().add_class("stats-container")
            
            period_title = Gtk.Label()
            period_title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Period Cycle Analysis</span>")
            period_stats.pack_start(period_title, False, False, 10)
            
            # Calculate period stats
            period_history = self.health_data['period']['history']
            if len(period_history) >= 2:
                # Get cycle lengths
                cycle_lengths = []
                dates = []
                for i in range(len(period_history) - 1):
                    try:
                        start_date = datetime.datetime.strptime(period_history[i]['date'], "%Y-%m-%d").date()
                        end_date = datetime.datetime.strptime(period_history[i+1]['date'], "%Y-%m-%d").date()
                        cycle_length = (end_date - start_date).days
                        cycle_lengths.append(cycle_length)
                        dates.append(start_date)
                    except (ValueError, KeyError):
                        continue
                
                if cycle_lengths:
                    avg_cycle = sum(cycle_lengths) / len(cycle_lengths)
                    min_cycle = min(cycle_lengths)
                    max_cycle = max(cycle_lengths)
                    
                    stats_info = Gtk.Label()
                    stats_info.set_markup(
                        f"<span color='#FFFFFF'>Average Cycle Length: <span color='#00FFFF'>{avg_cycle:.1f} days</span>\n"
                        f"Shortest Cycle: <span color='#00FFFF'>{min_cycle} days</span>\n"
                        f"Longest Cycle: <span color='#00FFFF'>{max_cycle} days</span>\n"
                        f"Number of Tracked Cycles: <span color='#00FFFF'>{len(cycle_lengths)}</span></span>"
                    )
                    stats_info.set_xalign(0)
                    period_stats.pack_start(stats_info, False, False, 5)
                    
                    # Create plot
                    if len(cycle_lengths) > 1:
                        figure = Figure(figsize=(8, 4), dpi=100)
                        figure.patch.set_facecolor((0, 0.05, 0.1, 0.9))
                        ax = figure.add_subplot(111)
                        ax.plot(range(len(cycle_lengths)), cycle_lengths, 'o-', color='#00BFFF')
                        ax.set_xlabel('Cycle Number')
                        ax.set_ylabel('Days')
                        ax.set_title('Period Cycle Length Over Time')
                        ax.set_facecolor((0, 0.05, 0.1, 0.9))
                        ax.tick_params(colors='#00BFFF')
                        ax.grid(True, alpha=0.3)
                        for spine in ax.spines.values():
                            spine.set_color('#00BFFF')
                        
                        canvas = FigureCanvas(figure)
                        canvas.set_size_request(600, 300)
                        period_stats.pack_start(canvas, False, False, 10)
            else:
                no_data_label = Gtk.Label()
                no_data_label.set_markup("<span color='#AAAAAA'>Not enough data for period cycle analysis</span>")
                period_stats.pack_start(no_data_label, False, False, 5)
                
            stats_box.pack_start(period_stats, False, False, 10)
        
        # Sexual activity statistics
        sexual_activity = self.health_data.get('sexual_activity', [])
        if sexual_activity:
            sexual_stats = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
            sexual_stats.get_style_context().add_class("stats-container")
            
            sexual_title = Gtk.Label()
            sexual_title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Sexual Activity Analysis</span>")
            sexual_stats.pack_start(sexual_title, False, False, 10)
            
            # Calculate stats
            total_activities = len(sexual_activity)
            
            # Count protection used
            protection_used = len([a for a in sexual_activity if a.get('protection') == 'Yes'])
            protection_percent = (protection_used / total_activities * 100) if total_activities > 0 else 0
            
            # Count unique partners
            unique_partners = len(set(a.get('partner', '') for a in sexual_activity))
            
            stats_info = Gtk.Label()
            stats_info.set_markup(
                f"<span color='#FFFFFF'>Total Recorded Activities: <span color='#00FFFF'>{total_activities}</span>\n"
                f"Protection Used: <span color='#00FFFF'>{protection_percent:.1f}%</span>\n"
                f"Unique Partners: <span color='#00FFFF'>{unique_partners}</span></span>"
            )
            stats_info.set_xalign(0)
            sexual_stats.pack_start(stats_info, False, False, 5)
            
            # Create protection usage chart
            figure = Figure(figsize=(5, 3), dpi=100)
            figure.patch.set_facecolor((0, 0.05, 0.1, 0.9))
            ax = figure.add_subplot(111)
            
            labels = ['Protection Used', 'No Protection']
            sizes = [protection_used, total_activities - protection_used]
            colors = ['#00BFFF', '#FF6666']
            
            ax.pie(sizes, labels=labels, colors=colors, autopct='%1.1f%%', startangle=90)
            ax.axis('equal')
            ax.set_title('Protection Usage', color='#00BFFF')
            
            canvas = FigureCanvas(figure)
            canvas.set_size_request(300, 200)
            sexual_stats.pack_start(canvas, False, False, 10)
            
            stats_box.pack_start(sexual_stats, False, False, 10)
            
        # Masturbation statistics
        masturbation_data = self.health_data.get('masturbation', [])
        if masturbation_data:
            masturbation_stats = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
            masturbation_stats.get_style_context().add_class("stats-container")
            
            masturbation_title = Gtk.Label()
            masturbation_title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Masturbation Analysis</span>")
            masturbation_stats.pack_start(masturbation_title, False, False, 10)
            
            # Calculate stats
            total_records = len(masturbation_data)
            
            # Calculate average duration
            durations = []
            for record in masturbation_data:
                try:
                    duration = float(record.get('duration', 0))
                    durations.append(duration)
                except (ValueError, TypeError):
                    continue
                    
            avg_duration = sum(durations) / len(durations) if durations else 0
            
            # Calculate frequency stats
            date_counts = {}
            for record in masturbation_data:
                try:
                    date = datetime.datetime.strptime(record.get('date', ''), "%Y-%m-%d").date()
                    month_year = date.strftime("%Y-%m")
                    date_counts[month_year] = date_counts.get(month_year, 0) + 1
                except (ValueError, KeyError):
                    continue
            
            stats_info = Gtk.Label()
            stats_info.set_markup(
                f"<span color='#FFFFFF'>Total Records: <span color='#00FFFF'>{total_records}</span>\n"
                f"Average Duration: <span color='#00FFFF'>{avg_duration:.1f} mins</span></span>"
            )
            stats_info.set_xalign(0)
            masturbation_stats.pack_start(stats_info, False, False, 5)
            
            # Create monthly frequency chart
            if date_counts:
                figure = Figure(figsize=(8, 4), dpi=100)
                figure.patch.set_facecolor((0, 0.05, 0.1, 0.9))
                ax = figure.add_subplot(111)
                
                months = sorted(date_counts.keys())
                frequencies = [date_counts[m] for m in months]
                
                # Convert to readable month names
                month_labels = [datetime.datetime.strptime(m, "%Y-%m").strftime("%b %Y") for m in months]
                
                ax.bar(range(len(months)), frequencies, color='#00BFFF')
                ax.set_xticks(range(len(months)))
                ax.set_xticklabels(month_labels, rotation=45, ha='right')
                ax.set_xlabel('Month')
                ax.set_ylabel('Frequency')
                ax.set_title('Monthly Frequency', color='#00BFFF')
                ax.set_facecolor((0, 0.05, 0.1, 0.9))
                ax.tick_params(colors='#00BFFF')
                ax.grid(True, alpha=0.3)
                for spine in ax.spines.values():
                    spine.set_color('#00BFFF')
                
                figure.tight_layout()
                canvas = FigureCanvas(figure)
                canvas.set_size_request(600, 300)
                masturbation_stats.pack_start(canvas, False, False, 10)
                
            stats_box.pack_start(masturbation_stats, False, False, 10)
            
        # If no data at all
        if not self.health_data or (not self.health_data.get('period') and 
                                     not self.health_data.get('sexual_activity') and 
                                     not self.health_data.get('masturbation')):
            no_data_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
            no_data_box.get_style_context().add_class("stats-container")
            
            no_data_label = Gtk.Label()
            no_data_label.set_markup("<span color='#AAAAAA'>No health data available for statistics. Start tracking your health to see insights.</span>")
            no_data_box.pack_start(no_data_label, False, False, 10)
            
            stats_box.pack_start(no_data_box, False, False, 10)
        
        self.content_box.pack_start(stats_box, True, True, 0)
        self.content_box.show_all()
        
    def setup_settings(self):
        """Set up settings view"""
        # Clear existing content
        for child in self.content_box.get_children():
            self.content_box.remove(child)
            
        # Settings container
        settings_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        settings_box.set_margin_top(20)
        settings_box.set_margin_bottom(20)
        settings_box.set_margin_start(20)
        settings_box.set_margin_end(20)
        
        # Title
        title_label = Gtk.Label()
        title_label.set_markup("<span font='18' weight='bold' color='#00BFFF'>Settings</span>")
        settings_box.pack_start(title_label, False, False, 10)
        
        # General settings
        general_settings = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        general_settings.get_style_context().add_class("settings-section")
        
        general_title = Gtk.Label()
        general_title.set_markup("<span font='14' weight='bold' color='#00BFFF'>General Settings</span>")
        general_title.set_xalign(0)
        general_settings.pack_start(general_title, False, False, 5)
        
        # Encryption setting
        encryption_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        encryption_label = Gtk.Label()
        encryption_label.set_markup("<span color='#FFFFFF'>Enable Data Encryption</span>")
        encryption_label.set_xalign(0)
        
        self.encryption_switch = Gtk.Switch()
        self.encryption_switch.set_active(self.settings.get('encryption_enabled', False))
        self.encryption_switch.connect("notify::active", self.on_encryption_toggled)
        
        encryption_box.pack_start(encryption_label, True, True, 0)
        encryption_box.pack_start(self.encryption_switch, False, False, 0)
        general_settings.pack_start(encryption_box, False, False, 5)
        
        # Notifications setting
        notifications_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        notifications_label = Gtk.Label()
        notifications_label.set_markup("<span color='#FFFFFF'>Enable Notifications</span>")
        notifications_label.set_xalign(0)
        
        self.notifications_switch = Gtk.Switch()
        self.notifications_switch.set_active(self.settings.get('notifications_enabled', True))
        self.notifications_switch.connect("notify::active", self.on_notifications_toggled)
        
        notifications_box.pack_start(notifications_label, True, True, 0)
        notifications_box.pack_start(self.notifications_switch, False, False, 0)
        general_settings.pack_start(notifications_box, False, False, 5)
        
        settings_box.pack_start(general_settings, False, False, 10)
        
        # Period tracker settings
        period_settings = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        period_settings.get_style_context().add_class("settings-section")
        
        period_title = Gtk.Label()
        period_title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Period Tracker Settings</span>")
        period_title.set_xalign(0)
        period_settings.pack_start(period_title, False, False, 5)
        
        # Cycle length
        cycle_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        cycle_label = Gtk.Label()
        cycle_label.set_markup("<span color='#FFFFFF'>Default Cycle Length (days)</span>")
        cycle_label.set_xalign(0)
        
        self.cycle_entry = Gtk.Entry()
        self.cycle_entry.set_text(str(self.settings.get('default_cycle_length', 28)))
        self.cycle_entry.set_width_chars(5)
        self.cycle_entry.get_style_context().add_class("settings-entry")
        self.cycle_entry.connect("changed", self.on_cycle_length_changed)
        
        cycle_box.pack_start(cycle_label, True, True, 0)
        cycle_box.pack_start(self.cycle_entry, False, False, 0)
        period_settings.pack_start(cycle_box, False, False, 5)
        
        # Period reminders
        reminder_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        reminder_label = Gtk.Label()
        reminder_label.set_markup("<span color='#FFFFFF'>Period Reminders</span>")
        reminder_label.set_xalign(0)
        
        self.reminder_switch = Gtk.Switch()
        self.reminder_switch.set_active(self.settings.get('period_reminders', True))
        self.reminder_switch.connect("notify::active", self.on_period_reminder_toggled)
        
        reminder_box.pack_start(reminder_label, True, True, 0)
        reminder_box.pack_start(self.reminder_switch, False, False, 0)
        period_settings.pack_start(reminder_box, False, False, 5)
        
        settings_box.pack_start(period_settings, False, False, 10)
        
        # Security settings
        security_settings = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        security_settings.get_style_context().add_class("settings-section")
        
        security_title = Gtk.Label()
        security_title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Security Settings</span>")
        security_title.set_xalign(0)
        security_settings.pack_start(security_title, False, False, 5)
        
        # PIN protection
        pin_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        pin_label = Gtk.Label()
        pin_label.set_markup("<span color='#FFFFFF'>Enable PIN Protection</span>")
        pin_label.set_xalign(0)
        
        self.pin_switch = Gtk.Switch()
        self.pin_switch.set_active(self.settings.get('pin_enabled', False))
        self.pin_switch.connect("notify::active", self.on_pin_toggled)
        
        pin_box.pack_start(pin_label, True, True, 0)
        pin_box.pack_start(self.pin_switch, False, False, 0)
        security_settings.pack_start(pin_box, False, False, 5)
        
        # Set PIN
        self.pin_setting_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        pin_setting_label = Gtk.Label()
        pin_setting_label.set_markup("<span color='#FFFFFF'>Set PIN</span>")
        pin_setting_label.set_xalign(0)
        
        self.pin_entry = Gtk.Entry()
        self.pin_entry.set_visibility(False)
        self.pin_entry.set_placeholder_text("Enter 4-digit PIN")
        self.pin_entry.set_max_length(4)
        self.pin_entry.get_style_context().add_class("settings-entry")
        
        self.pin_setting_box.pack_start(pin_setting_label, True, True, 0)
        self.pin_setting_box.pack_start(self.pin_entry, False, False, 0)
        security_settings.pack_start(self.pin_setting_box, False, False, 5)
        
        # Show/hide PIN entry based on switch state
        self.pin_setting_box.set_visible(self.pin_switch.get_active())
        
        settings_box.pack_start(security_settings, False, False, 10)
        
        # Data management
        data_settings = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        data_settings.get_style_context().add_class("settings-section")
        
        data_title = Gtk.Label()
        data_title.set_markup("<span font='14' weight='bold' color='#00BFFF'>Data Management</span>")
        data_title.set_xalign(0)
        data_settings.pack_start(data_title, False, False, 5)
        
        # Export data
        export_button = Gtk.Button(label="Export Health Data")
        export_button.connect("clicked", self.on_export_data)
        data_settings.pack_start(export_button, False, False, 5)
        
        # Import data
        import_button = Gtk.Button(label="Import Health Data")
        import_button.connect("clicked", self.on_import_data)
        data_settings.pack_start(import_button, False, False, 5)
        
        # Erase all data
        erase_button = Gtk.Button(label="Erase All Data")
        erase_button.connect("clicked", self.on_erase_data)
        data_settings.pack_start(erase_button, False, False, 5)
        
        settings_box.pack_start(data_settings, False, False, 10)
        
        # Save settings button
        save_button = Gtk.Button(label="Save Settings")
        save_button.connect("clicked", self.on_save_settings)
        settings_box.pack_start(save_button, False, False, 10)
        
        self.content_box.pack_start(settings_box, True, True, 0)
        self.content_box.show_all()
        
    def on_nav_clicked(self, button, view):
        """Handle navigation button clicks"""
        self.current_view = view
        if view == "dashboard":
            self.setup_dashboard()
        elif view == "period":
            self.setup_period_tracker()
        elif view == "sexual":
            self.setup_sexual_health()
        elif view == "masturbation":
            self.setup_masturbation_tracker()
        elif view == "stats":
            self.setup_statistics()
        elif view == "settings":
            self.setup_settings()
            
    def on_save_period(self, button):
        """Save period tracking data"""
        period_data = {
            'date': self.cycle_start.get_text(),
            'symptoms': self.symptoms_entry.get_buffer().get_text(
                self.symptoms_entry.get_buffer().get_start_iter(),
                self.symptoms_entry.get_buffer().get_end_iter(),
                True
            )
        }
        
        if 'period' not in self.health_data:
            self.health_data['period'] = {}
            
        self.health_data['period']['current_status'] = "Active"
        self.health_data['period']['history'] = self.health_data['period'].get('history', [])
        self.health_data['period']['history'].append(period_data)
        
        self.save_health_data()
        self.setup_dashboard()
        
    def on_save_sexual_activity(self, button):
        """Save sexual activity data"""
        activity = {
            'date': self.sex_date.get_text(),
            'partner': self.partner_entry.get_text(),
            'protection': self.protection_combo.get_active_text()
        }
        
        self.health_data['sexual_activity'] = self.health_data.get('sexual_activity', [])
        self.health_data['sexual_activity'].append(activity)
        
        self.save_health_data()
        self.setup_dashboard()
        
    def on_save_masturbation(self, button):
        """Save masturbation data"""
        notes_buffer = self.notes_entry.get_buffer()
        notes_text = notes_buffer.get_text(
            notes_buffer.get_start_iter(),
            notes_buffer.get_end_iter(),
            True
        )
        
        activity = {
            'date': self.masturbation_date.get_text(),
            'duration': self.duration_entry.get_text(),
            'notes': notes_text
        }
        
        self.health_data['masturbation'] = self.health_data.get('masturbation', [])
        self.health_data['masturbation'].append(activity)
        
        self.save_health_data()
        self.setup_dashboard()
        
    def on_encryption_toggled(self, switch, param):
        """Handle encryption toggle"""
        self.settings['encryption_enabled'] = switch.get_active()
        
    def on_notifications_toggled(self, switch, param):
        """Handle notifications toggle"""
        self.settings['notifications_enabled'] = switch.get_active()
        
    def on_cycle_length_changed(self, entry):
        """Handle cycle length change"""
        try:
            length = int(entry.get_text())
            self.settings['default_cycle_length'] = length
        except ValueError:
            pass
            
    def on_period_reminder_toggled(self, switch, param):
        """Handle period reminder toggle"""
        self.settings['period_reminders'] = switch.get_active()
        
    def on_pin_toggled(self, switch, param):
        """Handle PIN toggle"""
        self.settings['pin_enabled'] = switch.get_active()
        self.pin_setting_box.set_visible(switch.get_active())
        
    def on_save_settings(self, button):
        """Save settings"""
        # Update PIN if set
        if self.settings.get('pin_enabled', False):
            pin = self.pin_entry.get_text()
            if pin.isdigit() and len(pin) == 4:
                self.settings['pin'] = pin
            
        self.save_settings()
        
        # Show confirmation dialog
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.INFO,
            buttons=Gtk.ButtonsType.OK,
            text="Settings Saved"
        )
        dialog.format_secondary_text("Your settings have been saved successfully.")
        dialog.run()
        dialog.destroy()
        
    def on_export_data(self, button):
        """Export health data to a file"""
        dialog = Gtk.FileChooserDialog(
            title="Export Health Data",
            parent=self,
            action=Gtk.FileChooserAction.SAVE
        )
        dialog.set_do_overwrite_confirmation(True)
        dialog.set_current_name("hextrix_health_data.json")
        
        dialog.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_SAVE, Gtk.ResponseType.OK
        )
        
        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            filename = dialog.get_filename()
            try:
                with open(filename, 'w') as f:
                    json.dump(self.health_data, f, indent=2)
                    
                success_dialog = Gtk.MessageDialog(
                    transient_for=self,
                    flags=0,
                    message_type=Gtk.MessageType.INFO,
                    buttons=Gtk.ButtonsType.OK,
                    text="Export Successful"
                )
                success_dialog.format_secondary_text(f"Data exported to {filename}")
                success_dialog.run()
                success_dialog.destroy()
            except Exception as e:
                error_dialog = Gtk.MessageDialog(
                    transient_for=self,
                    flags=0,
                    message_type=Gtk.MessageType.ERROR,
                    buttons=Gtk.ButtonsType.OK,
                    text="Export Failed"
                )
                error_dialog.format_secondary_text(str(e))
                error_dialog.run()
                error_dialog.destroy()
                
        dialog.destroy()
        
    def on_import_data(self, button):
        """Import health data from a file"""
        dialog = Gtk.FileChooserDialog(
            title="Import Health Data",
            parent=self,
            action=Gtk.FileChooserAction.OPEN
        )
        
        dialog.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OPEN, Gtk.ResponseType.OK
        )
        
        filter_json = Gtk.FileFilter()
        filter_json.set_name("JSON files")
        filter_json.add_mime_type("application/json")
        filter_json.add_pattern("*.json")
        dialog.add_filter(filter_json)
        
        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            filename = dialog.get_filename()
            try:
                with open(filename, 'r') as f:
                    data = json.load(f)
                    
                # Confirm import
                confirm_dialog = Gtk.MessageDialog(
                    transient_for=self,
                    flags=0,
                    message_type=Gtk.MessageType.QUESTION,
                    buttons=Gtk.ButtonsType.YES_NO,
                    text="Confirm Import"
                )
                confirm_dialog.format_secondary_text(
                    "Importing will replace your current health data. Continue?"
                )
                confirm_response = confirm_dialog.run()
                confirm_dialog.destroy()
                
                if confirm_response == Gtk.ResponseType.YES:
                    self.health_data = data
                    self.save_health_data()
                    
                    success_dialog = Gtk.MessageDialog(
                        transient_for=self,
                        flags=0,
                        message_type=Gtk.MessageType.INFO,
                        buttons=Gtk.ButtonsType.OK,
                        text="Import Successful"
                    )
                    success_dialog.format_secondary_text("Health data has been imported successfully.")
                    success_dialog.run()
                    success_dialog.destroy()
                    
                    # Refresh view
                    self.setup_dashboard()
            except Exception as e:
                error_dialog = Gtk.MessageDialog(
                    transient_for=self,
                    flags=0,
                    message_type=Gtk.MessageType.ERROR,
                    buttons=Gtk.ButtonsType.OK,
                    text="Import Failed"
                )
                error_dialog.format_secondary_text(str(e))
                error_dialog.run()
                error_dialog.destroy()
                
        dialog.destroy()
        
    def on_erase_data(self, button):
        """Erase all health data"""
        confirm_dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.WARNING,
            buttons=Gtk.ButtonsType.YES_NO,
            text="⚠️ Erase All Data"
        )
        confirm_dialog.format_secondary_text(
            "This will permanently delete ALL your health data. This action cannot be undone. Continue?"
        )
        response = confirm_dialog.run()
        confirm_dialog.destroy()
        
        if response == Gtk.ResponseType.YES:
            # Ask for secondary confirmation with PIN if enabled
            if self.settings.get('pin_enabled', False):
                pin_dialog = Gtk.MessageDialog(
                    transient_for=self,
                    flags=0,
                    message_type=Gtk.MessageType.WARNING,
                    buttons=Gtk.ButtonsType.OK_CANCEL,
                    text="Enter PIN to Confirm"
                )
                
                pin_entry = Gtk.Entry()
                pin_entry.set_visibility(False)
                pin_entry.set_max_length(4)
                pin_dialog.get_content_area().pack_end(pin_entry, False, False, 0)
                pin_dialog.show_all()
                
                response = pin_dialog.run()
                entered_pin = pin_entry.get_text()
                pin_dialog.destroy()
                
                if response != Gtk.ResponseType.OK or entered_pin != self.settings.get('pin', ''):
                    # PIN verification failed
                    error_dialog = Gtk.MessageDialog(
                        transient_for=self,
                        flags=0,
                        message_type=Gtk.MessageType.ERROR,
                        buttons=Gtk.ButtonsType.OK,
                        text="Data Erase Cancelled"
                    )
                    error_dialog.format_secondary_text("PIN verification failed.")
                    error_dialog.run()
                    error_dialog.destroy()
                    return
            
            # If we got here, the erase is confirmed
            self.health_data = {}
            self.save_health_data()
            
            success_dialog = Gtk.MessageDialog(
                transient_for=self,
                flags=0,
                message_type=Gtk.MessageType.INFO,
                buttons=Gtk.ButtonsType.OK,
                text="Data Erased"
            )
            success_dialog.format_secondary_text("All health data has been permanently erased.")
            success_dialog.run()
            success_dialog.destroy()
            
            # Refresh view
            self.setup_dashboard()
            
    def load_health_data(self):
        """Load health data from file"""
        return self.data_handler.load_data("health_data.json") or {}
        
    def save_health_data(self):
        """Save health data to file"""
        self.data_handler.save_data("health_data.json", self.health_data)
        
    def load_settings(self):
        """Load settings from file"""
        return self.data_handler.load_data("health_settings.json") or {
            'encryption_enabled': False,
            'notifications_enabled': True,
            'default_cycle_length': 28,
            'period_reminders': True,
            'pin_enabled': False
        }
        
    def save_settings(self):
        """Save settings to file"""
        self.data_handler.save_data("health_settings.json", self.settings)
        
if __name__ == "__main__":
    app = HextrixHealth()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()