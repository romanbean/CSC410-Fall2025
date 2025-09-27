#!/bin/bash

# List of program directories and executables
declare -a programs=(
	"/home/phiro/Documents/CSC410/A1/sequential/arraysum"
	"/home/phiro/Documents/CSC410/A1/sequential/numI"
	"/home/phiro/Documents/CSC410/A1/sequential/matrixMul/mainM"
	"/home/phiro/Documents/CSC410/A1/sequential/Nqueens/mainQ"
	"/home/phiro/Documents/CSC410/A1/sequential/sorts/sorts"
)

echo "=== Timing all Programs ==="
for prog_path in "${programs[@]}"; do
	echo "Running $prog_path"
	if [[ -x "$prog_path" ]]; then
		/usr/bin/time -f "Time: %E (real) | %U user | %S sys" "$prog_path"
	else
		echo "Error: '$prog_path' not found or not executable"
	fi
	echo "------------------------------"
done
