CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = include

# Encontra todos os arquivos .cpp
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
# Substitui o caminho do diretório src por obj e a extensão .cpp por .o
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Nome do executável final
TARGET = $(BIN_DIR)/btree_test

# Regra principal
all: directories $(TARGET)

# Cria os diretórios necessários
directories:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

# Regra para o executável
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Regra para os objetos
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regra para testar o programa
run: all
	./$(TARGET)

# Regra para visualizar a árvore (requer Graphviz)
visualize: run
	dot -Tpng btree_visual.dot -o btree_visual.png
	xdg-open btree_visual.png 2>/dev/null || open btree_visual.png 2>/dev/null || echo "Abra manualmente o arquivo btree_visual.png"

# Regra para limpar arquivos gerados
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) *.dat *.dot *.png

.PHONY: all directories run visualize clean