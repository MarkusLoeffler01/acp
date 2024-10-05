# Name des ausführbaren Programms
TARGET = filechecker

# Compiler
CC = gcc

# Compiler-Flags
CFLAGS = -Wall -Wextra -Werror

# Linker-Flags, hier werden die OpenSSL-Bibliotheken hinzugefügt
LDFLAGS = -lcrypto -lssl

# Quellcode-Dateien
SRC = main.c

# Regel für die Kompilierung des Programms
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Regel zum Bereinigen (löscht das ausführbare Programm)
clean:
	rm -f $(TARGET)
