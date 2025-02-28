#ifndef BTREE_DISK_H
#define BTREE_DISK_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

// Estrutura para representar um nó da árvore B em disco
template <typename KeyType>
struct BTreeDiskNode {
    int address;       // Endereço do nó no arquivo
    int parentAddress; // Endereço do nó pai (-1 se for raiz)
    vector<KeyType> keys;  // Chaves armazenadas neste nó
    vector<int> childAddresses; // Endereços dos filhos (vazio se for folha)
    bool isLeaf;       // Flag que indica se é um nó folha

    BTreeDiskNode(int addr = -1, bool leaf = true)
            : address(addr), parentAddress(-1), isLeaf(leaf) {}
};

template <typename KeyType, typename AddressType>
class BTreeDisk {
private:
    string indexFilePath;  // Caminho do arquivo de índice
    int order;                  // Ordem da árvore B (máximo de chaves = 2*order-1)
    int rootAddress;            // Endereço do nó raiz no arquivo
    int nextAddress;            // Próximo endereço disponível para alocação

    // Cache para nós já lidos do disco (otimização)
    unordered_map<int, BTreeDiskNode<KeyType> > nodeCache;

    // Métodos auxiliares para manipulação de arquivo
    void writeNodeToDisk(const BTreeDiskNode<KeyType>& node);
    BTreeDiskNode<KeyType> readNodeFromDisk(int address);
    void writeHeader();
    void readHeader();

    // Métodos auxiliares para operações de árvore B
    void splitChild(BTreeDiskNode<KeyType>& parent, int index);
    void insertNonFull(BTreeDiskNode<KeyType>& node, const KeyType& key, const AddressType& dataAddress);
    int findKey(const BTreeDiskNode<KeyType>& node, const KeyType& key) const;
    bool searchKeyInNode(const BTreeDiskNode<KeyType>& node, const KeyType& key, AddressType& dataAddress) const;

public:
    BTreeDisk(const string& filePath, int btreeOrder);
    ~BTreeDisk();

    // Operações básicas
    void insert(const KeyType& key, const AddressType& dataAddress);
    bool search(const KeyType& key, AddressType& dataAddress);
    void remove(const KeyType& key); // Implementação opcional

    // Métodos para persistência
    void saveToFile();
    void loadFromFile();

    // Métodos para visualização
    void printTree() const;
    void printNode(int address, int level = 0) const;
};

// ==== IMPLEMENTAÇÃO DOS MÉTODOS ====

template <typename KeyType, typename AddressType>
BTreeDisk<KeyType, AddressType>::BTreeDisk(const string& filePath, int btreeOrder)
        : indexFilePath(filePath), order(btreeOrder), rootAddress(-1), nextAddress(0) {

    // Verifica se o arquivo de índice já existe
    ifstream fileCheck(indexFilePath, ios::binary);
    if (fileCheck.good()) {
        fileCheck.close();
        loadFromFile();
    } else {
        // Cria um nó raiz vazio
        BTreeDiskNode<KeyType> root(nextAddress++, true);
        rootAddress = root.address;

        // Persiste o nó raiz e o cabeçalho
        writeNodeToDisk(root);
        writeHeader();
    }
}

