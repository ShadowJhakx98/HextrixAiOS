#!/usr/bin/env python3
import cairo
import math
import random

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