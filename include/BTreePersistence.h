#ifndef BTREEPERSISTENCE_H
#define BTREEPERSISTENCE_H

#include "BTree.h"
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <queue>
#include <set>

template <typename T>
class BTreePersistence {
private:
    BTreeNode<T> *root;
    unordered_map<int, BTreeNode<T> *> pageCache;
    set<int> dirtyPages; // Conjunto de páginas modificadas que precisam ser salvas
    int nextPageId;
    int t; // Grau mínimo da árvore B (t=3 para ordem 3)
    string filePath; // Caminho do arquivo para persistência

public:
    // Construtor
    BTreePersistence(int degree = 3, const string &path = "btree_persistence.dat")
            : root(nullptr), nextPageId(0), t(degree), filePath(path) {

        // Verifica se o arquivo existe e tenta carregar
        ifstream testFile(filePath, ios::binary);
        if (testFile.good()) {
            testFile.close();
            load();
        } else {
            // Cria uma nova árvore vazia
            root = new BTreeNode<T>(true);
            root->n = 0;
            assignPageId(root);
            markDirty(root->pageId);
        }
    }

    // Destrutor
    ~BTreePersistence() {
        try {
            // Salva as alterações antes de destruir (opcional)
            if (!pageCache.empty()) {
                save();
            }

            // Forma correta de limpar a memória sem causar problemas
            if (root != nullptr) {
                // Ao invés de simplesmente deletar o nó raiz, usamos uma função de limpeza segura
                safeDeleteTree(root);
                root = nullptr;
            }

            // Limpa o cache sem deletar os nós (já foram deletados de forma segura)
            pageCache.clear();
        } catch(...) {
            // Em caso de erro, garantimos que não há crash
            std::cerr << "Erro na limpeza de memória." << std::endl;
        }
    }

    /**
     * @brief Insere uma chave na árvore
     * @param key Chave a ser inserida
     */
    void insert(T key) {
        // Se a raiz não existe, cria
        if (root == nullptr) {
            root = new BTreeNode<T>(true);
            root->n = 0;
            assignPageId(root);
            markDirty(root->pageId);
        }

        // Se a raiz está cheia, divide-a
        if (root->n == 2 * t - 1) {
            BTreeNode<T> *newRoot = new BTreeNode<T>(false);
            assignPageId(newRoot);
            markDirty(newRoot->pageId);

            newRoot->children[0] = root;
            root = newRoot;

            // Divide a antiga raiz
            splitChild(newRoot, 0);

            // Insere a chave no local adequado
            insertNonFull(newRoot, key);
        } else {
            // Senão, insere diretamente
            insertNonFull(root, key);
        }
    }

    /**
     * @brief Busca uma chave na árvore
     * @param key Chave a ser buscada
     * @return true se a chave foi encontrada, false caso contrário
     */
    bool search(T key) {
        if (root == nullptr) {
            return false;
        }
        return searchKey(root, key);
    }

    /**
     * @brief Remove uma chave da árvore
     * @param key Chave a ser removida
     */
    void remove(T key) {
        if (root == nullptr) {
            cout << "Árvore vazia." << endl;
            return;
        }

        // Implementa a remoção
        removeKey(root, key);

        // Se a raiz ficar vazia e não for folha, atualiza a raiz
        if (root->n == 0 && !root->leaf) {
            BTreeNode<T> *oldRoot = root;
            root = root->children[0];
            delete oldRoot;
            markDirty(root->pageId);
        }
    }

