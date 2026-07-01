# reproductor_mp3
Reproductor MP3 para ESP32 con pantalla CYD 2.8" y salida de audio por DAC I2S o DAC interno.

## Resumen
Este proyecto implementa un reproductor de musica con:

- Navegacion por carpeta y seleccion de archivos desde la pantalla tactil.
- Reproduccion de archivos de audio desde la tarjeta SD.
- Control de volumen, pausa, siguiente, anterior y detener.
- Pantalla de reproduccion con metadatos, tiempo, barra de progreso y VU meters.
- Persistencia de la ultima pista y el volumen en un archivo de la SD.
- Modo de ahorro de energia: si no hay interaccion durante 30 segundos, se apaga la retroiluminacion y se deshabilita el touch.

## Hardware
Configuracion pensada para una placa CYD 2.8" con ESP32.

### Salida de audio
El proyecto soporta dos variantes:

- Mod I2S con DAC externo tipo PCM5102.
- Mod con DAC interno del ESP32.

La configuracion activa por defecto en `platformio.ini` es la de I2S:

- `USE_I2S_DAC`
- `I2S_BCK_PIN=4`
- `I2S_LRCLK_PIN=22`
- `I2S_DIN_PIN=27`

### Pantalla y touch
- TFT ILI9341 con `TFT_eSPI`.
- Touch XPT2046 con la libreria del proyecto `CYD28_Touchscreen`.

### Boton BOOT
El boton BOOT del reverso de la pantalla se usa para:

- Despertar la interfaz cuando esta en reposo.
- Apagar la interfaz manualmente cuando esta activa.

## Funciones principales

- Explorador de archivos en SD.
- Reproduccion de pistas por seleccion tactil.
- Reproduccion automatica de la ultima pista guardada al encender.
- Restauracion del volumen guardado junto con la ultima pista.
- Soporte de metadatos `Artist` y `Title`.
- Interfaz traducida al espanol.
- Apagado de pantalla y touch por inactividad.

## Persistencia
El estado se guarda en la SD en:

- `/player_state.txt`

Formato del archivo:

```txt
track=/ruta/completa/a/la/pista.mp3
volume=65
```

Comportamiento:

- Si el archivo existe al arrancar, se intenta restaurar la ultima pista.
- Si la pista ya no existe en la SD, el equipo vuelve al listado normal.
- El volumen se guarda junto con el cambio de pista.
- El archivo se reescribe de forma atómica usando un temporal.

## Interfaz de uso

### Pantalla principal
La pantalla principal muestra:

- Lista de archivos y carpetas.
- Estado de carga o error.
- Prefijo visual para directorios.

### Pantalla de reproduccion
La pantalla de reproduccion muestra:

- Titulo "Reproduciendo".
- Artista.
- Pista.
- Disco o carpeta.
- Tiempo transcurrido y tiempo total.
- Barra de progreso.
- Medidores VU izquierdo y derecho.
- Control de volumen.
- Controles de reproduccion.

## Controles tactiles

### Vista de archivos
- `UP`: subir una fila.
- `BACK`: volver a la carpeta anterior.
- `DOWN`: bajar una fila.
- `OPEN`: abrir carpeta o seleccionar archivo.

### Vista de reproduccion
- Boton principal: pausa o reanudar.
- `VOL+`: subir volumen.
- `PREV`: pista anterior.
- `NEXT`: siguiente pista.
- `STOP`: detener reproduccion.
- `VOL-`: bajar volumen.

### Navegacion entre pantallas
- Tocar el area de botones cambia entre lista y reproductor.

## Ahorro de energia
Si no hay interaccion tactil durante 30 segundos:

- Se apaga la retroiluminacion.
- Se deshabilita la lectura del touch.

Para volver:

- Pulsar el boton BOOT.

El mismo boton BOOT tambien funciona como interruptor:

- Si la interfaz esta activa, la apaga.
- Si esta dormida, la despierta.

## Consola serial
El proyecto incluye comandos por serial a traves de `SimpleCLI`.

### Comandos disponibles
- `list`
  - Lista el contenido de la SD en la consola.
- `play <ruta>`
  - Reproduce un archivo de audio por ruta completa.

## Compilacion
Proyecto preparado para PlatformIO.

### Entorno por defecto
El entorno por defecto es:

- `mp3player_i2c_audio_mod`

### Compilar
```bash
pio run
```

### Cargar
```bash
pio run -t upload
```

### Monitor serie
```bash
pio device monitor
```

## Dependencias
Dependencias principales del proyecto:

- `CYD28_SD`
- `CYD28_audio`
- `CYD28_Touchscreen`
- `CYD28_RGBled`
- `TFT_eSPI`
- `SimpleCLI`
- `WiFiManager`

## Archivos relevantes

- `src/main.cpp`: flujo principal, arranque, restauracion, sleep y control tactil.
- `src/gui.cpp`: interfaz grafica y manejo de pantallas.
- `src/app_state.cpp`: persistencia del estado en SD.
- `src/ui_text.cpp`: textos de la interfaz y traducciones.
- `src/console.cpp`: comandos por serial.

## Notas

- El proyecto esta orientado a hardware real de la familia CYD 2.8".
- Si la ruta guardada ya no existe en la SD, se vuelve automaticamente al navegador de archivos.
- Si el boton BOOT no coincide con tu revision de hardware, revisa `kBootButtonPin` en `src/main.cpp`.
