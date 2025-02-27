#ifndef BTREE_FILE_MANAGER_H
#define BTREE_FILE_MANAGER_H

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <functional>
#include "BTree.h"

// Classe de entrada para índice usando árvore B
template <typename KeyType, typename ValueType>
struct BTreeEntry {
    KeyType key;
    ValueType value;

    // Operadores de comparação completos
    bool operator<(const BTreeEntry& other) const {
        return key < other.key;
    }

    bool operator>(const BTreeEntry& other) const {
        return key > other.key;
    }

    bool operator==(const BTreeEntry& other) const {
        return key == other.key;
    }

    bool operator!=(const BTreeEntry& other) const {
        return key != other.key;
    }

    bool operator<=(const BTreeEntry& other) const {
        return key <= other.key;
    }

    bool operator>=(const BTreeEntry& other) const {
        return key >= other.key;
    }
};

template <typename KeyType, typename RecordType>
class BTreeFileManager {
private:
    string dataFilePath;   // Caminho do arquivo de dados
    string indexFilePath;  // Caminho do arquivo de índice
    BTree<BTreeEntry<KeyType, long>> indexTree; // Árvore B para índice em memória

    // Função para serializar um registro
    string serializeRecord(const RecordType& record) {
        // Esta função deve ser implementada pelo usuário para cada tipo de registro
        // e deve retornar uma representação serializada do registro
        return record.pack();
    }

    // Função para deserializar um registro
    bool deserializeRecord(const string& buffer, RecordType& record) {
        // Esta função deve ser implementada pelo usuário para cada tipo de registro
        // e deve preencher o registro a partir da representação serializada
        return record.unpack(buffer);
    }

    // Salva a árvore B inteira em um arquivo binário
    void saveIndexToFile() {
        ofstream indexFile(indexFilePath, ios::binary | ios::trunc);
        if (!indexFile) {
            throw runtime_error("Não foi possível abrir o arquivo de índice para escrita");
        }

        // Armazena as entradas de índice em modo inorder para preservar a ordem
        vector<BTreeEntry<KeyType, long>> entries;

        // Função para percorrer a árvore e coletar entradas em ordem
        function<void(BTreeDiskNode<BTreeEntry<KeyType, long>>*)> traverseInorder;
        traverseInorder = [&](BTreeDiskNode<BTreeEntry<KeyType, long>>* node) {
            if (!node || node->leaf) {
                if (node) {
                    for (int i = 0; i < node->n; i++) {
                        entries.push_back(node->keys[i]);
                    }
                }
                return;
            }

            for (int i = 0; i < node->n; i++) {
                if (i == 0) {
                    traverseInorder(node->children[i]);
                }
                entries.push_back(node->keys[i]);
                traverseInorder(node->children[i+1]);
            }
        };

        // Coleta todas as entradas percorrendo a árvore em ordem
        traverseInorder(indexTree.getRoot());

        // Escreve o número de entradas
        size_t numEntries = entries.size();
        indexFile.write(reinterpret_cast<const char*>(&numEntries), sizeof(numEntries));

        // Escreve cada entrada no arquivo
        for (const auto& entry : entries) {
            indexFile.write(reinterpret_cast<const char*>(&entry.key), sizeof(KeyType));
            indexFile.write(reinterpret_cast<const char*>(&entry.value), sizeof(long));
        }

        indexFile.close();
    }

    // Carrega a árvore B a partir de um arquivo binário
    void loadIndexFromFile() {
        ifstream indexFile(indexFilePath, ios::binary);
        if (!indexFile) {
            // Não é um erro se o arquivo de índice não existir ainda
            return;
        }

        // Lê o número de entradas
        size_t numEntries;
        indexFile.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));

        // Limpa a árvore atual
        indexTree.clear();

        // Lê cada entrada e insere na árvore
        for (size_t i = 0; i < numEntries; i++) {
            BTreeEntry<KeyType, long> entry;
            indexFile.read(reinterpret_cast<char*>(&entry.key), sizeof(KeyType));
            indexFile.read(reinterpret_cast<char*>(&entry.value), sizeof(long));
            indexTree.insert(entry);
        }

        indexFile.close();
    }