    /**
     * @brief Salva a árvore em arquivo
     */
    void save() {
        ofstream file(filePath, ios::binary | ios::trunc);
        if (!file) {
            throw runtime_error("Não foi possível abrir o arquivo para escrita");
        }

        // Salva metadados da árvore
        int rootId = (root != nullptr) ? root->pageId : -1;
        file.write(reinterpret_cast<const char *>(&t), sizeof(t));
        file.write(reinterpret_cast<const char *>(&nextPageId), sizeof(nextPageId));
        file.write(reinterpret_cast<const char *>(&rootId), sizeof(rootId));

        // Salva somente as páginas sujas
        int numDirtyPages = dirtyPages.size();
        file.write(reinterpret_cast<const char *>(&numDirtyPages), sizeof(numDirtyPages));

        for (int pageId: dirtyPages) {
            auto it = pageCache.find(pageId);
            if (it != pageCache.end()) {
                BTreeNode<T> *node = it->second;
                serializeNode(file, node);
            }
        }

        file.close();
        dirtyPages.clear();
    }

    /**
     * @brief Carrega a árvore de um arquivo
     */
    void load() {
        ifstream file(filePath, ios::binary);
        if (!file) {
            throw runtime_error("Não foi possível abrir o arquivo para leitura");
        }

        // Limpa as estruturas existentes
        for (auto &pair: pageCache) {
            delete pair.second;
        }
        pageCache.clear();
        dirtyPages.clear();
        root = nullptr;

        // Carrega metadados da árvore
        int rootId;
        file.read(reinterpret_cast<char *>(&t), sizeof(t));
        file.read(reinterpret_cast<char *>(&nextPageId), sizeof(nextPageId));
        file.read(reinterpret_cast<char *>(&rootId), sizeof(rootId));

        // Carrega as páginas
        int numPages;
        file.read(reinterpret_cast<char *>(&numPages), sizeof(numPages));

        for (int i = 0; i < numPages; i++) {
            BTreeNode<T> *node = deserializeNode(file);
            pageCache[node->pageId] = node;

            // Se for a raiz, atualiza o ponteiro
            if (node->pageId == rootId) {
                root = node;
            }
        }

        file.close();
    }

    /**
     * @brief Imprime a estrutura da árvore
     */
    void printTree() {
        if (root == nullptr) {
            cout << "Árvore vazia." << endl;
            return;
        }

        cout << "Estrutura da Árvore B Persistente:" << endl;
        printNode(root, 0, "");
        cout << endl;
    }

    /**
     * @brief Verifica a integridade da árvore
     * @return true se a árvore estiver íntegra, false caso contrário
     */
    bool verifyIntegrity() {
        if (root == nullptr) {
            return true;  // Árvore vazia é íntegra
        }

        return verifyNodeIntegrity(root, nullptr, nullptr);
    }

    /**
     * @brief Gera uma visualização em DOT (Graphviz) da árvore
     * @param outputFile Arquivo de saída para o formato DOT
     */
    void generateDotVisualization(const string &outputFile) {
        ofstream dotFile(outputFile);
        if (!dotFile) {
            throw runtime_error("Não foi possível criar o arquivo DOT");
        }

        dotFile << "digraph BTree {" << endl;
        dotFile << "  node [shape=record];" << endl;

        if (root != nullptr) {
            // Gera os nós
            queue < BTreeNode<T> * > nodeQueue;
            nodeQueue.push(root);

            while (!nodeQueue.empty()) {
                BTreeNode<T> *node = nodeQueue.front();
                nodeQueue.pop();

                // Define o label do nó
                dotFile << "  node" << node->pageId << " [label=\"";
                for (int i = 0; i < node->n; i++) {
                    dotFile << "<f" << i << "> | " << node->keys[i] << " | ";
                }
                dotFile << "<f" << node->n << ">\"];" << endl;

                // Adiciona arestas para os filhos
                if (!node->leaf) {
                    for (int i = 0; i <= node->n; i++) {
                        if (node->children[i] != nullptr) {
                            dotFile << "  node" << node->pageId << ":f" << i
                                    << " -> node" << node->children[i]->pageId << ";" << endl;
                            nodeQueue.push(node->children[i]);
                        }
                    }
                }
            }
        }

        dotFile << "}" << endl;
        dotFile.close();

        cout << "Visualização DOT gerada em: " << outputFile << endl;
    }

