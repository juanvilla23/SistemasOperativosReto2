# Sistema de Chat con Colas de Mensajes

Un sistema de chat cliente-servidor implementado en C usando colas de mensajes de System V IPC.

## Características

- **Múltiples canales**: Los usuarios pueden unirse a diferentes canales de chat
- **Mensajería en tiempo real**: Los mensajes se distribuyen instantáneamente a todos los usuarios del canal
- **Gestión de usuarios**: Control de entrada y salida de usuarios de los canales
- **Lista de canales**: Los usuarios pueden ver todos los canales disponibles
- **Interfaz de línea de comandos**: Cliente simple y fácil de usar

## Compilación

```bash
# Compilar todo (servidor y cliente)
make all

# O compilar por separado
make server
make client
```

## Uso

### 1. Iniciar el servidor

```bash
# Opción 1: Usando make
make run-server

# Opción 2: Directamente
./server
```

El servidor se iniciará y mostrará:
```
Servidor inicializado correctamente
ID de cola de mensajes: [ID]
```

### 2. Iniciar cliente(s)

En otra terminal:

```bash
# Opción 1: Usando make
make run-client

# Opción 2: Directamente
./client
```

## Comandos del Cliente

Una vez conectado, el cliente admite los siguientes comandos:

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `help` | Mostrar ayuda | `help` |
| `create <canal>` | Crear un nuevo canal | `create General` |
| `join <canal>` | Unirse a un canal | `join General` |
| `leave` | Salir del canal actual | `leave` |
| `msg <mensaje>` | Enviar mensaje al canal | `msg Hola a todos!` |
| `list` | Listar canales disponibles | `list` |
| `status` | Ver estado actual | `status` |
| `clear` | Limpiar pantalla | `clear` |
| `quit` o `exit` | Salir del programa | `quit` |

## Ejemplo de Sesión

```
=== Cliente de Chat ===
Ingresa tu nombre: Juan

¡Conectado al servidor exitosamente!
Escribe 'help' para ver los comandos disponibles

> list
[SERVIDOR]: No hay canales disponibles

> create General
[SERVIDOR]: Canal 'General' creado exitosamente

> join General
[SERVIDOR]: Te has unido al canal exitosamente
[Sistema]: Juan se ha unido al canal

> msg ¡Hola a todos!
[SERVIDOR]: Mensaje enviado correctamente

> list
[SERVIDOR]: Canales disponibles:
- General (1/10 usuarios)

> leave
[SERVIDOR]: Has salido del canal exitosamente
[Sistema]: Juan ha salido del canal

> quit
¡Hasta luego!
```

## Arquitectura del Sistema

### Servidor (`server`)
- **Archivo principal**: `src/main.c`
- **Handlers**: `src/handlers.c` - Maneja diferentes tipos de mensajes
- **Funciones auxiliares**: `src/auxiliar_functions.c` - Utilidades del servidor

### Cliente (`client`)
- **Archivo principal**: `client_main.c`
- **Funciones**: `client_functions.c` - Comunicación con el servidor

### Protocolo de Comunicación

El sistema usa colas de mensajes con los siguientes tipos:

- `MSG_JOIN_REQUEST` (1): Solicitud para unirse a un canal
- `MSG_LEAVE_REQUEST` (2): Solicitud para salir de un canal
- `MSG_SEND_MESSAGE` (3): Enviar mensaje a un canal
- `MSG_SERVER_RESPONSE` (4): Respuesta del servidor
- `MSG_BROADCAST` (5): Mensaje difundido a todos los usuarios del canal
- `MSG_LIST_CHANNELS` (6): Solicitud de lista de canales

## Limitaciones

- Máximo 10 canales simultáneos
- Máximo 10 clientes por canal
- Máximo 256 caracteres por mensaje
- Máximo 50 caracteres para nombres

## Compilación Avanzada

```bash
# Limpiar archivos compilados
make clean

# Limpiar colas de mensajes del sistema (útil para debugging)
make clean-queues

# Verificar dependencias
make install-deps
```

## Solución de Problemas

### Error: "msgget (servidor no encontrado)"
- Asegúrate de que el servidor esté ejecutándose primero
- Verifica que no haya problemas de permisos

### Error: "Permission denied"
- Ejecuta `make clean-queues` para limpiar colas anteriores
- Verifica permisos de escritura en `/tmp`

### El cliente no recibe mensajes
- Verifica que el hilo de escucha esté funcionando
- Comprueba que estés en un canal activo

## Desarrollo

Para contribuir al proyecto:

1. Modifica los archivos fuente según sea necesario
2. Compila con `make all`
3. Prueba con múltiples clientes
4. Usa `make clean-queues` si hay problemas con colas

## Dependencias

- GCC (compilador C)
- pthread library (para hilos)
- System V IPC support (colas de mensajes)

En sistemas Ubuntu/Debian:
```bash
sudo apt-get install build-essential
```