public:
    BTreeFileManager(const string& dataPath, const string& indexPath)
            : dataFilePath(dataPath), indexFilePath(indexPath) {
        // Carrega o índice se ele existir
        loadIndexFromFile();
    }

    // Insere um novo registro no arquivo de dados e atualiza o índice
    void insertRecord(const RecordType& record, KeyType key) {
        // Abre o arquivo de dados em modo append
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

        // Cria uma entrada de índice e insere na árvore B
        BTreeEntry<KeyType, long> entry{key, position};
        indexTree.insert(entry);

        // Salva o índice atualizado
        saveIndexToFile();
    }

    // Busca um registro pelo valor da chave
    RecordType getRecordByKey(KeyType key) {
        // Cria uma entrada de busca
        BTreeEntry<KeyType, long> searchEntry{key, 0};

        // Busca na árvore B
        BTreeDiskNode<BTreeEntry<KeyType, long>>* node = indexTree.search(searchEntry);
        if (!node) {
            throw runtime_error("Registro não encontrado");
        }

        // Encontra a entrada exata no nó
        long position = -1;
        for (int i = 0; i < node->n; i++) {
            if (node->keys[i].key == key) {
                position = node->keys[i].value;
                break;
            }
        }

        if (position == -1) {
            throw runtime_error("Registro não encontrado");
        }

        // Abre o arquivo de dados para leitura
        ifstream dataFile(dataFilePath, ios::binary);
        if (!dataFile) {
            throw runtime_error("Não foi possível abrir o arquivo de dados para leitura");
        }

        // Posiciona o cursor no registro
        dataFile.seekg(position);

        // Lê o tamanho do registro (assumindo que os primeiros bytes contêm o tamanho)
        int recordSize;
        dataFile.read(reinterpret_cast<char*>(&recordSize), sizeof(int));

        // Lê o registro completo
        string buffer;
        buffer.resize(recordSize);
        dataFile.read(&buffer[0], recordSize);

        // Cria o buffer completo para desserialização (inclui o tamanho)
        string completeBuffer;
        completeBuffer.resize(sizeof(int));
        memcpy(&completeBuffer[0], &recordSize, sizeof(int));
        completeBuffer += buffer;

        // Desserializa o registro
        RecordType record;
        bool success = deserializeRecord(completeBuffer, record);
        if (!success) {
            throw runtime_error("Falha ao desserializar o registro");
        }

        return record;
    }

    // Cria arquivos de dados e índice a partir de um vetor de registros
    void createFromRecords(const vector<RecordType>& records,
                           function<KeyType(const RecordType&)> keyExtractor) {
        // Limpa os arquivos existentes
        ofstream dataFile(dataFilePath, ios::binary | ios::trunc);
        if (!dataFile) {
            throw runtime_error("Não foi possível criar o arquivo de dados");
        }
        dataFile.close();

        // Limpa a árvore em memória
        indexTree.clear();

        // Insere cada registro
        for (const auto& record : records) {
            KeyType key = keyExtractor(record);
            insertRecord(record, key);
        }
    }

    // Cria arquivos de dados e índice a partir de um arquivo CSV
    template <typename Parser>
    void createFromCSV(const string& csvPath, Parser parser) {
        // Limpa os arquivos existentes
        ofstream dataFile(dataFilePath, ios::binary | ios::trunc);
        if (!dataFile) {
            throw runtime_error("Não foi possível criar o arquivo de dados");
        }
        dataFile.close();

        // Limpa a árvore em memória
        indexTree.clear();

        // Abre o arquivo CSV
        ifstream csvFile(csvPath);
        if (!csvFile.is_open()) {
            throw runtime_error("Não foi possível abrir o arquivo CSV");
        }

        string line;
        getline(csvFile, line); // Pula o cabeçalho

        // Processa cada linha do CSV
        while (getline(csvFile, line)) {
            try {
                // Usa o parser fornecido para converter a linha em um registro e chave
                auto [record, key] = parser(line);

                // Insere o registro
                insertRecord(record, key);
            } catch (const exception& e) {
                cerr << "Erro ao processar linha do CSV: " << e.what() << endl;
                continue;
            }
        }

        csvFile.close();
    }

    // Imprime a estrutura da árvore B (modificada para não depender de printHierarchy)
    void printIndexStructure() {
        cout << "Estrutura do índice (Árvore B):" << endl;

        // Implementa uma visualização da árvore aqui
        function<void(BTreeDiskNode<BTreeEntry<KeyType, long>>*, int, string)>
                printNode = [&](BTreeDiskNode<BTreeEntry<KeyType, long>>* node, int depth, string prefix) {
            if (node == nullptr) return;

            // Imprime a indentação e o prefixo
            cout << prefix;

            // Imprime as chaves do nó atual
            cout << "[";
            for (int i = 0; i < node->n; i++) {
                cout << node->keys[i].key << ":" << node->keys[i].value;
                if (i < node->n - 1) cout << "|";
            }
            cout << "]" << endl;

            // Se não for uma folha, imprime os filhos
            if (!node->leaf) {
                for (int i = 0; i <= node->n; i++) {
                    string newPrefix = prefix;
                    // Para filhos exceto o último, usa ├── e adiciona │   na indentação
                    if (i < node->n) {
                        cout << prefix << "├── ";
                        newPrefix += "│   ";
                    }
                        // Para o último filho, usa └── e adiciona     na indentação
                    else {
                        cout << prefix << "└── ";
                        newPrefix += "    ";
                    }

                    // Chama recursivamente para o filho
                    printNode(node->children[i], depth + 1, newPrefix);
                }
            }
        };

        // Inicia a impressão a partir da raiz
        printNode(indexTree.getRoot(), 0, "");

        cout << endl;
    }

    // Retorna a quantidade de registros indexados
    size_t getIndexSize() {
        // Conta os elementos na árvore
        size_t count = 0;

        function<void(BTreeDiskNode<BTreeEntry<KeyType, long>>*)> countNodes;
        countNodes = [&](BTreeDiskNode<BTreeEntry<KeyType, long>>* node) {
            if (!node) return;

            count += node->n; // Adiciona as chaves deste nó

            // Se não for folha, visita todos os filhos
            if (!node->leaf) {
                for (int i = 0; i <= node->n; i++) {
                    countNodes(node->children[i]);
                }
            }
        };

        countNodes(indexTree.getRoot());
        return count;
    }
};

#endif // BTREE_FILE_MANAGER_H