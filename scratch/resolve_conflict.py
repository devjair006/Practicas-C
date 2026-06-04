import re

filepath = r"c:\Users\jahir\Practicas-C\src\main.cpp"

with open(filepath, "r", encoding="utf-8") as f:
    content = f.read()

# Resolve first conflict block
# <<<<<<< HEAD
#     int editorLampDrawIdx = kEditorLightBaseIdx; // sincroniza glow con su luz
#     for (const auto& prop : placedProps) {
#       GLTFModel* model = modelRegistry[prop.modelName];
#       if (!model || model->meshes.empty()) continue;
# =======
#     for (const auto &prop : placedProps) {
#       GLTFModel *model = modelRegistry[prop.modelName];
#       if (!model || model->meshes.empty())
#         continue;
# >>>>>>> origin/feature/area-contencion

pattern1 = r"<<<<<<< HEAD\n\s*int editorLampDrawIdx = kEditorLightBaseIdx;[^\n]*\n\s*for[^\n]*\n\s*GLTFModel\*[^\n]*\n\s*if[^\n]*\n=======\n\s*for[^\n]*\n\s*GLTFModel\*[^\n]*\n\s*if[^\n]*\n\s*continue;[^\n]*\n>>>>>>> origin/feature/area-contencion"

# Let's do it by literal index matching to be extremely precise and safe
lines = content.splitlines(keepends=True)

# Find conflict markers
conflict_markers = []
for idx, line in enumerate(lines):
    if line.startswith("<<<<<<< HEAD"):
        conflict_markers.append(("START", idx))
    elif line.startswith("======="):
        conflict_markers.append(("MID", idx))
    elif line.startswith(">>>>>>> origin/feature/area-contencion"):
        conflict_markers.append(("END", idx))

print("Found markers:", conflict_markers)

if len(conflict_markers) == 6:
    # First conflict block:
    # START at idx1, MID at idx2, END at idx3
    # Second conflict block:
    # START at idx4, MID at idx5, END at idx6
    start1 = conflict_markers[0][1]
    mid1 = conflict_markers[1][1]
    end1 = conflict_markers[2][1]
    
    start2 = conflict_markers[3][1]
    mid2 = conflict_markers[4][1]
    end2 = conflict_markers[5][1]
    
    # We want to keep lines[start1+1 : mid1] for block 1
    block1_replacement = lines[start1+1 : mid1]
    
    # We want to keep lines[mid2+1 : end2] for block 2
    block2_replacement = lines[mid2+1 : end2]
    
    # Build new lines
    new_lines = []
    # Up to start of block 1
    new_lines.extend(lines[:start1])
    # Block 1 resolved
    new_lines.extend(block1_replacement)
    # Between block 1 and block 2
    new_lines.extend(lines[end1+1 : start2])
    # Block 2 resolved
    new_lines.extend(block2_replacement)
    # Rest of the file
    new_lines.extend(lines[end2+1:])
    
    new_content = "".join(new_lines)
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(new_content)
    print("Conflict markers resolved successfully!")
else:
    print("Error: Could not find exactly the expected conflict markers.")
