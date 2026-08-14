import os
import sys

def create_dummy_kicad_project(dir_path, name):
    os.makedirs(dir_path, exist_ok=True)

    # Create empty schematic
    with open(os.path.join(dir_path, f"{name}.kicad_sch"), "w") as f:
        f.write('(kicad_sch (version 20230121) (generator eeschema)\n')
        f.write('  (uuid "00000000-0000-0000-0000-000000000000")\n')
        f.write('  (paper "A4")\n')
        f.write('  (lib_symbols\n')
        f.write('  )\n')
        f.write(')\n')

    # Create empty PCB
    with open(os.path.join(dir_path, f"{name}.kicad_pcb"), "w") as f:
        f.write('(kicad_pcb (version 20240108) (generator pcbnew)\n')
        f.write('  (general\n')
        f.write('    (thickness 1.6)\n')
        f.write('  )\n')
        f.write('  (paper "A4")\n')
        f.write('  (layers\n')
        f.write('    (0 "F.Cu" signal)\n')
        f.write('    (31 "B.Cu" signal)\n')
        f.write('  )\n')
        f.write(')\n')

    # Create project file
    with open(os.path.join(dir_path, f"{name}.kicad_pro"), "w") as f:
        f.write('{\n')
        f.write('  "board": {\n')
        f.write('    "design_settings": {\n')
        f.write('      "defaults": {\n')
        f.write('        "board_outline_line_width": 0.05\n')
        f.write('      }\n')
        f.write('    }\n')
        f.write('  }\n')
        f.write('}\n')

create_dummy_kicad_project("hardware/main_board", "main_board")
print("Main board KiCad files generated.")
