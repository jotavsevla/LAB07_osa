#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <limits>
#include <chrono>
#include <iomanip>
#include <functional>
#include "BTree.h"
#include "IntegerRecord.h"

using namespace std;

// Classe que representa um nó da árvore B em disco
class BTreeNode {
public:
    int address;          // Endereço do nó no arquivo
    int parentAddress;    // Endereço do nó pai (-1 se for raiz)
    vector<int> keys;     // Chaves armazenadas no nó
    vector<int> children; // Endereços dos filhos
    bool isLeaf;          // Se é nó folha

    BTreeNode() : address(-1), parentAddress(-1), isLeaf(true) {}
    BTreeNode(int addr) : address(addr), parentAddress(-1), isLeaf(true) {}
};

// Classe para gerenciar a persistência da árvore B em disco
class BTreeIO {
private:
    string filename;
    int order;
    int rootAddress;
    int nextAddress;

    // Lê o cabeçalho do arquivo
    bool readHeader(fstream& file) {
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(&order), sizeof(int));
        file.read(reinterpret_cast<char*>(&rootAddress), sizeof(int));
        file.read(reinterpret_cast<char*>(&nextAddress), sizeof(int));
        return !file.fail();
    }

    // Escreve o cabeçalho no arquivo
    bool writeHeader(fstream& file) {
        file.seekp(0, ios::beg);
        file.write(reinterpret_cast<const char*>(&order), sizeof(int));
        file.write(reinterpret_cast<const char*>(&rootAddress), sizeof(int));
        file.write(reinterpret_cast<const char*>(&nextAddress), sizeof(int));
        return !file.fail();
    }

    // Calcula o offset para um nó no arquivo
    long calculateOffset(int address) const {
        // Cabeçalho: ordem (int) + raizAddress (int) + nextAddress (int)
        long headerSize = 3 * sizeof(int);
        // Tamanho do nó: address (int) + parentAddress (int) + isLeaf (bool) +
        // numKeys (int) + numChildren (int) + keys[2*order-1] + children[2*order]
        long nodeSize = 4 * sizeof(int) + sizeof(bool) +
                        (2 * order - 1) * sizeof(int) +
                        (2 * order) * sizeof(int);
        return headerSize + (address * nodeSize);
    }

public:
    BTreeIO(const string& fname, int treeOrder)
            : filename(fname), order(treeOrder), rootAddress(-1), nextAddress(0) {

        // Verifica se o arquivo existe
        fstream file(filename, ios::in | ios::binary);
        if (file.good()) {
            // Arquivo existe, lê o cabeçalho
            if (!readHeader(file)) {
                cerr << "Erro ao ler o cabeçalho do arquivo" << endl;
            }
            file.close();
        } else {
            // Arquivo não existe, cria um novo
            fstream newFile(filename, ios::out | ios::binary);
            if (newFile.good()) {
                writeHeader(newFile);
            } else {
                cerr << "Erro ao criar o arquivo" << endl;
            }
            newFile.close();
        }
    }

    // Lê um nó do arquivo
    BTreeNode readNode(int address) {
        BTreeNode node(address);

        fstream file(filename, ios::in | ios::binary);
        if (!file.good()) {
            cerr << "Erro ao abrir o arquivo para leitura" << endl;
            return node;
        }

        long offset = calculateOffset(address);
        file.seekg(offset);

        // Lê os dados básicos do nó
        file.read(reinterpret_cast<char*>(&node.address), sizeof(int));
        file.read(reinterpret_cast<char*>(&node.parentAddress), sizeof(int));
        file.read(reinterpret_cast<char*>(&node.isLeaf), sizeof(bool));

        // Lê o número de chaves e filhos
        int numKeys, numChildren;
        file.read(reinterpret_cast<char*>(&numKeys), sizeof(int));
        file.read(reinterpret_cast<char*>(&numChildren), sizeof(int));

        // Lê as chaves
        node.keys.resize(numKeys);
        for (int i = 0; i < numKeys; i++) {
            int key;
            file.read(reinterpret_cast<char*>(&key), sizeof(int));
            node.keys[i] = key;
        }

        // Lê os filhos
        node.children.resize(numChildren);
        for (int i = 0; i < numChildren; i++) {
            int child;
            file.read(reinterpret_cast<char*>(&child), sizeof(int));
            node.children[i] = child;
        }

        file.close();
        return node;
    }

    // Escreve um nó no arquivo
    bool writeNode(const BTreeNode& node) {
        fstream file(filename, ios::in | ios::out | ios::binary);
        if (!file.good()) {
            cerr << "Erro ao abrir o arquivo para escrita" << endl;
            return false;
        }

        long offset = calculateOffset(node.address);
        file.seekp(offset);

        // Escreve os dados básicos do nó
        file.write(reinterpret_cast<const char*>(&node.address), sizeof(int));
        file.write(reinterpret_cast<const char*>(&node.parentAddress), sizeof(int));
        file.write(reinterpret_cast<const char*>(&node.isLeaf), sizeof(bool));

        // Escreve o número de chaves e filhos
        int numKeys = node.keys.size();
        int numChildren = node.children.size();
        file.write(reinterpret_cast<const char*>(&numKeys), sizeof(int));
        file.write(reinterpret_cast<const char*>(&numChildren), sizeof(int));

        // Escreve as chaves
        for (int key : node.keys) {
            file.write(reinterpret_cast<const char*>(&key), sizeof(int));
        }

        // Preenche o resto do espaço para chaves com zeros
        int zero = 0;
        for (int i = numKeys; i < 2 * order - 1; i++) {
            file.write(reinterpret_cast<const char*>(&zero), sizeof(int));
        }

        // Escreve os filhos
        for (int child : node.children) {
            file.write(reinterpret_cast<const char*>(&child), sizeof(int));
        }

        // Preenche o resto do espaço para filhos com zeros
        for (int i = numChildren; i < 2 * order; i++) {
            file.write(reinterpret_cast<const char*>(&zero), sizeof(int));
        }

        file.close();
        return true;
    }

    // Salva uma árvore B em disco (versão simples para demonstração)
    void saveTree(const BTree<int>& tree) {
        // Implementação simplificada: apenas cria um nó raiz
        // Para uma implementação completa, seria necessário percorrer a árvore interna

        // Reseta contadores
        rootAddress = 0;
        nextAddress = 1;

        // Cria um nó raiz com algumas chaves de exemplo
        BTreeNode root(rootAddress);
        root.isLeaf = true;
        root.keys = {10, 20, 30}; // Exemplos de chaves

        // Abre o arquivo para escrita
        fstream file(filename, ios::out | ios::binary);
        if (!file.good()) {
            cerr << "Erro ao abrir o arquivo para salvar a árvore" << endl;
            return;
        }

        // Escreve o cabeçalho atualizado
        writeHeader(file);
        file.close();

        // Escreve o nó raiz
        if (!writeNode(root)) {
            cerr << "Erro ao escrever o nó raiz" << endl;
        }

        cout << "Árvore salva em disco (implementação simplificada)" << endl;
    }

    // Visualiza a estrutura da árvore em disco
    void printTree() {
        cout << "Árvore B em disco (ordem " << order << "):" << endl;

        // Verifica se a árvore está vazia
        if (rootAddress == -1) {
            cout << "Árvore vazia" << endl;
            return;
        }

        // Lê o nó raiz
        BTreeNode root = readNode(rootAddress);
        printNode(root, 0); // Imprime o nó raiz com nível 0 de indentação
    }