    /**
     * @brief Retorna estatísticas da árvore
     */
    void getStats() {
        int totalNodes = 0;
        int totalKeysInMemory = 0;
        int maxHeight = 0;

        function<void(BTreeNode<T> *, int)> calculateStats =
                [&](BTreeNode<T> *node, int height) -> void {
                    if (node == nullptr) return;

                    totalNodes++;
                    totalKeysInMemory += node->n;
                    maxHeight = max(maxHeight, height);

                    if (!node->leaf) {
                        for (int i = 0; i <= node->n; i++) {
                            if (node->children[i] != nullptr) {
                                calculateStats(node->children[i], height + 1);
                            }
                        }
                    }
                };

        calculateStats(root, 0);

        cout << "Estatísticas da Árvore B Persistente:" << endl;
        cout << "- Total de nós em memória: " << totalNodes << endl;
        cout << "- Total de nós no cache: " << pageCache.size() << endl;
        cout << "- Total de chaves em memória: " << totalKeysInMemory << endl;
        cout << "- Altura máxima: " << maxHeight << endl;
        cout << "- Próximo ID de página: " << nextPageId << endl;
        cout << "- Páginas modificadas: " << dirtyPages.size() << endl;
    }

private:
    // Adiciona ID de página a um nó
    void assignPageId(BTreeNode<T> *node) {
        if (node == nullptr) return;

        node->pageId = nextPageId++;
        pageCache[node->pageId] = node;
    }

    // Marca uma página como suja (modificada)
    void markDirty(int pageId) {
        dirtyPages.insert(pageId);
    }

    // Verifica se os filhos de um nó estão carregados
    void ensureChildrenLoaded(BTreeNode<T> *node) {
        if (node == nullptr || node->leaf) return;

        for (int i = 0; i <= node->n; i++) {
            // Aqui simulamos o carregamento, mas na implementação real
            // haveria uma verificação se o nó já está em memória
            if (node->children[i] == nullptr) {
                // Em uma implementação real, carregaria do disco
                // node->children[i] = loadFromDisk(childIds[i]);
            }
        }
    }

    // Divide um filho que está cheio
    void splitChild(BTreeNode<T> *parent, int index) {
        ensureChildrenLoaded(parent);
        BTreeNode<T> *child = parent->children[index];
        ensureChildrenLoaded(child);

        BTreeNode<T> *newChild = new BTreeNode<T>(child->leaf);
        assignPageId(newChild);
        markDirty(newChild->pageId);
        markDirty(parent->pageId);
        markDirty(child->pageId);

        // Copia as últimas (t-1) chaves do filho para o novo nó
        newChild->n = t - 1;
        for (int j = 0; j < t - 1; j++) {
            newChild->keys[j] = child->keys[j + t];
        }

        // Se não for folha, copia os últimos t filhos também
        if (!child->leaf) {
            for (int j = 0; j < t; j++) {
                newChild->children[j] = child->children[j + t];
            }
        }

        // Reduz o número de chaves no filho original
        child->n = t - 1;

        // Move os filhos para abrir espaço para o novo filho
        for (int j = parent->n; j >= index + 1; j--) {
            parent->children[j + 1] = parent->children[j];
        }

        // Adiciona o novo filho ao pai
        parent->children[index + 1] = newChild;

        // Move as chaves do pai para abrir espaço para a chave do meio do filho
        for (int j = parent->n - 1; j >= index; j--) {
            parent->keys[j + 1] = parent->keys[j];
        }

        // Copia a chave do meio do filho para o pai
        parent->keys[index] = child->keys[t - 1];
        parent->n++;
    }

