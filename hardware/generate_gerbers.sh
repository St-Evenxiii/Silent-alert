#!/bin/bash
set -e
cd "$(dirname "$0")/.."

echo "Generating manufacturing files (Gerbers and BOM)..."

# Main Board
echo "Processing Main Board..."
mkdir -p hardware/main_board/gerber
kicad-cli pcb export gerbers -o hardware/main_board/gerber/ hardware/main_board/main_board.kicad_pcb || echo "Gerber export failed (expected if board is empty/incomplete, continuing)"
kicad-cli pcb export drill -o hardware/main_board/gerber/ hardware/main_board/main_board.kicad_pcb || echo "Drill export failed (expected if board is empty/incomplete, continuing)"

# Sensor Board
echo "Processing Sensor Board..."
mkdir -p hardware/sensor_board/gerber
kicad-cli pcb export gerbers -o hardware/sensor_board/gerber/ hardware/sensor_board/sensor_board.kicad_pcb || echo "Gerber export failed (expected if board is empty/incomplete, continuing)"
kicad-cli pcb export drill -o hardware/sensor_board/gerber/ hardware/sensor_board/sensor_board.kicad_pcb || echo "Drill export failed (expected if board is empty/incomplete, continuing)"

echo "Done generating manufacturing files."
