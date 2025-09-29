# Makefile para el sistema de chat con colas de mensajes

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
LDFLAGS = -lpthread

# Directorios
SRCDIR = src
INCDIR = include
OBJDIR = obj

# Archivos fuente del servidor
SERVER_SOURCES = $(SRCDIR)/main.c $(SRCDIR)/handlers.c $(SRCDIR)/auxiliar_functions.c
SERVER_OBJECTS = $(SERVER_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
SERVER_TARGET = server

# Archivos fuente del cliente
CLIENT_SOURCES = client_main.c client_functions.c
CLIENT_OBJECTS = $(CLIENT_SOURCES:%.c=$(OBJDIR)/%.o)
CLIENT_TARGET = client

# Regla por defecto
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Crear directorio de objetos
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Compilar servidor
$(SERVER_TARGET): $(SERVER_OBJECTS)
	$(CC) $(SERVER_OBJECTS) -o $(SERVER_TARGET) $(LDFLAGS)
	@echo "Servidor compilado exitosamente"

# Compilar cliente
$(CLIENT_TARGET): $(CLIENT_OBJECTS)
	$(CC) $(CLIENT_OBJECTS) -o $(CLIENT_TARGET) $(LDFLAGS)
	@echo "Cliente compilado exitosamente"

# Reglas para objetos del servidor
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Reglas para objetos del cliente
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos compilados
clean:
	rm -rf $(OBJDIR)
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
	@echo "Archivos limpiados"

# Limpiar colas de mensajes (útil para desarrollo)
clean-queues:
	@echo "Limpiando colas de mensajes..."
	@ipcs -q | awk '/^0x/ {print $$2}' | xargs -r ipcrm -q 2>/dev/null || true
	@echo "Colas de mensajes limpiadas"

# Instalar dependencias (si es necesario)
install-deps:
	@echo "Verificando dependencias..."
	@which gcc > /dev/null || (echo "gcc no encontrado. Instala build-essential" && exit 1)
	@echo "Dependencias OK"

# Ejecutar servidor
run-server: $(SERVER_TARGET)
	@echo "Iniciando servidor..."
	./$(SERVER_TARGET)

# Ejecutar cliente
run-client: $(CLIENT_TARGET)
	@echo "Iniciando cliente..."
	./$(CLIENT_TARGET)

# Mostrar ayuda
help:
	@echo "Makefile para sistema de chat"
	@echo ""
	@echo "Targets disponibles:"
	@echo "  all          - Compilar servidor y cliente"
	@echo "  server       - Compilar solo el servidor"
	@echo "  client       - Compilar solo el cliente"
	@echo "  clean        - Limpiar archivos compilados"
	@echo "  clean-queues - Limpiar colas de mensajes del sistema"
	@echo "  run-server   - Compilar y ejecutar servidor"
	@echo "  run-client   - Compilar y ejecutar cliente"
	@echo "  install-deps - Verificar dependencias"
	@echo "  help         - Mostrar esta ayuda"

# Declarar targets que no son archivos
.PHONY: all clean clean-queues install-deps run-server run-client help

# Dependencias específicas
$(OBJDIR)/main.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/server.h
$(OBJDIR)/handlers.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/server.h
$(OBJDIR)/auxiliar_functions.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/server.h
$(OBJDIR)/client_main.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/client.h
$(OBJDIR)/client_functions.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/client.h