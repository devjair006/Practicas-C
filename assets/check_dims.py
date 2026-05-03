import sys

def check_obj(filename):
    min_x = min_y = min_z = float('inf')
    max_x = max_y = max_z = float('-inf')

    try:
        with open(filename, 'r') as f:
            for line in f:
                if line.startswith('v '):
                    parts = line.strip().split()
                    x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
                    min_x = min(min_x, x)
                    max_x = max(max_x, x)
                    min_y = min(min_y, y)
                    max_y = max(max_y, y)
                    min_z = min(min_z, z)
                    max_z = max(max_z, z)
        
        print(f"[{filename}]")
        print(f"X: {min_x:.3f} to {max_x:.3f} (Width: {max_x - min_x:.3f})")
        print(f"Y: {min_y:.3f} to {max_y:.3f} (Height: {max_y - min_y:.3f})")
        print(f"Z: {min_z:.3f} to {max_z:.3f} (Depth: {max_z - min_z:.3f})")
        print(f"Center: {(min_x+max_x)/2:.3f}, {(min_y+max_y)/2:.3f}, {(min_z+max_z)/2:.3f}\n")
    except Exception as e:
        print(f"Error reading {filename}: {e}")

check_obj('assets/puertadere.obj')
check_obj('assets/puertaizqui.obj')
