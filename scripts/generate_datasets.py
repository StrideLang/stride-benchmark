#!/usr/bin/env python3
import json
import os
import sys

def get_base_dir():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.abspath(os.path.join(script_dir, "..", "data"))

def generate_config(data_dir):
    stride_content = """# Game Configuration Benchmark Dataset
gameConfig Config {
    gameTitle: "Starfleet Command"
    version: "1.4.0"
    maxPlayers: 8
    tickRateHz: 60
    allowSpectators: on
    difficultyMultiplier: 1.25
    serverRegion: "us-east-1"
    resolution: [ 2560, 1440 ]
    renderDistance: 1200
    enableHDR: on
}
"""
    json_data = {
        "gameConfig": {
            "gameTitle": "Starfleet Command",
            "version": "1.4.0",
            "maxPlayers": 8,
            "tickRateHz": 60,
            "allowSpectators": True,
            "difficultyMultiplier": 1.25,
            "serverRegion": "us-east-1",
            "resolution": [2560, 1440],
            "renderDistance": 1200,
            "enableHDR": True
        }
    }

    yaml_content = """# Game Configuration Benchmark Dataset
gameConfig:
  gameTitle: "Starfleet Command"
  version: "1.4.0"
  maxPlayers: 8
  tickRateHz: 60
  allowSpectators: true
  difficultyMultiplier: 1.25
  serverRegion: "us-east-1"
  resolution:
    - 2560
    - 1440
  renderDistance: 1200
  enableHDR: true
"""

    with open(os.path.join(data_dir, "config.stride"), "w", encoding="utf-8", newline="\r\n") as f:
        f.write(stride_content)
    with open(os.path.join(data_dir, "config.json"), "w", encoding="utf-8", newline="\r\n") as f:
        json.dump(json_data, f, indent=2)
        f.write("\r\n")
    with open(os.path.join(data_dir, "config.yaml"), "w", encoding="utf-8", newline="\r\n") as f:
        f.write(yaml_content)

def generate_entities(data_dir, count=100, prefix="entities"):
    roles = ["warrior", "mage", "ranger", "paladin", "rogue"]
    stride_lines = ["# Entities Benchmark Dataset\n"]
    stride_lines.append(f"worldSector Sector_{prefix} {{\n")

    json_units = []
    yaml_lines = [f"worldSector:\n  name: \"Sector_{prefix}\"\n  units:\n"]

    for i in range(count):
        role = roles[i % len(roles)]
        unit_id = i + 1
        name = f"Hero_{role}_{unit_id}"
        hp = 100 + (i * 7) % 500
        mp = 50.0 + (i * 3.5) % 150.0
        x = round((i * 12.34) % 1000.0, 2)
        y = round((i * 56.78) % 1000.0, 2)
        z = round((i * 2.5) % 100.0, 2)
        alive = "on" if i % 10 != 0 else "off"
        alive_bool = (i % 10 != 0)

        stride_lines.append(f"    gameUnit Unit_{unit_id} {{\n")
        stride_lines.append(f"        id: {unit_id}\n")
        stride_lines.append(f"        name: \"{name}\"\n")
        stride_lines.append(f"        role: \"{role}\"\n")
        stride_lines.append(f"        health: {hp}\n")
        stride_lines.append(f"        mana: {mp}\n")
        stride_lines.append(f"        isAlive: {alive}\n")
        stride_lines.append(f"        position: [ {x}, {y}, {z} ]\n")
        stride_lines.append(f"        inventory: [ \"item_{unit_id}_a\", \"item_{unit_id}_b\" ]\n")
        stride_lines.append("    }\n")

        json_units.append({
            "id": unit_id,
            "name": name,
            "role": role,
            "health": hp,
            "mana": mp,
            "isAlive": alive_bool,
            "position": [x, y, z],
            "inventory": [f"item_{unit_id}_a", f"item_{unit_id}_b"]
        })

        yaml_lines.append(f"    - id: {unit_id}\n")
        yaml_lines.append(f"      name: \"{name}\"\n")
        yaml_lines.append(f"      role: \"{role}\"\n")
        yaml_lines.append(f"      health: {hp}\n")
        yaml_lines.append(f"      mana: {mp}\n")
        yaml_lines.append(f"      isAlive: {'true' if alive_bool else 'false'}\n")
        yaml_lines.append(f"      position: [{x}, {y}, {z}]\n")
        yaml_lines.append(f"      inventory:\n        - \"item_{unit_id}_a\"\n        - \"item_{unit_id}_b\"\n")

    stride_lines.append("}\n")

    json_data = {
        "worldSector": {
            "name": f"Sector_{prefix}",
            "units": json_units
        }
    }

    with open(os.path.join(data_dir, f"{prefix}.stride"), "w", encoding="utf-8", newline="\r\n") as f:
        f.writelines(stride_lines)
    with open(os.path.join(data_dir, f"{prefix}.json"), "w", encoding="utf-8", newline="\r\n") as f:
        json.dump(json_data, f, indent=2)
        f.write("\r\n")
    with open(os.path.join(data_dir, f"{prefix}.yaml"), "w", encoding="utf-8", newline="\r\n") as f:
        f.writelines(yaml_lines)

