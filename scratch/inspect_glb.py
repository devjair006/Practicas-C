import json
import struct
import sys

def get_glb_nodes(filepath):
    with open(filepath, 'rb') as f:
        magic = f.read(4)
        if magic != b'glTF':
            print("Not a valid glTF 2.0 binary file.")
            return
        
        version, length = struct.unpack('<II', f.read(8))
        
        # Read first chunk (JSON)
        chunk_length, chunk_type = struct.unpack('<II', f.read(8))
        if chunk_type != 0x4E4F534A: # 'JSON'
            print("First chunk is not JSON.")
            return
            
        json_data = f.read(chunk_length)
        gltf = json.loads(json_data.decode('utf-8'))
        
        if 'nodes' in gltf:
            print("Nodes in GLB:")
            for i, node in enumerate(gltf['nodes']):
                print(f"  {i}: {node.get('name', 'Unnamed')}")
        else:
            print("No nodes found.")

if __name__ == '__main__':
    get_glb_nodes(sys.argv[1])