template <typename KeyType, typename AddressType>
BTreeDisk<KeyType, AddressType>::~BTreeDisk() {
    // Salva as alterações antes de destruir o objeto
    saveToFile();
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::writeHeader() {
    ofstream indexFile(indexFilePath, ios::binary | ios::out | ios::in);
    if (!indexFile) {
        indexFile.open(indexFilePath, ios::binary | ios::out);
    }

    if (!indexFile) {
        throw runtime_error("Não foi possível abrir o arquivo de índice para escrita (cabeçalho)");
    }

    // Posiciona no início do arquivo
    indexFile.seekp(0, ios::beg);

    // Escreve o cabeçalho: ordem, endereço raiz, próximo endereço disponível
    indexFile.write(reinterpret_cast<const char*>(&order), sizeof(order));
    indexFile.write(reinterpret_cast<const char*>(&rootAddress), sizeof(rootAddress));
    indexFile.write(reinterpret_cast<const char*>(&nextAddress), sizeof(nextAddress));

    indexFile.close();
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::readHeader() {
    ifstream indexFile(indexFilePath, ios::binary);
    if (!indexFile) {
        throw runtime_error("Não foi possível abrir o arquivo de índice para leitura (cabeçalho)");
    }

    // Posiciona no início do arquivo
    indexFile.seekg(0, ios::beg);

    // Lê o cabeçalho
    indexFile.read(reinterpret_cast<char*>(&order), sizeof(order));
    indexFile.read(reinterpret_cast<char*>(&rootAddress), sizeof(rootAddress));
    indexFile.read(reinterpret_cast<char*>(&nextAddress), sizeof(nextAddress));

    indexFile.close();
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::writeNodeToDisk(const BTreeDiskNode<KeyType>& node) {
    ofstream indexFile(indexFilePath, ios::binary | ios::out | ios::in);
    if (!indexFile) {
        indexFile.open(indexFilePath, ios::binary | ios::out);
    }

    if (!indexFile) {
        throw runtime_error("Não foi possível abrir o arquivo de índice para escrita (nó)");
    }

    // Calcula o tamanho do cabeçalho
    const int headerSize = sizeof(order) + sizeof(rootAddress) + sizeof(nextAddress);

    // Calcula o offset para este nó
    long offset = headerSize + node.address * sizeof(BTreeDiskNode<KeyType>);

    // Posiciona no local correto para este nó
    indexFile.seekp(offset);

    // Escreve os dados do nó
    int nodeSize = node.keys.size();
    int childrenSize = node.childAddresses.size();
    bool isLeaf = node.isLeaf;

    // Escreve os metadados do nó
    indexFile.write(reinterpret_cast<const char*>(&node.address), sizeof(node.address));
    indexFile.write(reinterpret_cast<const char*>(&node.parentAddress), sizeof(node.parentAddress));
    indexFile.write(reinterpret_cast<const char*>(&nodeSize), sizeof(nodeSize));
    indexFile.write(reinterpret_cast<const char*>(&childrenSize), sizeof(childrenSize));
    indexFile.write(reinterpret_cast<const char*>(&isLeaf), sizeof(isLeaf));

    // Escreve as chaves
    for (const KeyType& key : node.keys) {
        indexFile.write(reinterpret_cast<const char*>(&key), sizeof(KeyType));
    }

    // Escreve os endereços dos filhos
    for (int childAddr : node.childAddresses) {
        indexFile.write(reinterpret_cast<const char*>(&childAddr), sizeof(int));
    }

    indexFile.close();

    // Atualiza o cache
    nodeCache[node.address] = node;
}

template <typename KeyType, typename AddressType>
BTreeDiskNode<KeyType> BTreeDisk<KeyType, AddressType>::readNodeFromDisk(int address) {
    // Verifica se o nó está no cache
    auto it = nodeCache.find(address);
    if (it != nodeCache.end()) {
        return it->second;
    }

    ifstream indexFile(indexFilePath, ios::binary);
    if (!indexFile) {
        throw runtime_error("Não foi possível abrir o arquivo de índice para leitura (nó)");
    }

    // Calcula o tamanho do cabeçalho
    const int headerSize = sizeof(order) + sizeof(rootAddress) + sizeof(nextAddress);

    // Calcula o offset para este nó
    long offset = headerSize + address * sizeof(BTreeDiskNode<KeyType>);

    // Posiciona no local correto para este nó
    indexFile.seekg(offset);

    // Lê os metadados do nó
    BTreeDiskNode<KeyType> node;
    int nodeSize, childrenSize;

    indexFile.read(reinterpret_cast<char*>(&node.address), sizeof(node.address));
    indexFile.read(reinterpret_cast<char*>(&node.parentAddress), sizeof(node.parentAddress));
    indexFile.read(reinterpret_cast<char*>(&nodeSize), sizeof(nodeSize));
    indexFile.read(reinterpret_cast<char*>(&childrenSize), sizeof(childrenSize));
    indexFile.read(reinterpret_cast<char*>(&node.isLeaf), sizeof(node.isLeaf));

    // Lê as chaves
    node.keys.resize(nodeSize);
    for (int i = 0; i < nodeSize; i++) {
        KeyType key;
        indexFile.read(reinterpret_cast<char*>(&key), sizeof(KeyType));
        node.keys[i] = key;
    }

    // Lê os endereços dos filhos
    node.childAddresses.resize(childrenSize);
    for (int i = 0; i < childrenSize; i++) {
        int childAddr;
        indexFile.read(reinterpret_cast<char*>(&childAddr), sizeof(int));
        node.childAddresses[i] = childAddr;
    }

    indexFile.close();

    // Atualiza o cache
    nodeCache[address] = node;

    return node;
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::saveToFile() {
    // Salva o cabeçalho
    writeHeader();

    // Os nós individuais já são salvos durante as operações
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::loadFromFile() {
    // Limpa o cache
    nodeCache.clear();

    // Lê o cabeçalho
    readHeader();

    // Carrega o nó raiz para o cache
    if (rootAddress >= 0) {
        readNodeFromDisk(rootAddress);
    }
}

template <typename KeyType, typename AddressType>
int BTreeDisk<KeyType, AddressType>::findKey(const BTreeDiskNode<KeyType>& node, const KeyType& key) const {
    int idx = 0;
    while (idx < node.keys.size() && key > node.keys[idx]) {
        idx++;
    }
    return idx;
}

template <typename KeyType, typename AddressType>
bool BTreeDisk<KeyType, AddressType>::searchKeyInNode(const BTreeDiskNode<KeyType>& node, const KeyType& key, AddressType& dataAddress) const {
    int i = 0;
    while (i < node.keys.size() && key > node.keys[i]) {
        i++;
    }

    if (i < node.keys.size() && key == node.keys[i]) {
        // Chave encontrada neste nó
        dataAddress = static_cast<AddressType>(i); // Aqui você precisaria ter o mapeamento para o endereço real dos dados
        return true;
    }

    if (node.isLeaf) {
        // Se for folha e não encontrou, a chave não existe
        return false;
    }

    // Desce para o filho apropriado
    BTreeDiskNode<KeyType> childNode = readNodeFromDisk(node.childAddresses[i]);
    return searchKeyInNode(childNode, key, dataAddress);
}

template <typename KeyType, typename AddressType>
bool BTreeDisk<KeyType, AddressType>::search(const KeyType& key, AddressType& dataAddress) {
    if (rootAddress < 0) {
        return false;
    }

    BTreeDiskNode<KeyType> rootNode = readNodeFromDisk(rootAddress);
    return searchKeyInNode(rootNode, key, dataAddress);
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::splitChild(BTreeDiskNode<KeyType>& parent, int index) {
    // Implementação da divisão de nó filho cheio
    int childAddress = parent.childAddresses[index];
    BTreeDiskNode<KeyType> child = readNodeFromDisk(childAddress);

    // Cria um novo nó que receberá metade das chaves
    BTreeDiskNode<KeyType> newChild(nextAddress++, child.isLeaf);
    newChild.parentAddress = parent.address;

    // Ponto médio para divisão (depende da ordem da árvore)
    int midIndex = order - 1;

    // Move metade das chaves do filho para o novo nó
    for (int j = 0; j < order - 1; j++) {
        newChild.keys.push_back(child.keys[j + order]);
    }

    // Se não for folha, move também os ponteiros para filhos
    if (!child.isLeaf) {
        for (int j = 0; j < order; j++) {
            newChild.childAddresses.push_back(child.childAddresses[j + order]);

            // Atualiza o pai dos filhos movidos
            BTreeDiskNode<KeyType> movedChild = readNodeFromDisk(child.childAddresses[j + order]);
            movedChild.parentAddress = newChild.address;
            writeNodeToDisk(movedChild);
        }

        // Remove os filhos movidos do nó original
        child.childAddresses.resize(order);
    }

    // Pega a chave do meio que subirá para o pai
    KeyType midKey = child.keys[midIndex];

    // Remove a segunda metade das chaves do filho original
    child.keys.resize(midIndex);

    // Insere a chave do meio no pai e ajusta os ponteiros
    parent.childAddresses.insert(parent.childAddresses.begin() + index + 1, newChild.address);
    parent.keys.insert(parent.keys.begin() + index, midKey);

    // Salva as alterações em disco
    writeNodeToDisk(child);
    writeNodeToDisk(newChild);
    writeNodeToDisk(parent);
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::insertNonFull(BTreeDiskNode<KeyType>& node, const KeyType& key, const AddressType& dataAddress) {
    // Localiza a posição para inserir a nova chave
    int i = node.keys.size() - 1;

    if (node.isLeaf) {
        // Caso 1: Nó é folha, insere diretamente
        // Encontra a posição para inserir mantendo a ordem
        while (i >= 0 && key < node.keys[i]) {
            i--;
        }
        i++;

        // Insere a chave na posição correta
        node.keys.insert(node.keys.begin() + i, key);

        // Salva o nó atualizado em disco
        writeNodeToDisk(node);
    } else {
        // Caso 2: Nó não é folha, deve descer na árvore
        // Encontra o filho que deve conter a chave
        while (i >= 0 && key < node.keys[i]) {
            i--;
        }
        i++;

        // Lê o filho do disco
        int childAddress = node.childAddresses[i];
        BTreeDiskNode<KeyType> child = readNodeFromDisk(childAddress);

        // Verifica se o filho está cheio
        if (child.keys.size() == 2 * order - 1) {
            // Se o filho está cheio, divide-o
            splitChild(node, i);

            // Depois da divisão, a chave média do filho foi inserida em node
            // Precisamos determinar qual filho receberá a nova chave
            if (key > node.keys[i]) {
                i++;
                // Atualiza o ponteiro para o filho correto
                childAddress = node.childAddresses[i];
                child = readNodeFromDisk(childAddress);
            }
        }

        // Recursivamente insere no filho correto
        insertNonFull(child, key, dataAddress);
    }
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::insert(const KeyType& key, const AddressType& dataAddress) {
    // Se a raiz não existir, cria uma
    if (rootAddress < 0) {
        BTreeDiskNode<KeyType> root(nextAddress++, true);
        root.keys.push_back(key);
        rootAddress = root.address;
        writeNodeToDisk(root);
        writeHeader();
        return;
    }

    // Lê o nó raiz do disco
    BTreeDiskNode<KeyType> root = readNodeFromDisk(rootAddress);

    // Verifica se a raiz está cheia
    if (root.keys.size() == 2 * order - 1) {
        // Cria uma nova raiz
        BTreeDiskNode<KeyType> newRoot(nextAddress++, false);
        newRoot.childAddresses.push_back(rootAddress);

        // A antiga raiz se torna filha da nova raiz
        root.parentAddress = newRoot.address;
        writeNodeToDisk(root);

        // Divide a antiga raiz
        splitChild(newRoot, 0);

        // Atualiza a raiz
        rootAddress = newRoot.address;
        writeHeader();

        // Insere na nova raiz (que agora tem espaço)
        insertNonFull(newRoot, key, dataAddress);
    } else {
        // A raiz tem espaço, insere diretamente
        insertNonFull(root, key, dataAddress);
    }
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::printNode(int address, int level) const {
    BTreeDiskNode<KeyType> node = readNodeFromDisk(address);

    // Imprime indentação conforme o nível
    for (int i = 0; i < level; i++) {
        cout << "  ";
    }

    // Imprime as chaves do nó
    cout << "[";
    for (size_t i = 0; i < node.keys.size(); i++) {
        cout << node.keys[i];
        if (i < node.keys.size() - 1) {
            cout << " ";
        }
    }
    cout << "] (Addr: " << node.address << ", Parent: " << node.parentAddress
              << ", Leaf: " << (node.isLeaf ? "Yes" : "No") << ")" << endl;

    // Recursivamente imprime os filhos
    if (!node.isLeaf) {
        for (int childAddr : node.childAddresses) {
            printNode(childAddr, level + 1);
        }
    }
}

template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::printTree() const {
    if (rootAddress < 0) {
        cout << "Árvore vazia" << endl;
        return;
    }

    cout << "Estrutura da Árvore B (ordem " << order << "):" << endl;
    printNode(rootAddress, 0);
}

// A implementação do método remove é opcional e mais complexa
template <typename KeyType, typename AddressType>
void BTreeDisk<KeyType, AddressType>::remove(const KeyType& key) {
    // Implementação da remoção (opcional)
    cout << "Método de remoção ainda não implementado" << endl;
}

#endif // BTREE_DISK_H