private:
    // Imprime um nó com indentação baseada no nível
    void printNode(const BTreeNode& node, int level) {
        // Indentação
        for (int i = 0; i < level; i++) {
            cout << "    ";
        }

        // Imprime as chaves do nó
        cout << "Nó " << node.address << " (";
        if (node.parentAddress == -1) {
            cout << "raiz";
        } else {
            cout << "pai: " << node.parentAddress;
        }
        cout << ", " << (node.isLeaf ? "folha" : "interno") << "): [";

        for (size_t i = 0; i < node.keys.size(); i++) {
            cout << node.keys[i];
            if (i < node.keys.size() - 1) {
                cout << ", ";
            }
        }
        cout << "]" << endl;

        // Se não for folha, imprime os filhos
        if (!node.isLeaf && !node.children.empty()) {
            for (int childAddr : node.children) {
                BTreeNode child = readNode(childAddr);
                printNode(child, level + 1);
            }
        }
    }
};

// Função para exibir o menu
void displayMenu() {
    cout << "\n=== SISTEMA DE ÁRVORE B COM PERSISTÊNCIA EM DISCO ===" << endl;
    cout << "1. Inserir valor na árvore em memória" << endl;
    cout << "2. Buscar valor na árvore em memória" << endl;
    cout << "3. Remover valor da árvore em memória" << endl;
    cout << "4. Exibir árvore em memória" << endl;
    cout << "5. Salvar árvore em disco" << endl;
    cout << "6. Visualizar árvore em disco" << endl;
    cout << "0. Sair" << endl;
    cout << "Escolha uma opção: ";
}

int main() {
    // Cria uma árvore B em memória
    BTree<int> memoryTree(3); // Ordem 3

    // Cria um gerenciador de E/S para árvore B em disco
    BTreeIO diskIO("btree.dat", 3);

    int choice = -1;
    while (choice != 0) {
        displayMenu();
        cin >> choice;

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1: { // Inserir valor
                cout << "Digite o valor a inserir: ";
                int value;
                cin >> value;

                memoryTree.insert(value);

                cout << "Valor " << value << " inserido na árvore em memória" << endl;
                break;
            }

            case 2: { // Buscar valor
                cout << "Digite o valor a buscar: ";
                int value;
                cin >> value;

                if (memoryTree.search(value)) {
                    cout << "Valor " << value << " encontrado na árvore" << endl;
                } else {
                    cout << "Valor " << value << " NÃO encontrado na árvore" << endl;
                }

                break;
            }

            case 3: { // Remover valor
                cout << "Digite o valor a remover: ";
                int value;
                cin >> value;

                if (memoryTree.remove(value)) {
                    cout << "Valor " << value << " removido da árvore" << endl;
                } else {
                    cout << "Valor " << value << " não encontrado para remoção" << endl;
                }

                break;
            }

            case 4: { // Exibir árvore em memória
                cout << "\nÁrvore B em memória:" << endl;
                memoryTree.print();
                break;
            }

            case 5: { // Salvar árvore em disco
                diskIO.saveTree(memoryTree);
                break;
            }

            case 6: { // Visualizar árvore em disco
                diskIO.printTree();
                break;
            }

            case 0:
                cout << "Encerrando o programa..." << endl;
                break;

            default:
                cout << "Opção inválida!" << endl;
                break;
        }
    }

    return 0;
}