    // Insere uma chave em um nó não cheio
    void insertNonFull(BTreeNode<T> *node, T key) {
        ensureChildrenLoaded(node);
        markDirty(node->pageId);

        int i = node->n - 1;

        // Se for folha, insere diretamente
        if (node->leaf) {
            // Encontra a posição correta para inserir
            while (i >= 0 && key < node->keys[i]) {
                node->keys[i + 1] = node->keys[i];
                i--;
            }

            node->keys[i + 1] = key;
            node->n++;
        } else {
            // Encontra o filho que deve receber a chave
            while (i >= 0 && key < node->keys[i]) {
                i--;
            }
            i++;

            // Verifica se o filho está cheio
            if (node->children[i]->n == 2 * t - 1) {
                splitChild(node, i);

                // Após a divisão, determina o filho correto
                if (key > node->keys[i]) {
                    i++;
                }
            }

            insertNonFull(node->children[i], key);
        }
    }

    // Busca uma chave na árvore
    bool searchKey(BTreeNode<T> *node, T key) {
        if (node == nullptr) return false;
        ensureChildrenLoaded(node);

        int i = 0;
        // Encontra a primeira chave maior ou igual a key
        while (i < node->n && key > node->keys[i]) {
            i++;
        }

        // Verifica se encontrou a chave
        if (i < node->n && key == node->keys[i]) {
            return true;
        }

        // Se for folha e não encontrou, a chave não existe
        if (node->leaf) {
            return false;
        }

        // Busca no filho apropriado
        return searchKey(node->children[i], key);
    }

    // Remove uma chave da árvore
    void removeKey(BTreeNode<T> *node, T key) {
        if (node == nullptr) return;
        ensureChildrenLoaded(node);
        markDirty(node->pageId);

        int idx = 0;
        // Encontra a posição da chave
        while (idx < node->n && node->keys[idx] < key) {
            idx++;
        }

        // Caso 1: A chave está no nó atual
        if (idx < node->n && node->keys[idx] == key) {
            // Caso 1a: Nó é folha, remove a chave
            if (node->leaf) {
                for (int i = idx + 1; i < node->n; i++) {
                    node->keys[i - 1] = node->keys[i];
                }
                node->n--;
            }
                // Caso 1b: Nó interno, situação mais complexa
            else {
                // Substitui a chave pelo predecessor
                if (node->children[idx]->n >= t) {
                    // Encontra o predecessor
                    BTreeNode<T> *pred = node->children[idx];
                    while (!pred->leaf) {
                        ensureChildrenLoaded(pred);
                        pred = pred->children[pred->n];
                    }
                    T predKey = pred->keys[pred->n - 1];

                    // Substitui a chave e remove o predecessor
                    node->keys[idx] = predKey;
                    removeKey(node->children[idx], predKey);
                }
                    // Substitui a chave pelo sucessor
                else if (node->children[idx + 1]->n >= t) {
                    // Encontra o sucessor
                    BTreeNode<T> *succ = node->children[idx + 1];
                    while (!succ->leaf) {
                        ensureChildrenLoaded(succ);
                        succ = succ->children[0];
                    }
                    T succKey = succ->keys[0];

                    // Substitui a chave e remove o sucessor
                    node->keys[idx] = succKey;
                    removeKey(node->children[idx + 1], succKey);
                }
                    // Mescla os filhos e remove
                else {
                    mergeChildren(node, idx);
                    removeKey(node->children[idx], key);
                }
            }
        }
            // Caso 2: A chave não está no nó atual
        else {
            // Se o nó é folha e não tem a chave, ela não existe
            if (node->leaf) {
                cout << "Chave " << key << " não encontrada para remoção." << endl;
                return;
            }

            // Define uma flag para caso o último filho seja processado
            bool lastChild = (idx == node->n);

            // Verifica se o filho tem pelo menos t chaves
            if (node->children[idx]->n < t) {
                // Tenta reequilibrar a árvore
                rebalanceNode(node, idx);
            }

            // Se o último filho foi mesclado, verifica índice
            if (lastChild && idx > node->n) {
                removeKey(node->children[idx - 1], key);
            } else {
                removeKey(node->children[idx], key);
            }
        }
    }

