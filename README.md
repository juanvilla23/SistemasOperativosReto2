# Sistema de Chat con Colas de Mensajes

Sistema de chat cliente-servidor implementado en C usando colas de mensajes de System V IPC.

## Estructura del Proyecto

```
SistemasOperativosReto2/
├── server/
│   ├── main.c
│   ├── handlers.c
│   └── auxiliar_functions.c
├── client/
│   ├── main.c
│   └── client_functions.c
├── include/
│   └── queues/
│       ├── core.h
│       ├── server.h
│       └── client.h
├── obj/
├── Makefile
└── README.md
```

## Características

- Múltiples canales de chat
- Mensajería en tiempo real
- Gestión completa de canales
- Control de usuarios
- Interfaz de línea de comandos
- Arquitectura modular

## Compilación

```bash
make all
make help
```

## Comandos del Makefile

```bash
make all          
make server       
make client       
make debug        
make release      
make run-server   
make run-client   
make clean        
make clean-queues 
```

## Uso

### Iniciar servidor
```bash
make run-server
```

### Conectar cliente
```bash
make run-client
```

### Comandos del Cliente

| Comando | Descripción |
|---------|-------------|
| `help` | Mostrar ayuda |
| `create <canal>` | Crear canal |
| `join <canal>` | Unirse a canal |
| `leave` | Salir del canal |
| `msg <mensaje>` | Enviar mensaje |
| `list` | Listar canales |
| `status` | Ver estado |
| `clear` | Limpiar pantalla |
| `quit` | Salir |

## Ejemplo de Uso

```
$ make run-server
Servidor inicializado correctamente

$ make run-client
Ingresa tu nombre: Juan
> create General
[SERVIDOR]: Canal creado exitosamente
> join General  
[SERVIDOR]: Te has unido al canal
> msg Hola mundo
[SERVIDOR]: Mensaje enviado
```

## Protocolo de Comunicación

Tipos de mensaje:
- MSG_JOIN_REQUEST (1)
- MSG_LEAVE_REQUEST (2)  
- MSG_SEND_MESSAGE (3)
- MSG_SERVER_RESPONSE (4)
- MSG_BROADCAST (5)
- MSG_LIST_CHANNELS (6)
- MSG_CREATE_CHANNEL (7)

## Configuración

- Máximo 10 canales
- Máximo 10 clientes por canal
- Máximo 256 caracteres por mensaje
- Máximo 50 caracteres para nombres

## Dependencias

- GCC
- pthread
- System V IPC

```bash
sudo apt-get install build-essential
```

## Solución de Problemas

```bash
make clean-queues
make clean
make all
```