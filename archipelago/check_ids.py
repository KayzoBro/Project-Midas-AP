"""The C client and the Python world must share names and ids."""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "archipelago" / "goldeneye"))

import data  # noqa: E402


def block(text, name):
    marker = f"/* AP_SYNC:{name} */"
    start = text.find(marker)
    if start < 0:
        raise SystemExit(f"missing {marker}")
    end = text.find("/* AP_SYNC:", start + len(marker))
    chunk = text[start:end if end >= 0 else None]
    return re.findall(r'"([^"]*)"', chunk)


def main():
    client = (ROOT / "port" / "src" / "ap_client.c").read_text(encoding="utf-8")
    missions = block(client, "missions")
    weapons = block(client, "weapons")
    diffs = block(client, "difficulties")
    if missions != data.MISSIONS:
        raise SystemExit(f"mission names differ:\n  C {missions}\n  py {data.MISSIONS}")
    if weapons != data.WEAPONS:
        raise SystemExit(f"weapon names differ:\n  C {weapons}\n  py {data.WEAPONS}")
    if diffs != data.DIFFICULTIES:
        raise SystemExit(f"difficulty names differ:\n  C {diffs}\n  py {data.DIFFICULTIES}")
    tools = [name for name, _id in data.TOOLS]
    if block(client, "tools") != tools:
        raise SystemExit("tool names differ")
    if data.dossier_id(0) != 640001 or data.weapon_id(23) != 640124:
        raise SystemExit("item ids drifted")
    if data.dossier_diff_id(0, 0) != 640501 or data.dossier_diff_id(19, 2) != 640560:
        raise SystemExit("difficulty dossier ids drifted")
    if data.location_id(0, 0) != 641000 or data.location_id(19, 2) != 641192:
        raise SystemExit("location ids drifted")
    if data.objective_location_id(0, 0, 0) != 642000 or data.objective_location_id(1, 3, 2) != 642072:
        raise SystemExit("objective ids drifted")
    if data.cheat_location_id(0) != 643000 or data.cheat_location_id(22) != 643022:
        raise SystemExit("cheat ids drifted")
    if block(client, "cheats") != data.CHEATS:
        raise SystemExit("cheat names differ")
    if len(data.item_name_to_id()) != 20 + 60 + 24 + 15 + len(data.TOOLS):
        raise SystemExit("unexpected item count")
    if len(data.location_name_to_id()) != 60 + len(data.objective_rows(3, 20)) + len(data.objective_rows(1, 20)) + len(data.CHEATS):
        raise SystemExit("unexpected location count")
    print("check_ids ok")


if __name__ == "__main__":
    main()