    // Mescla dois filhos de um nó
    void mergeChildren(BTreeNode<T>* parent, int idx) {
        if (parent == nullptr) return;

        BTreeNode<T>* child = parent->children[idx];
        BTreeNode<T>* sibling = parent->children[idx + 1];

        // Garantir que os nós existem
        if (child == nullptr || sibling == nullptr) {
            std::cerr << "Erro: Nó nulo durante mesclagem." << std::endl;
            return;
        }

        ensureChildrenLoaded(child);
        ensureChildrenLoaded(sibling);
        markDirty(parent->pageId);
        markDirty(child->pageId);

        // Move a chave do pai para o filho
        child->keys[child->n] = parent->keys[idx];

        // Copia as chaves do irmão para o filho
        for (int i = 0; i < sibling->n; i++) {
            child->keys[child->n + 1 + i] = sibling->keys[i];
        }

        // Se não for folha, copia os filhos também
        if (!child->leaf) {
            for (int i = 0; i <= sibling->n; i++) {
                if (sibling->children[i] != nullptr) {
                    child->children[child->n + 1 + i] = sibling->children[i];
                    // Anula o ponteiro em sibling para evitar double-free
                    sibling->children[i] = nullptr;
                }
            }
        }

        // Atualiza o número de chaves
        child->n = child->n + 1 + sibling->n;

        // Remove a chave do pai e ajusta os filhos
        for (int i = idx + 1; i < parent->n; i++) {
            parent->keys[i - 1] = parent->keys[i];
        }

        for (int i = idx + 2; i <= parent->n; i++) {
            parent->children[i - 1] = parent->children[i];
        }

        // Diminui o número de chaves do pai
        parent->n--;

        // Importante: anula o último ponteiro para evitar problemas
        parent->children[parent->n + 1] = nullptr;

        // Remove o irmão do cache antes de deletá-lo
        int siblingId = sibling->pageId;

        // Importante: marcar o nó como removido do cache
        if (pageCache.find(siblingId) != pageCache.end()) {
            pageCache.erase(siblingId);

            // Deleta com segurança
            delete sibling;
        }
    }

    // Reequilibra um nó que tem menos que t chaves
    void rebalanceNode(BTreeNode<T> *parent, int idx) {
        if (parent == nullptr) return;

        BTreeNode<T> *child = parent->children[idx];

        // Caso 1: O irmão da esquerda tem chaves extras
        if (idx > 0 && parent->children[idx - 1]->n >= t) {
            BTreeNode<T> *leftSibling = parent->children[idx - 1];
            ensureChildrenLoaded(leftSibling);
            markDirty(parent->pageId);
            markDirty(child->pageId);
            markDirty(leftSibling->pageId);

            // Abre espaço para a nova chave no filho
            for (int i = child->n - 1; i >= 0; i--) {
                child->keys[i + 1] = child->keys[i];
            }

            // Move a chave do pai para o filho
            child->keys[0] = parent->keys[idx - 1];

            // Abre espaço para o novo filho se necessário
            if (!child->leaf) {
                for (int i = child->n; i >= 0; i--) {
                    child->children[i + 1] = child->children[i];
                }
                child->children[0] = leftSibling->children[leftSibling->n];
            }

            // Atualiza a chave do pai
            parent->keys[idx - 1] = leftSibling->keys[leftSibling->n - 1];

            child->n++;
            leftSibling->n--;
        }
            // Caso 2: O irmão da direita tem chaves extras
        else if (idx < parent->n && parent->children[idx + 1]->n >= t) {
            BTreeNode<T> *rightSibling = parent->children[idx + 1];
            ensureChildrenLoaded(rightSibling);
            markDirty(parent->pageId);
            markDirty(child->pageId);
            markDirty(rightSibling->pageId);

            // Move a chave do pai para o filho
            child->keys[child->n] = parent->keys[idx];

            // Move a primeira chave do irmão para o pai
            parent->keys[idx] = rightSibling->keys[0];

            // Ajusta as chaves do irmão
            for (int i = 1; i < rightSibling->n; i++) {
                rightSibling->keys[i - 1] = rightSibling->keys[i];
            }

            // Ajusta os filhos do irmão se necessário
            if (!rightSibling->leaf) {
                child->children[child->n + 1] = rightSibling->children[0];
                for (int i = 1; i <= rightSibling->n; i++) {
                    rightSibling->children[i - 1] = rightSibling->children[i];
                }
            }

            child->n++;
            rightSibling->n--;
        }
            // Caso 3: Nenhum irmão tem chaves extras, mescla com um irmão
        else {
            if (idx < parent->n) {
                mergeChildren(parent, idx);
            } else {
                mergeChildren(parent, idx - 1);
            }
        }
    }

