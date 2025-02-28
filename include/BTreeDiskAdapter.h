#ifndef BTREE_DISK_ADAPTER_H
#define BTREE_DISK_ADAPTER_H

#include "BTree.h"
#include "BTreeDisk.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <functional>

using namespace std;

// Definição de KeyAddressPair caso seja necessária
template <typename T>
struct KeyAddressPair {
    T key;
    size_t address;

    // Operadores de comparação necessários
    bool operator<(const KeyAddressPair& other) const { return key < other.key; }
    bool operator>(const KeyAddressPair& other) const { return key > other.key; }
    bool operator==(const KeyAddressPair& other) const { return key == other.key; }
};

// Classe adaptadora que integra a implementação existente de BTree com a persistência em disco
template <typename KeyType, typename RecordType>
class BTreeDiskAdapter {
private:
    BTree<KeyType> memoryTree;                     // Implementação em memória
    BTreeDisk<KeyType, long>* diskTree;               // Implementação em disco
    string dataFilePath;                         // Arquivo de dados
    string indexFilePath;                        // Arquivo de índice
    bool usesDiskStorage;                             // Define se usa armazenamento em disco

    // Função para serializar um registro
    string serializeRecord(const RecordType& record) {
        return record.pack();
    }

    // Função para deserializar um registro
    bool deserializeRecord(const string& buffer, RecordType& record) {
        return record.unpack(buffer);
    }

public:
    // Construtor com opção para armazenamento em disco
    BTreeDiskAdapter(int order, const string& dataFile = "", const string& indexFile = "")
            : memoryTree(order), dataFilePath(dataFile), indexFilePath(indexFile), usesDiskStorage(false) {

        // Se foram fornecidos caminhos para arquivos, usa armazenamento em disco
        if (!dataFile.empty() && !indexFile.empty()) {
            usesDiskStorage = true;
            diskTree = new BTreeDisk<KeyType, long>(indexFile, order);
        } else {
            diskTree = nullptr;
        }
    }

    // Destrutor
    ~BTreeDiskAdapter() {
        if (diskTree) {
            delete diskTree;
        }
    }

    // Método para inserir um registro
    void insert(const RecordType& record, KeyType key) {
        if (usesDiskStorage) {
            // Modo disco: salva o registro no arquivo de dados e a chave no índice
            ofstream dataFile(dataFilePath, ios::binary | ios::app);
            if (!dataFile) {
                throw runtime_error("Não foi possível abrir o arquivo de dados para escrita");
            }

            // Obtém a posição atual no arquivo (onde o registro será gravado)
            long position = dataFile.tellp();

            // Serializa o registro
            string serializedRecord = serializeRecord(record);

            // Grava o registro no arquivo
            dataFile.write(serializedRecord.c_str(), serializedRecord.size());
            dataFile.close();

            // Insere no índice em disco
            diskTree->insert(key, position);
        } else {
            // Modo memória: insere somente na árvore em memória
            memoryTree.insert(key);
        }
    }

    // Método para buscar um registro por chave
    RecordType search(KeyType key) {
        if (usesDiskStorage) {
            // Modo disco: busca a posição no índice e lê do arquivo de dados
            long position;
            if (!diskTree->search(key, position)) {
                throw runtime_error("Registro não encontrado");
            }

            // Abre o arquivo de dados para leitura
            ifstream dataFile(dataFilePath, ios::binary);
            if (!dataFile) {
                throw runtime_error("Não foi possível abrir o arquivo de dados para leitura");
            }

            // Posiciona no registro
            dataFile.seekg(position);

            // Lê o tamanho do registro
            int recordSize;
            dataFile.read(reinterpret_cast<char*>(&recordSize), sizeof(int));

            // Lê o registro completo
            string buffer;
            buffer.resize(sizeof(int) + recordSize);

            // Reposiciona para ler novamente desde o início do registro
            dataFile.seekg(position);
            dataFile.read(&buffer[0], sizeof(int) + recordSize);

            // Desserializa o registro
            RecordType record;
            if (!deserializeRecord(buffer, record)) {
                throw runtime_error("Falha ao desserializar o registro");
            }

            return record;
        } else {
            // Modo memória: busca diretamente na árvore em memória
            // NOTA: Esta implementação é simplificada e não reflete o uso real,
            // pois não armazenamos registros completos na árvore em memória

            if (!memoryTree.search(key)) {
                throw runtime_error("Registro não encontrado");
            }

            // Retorna um registro vazio em modo memória
            return RecordType();
        }
    }

    // Método para remover um registro
    bool remove(KeyType key) {
        if (usesDiskStorage) {
            // Modo disco: remove do índice (a implementação real precisaria lidar com o arquivo de dados)
            diskTree->remove(key);
            return true;
        } else {
            // Modo memória: remove da árvore em memória
            return memoryTree.remove(key);
        }
    }

    // Método para imprimir a estrutura da árvore
    void printTree() {
        if (usesDiskStorage) {
            // Modo disco: imprime a árvore em disco
            diskTree->printTree();
        } else {
            // Modo memória: imprime a árvore em memória
            memoryTree.print(cout);
        }
    }

    // Método para salvar a árvore em disco (quando está em modo memória)
    void saveTreeToDisk(const string& indexFile) {
        if (!usesDiskStorage) {
            // Implementação para converter a árvore de memória para disco
            // (Uma implementação completa percorreria a árvore em memória e criaria a árvore em disco)
            cout << "Conversão de árvore em memória para disco não implementada." << endl;
        }
    }

    // Método para carregar a árvore do disco para a memória
    void loadTreeFromDisk(const string& indexFile) {
        if (usesDiskStorage) {
            // Implementação para converter a árvore em disco para memória
            // (Uma implementação completa percorreria a árvore em disco e criaria a árvore em memória)
            cout << "Conversão de árvore em disco para memória não implementada." << endl;
        }
    }
};

#endif // BTREE_DISK_ADAPTER_H