def generate_nested(data_dir, depth=20):
    stride_lines = ["# Deeply Nested Structure Benchmark\n"]
    yaml_lines = ["# Deeply Nested Structure Benchmark\n"]
    json_obj = {}
    curr_json = json_obj

    for d in range(depth):
        indent = "    " * d
        stride_lines.append(f"{indent}levelSection Level_{d} {{\n")
        stride_lines.append(f"{indent}    depth: {d}\n")
        stride_lines.append(f"{indent}    name: \"DepthLevel_{d}\"\n")

        yaml_indent = "  " * d
        yaml_lines.append(f"{yaml_indent}levelSection_{d}:\n")
        yaml_lines.append(f"{yaml_indent}  depth: {d}\n")
        yaml_lines.append(f"{yaml_indent}  name: \"DepthLevel_{d}\"\n")

        if d < depth - 1:
            next_obj = {}
            curr_json[f"levelSection_{d}"] = {
                "depth": d,
                "name": f"DepthLevel_{d}",
                "nested": next_obj
            }
            curr_json = next_obj
            yaml_lines.append(f"{yaml_indent}  nested:\n")
        else:
            curr_json[f"levelSection_{d}"] = {
                "depth": d,
                "name": f"DepthLevel_{d}",
                "payload": "deepest_node"
            }
            stride_lines.append(f"{indent}    payload: \"deepest_node\"\n")
            yaml_lines.append(f"{yaml_indent}  payload: \"deepest_node\"\n")

    for d in range(depth - 1, -1, -1):
        indent = "    " * d
        stride_lines.append(f"{indent}}}\n")

    with open(os.path.join(data_dir, "nested.stride"), "w", encoding="utf-8", newline="\r\n") as f:
        f.writelines(stride_lines)
    with open(os.path.join(data_dir, "nested.json"), "w", encoding="utf-8", newline="\r\n") as f:
        json.dump(json_obj, f, indent=2)
        f.write("\r\n")
    with open(os.path.join(data_dir, "nested.yaml"), "w", encoding="utf-8", newline="\r\n") as f:
        f.writelines(yaml_lines)

def main():
    data_dir = get_base_dir()
    os.makedirs(data_dir, exist_ok=True)
    print(f"Generating benchmark datasets in: {data_dir}")

    generate_config(data_dir)
    print(" - Generated config datasets (small)")

    generate_entities(data_dir, count=100, prefix="entities")
    print(" - Generated entities datasets (medium, 100 records)")

    generate_entities(data_dir, count=1500, prefix="dataset")
    print(" - Generated dataset datasets (large, 1500 records)")

    generate_nested(data_dir, depth=20)
    print(" - Generated nested datasets (20 levels)")

    print("All datasets generated successfully!")

if __name__ == "__main__":
    main()