    // Serializa um nó para um arquivo
    void serializeNode(ofstream &file, BTreeNode<T> *node) {
        if (node == nullptr) return;

        // Escreve o ID da página e metadados
        file.write(reinterpret_cast<const char *>(&node->pageId), sizeof(node->pageId));
        file.write(reinterpret_cast<const char *>(&node->n), sizeof(node->n));
        bool isLeaf = node->leaf;
        file.write(reinterpret_cast<const char *>(&isLeaf), sizeof(isLeaf));

        // Escreve as chaves
        for (int i = 0; i < node->n; i++) {
            file.write(reinterpret_cast<const char *>(&node->keys[i]), sizeof(T));
        }

        // Escreve os IDs dos filhos
        if (!node->leaf) {
            for (int i = 0; i <= node->n; i++) {
                int childId = (node->children[i] != nullptr) ? node->children[i]->pageId : -1;
                file.write(reinterpret_cast<const char *>(&childId), sizeof(childId));
            }
        }
    }

    // Desserializa um nó de um arquivo
    BTreeNode<T> *deserializeNode(ifstream &file) {
        // Cria um novo nó
        BTreeNode<T> *node = new BTreeNode<T>();

        // Lê o ID da página e metadados
        file.read(reinterpret_cast<char *>(&node->pageId), sizeof(node->pageId));
        file.read(reinterpret_cast<char *>(&node->n), sizeof(node->n));
        bool isLeaf;
        file.read(reinterpret_cast<char *>(&isLeaf), sizeof(isLeaf));
        node->leaf = isLeaf;

        // Lê as chaves
        for (int i = 0; i < node->n; i++) {
            file.read(reinterpret_cast<char *>(&node->keys[i]), sizeof(T));
        }

        // Lê os IDs dos filhos
        if (!node->leaf) {
            for (int i = 0; i <= node->n; i++) {
                int childId;
                file.read(reinterpret_cast<char *>(&childId), sizeof(childId));

                // Os filhos serão carregados sob demanda através de lazy loading
                node->children[i] = nullptr;
                node->childIds[i] = childId;
            }
        }

        return node;
    }

    // Método auxiliar para impressão de nós
    void printNode(BTreeNode<T> *node, int level, const string &prefix) {
        if (node == nullptr) return;
        ensureChildrenLoaded(node);

        // Imprime a indentação correta
        cout << prefix;

        // Imprime o conteúdo do nó
        cout << "[";
        for (int i = 0; i < node->n; i++) {
            cout << node->keys[i];
            if (i < node->n - 1) cout << " ";
        }
        cout << "] (ID: " << node->pageId << ")" << endl;

        // Imprime os filhos recursivamente
        if (!node->leaf) {
            string childPrefix = prefix + "  ";
            for (int i = 0; i <= node->n; i++) {
                printNode(node->children[i], level + 1, childPrefix);
            }
        }
    }

