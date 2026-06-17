import bpy

# Configuracion
# Nombres de las puertas despues de separarlas en Blender
puerta_izq_name = "Puerta_Izquierda"
puerta_der_name = "Puerta_Derecha"

# Distancia de apertura y eje (0=X, 1=Y, 2=Z)
open_distance = 1.2
axis = 0 

# Tiempos (24 fps)
fps = 24
frame_start = 1
frame_open = fps * 2      # Abre en 2s
frame_hold = fps * 6      # Queda abierta 4s
frame_close = fps * 8     # Cierra en 2s

def animate_door(obj_name, direction):
    obj = bpy.data.objects.get(obj_name)
    if not obj:
        print(f"Error: No se encontro el objeto '{obj_name}'")
        return
        
    obj.animation_data_create()
    
    # Posicion inicial
    obj.location[axis] = 0.0
    obj.keyframe_insert(data_path="location", index=axis, frame=frame_start)
    
    # Posicion abierta
    obj.location[axis] = open_distance * direction
    obj.keyframe_insert(data_path="location", index=axis, frame=frame_open)
    
    # Mantener abierto
    obj.keyframe_insert(data_path="location", index=axis, frame=frame_hold)
    
    # Posicion cerrada
    obj.location[axis] = 0.0
    obj.keyframe_insert(data_path="location", index=axis, frame=frame_close)

animate_door(puerta_izq_name, -1)
animate_door(puerta_der_name, 1)

bpy.context.scene.frame_start = frame_start
bpy.context.scene.frame_end = frame_close

print("Animacion de puertas generada.")
