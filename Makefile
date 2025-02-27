# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

# Diretórios
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin

# Encontrar todos os arquivos fonte
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

# Nome do executável final
TARGET = $(BIN_DIR)/BTreeDiskNode

# Regra principal
all: diretorio $(TARGET)

# Criar diretório para o binário
diretorio:
	mkdir -p $(BIN_DIR)

# Regra para o executável principal
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regra para compilar cada arquivo .cpp em .o
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regra para limpar arquivos gerados
clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)

# Regra para executar o programa
run: all
	./$(TARGET)

.PHONY: all diretorio clean run