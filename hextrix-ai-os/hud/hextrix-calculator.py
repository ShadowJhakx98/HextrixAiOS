#!/usr/bin/env python3
# Hextrix AI OS - Advanced Calculator Application

import gi
import math
import cmath
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango

class HextrixCalculator(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Hextrix Calculator")
        self.set_default_size(800, 600)
        
        # Initialize data
        self.history = []
        self.memory = 0
        self.current_operation = None
        self.current_value = '0'
        self.scientific_mode = False
        self.graph_mode = False
        
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
            
            .display {
                background-color: rgba(0, 15, 30, 0.9);
                border: 1px solid rgba(0, 191, 255, 0.5);
                color: #00BFFF;
                font-size: 24px;
                padding: 10px;
                margin: 10px;
                border-radius: 5px;
            }
            
            .button {
                background-color: rgba(0, 20, 40, 0.8);
                color: #00BFFF;
                border: 1px solid rgba(0, 191, 255, 0.3);
                border-radius: 4px;
                padding: 15px;
                margin: 2px;
                font-size: 18px;
            }
            
            .button:hover {
                background-color: rgba(0, 100, 200, 0.5);
            }
            
            .scientific-button {
                background-color: rgba(0, 30, 60, 0.8);
            }
            
            .memory-button {
                background-color: rgba(0, 40, 80, 0.8);
            }
            
            .history-panel {
                background-color: rgba(0, 10, 20, 0.9);
                border-left: 1px solid rgba(0, 191, 255, 0.5);
                padding: 10px;
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
        
        # Calculator panel
        self.setup_calculator_panel()
        
        # History panel
        self.setup_history_panel()
        
    def setup_calculator_panel(self):
        """Set up the calculator interface"""
        self.calculator_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        
        # Display
        self.display = Gtk.Label(label="0")
        self.display.set_xalign(1)
        self.display.get_style_context().add_class("display")
        self.calculator_box.pack_start(self.display, False, False, 0)
        
        # Button grid
        self.setup_buttons()
        
        # Add to main box
        self.main_box.pack_start(self.calculator_box, True, True, 0)
        
    def setup_history_panel(self):
        """Set up the history panel"""
        self.history_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.history_box.set_size_request(300, -1)
        self.history_box.get_style_context().add_class("history-panel")
        
        # History label
        history_label = Gtk.Label(label="History")
        self.history_box.pack_start(history_label, False, False, 0)
        
        # History list
        self.history_list = Gtk.ListBox()
        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.add(self.history_list)
        self.history_box.pack_start(scrolled_window, True, True, 0)
        
        # Add to main box
        self.main_box.pack_start(self.history_box, False, False, 0)
        
    def setup_buttons(self):
        """Set up the calculator buttons"""
        button_grid = Gtk.Grid()
        button_grid.set_column_homogeneous(True)
        button_grid.set_row_homogeneous(True)
        button_grid.set_row_spacing(5)
        button_grid.set_column_spacing(5)
        
        # Basic operations
        buttons = [
            ('7', 0, 0), ('8', 0, 1), ('9', 0, 2), ('/', 0, 3),
            ('4', 1, 0), ('5', 1, 1), ('6', 1, 2), ('*', 1, 3),
            ('1', 2, 0), ('2', 2, 1), ('3', 2, 2), ('-', 2, 3),
            ('0', 3, 0), ('.', 3, 1), ('=', 3, 2), ('+', 3, 3),
            ('C', 4, 0), ('CE', 4, 1), ('±', 4, 2), ('%', 4, 3)
        ]
        
        # Scientific operations
        scientific_buttons = [
            ('sin', 0, 4), ('cos', 0, 5), ('tan', 0, 6),
            ('asin', 1, 4), ('acos', 1, 5), ('atan', 1, 6),
            ('log', 2, 4), ('ln', 2, 5), ('exp', 2, 6),
            ('π', 3, 4), ('e', 3, 5), ('√', 3, 6),
            ('x²', 4, 4), ('x³', 4, 5), ('xⁿ', 4, 6)
        ]
        
        # Memory operations
        memory_buttons = [
            ('MC', 0, 7), ('MR', 1, 7),
            ('M+', 2, 7), ('M-', 3, 7),
            ('MS', 4, 7)
        ]
        
        # Create and add buttons
        for label, row, col in buttons:
            button = self.create_button(label)
            button_grid.attach(button, col, row, 1, 1)
            
        for label, row, col in scientific_buttons:
            button = self.create_button(label, "scientific-button")
            button_grid.attach(button, col, row, 1, 1)
            
        for label, row, col in memory_buttons:
            button = self.create_button(label, "memory-button")
            button_grid.attach(button, col, row, 1, 1)
            
        self.calculator_box.pack_start(button_grid, True, True, 0)
        
    def create_button(self, label, style_class="button"):
        """Create a calculator button"""
        button = Gtk.Button(label=label)
        button.get_style_context().add_class(style_class)
        button.connect("clicked", self.on_button_clicked)
        return button
        
    def on_button_clicked(self, button):
        """Handle button clicks"""
        label = button.get_label()
        
        if label in '0123456789':
            self.handle_digit(label)
        elif label in '+-*/':
            self.handle_operator(label)
        elif label == '=':
            self.calculate_result()
        elif label == 'C':
            self.clear_all()
        elif label == 'CE':
            self.clear_entry()
        elif label == '±':
            self.toggle_sign()
        elif label == '%':
            self.handle_percentage()
        elif label in ['sin', 'cos', 'tan', 'asin', 'acos', 'atan']:
            self.handle_trig_function(label)
        elif label in ['log', 'ln', 'exp']:
            self.handle_log_function(label)
        elif label in ['π', 'e']:
            self.handle_constant(label)
        elif label in ['√', 'x²', 'x³', 'xⁿ']:
            self.handle_power_root(label)
        elif label in ['MC', 'MR', 'M+', 'M-', 'MS']:
            self.handle_memory(label)
            
    def handle_digit(self, digit):
        """Handle digit input"""
        if self.current_value == '0':
            self.current_value = digit
        else:
            self.current_value += digit
        self.update_display()
        
    def handle_operator(self, operator):
        """Handle operator input"""
        if self.current_operation:
            self.calculate_result()
        self.current_operation = operator
        self.history.append(float(self.current_value))
        self.current_value = '0'
        
    def calculate_result(self):
        """Calculate the result of the current operation"""
        if self.current_operation and len(self.history) > 0:
            try:
                a = self.history.pop()
                b = float(self.current_value)
                if self.current_operation == '+':
                    result = a + b
                elif self.current_operation == '-':
                    result = a - b
                elif self.current_operation == '*':
                    result = a * b
                elif self.current_operation == '/':
                    result = a / b
                    
                self.current_value = str(result)
                self.update_display()
                self.add_to_history(f"{a} {self.current_operation} {b} = {result}")
                self.current_operation = None
            except Exception as e:
                self.show_error(str(e))
                
    def clear_all(self):
        """Clear all calculator state"""
        self.current_value = '0'
        self.current_operation = None
        self.history = []
        self.update_display()
        
    def clear_entry(self):
        """Clear the current entry"""
        self.current_value = '0'
        self.update_display()
        
    def toggle_sign(self):
        """Toggle the sign of the current value"""
        if self.current_value != '0':
            if self.current_value[0] == '-':
                self.current_value = self.current_value[1:]
            else:
                self.current_value = '-' + self.current_value
            self.update_display()
            
    def handle_percentage(self):
        """Handle percentage calculation"""
        try:
            value = float(self.current_value)
            if self.history:
                base = self.history[-1]
                self.current_value = str((base * value) / 100)
            else:
                self.current_value = str(value / 100)
            self.update_display()
        except Exception as e:
            self.show_error(str(e))
            
    def handle_trig_function(self, func):
        """Handle trigonometric functions"""
        try:
            value = float(self.current_value)
            if func == 'sin':
                result = math.sin(math.radians(value))
            elif func == 'cos':
                result = math.cos(math.radians(value))
            elif func == 'tan':
                result = math.tan(math.radians(value))
            elif func == 'asin':
                result = math.degrees(math.asin(value))
            elif func == 'acos':
                result = math.degrees(math.acos(value))
            elif func == 'atan':
                result = math.degrees(math.atan(value))
                
            self.current_value = str(result)
            self.update_display()
            self.add_to_history(f"{func}({value}) = {result}")
        except Exception as e:
            self.show_error(str(e))
            
    def handle_log_function(self, func):
        """Handle logarithmic functions"""
        try:
            value = float(self.current_value)
            if func == 'log':
                result = math.log10(value)
            elif func == 'ln':
                result = math.log(value)
            elif func == 'exp':
                result = math.exp(value)
                
            self.current_value = str(result)
            self.update_display()
            self.add_to_history(f"{func}({value}) = {result}")
        except Exception as e:
            self.show_error(str(e))
            
    def handle_constant(self, const):
        """Handle mathematical constants"""
        if const == 'π':
            self.current_value = str(math.pi)
        elif const == 'e':
            self.current_value = str(math.e)
        self.update_display()
        
    def handle_power_root(self, func):
        """Handle power and root functions"""
        try:
            value = float(self.current_value)
            if func == '√':
                result = math.sqrt(value)
            elif func == 'x²':
                result = value ** 2
            elif func == 'x³':
                result = value ** 3
            elif func == 'xⁿ':
                self.current_operation = '^'
                self.history.append(value)
                self.current_value = '0'
                return
                
            self.current_value = str(result)
            self.update_display()
            self.add_to_history(f"{func}({value}) = {result}")
        except Exception as e:
            self.show_error(str(e))
            
    def handle_memory(self, func):
        """Handle memory operations"""
        try:
            value = float(self.current_value)
            if func == 'MC':
                self.memory = 0
            elif func == 'MR':
                self.current_value = str(self.memory)
                self.update_display()
            elif func == 'M+':
                self.memory += value
            elif func == 'M-':
                self.memory -= value
            elif func == 'MS':
                self.memory = value
        except Exception as e:
            self.show_error(str(e))
            
    def update_display(self):
        """Update the calculator display"""
        self.display.set_label(self.current_value)
        
    def add_to_history(self, entry):
        """Add an entry to the history"""
        self.history_list.add(Gtk.Label(label=entry))
        self.history_list.show_all()
        
    def show_error(self, message):
        """Show an error message"""
        dialog = Gtk.MessageDialog(
            parent=self,
            flags=Gtk.DialogFlags.MODAL,
            message_type=Gtk.MessageType.ERROR,
            buttons=Gtk.ButtonsType.OK,
            text=message
        )
        dialog.run()
        dialog.destroy()
        
if __name__ == "__main__":
    app = HextrixCalculator()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()