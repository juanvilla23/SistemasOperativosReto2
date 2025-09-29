CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
LDFLAGS = -lpthread

SERVERDIR = server
CLIENTDIR = client
INCDIR = include
OBJDIR = obj

SERVEROBJDIR = $(OBJDIR)/server
CLIENTOBJDIR = $(OBJDIR)/client

SERVER_SOURCES = $(wildcard $(SERVERDIR)/*.c)
SERVER_OBJECTS = $(SERVER_SOURCES:$(SERVERDIR)/%.c=$(SERVEROBJDIR)/%.o)
SERVER_TARGET = server_app

CLIENT_SOURCES = $(wildcard $(CLIENTDIR)/*.c)
CLIENT_OBJECTS = $(CLIENT_SOURCES:$(CLIENTDIR)/%.c=$(CLIENTOBJDIR)/%.o)
CLIENT_TARGET = client_app

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(SERVEROBJDIR): | $(OBJDIR)
	mkdir -p $(SERVEROBJDIR)

$(CLIENTOBJDIR): | $(OBJDIR)
	mkdir -p $(CLIENTOBJDIR)

$(SERVER_TARGET): $(SERVER_OBJECTS)
	$(CC) $(SERVER_OBJECTS) -o $(SERVER_TARGET) $(LDFLAGS)
	@echo "Servidor compilado exitosamente"

$(CLIENT_TARGET): $(CLIENT_OBJECTS)
	$(CC) $(CLIENT_OBJECTS) -o $(CLIENT_TARGET) $(LDFLAGS)
	@echo "Cliente compilado exitosamente"

$(SERVEROBJDIR)/%.o: $(SERVERDIR)/%.c | $(SERVEROBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENTOBJDIR)/%.o: $(CLIENTDIR)/%.c | $(CLIENTOBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

server: $(SERVER_TARGET)

client: $(CLIENT_TARGET)

clean:
	rm -rf $(OBJDIR)
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

clean-queues:
	@ipcs -q | awk '/^0x/ {print $$2}' | xargs -r ipcrm -q 2>/dev/null || true

show-queues:
	@ipcs -q || echo "No hay colas activas"

install-deps:
	@which gcc > /dev/null || (echo "gcc no encontrado" && exit 1)

run-server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

run-client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET)

run-server-bg: $(SERVER_TARGET)
	@./$(SERVER_TARGET) &

stop-server:
	@pkill -f "./$(SERVER_TARGET)" || echo "No hay servidor ejecutandose"

demo: $(SERVER_TARGET) $(CLIENT_TARGET)
	@./$(SERVER_TARGET) &
	@sleep 2
	@echo "Servidor iniciado. Ejecuta 'make run-client' para conectar clientes"

debug: CFLAGS += -g -DDEBUG
debug: clean all

release: CFLAGS += -O2 -DNDEBUG
release: clean all

check-structure:
	@test -d $(SERVERDIR) || (echo "Falta directorio $(SERVERDIR)" && exit 1)
	@test -d $(CLIENTDIR) || (echo "Falta directorio $(CLIENTDIR)" && exit 1)
	@test -d $(INCDIR) || (echo "Falta directorio $(INCDIR)" && exit 1)
	@test -f $(INCDIR)/queues/core.h || (echo "Falta $(INCDIR)/queues/core.h" && exit 1)
	@test -f $(INCDIR)/queues/server.h || (echo "Falta $(INCDIR)/queues/server.h" && exit 1)
	@test -f $(INCDIR)/queues/client.h || (echo "Falta $(INCDIR)/queues/client.h" && exit 1)

info:
	@echo "Servidor: $(words $(SERVER_SOURCES)) archivos"
	@echo "Cliente: $(words $(CLIENT_SOURCES)) archivos"
	@echo "Headers: $(shell find $(INCDIR) -name '*.h' | wc -l) archivos"
	@echo "Compilador: $(CC)"

help:
	@echo "Targets principales:"
	@echo "  all              - Compilar servidor y cliente"
	@echo "  server           - Compilar solo el servidor"
	@echo "  client           - Compilar solo el cliente"
	@echo "  run-server       - Ejecutar servidor"
	@echo "  run-client       - Ejecutar cliente"
	@echo "  clean            - Limpiar archivos compilados"
	@echo "  clean-queues     - Limpiar colas de mensajes"
	@echo "  debug            - Compilar con debug"
	@echo "  release          - Compilar optimizado"
	@echo "  help             - Mostrar ayuda"

.PHONY: all server client clean clean-queues show-queues install-deps run-server run-client run-server-bg stop-server demo debug release check-structure info help

$(SERVEROBJDIR)/main.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/server.h
$(SERVEROBJDIR)/handlers.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/server.h
$(SERVEROBJDIR)/auxiliar_functions.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/server.h
$(CLIENTOBJDIR)/main.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/client.h
$(CLIENTOBJDIR)/client_functions.o: $(INCDIR)/queues/core.h $(INCDIR)/queues/client.h