# Yet Another MP3 Player (YAMP)
Reproductor MP3 para ESP32 con pantalla CYD 2.8" y salida de audio por DAC I2S o DAC interno.

## Por qué?
Pues abrir spotify es molesto por lo lenta y pesada de la aplicación. Desde hace tiempos quería comprar un reproductor mp3 dedicado, pero aprovechando un poco de tiempo libre (y algo de IA) decidí armar uno propio, aprovechando la potencia actual de los microcontroladores

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
https://www.amazon.com/-/es/Hosyond-pantalla-resistiva-controlador-ILI9341/dp/B0D92C9MMH/ref=sr_1_3?crid=2PL94VUHTK0LW&dib=eyJ2IjoiMSJ9.iIxxc2zixmbD_rQOntfgcJjWOP5lc6ioFe4-fVR48PGi3JTw0hhSTWM7i69by9mNEbZeTdqho05nqgWU1zQHZ83WutYC-E_Ni1Is-XexZ4TqW_J_L_Qx6w302J7Bylh1EC08YGRSikVcBGzFPEazL55EgIQ9PX0nxL909gNz_0F3HKWjUiGfm5U-6WsD1IA2HvudOcPKHwDFftVc-WH77SZiDpECXlX9PiywRDEmCbY.AAoDPogaI7zEV5VjXpZFFnq54MGBqPimzdrPITpZt4M&dib_tag=se&keywords=cheap%2Byellow%2Bdisplay&qid=1782941141&sprefix=cheap%2Byellow%2Bdi%2Caps%2C226&sr=8-3&th=1

Para una mejor calidad de audio, vamos a usar un DAC I2S tipo PCM5102
https://www.amazon.com/Psiriol-5435VFFG5586/dp/B0H2F4LV46/ref=sr_1_1_sspa?__mk_es_US=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=368XJLWXDNHOZ&dib=eyJ2IjoiMSJ9.rgc8vO5ANxLwVNAw9_l4-b3bX9zdjCnzsEv2LlI78vq6QtF3_R6R9qxFo_oVySPlG0Tgmn3NVg6eK7XjsvoOIWHvDu_FkuH9hcJo_JDV69TOktWx79aDsbUbxb8Z-b-oTFHUiTUseDEvQJPC-WjkeFWcHZl6tRv5afNYTyIy5V2GC5am9FNjfFX_wPCXbZZbyc-JE2YWlYPCFDfQdMeb9gJl3oxpMyt7fPKu6T477Nc.vl9X7XOBo2ELsyYpCYCpUEDwc09iJ-h1hSoEfN3QO5U&dib_tag=se&keywords=pcm5102&qid=1782941194&sprefix=pcm5102%2Caps%2C235&sr=8-1-spons&sp_csd=d2lkZ2V0TmFtZT1zcF9hdGY&psc=1

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

## Conexion sugerida

El firmware esta basado en el cableado tipico de la CYD 2.8" y los modulos ya incluidos en el proyecto.

### Tarjeta SD
La libreria `CYD28_SD` usa estos pines:

- `SCK = GPIO18`
- `MISO = GPIO19`
- `MOSI = GPIO23`
- `CS = GPIO5`

### Touch
La pantalla tactil usa `CYD28_Touchscreen` con estos pines:

- `IRQ = GPIO36`
- `MOSI = GPIO32`
- `MISO = GPIO39`
- `CLK = GPIO25`
- `CS = GPIO33`

### Backlight
- Control de retroiluminacion: `GPIO21`

### Audio I2S
En el entorno `mp3player_i2c_audio_mod`:

- `BCK = GPIO4` (hay que romper la pista que va hacia el led RGB desde este pin)
- `LRCLK = GPIO22`
- `DIN = GPIO27`

### Boton BOOT
- `GPIO0`

## Instalacion rapida

1. Abrir el proyecto en PlatformIO.
2. Verificar que la configuracion por defecto sea `mp3player_i2c_audio_mod`.
3. Copiar archivos MP3 a la tarjeta SD.
4. Cargar el firmware en la placa.
5. Encender el equipo y navegar con la pantalla tactil.

## Arbol de arranque

```text
Encendido
  -> inicializa SD y UI
  -> monta audio
  -> carga estado guardado de la SD
  -> restaura volumen
  -> intenta reproducir la ultima pista
  -> si no existe, muestra el navegador
```

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
