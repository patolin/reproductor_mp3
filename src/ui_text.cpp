#include "ui_text.h"

namespace UIText
{
namespace
{
constexpr Strings kEnglish = {
    "PLAYER",
    "Initializing...",
    "No track",
    "selected",
    "No SD card or invalid folder",
    "Empty folder",
    "No entries",
    "Now Playing",
    "Disc: ",
    "Elapsed: ",
    " - Total: ",
    "0:00",
    "--:--",
    "...",
    "[DIR] ",
    "      ",
    "PAUSE",
    "RESUME",
    "VOL+",
    "PREV",
    "NEXT",
    "STOP",
    "VOL-",
    "UP",
    "BACK",
    "DOWN",
    "OPEN"
};

constexpr Strings kSpanish = {
    "REPRODUCTOR",
    "Iniciando...",
    "Sin pista",
    "seleccionada",
    "Tarjeta SD ausente o carpeta invalida",
    "Carpeta vacia",
    "Sin elementos",
    "Reproduciendo",
    "Disco: ",
    "Tiempo: ",
    " - Total: ",
    "0:00",
    "--:--",
    "...",
    "[DIR] ",
    "      ",
    "PAUSA",
    "REANUDAR",
    "VOL+",
    "ANTERIOR",
    "SIGUIENTE",
    "DETENER",
    "VOL-",
    "ARRIBA",
    "ATRAS",
    "ABAJO",
    "ABRIR"
};

Language gLanguage = Language::Spanish;
}

void setLanguage(Language language)
{
    gLanguage = language;
}

Language currentLanguage()
{
    return gLanguage;
}

const Strings &strings()
{
    return (gLanguage == Language::Spanish) ? kSpanish : kEnglish;
}
}
