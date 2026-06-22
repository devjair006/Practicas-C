#include "headers/localization.h"
#include <map>

Language currentLanguage = LANG_EN;

std::map<std::string, std::map<Language, std::string>> dictionary = {
    // Menu Options
    {"MENU_TITLE", {{LANG_ES, "PROYECTO CONFIDENCIAL"}, {LANG_EN, "CONFIDENTIAL PROJECT"}}},
    {"MENU_SUBTITLE", {{LANG_ES, "LABORATORIO DE CONTENCION"}, {LANG_EN, "CONTAINMENT LAB"}}},
    {"MENU_SUBTITLE_AUDIO", {{LANG_ES, "AJUSTES DE AUDIO"}, {LANG_EN, "AUDIO SETTINGS"}}},
    {"MENU_START", {{LANG_ES, "INICIAR JUEGO"}, {LANG_EN, "START GAME"}}},
    {"MENU_OPTIONS", {{LANG_ES, "OPCIONES"}, {LANG_EN, "OPTIONS"}}},
    {"MENU_FILES", {{LANG_ES, "ARCHIVOS"}, {LANG_EN, "FILES"}}},
    {"MENU_CREDITS", {{LANG_ES, "CREDITOS"}, {LANG_EN, "CREDITS"}}},
    {"MENU_EXIT", {{LANG_ES, "SALIR"}, {LANG_EN, "EXIT"}}},
    {"MENU_HINT_BACK", {{LANG_ES, "PRESIONE VOLVER PARA IR AL MENU PRINCIPAL"}, {LANG_EN, "PRESS BACK TO RETURN TO MAIN MENU"}}},
    {"MENU_HINT_START", {{LANG_ES, "ENTER / ESPACIO tambien inicia"}, {LANG_EN, "ENTER / SPACE also starts"}}},
    {"MENU_MUTE_ACTIVE", {{LANG_ES, "QUITAR SONIDO (MUTE) [ACTIVO]"}, {LANG_EN, "MUTE AUDIO [ACTIVE]"}}},
    {"MENU_MUTE", {{LANG_ES, "QUITAR SONIDO (MUTE)"}, {LANG_EN, "MUTE AUDIO"}}},
    {"MENU_UNMUTE_ACTIVE", {{LANG_ES, "ACTIVAR SONIDO [ACTIVO]"}, {LANG_EN, "ENABLE AUDIO [ACTIVE]"}}},
    {"MENU_UNMUTE", {{LANG_ES, "ACTIVAR SONIDO"}, {LANG_EN, "ENABLE AUDIO"}}},
    {"MENU_BACK", {{LANG_ES, "VOLVER"}, {LANG_EN, "BACK"}}},
    {"MENU_LANG_ES", {{LANG_ES, "ESPANOL [ACTIVO]"}, {LANG_EN, "SPANISH"}}},
    {"MENU_LANG_ES_INACTIVE", {{LANG_ES, "ESPANOL"}, {LANG_EN, "SPANISH"}}},
    {"MENU_LANG_EN", {{LANG_ES, "INGLES"}, {LANG_EN, "ENGLISH [ACTIVE]"}}},
    {"MENU_LANG_EN_INACTIVE", {{LANG_ES, "INGLES"}, {LANG_EN, "ENGLISH"}}},

    // Gameplay Intro
    {"INTRO_1", {{LANG_ES, "========================================================="}, {LANG_EN, "========================================================="}}},
    {"INTRO_2", {{LANG_ES, "               PROYECTO CONFIDENCIAL - REINICIO          "}, {LANG_EN, "               CONFIDENTIAL PROJECT - REBOOT             "}}},
    {"INTRO_3", {{LANG_ES, "El entorno es silencioso y vacio."}, {LANG_EN, "The environment is silent and empty."}}},
    {"INTRO_4", {{LANG_ES, "Moverte: W A S D  | Mirar: MOUSE | Sprint: SHIFT"}, {LANG_EN, "Move: W A S D  | Look: MOUSE | Sprint: SHIFT"}}},
    {"INTRO_5", {{LANG_ES, "Interactuar/Abrir Puertas: E | Linterna: F"}, {LANG_EN, "Interact/Open Doors: E | Flashlight: F"}}},
    {"INTRO_6", {{LANG_ES, "Busca TARJETAS DE ACCESO para avanzar a las siguientes salas."}, {LANG_EN, "Find ACCESS CARDS to advance to the next rooms."}}},

    // Typewriter
    {"TYPE_SCENE_1", {{LANG_ES, "ESCENA 1: PASILLO DE ACCESO"}, {LANG_EN, "SCENE 1: ACCESS CORRIDOR"}}},
    {"TYPE_SCENE_2", {{LANG_ES, "ESCENA 2: SALA DE CONTROL \nLuz verde tenue. "}, {LANG_EN, "SCENE 2: CONTROL ROOM \nDim green light. "}}},
    {"TYPE_SCENE_3", {{LANG_ES, "ESCENA 3: LABORATORIO PRINCIPAL\nEncuentras la esfera "}, {LANG_EN, "SCENE 3: MAIN LABORATORY\nYou find the sphere "}}},
    {"TYPE_PANEL_INIT", {{LANG_ES, "[SISTEMA]: Inicializando panel de alineacion de simbolos."}, {LANG_EN, "[SYSTEM]: Initializing symbol alignment panel."}}},
    {"TYPE_PANEL_SOLVED", {{LANG_ES, "[SISTEMA]: Resolviendo panel. Camara estatica automatica."}, {LANG_EN, "[SYSTEM]: Solving panel. Automatic static camera."}}},
    {"TYPE_YELLOW_ACCEPTED", {{LANG_ES, "[PUERTA]: Tarjeta Amarilla Aceptada. Accediendo a Sala "}, {LANG_EN, "[DOOR]: Yellow Card Accepted. Accessing Room "}}},
    {"TYPE_RED_ACCEPTED", {{LANG_ES, "[PUERTA]: Tarjeta Roja Aceptada. Peligro: Zona de "}, {LANG_EN, "[DOOR]: Red Card Accepted. Warning: Zone of "}}},
    {"TYPE_BLUE_ACCEPTED", {{LANG_ES, "[PUERTA]: Tarjeta Azul Aceptada. Acceso concedido."}, {LANG_EN, "[DOOR]: Blue Card Accepted. Access granted."}}},
    {"TYPE_NOT_COPY", {{LANG_ES, "NO ES UNA COPIA... ESTA APRENDIENDO. CORRE."}, {LANG_EN, "IT IS NOT A COPY... IT IS LEARNING. RUN."}}},
    {"TYPE_DRAWER_STUCK", {{LANG_ES, "[CAJON]: Esta vacio o atascado."}, {LANG_EN, "[DRAWER]: It is empty or stuck."}}},
    {"TYPE_SCENE_9", {{LANG_ES, "ESCENA 9: FALLO TOTAL"}, {LANG_EN, "SCENE 9: TOTAL FAILURE"}}},
    {"TYPE_COPY_COMPLETE", {{LANG_ES, "COPIA COMPLETA. HAS SIDO REEMPLAZADO."}, {LANG_EN, "COPY COMPLETE. YOU HAVE BEEN REPLACED."}}},
    {"TYPE_BLACK_DOOR", {{LANG_ES, "[PUERTA NEGRA]: Acceso concedido."}, {LANG_EN, "[BLACK DOOR]: Access granted."}}},
    {"TYPE_DOOR_OPEN", {{LANG_ES, "[PUERTA]: Abierta."}, {LANG_EN, "[DOOR]: Opened."}}},
    {"TYPE_DOOR_LOCKED_YELLOW", {{LANG_ES, "[PUERTA BLOQUEADA]: Se requiere Tarjeta Amarilla."}, {LANG_EN, "[LOCKED DOOR]: Yellow Card required."}}},
    {"TYPE_DOOR_LOCKED_RED", {{LANG_ES, "[PUERTA BLOQUEADA]: Se requiere Tarjeta Roja."}, {LANG_EN, "[LOCKED DOOR]: Red Card required."}}},
    {"TYPE_DOOR_LOCKED_BLUE", {{LANG_ES, "[PUERTA BLOQUEADA]: Se requiere Tarjeta Azul."}, {LANG_EN, "[LOCKED DOOR]: Blue Card required."}}},
    {"TYPE_BLOOD_TRAIL", {{LANG_ES, "[SANGRE]: Alguien estuvo perdiendo mucha sangre por aqui..."}, {LANG_EN, "[BLOOD]: Someone was losing a lot of blood around here..."}}},

    // Minigames ImGui
    {"MINIGAME_WIRE_TITLE", {{LANG_ES, "PANEL DE CONEXION DE CABLES"}, {LANG_EN, "WIRE CONNECTION PANEL"}}},
    {"MINIGAME_WIRE_HINT", {{LANG_ES, "Conecta los terminales del mismo color."}, {LANG_EN, "Connect the terminals of the same color."}}},
    {"MINIGAME_SYMBOL_TITLE", {{LANG_ES, "ALINEACION DE SIMBOLOS"}, {LANG_EN, "SYMBOL ALIGNMENT"}}},
    {"MINIGAME_SYMBOL_HINT", {{LANG_ES, "Alinea todos los simbolos a '%s' para abrir la puerta."}, {LANG_EN, "Align all symbols to '%s' to open the door."}}},
    {"MINIGAME_SYMBOL_WHEEL", {{LANG_ES, "Rueda"}, {LANG_EN, "Wheel"}}},
    {"MINIGAME_ACCESS_GRANTED", {{LANG_ES, "ACCESO AUTORIZADO"}, {LANG_EN, "ACCESS GRANTED"}}},
    {"MINIGAME_SYSTEM_LOCKED", {{LANG_ES, "SISTEMA BLOQUEADO"}, {LANG_EN, "SYSTEM LOCKED"}}},

    // Entity Interactions
    {"ENTITY_WIRE", {{LANG_ES, "[CABLE SUELTO]: Hay un cable pelado aqui."}, {LANG_EN, "[LOOSE WIRE]: There is an exposed wire here."}}},
    {"ENTITY_LOG1", {{LANG_ES, "LOG 1 (Arrugado): 'Apagon general. Las compuertas se bloquearon.'"}, {LANG_EN, "LOG 1 (Crumpled): 'General blackout. The airlocks were locked.'"}}},
    {"ENTITY_MONITOR_AUX", {{LANG_ES, "[MONITOR AUXILIAR]: 'Sistema inestable.'"}, {LANG_EN, "[AUX MONITOR]: 'Unstable system.'"}}},
    {"ENTITY_ERROR_SCREEN", {{LANG_ES, "[PANTALLA ERROR]: 'Falla de contencion.'"}, {LANG_EN, "[ERROR SCREEN]: 'Containment failure.'"}}},
    {"ENTITY_MACHINE", {{LANG_ES, "[MAQUINA]: Unidad Frigorifica."}, {LANG_EN, "[MACHINE]: Refrigeration Unit."}}},
    {"ENTITY_LOG2", {{LANG_ES, "LOG 2 (Sangriento): 'La muestra escapo.'"}, {LANG_EN, "LOG 2 (Bloody): 'The specimen escaped.'"}}},
    {"ENTITY_STAIN", {{LANG_ES, "[MANCHA]: Rastro oscuro hacia ventilacion."}, {LANG_EN, "[STAIN]: Dark trail towards ventilation."}}},
    
    // Cards & Docs
    {"DOC_YELLOW", {{LANG_ES, "TARJETA AMARILLA"}, {LANG_EN, "YELLOW CARD"}}},
    {"DOC_RED", {{LANG_ES, "TARJETA ROJA"}, {LANG_EN, "RED CARD"}}},
    {"DOC_BLUE", {{LANG_ES, "TARJETA AZUL"}, {LANG_EN, "BLUE CARD"}}},
    {"DOC_BODY_YELLOW", {{LANG_ES, "Nivel de Acceso: 1\nSector: Pruebas Iniciales\n\nNotas: El sujeto muestra hiperactividad celular. No acercarse sin traje de proteccion."}, {LANG_EN, "Access Level: 1\nSector: Initial Tests\n\nNotes: Subject shows cellular hyperactivity. Do not approach without hazmat suit."}}},
    {"DOC_BODY_RED", {{LANG_ES, "Nivel de Acceso: 2\nSector: Contencion\n\nNotas: INCIDENTE 04 - Ruptura de capsula B. Personal de seguridad no responde."}, {LANG_EN, "Access Level: 2\nSector: Containment\n\nNotes: INCIDENT 04 - Capsule B breach. Security personnel unresponsive."}}},
    {"DOC_BODY_BLUE", {{LANG_ES, "Nivel de Acceso: 3\nSector: Administracion\n\nNotas: Evacuar de inmediato. Destruir los discos duros."}, {LANG_EN, "Access Level: 3\nSector: Administration\n\nNotes: Evacuate immediately. Destroy hard drives."}}},
    
    // Inventory and UI
    {"DOC_CLOSE_HINT", {{LANG_ES, "E o ESC para cerrar"}, {LANG_EN, "E or ESC to close"}}},
    {"INV_FLASHLIGHT_ON", {{LANG_ES, "[F] LINTERNA ON"}, {LANG_EN, "[F] LIGHT ON"}}},
    {"INV_FLASHLIGHT_OFF", {{LANG_ES, "[F] LINTERNA OFF"}, {LANG_EN, "[F] LIGHT OFF"}}},
    {"INV_KEY_YELLOW", {{LANG_ES, "T.Amarilla"}, {LANG_EN, "Y.Card"}}},
    {"INV_KEY_RED", {{LANG_ES, "T.Roja"}, {LANG_EN, "R.Card"}}},
    {"INV_KEY_BLUE", {{LANG_ES, "T.Azul"}, {LANG_EN, "B.Card"}}},
    {"INV_BATS", {{LANG_ES, "Bats:%d/3"}, {LANG_EN, "Bats:%d/3"}}},

    // Entities and combat
    {"[COMBATE]: cerdo eliminada.", {{LANG_ES, "[COMBATE]: Cerdo eliminado."}, {LANG_EN, "[COMBAT]: Pig eliminated."}}},
    {"[COMBATE]: esqueleto eliminada.", {{LANG_ES, "[COMBATE]: Esqueleto eliminado."}, {LANG_EN, "[COMBAT]: Skeleton eliminated."}}},
    {"[RUIDO]: El esqueleto se sobresalta...", {{LANG_ES, "[RUIDO]: El esqueleto se sobresalta..."}, {LANG_EN, "[NOISE]: The skeleton is startled..."}}},

    {"", {{LANG_ES, ""}, {LANG_EN, ""}}}
};

const char* getText(const std::string& key) {
    auto it = dictionary.find(key);
    if (it != dictionary.end()) {
        return it->second[currentLanguage].c_str();
    }
    return key.c_str(); // Fallback
}
