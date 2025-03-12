import json
import os
from pathlib import Path

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
        
    def get_shared_contacts(self):
        return self.load_data("shared_contacts.json") or []
        
    def get_shared_events(self):
        return self.load_data("shared_events.json") or []
        
    def get_shared_accounts(self):
        return self.load_data("shared_accounts.json") or []