    // Verifica a integridade de um nó
    // Método completo para verificação de integridade
    bool verifyNodeIntegrity(BTreeNode<T> *node, T *min, T *max) {
        if (node == nullptr) return true;
        ensureChildrenLoaded(node);

        // Verifica se o nó tem pelo menos t-1 chaves (exceto a raiz)
        if (node != root && node->n < t - 1) {
            cerr << "Erro: Nó com menos chaves que o mínimo (" << node->n << " < " << (t - 1) << ")" << endl;
            return false;
        }

        // Verifica se o nó tem no máximo 2t-1 chaves
        if (node->n > 2 * t - 1) {
            cerr << "Erro: Nó com mais chaves que o máximo (" << node->n << " > " << (2 * t - 1) << ")"
                      << endl;
            return false;
        }

        // Verifica se as chaves estão em ordem crescente
        for (int i = 1; i < node->n; i++) {
            if (node->keys[i - 1] >= node->keys[i]) {
                cerr << "Erro: Chaves não estão em ordem crescente: "
                          << node->keys[i - 1] << " >= " << node->keys[i] << endl;
                return false;
            }
        }

        // Verifica limites min/max
        if (min != nullptr && node->n > 0 && node->keys[0] <= *min) {
            cerr << "Erro: Chave " << node->keys[0] << " <= limite mínimo " << *min << endl;
            return false;
        }

        if (max != nullptr && node->n > 0 && node->keys[node->n - 1] >= *max) {
            cerr << "Erro: Chave " << node->keys[node->n - 1] << " >= limite máximo " << *max << endl;
            return false;
        }

        // Verifica recursivamente os filhos
        if (!node->leaf) {
            // Para cada filho, verifica a integridade
            for (int i = 0; i <= node->n; i++) {
                // Define os limites para o filho
                T *childMin = (i == 0) ? min : &node->keys[i - 1];
                T *childMax = (i == node->n) ? max : &node->keys[i];

                if (!verifyNodeIntegrity(node->children[i], childMin, childMax)) {
                    return false;
                }
            }
        }

        return true;
    }
    // Método auxiliar para deletar a árvore com segurança
    void safeDeleteTree(BTreeNode<T>* node) {
        if (node == nullptr) return;

        // Se não for folha, primeiro limpa todos os filhos
        if (!node->leaf) {
            for (int i = 0; i <= node->n; i++) {
                if (node->children[i] != nullptr) {
                    // Recursivamente libera a sub-árvore
                    safeDeleteTree(node->children[i]);
                    // Marca o ponteiro como nullptr para evitar uso depois de liberado
                    node->children[i] = nullptr;
                }
            }
        }

        // Remove o nó do cache antes de deletá-lo
        pageCache.erase(node->pageId);

        // Libera o nó
        delete node;
    }
// Verifica se um nó está no cache
    bool isInCache(int pageId) const {
        return pageCache.find(pageId) != pageCache.end();
    }

// Remove um nó do cache com segurança
    void removeSafelyFromCache(int pageId) {
        if (isInCache(pageId)) {
            // Removemos do cache, mas não deletamos o nó aqui
            pageCache.erase(pageId);
        }
    }

// Adiciona seguramente um nó ao cache
    void addSafelyToCache(BTreeNode<T>* node) {
        if (node == nullptr) return;

        // Verifica se já não existe um nó com este pageId
        if (isInCache(node->pageId)) {
            // Se já existe, sobrescrevemos apenas se for diferente
            if (pageCache[node->pageId] != node) {
                // Deletamos o antigo
                delete pageCache[node->pageId];
                // Adicionamos o novo
                pageCache[node->pageId] = node;
            }
        } else {
            // Se não existe, simplesmente adiciona
            pageCache[node->pageId] = node;
        }
    }

// Usado para garantir que a remoção de árvore não tente liberar ponteiros já liberados
    void clear() {
        // Não deletamos os nós aqui - isso será feito pelo método safeDeleteTree
        pageCache.clear();
        dirtyPages.clear();
        root = nullptr;
        nextPageId = 0;
    }
};

#endif //BTREEPERSISTENCE_H