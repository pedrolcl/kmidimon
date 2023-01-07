% Índice de ayuda

# Introducción

[Drumstick MIDI Monitor](https://kmidimon.sourceforge.io) registra los eventos que vienen 
desde una aplicación o puerto externo MIDI
a través del secuenciador ALSA, o almacenados como archivos MIDI estándar. Está
especialmente útil si desea depurar software MIDI o su configuración MIDI.
Cuenta con una agradable interfaz gráfica de usuario, filtros de eventos personalizables.
y parámetros del secuenciador, soporte para todos los mensajes MIDI y algunos ALSA
mensajes y guardar la lista de eventos grabados en un archivo de texto o SMF.

# Empezando

## Ventana principal

El programa comienza en estado de grabación, registrando todos los MIDI entrantes.
eventos hasta que presione el botón de parada. También hay botones para jugar,
pausar, rebobinar y avanzar, con el comportamiento habitual de cualquier otro medio
jugador.

Sobre la cuadrícula de la lista de eventos puede encontrar un conjunto de pestañas, una para cada
pista definida en un SMF. Puede agregar pestañas nuevas o cerrar pestañas sin
perder los eventos registrados, porque son solo vistas o evento
filtros.

Puede controlar las conexiones MIDI del secuenciador ALSA a programas y
dispositivos desde el interior del Drumstick MIDI Monitor. Para hacerlo, use las opciones en el menú
"Conexiones" en el menú principal. Hay opciones para conectarse y
desconecte todos los puertos de entrada disponibles para Drumstick MIDI Monitor, y también un cuadro de diálogo
donde puede elegir los puertos a monitorear y el de salida.

También puede utilizar una herramienta de conexión MIDI como
[aconnect (1)](https://linux.die.net/man/1/aconnect)
o [QJackCtl](https://qjackctl.sourceforge.io) para conectar la aplicación
o puerto MIDI a Drumstick MIDI Monitor.

Cuando se ha conectado un puerto MIDI OUT al puerto de entrada de Drumstick MIDI Monitor en
estado de grabación, mostrará los eventos MIDI entrantes si todo está
correcto.

Cada evento MIDI recibido se muestra en una sola fila. Las columnas tienen el
siguiente significado.

* **Tics**: El tiempo musical de la llegada del evento.
* **Tiempo**: El tiempo real en segundos de la llegada del evento.
* **Origen**: el identificador ALSA del dispositivo MIDI que origina el
    evento. Podrás reconocer qué evento pertenece a qué
    dispositivo si tiene varios conectados simultáneamente. Hay un
    opción de configuración para mostrar el nombre del cliente ALSA en lugar de un
    número
* **Tipo de evento **: el tipo de evento: nota activada / desactivada, cambio de control, ALSA y
    pronto
* **Canal** El canal MIDI del evento (si es un evento de canal). Eso
    también se utiliza para mostrar el canal Sysex.
* **Datos 1**: Depende del tipo de evento recibido. Por un control
    Cambiar evento o una nota, es el número de control o el número de nota
* **Datos 2**: Depende del tipo de evento recibido. Por un control
    Cambiarlo será el valor, y para un evento de Nota será el
    velocidad
* **Datos 3**: Representación de texto de exclusivos del sistema o metaeventos.

Puede ocultar o mostrar cualquier columna utilizando el menú contextual. Para abrir esto
menú, presione el botón secundario del mouse sobre la lista de eventos. Tú también puedes
utilice el cuadro de diálogo Configuración para elegir las columnas visibles.

La lista de eventos siempre muestra los eventos registrados más nuevos en la parte inferior de la
red.

Drumstick MIDI Monitor puede guardar los eventos grabados como un archivo de texto (en formato CSV) o
un archivo MIDI estándar (SMF).

## Opciones de configuración 

Para abrir el cuadro de diálogo Configuración, vaya a la Opción de menú Ajustes → Preferencias
o haga clic en el icono correspondiente en la barra de herramientas.

Esta es una lista de algunas de las opciones de configuración.

* **Pestaña Secuenciador**. La configuración predeterminada de la cola afecta a la hora del evento
    precisión.
* **Pestaña Filtros**. Aquí puede consultar varias familias de eventos para
    mostrado en la lista de eventos.
* **Pestaña de visualización**. El primer grupo de casillas de verificación permite mostrar / ocultar la
    columnas correspondientes de la lista de eventos.
* **Pestaña Miscelánea**. Las opciones misceláneas incluyen:
    + Traducir las ID de cliente de ALSA a nombres. Si está marcado, cliente de ALSA
      se utilizan nombres en lugar de números de identificación en la columna "Fuente" para
      todo el rey de los eventos, y las columnas de datos para los eventos de ALSA.
    + Traducir mensajes exclusivos del sistema universal. Si está marcado,
      Mensajes exclusivos del sistema universal (en tiempo real, no en tiempo real,
      MMC, MTC y algunos otros tipos) se interpretan y traducen.
      De lo contrario, se muestra el volcado hexadecimal.
    + Usar fuente fija. Por defecto, Drumstick MIDI Monitor utiliza la fuente del sistema en el
      lista de eventos. Si esta opción está marcada, se usa una fuente fija
      en lugar de.

# Créditos y Licencia

Programa Copyright © 2005-2023 Pedro Lopez-Cabanillas

Copyright de la documentación © 2005 Christoph Eckert

Copyright de la documentación © 2008-2021 Pedro Lopez-Cabanillas

# Instalación

## Cómo obtener Drumstick MIDI Monitor 

[Aquí](https://sourceforge.net/projects/kmidimon/files/)
puedes encontrar la última versión. También hay un espejo Git en
[GitHub](https://github.com/pedrolcl/kmidimon)

## Requisitos

Para utilizar con éxito Drumstick MIDI Monitor, necesita Qt 5, Drumstick 2
y controladores y biblioteca ALSA.

El sistema de compilación requiere [CMake](http://www.cmake.org) 3.14 o más reciente.

La biblioteca, los controladores y las utilidades de ALSA se pueden encontrar en
[Página de inicio de ALSA](http://www.alsa-project.org)

Puede encontrar una lista de cambios en https://sourceforge.net/p/kmidimon

## Compilación e instalación

Para compilar e instalar Drumstick MIDI Monitor en su sistema, escriba el
siguiente en el directorio base de la distribución Drumstick MIDI Monitor:

    $ cmake .
    $ make
    $ make install

Dado que Drumstick MIDI Monitor usa `cmake` y` make`, no debería tener problemas
compilándolo. Si tiene algún problema, infórmelo al
autor o el sistema de seguimiento de errores del proyecto.
