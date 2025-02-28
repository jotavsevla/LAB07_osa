#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <vector>

using namespace std;
// Classe para nós em uma árvore B de ordem 3
// Na definição original:
// - Cada nó pode ter no máximo 5 chaves (6 filhos)
// - Cada nó não-folha (exceto a raiz) tem pelo menos 3 filhos (2 chaves)
// - A raiz tem pelo menos 2 filhos se não for folha
// - Todas as folhas aparecem no mesmo nível
template <typename T>
class BTreeNode {
public:
    T keys[5];         // Um nó pode conter até 5 chaves
    BTreeNode* children[6]; // Um nó pode ter até 6 filhos
    int n;             // Número atual de chaves
    bool leaf;         // Verdadeiro se o nó for folha

    // Adicionado para persistência
    int pageId;        // ID da página do nó no armazenamento
    int childIds[6];   // IDs dos filhos (para lazy loading)

    // Construtor
    BTreeNode(bool isLeaf = true) : n(0), leaf(isLeaf), pageId(-1) {
        // Inicializa todos os ponteiros de filhos como nullptr
        for (int i = 0; i < 6; i++) {
            children[i] = nullptr;
            childIds[i] = -1; // Inicializa IDs de filho como inválidos
        }
    }

    // Destrutor para limpar a memória recursivamente
    ~BTreeNode() {
        if (!leaf) {
            for (int i = 0; i <= n; i++) {
                if (children[i] != nullptr) {
                    delete children[i];
                }
            }
        }
    }
};

// Classe para árvore B
template <typename T>
class BTree {
private:
    BTreeNode<T>* root; // Ponteiro para raiz da árvore
    int t;              // Grau mínimo (para ordem 3, t=3)

    // Função para dividir um filho cheio durante inserção
    void splitChild(BTreeNode<T>* x, int i) {
        // Cria um novo nó que vai receber metade das chaves de y
        BTreeNode<T>* y = x->children[i];
        BTreeNode<T>* z = new BTreeNode<T>(y->leaf);

        // Define z para receber t-1 (2) chaves de y
        z->n = t - 1;

        // Copia as últimas (t-1) chaves de y para z
        for (int j = 0; j < t - 1; j++) {
            z->keys[j] = y->keys[j + t];
        }

        // Se y não for folha, copia os últimos t filhos de y para z
        if (!y->leaf) {
            for (int j = 0; j < t; j++) {
                z->children[j] = y->children[j + t];
                y->children[j + t] = nullptr;  // Evita double-free
            }
        }

        // Reduz o número de chaves em y
        y->n = t - 1;

        // Cria espaço para o novo filho em x
        for (int j = x->n; j >= i + 1; j--) {
            x->children[j + 1] = x->children[j];
        }

        // Conecta o novo filho em x
        x->children[i + 1] = z;

        // Move uma chave de y para x
        for (int j = x->n - 1; j >= i; j--) {
            x->keys[j + 1] = x->keys[j];
        }
        x->keys[i] = y->keys[t - 1];

        // Incrementa o número de chaves em x
        x->n = x->n + 1;
    }

    // Função para inserir em um nó não-cheio
    void insertNonFull(BTreeNode<T>* x, T k) {
        // Inicializa o índice como o último elemento
        int i = x->n - 1;

        // Se for folha, insere a chave diretamente
        if (x->leaf) {
            // Encontra a posição para inserir a nova chave
            while (i >= 0 && k < x->keys[i]) {
                x->keys[i + 1] = x->keys[i];
                i--;
            }

            // Insere a nova chave na posição encontrada
            x->keys[i + 1] = k;
            x->n = x->n + 1;
        } else {
            // Encontra o filho que deve conter a chave
            while (i >= 0 && k < x->keys[i]) {
                i--;
            }
            i++;

            // Verifica se o filho está cheio
            if (x->children[i]->n == 2 * t - 1) {
                splitChild(x, i);

                // Após a divisão, a chave do meio sobe
                // e o filho se divide em dois
                if (k > x->keys[i]) {
                    i++;
                }
            }
            insertNonFull(x->children[i], k);
        }
    }

    // Função para percorrer a árvore (inorder)
    void traverse(BTreeNode<T>* x) {
        int i;
        for (i = 0; i < x->n; i++) {
            // Primeiro visita o filho da esquerda
            if (!x->leaf) {
                traverse(x->children[i]);
            }
            // Depois imprime a chave atual
            cout << " " << x->keys[i];
        }

        // Visita o último filho
        if (!x->leaf) {
            traverse(x->children[i]);
        }
    }

    // Função para buscar uma chave na árvore
    BTreeNode<T>* search(BTreeNode<T>* x, T k) {
        // Encontra a primeira chave maior ou igual a k
        int i = 0;
        while (i < x->n && k > x->keys[i]) {
            i++;
        }

        // Se a chave foi encontrada, retorna o nó
        if (i < x->n && k == x->keys[i]) {
            return x;
        }

        // Se for folha e não encontrou, então não existe
        if (x->leaf) {
            return nullptr;
        }

        // Senão, busca no filho apropriado
        return search(x->children[i], k);
    }

    // Função para obter o predecessor
    T getPredecessor(BTreeNode<T>* node, int idx) {
        // Continua movendo para o filho mais à direita
        // até chegar a uma folha
        BTreeNode<T>* current = node->children[idx];
        while (!current->leaf) {
            current = current->children[current->n];
        }
        // Retorna a última chave
        return current->keys[current->n - 1];
    }

    // Função para obter o sucessor
    T getSuccessor(BTreeNode<T>* node, int idx) {
        // Continua movendo para o filho mais à esquerda
        // até chegar a uma folha
        BTreeNode<T>* current = node->children[idx + 1];
        while (!current->leaf) {
            current = current->children[0];
        }
        // Retorna a primeira chave
        return current->keys[0];
    }

    // Função para preencher o nó filho que tem menos que o mínimo de chaves
    void fill(BTreeNode<T>* node, int idx) {
        // Se o filho anterior tem chaves extras
        if (idx != 0 && node->children[idx - 1]->n >= t) {
            borrowFromPrev(node, idx);
        }
            // Se o próximo filho tem chaves extras
        else if (idx != node->n && node->children[idx + 1]->n >= t) {
            borrowFromNext(node, idx);
        }
            // Senão, mescla com um irmão
        else {
            if (idx != node->n) {
                merge(node, idx);
            } else {
                merge(node, idx - 1);
            }
        }
    }

    // Função para pegar emprestado do irmão anterior
    void borrowFromPrev(BTreeNode<T>* node, int idx) {
        BTreeNode<T>* child = node->children[idx];
        BTreeNode<T>* sibling = node->children[idx - 1];

        // Desloca todas as chaves em child uma posição para frente
        for (int i = child->n - 1; i >= 0; --i) {
            child->keys[i + 1] = child->keys[i];
        }

        // Se child não for folha, desloca todos os seus filhos
        if (!child->leaf) {
            for (int i = child->n; i >= 0; --i) {
                child->children[i + 1] = child->children[i];
            }
        }

        // Define a primeira chave de child como a chave idx-1 do nó
        child->keys[0] = node->keys[idx - 1];

        // Move o último filho de sibling para child
        if (!child->leaf) {
            child->children[0] = sibling->children[sibling->n];
        }

        // Move a última chave de sibling para o nó
        node->keys[idx - 1] = sibling->keys[sibling->n - 1];

        // Atualiza contagem de chaves
        child->n++;
        sibling->n--;
    }

    // Função para pegar emprestado do irmão seguinte
    void borrowFromNext(BTreeNode<T>* node, int idx) {
        BTreeNode<T>* child = node->children[idx];
        BTreeNode<T>* sibling = node->children[idx + 1];

        // A chave idx do nó vai para child
        child->keys[child->n] = node->keys[idx];

        // O primeiro filho de sibling vai para o último filho de child
        if (!child->leaf) {
            child->children[child->n + 1] = sibling->children[0];
        }

        // A primeira chave de sibling vai para o nó
        node->keys[idx] = sibling->keys[0];

        // Desloca todas as chaves em sibling uma posição para trás
        for (int i = 1; i < sibling->n; ++i) {
            sibling->keys[i - 1] = sibling->keys[i];
        }

        // Desloca os filhos de sibling
        if (!sibling->leaf) {
            for (int i = 1; i <= sibling->n; ++i) {
                sibling->children[i - 1] = sibling->children[i];
            }
        }

        // Atualiza contagens
        child->n++;
        sibling->n--;
    }

    // Função para mesclar nós
    void merge(BTreeNode<T>* node, int idx) {
        BTreeNode<T>* child = node->children[idx];
        BTreeNode<T>* sibling = node->children[idx + 1];

        // Insere a chave do nó em child
        child->keys[t - 1] = node->keys[idx];

        // Copia todas as chaves de sibling para child
        for (int i = 0; i < sibling->n; ++i) {
            child->keys[i + t] = sibling->keys[i];
        }

        // Copia os filhos de sibling para child
        if (!child->leaf) {
            for (int i = 0; i <= sibling->n; ++i) {
                child->children[i + t] = sibling->children[i];
                sibling->children[i] = nullptr;  // Evita double-free
            }
        }

        // Move todas as chaves após idx no nó uma posição para trás
        for (int i = idx + 1; i < node->n; ++i) {
            node->keys[i - 1] = node->keys[i];
        }

        // Move os ponteiros de filhos uma posição para trás
        for (int i = idx + 2; i <= node->n; ++i) {
            node->children[i - 1] = node->children[i];
        }

        // Atualiza contagens
        child->n += sibling->n + 1;
        node->n--;

        // Deleta sibling
        delete sibling;
    }

    // Função para remover um nó não-folha
    void removeFromNonLeaf(BTreeNode<T>* node, int idx) {
        T k = node->keys[idx];

        // Caso 3a: Se o filho que precede k tem pelo menos t chaves
        if (node->children[idx]->n >= t) {
            // Encontrar o predecessor de k
            T pred = getPredecessor(node, idx);
            node->keys[idx] = pred;
            remove(node->children[idx], pred);
        }
            // Caso 3b: Se o filho após k tem pelo menos t chaves
        else if (node->children[idx + 1]->n >= t) {
            // Encontrar o sucessor de k
            T succ = getSuccessor(node, idx);
            node->keys[idx] = succ;
            remove(node->children[idx + 1], succ);
        }
            // Caso 3c: Ambos os filhos têm menos de t chaves
        else {
            merge(node, idx);
            remove(node->children[idx], k);
        }
    }

    // Função para remover de um nó folha
    void removeFromLeaf(BTreeNode<T>* node, int idx) {
        // Desloca todas as chaves após idx
        for (int i = idx + 1; i < node->n; ++i) {
            node->keys[i - 1] = node->keys[i];
        }

        // Reduz o número de chaves
        node->n--;
    }

    // Função principal para remover uma chave da árvore
    void remove(BTreeNode<T>* node, T k) {
        int idx = 0;
        // Encontra o índice da chave a ser removida
        while (idx < node->n && node->keys[idx] < k) {
            ++idx;
        }

        // A chave está presente neste nó
        if (idx < node->n && node->keys[idx] == k) {
            // Caso 1: Se o nó é folha
            if (node->leaf) {
                removeFromLeaf(node, idx);
            }
                // Caso 2: Se o nó não é folha
            else {
                removeFromNonLeaf(node, idx);
            }
        } else {
            // Se este nó é folha, então a chave não existe na árvore
            if (node->leaf) {
                cout << "A chave " << k << " não está presente na árvore\n";
                return;
            }

            // A chave a ser removida está no subtree enraizado no último filho
            bool flag = (idx == node->n);

            // Se o filho tem menos que t chaves, preenchê-lo
            if (node->children[idx]->n < t) {
                fill(node, idx);
            }

            // Se o último filho foi mesclado
            if (flag && idx > node->n) {
                remove(node->children[idx - 1], k);
            } else {
                remove(node->children[idx], k);
            }
        }
    }

public:
    // Construtor
    BTree() : root(new BTreeNode<T>(true)), t(3) {}  // t=3 para árvore de ordem 3

    // Destrutor
    ~BTree() {
        if (root != nullptr) {
            delete root;
        }
    }

    // Função para percorrer a árvore
    void traverse() {
        if (root != nullptr) {
            traverse(root);
            cout << endl;
        }
    }
    // Retorna o ponteiro para o nó raiz (necessário para o gerenciador de arquivos)
    BTreeNode<T>* getRoot() const {
        return root;
    }

    // Função para buscar uma chave
    BTreeNode<T>* search(T k) {
        return (root == nullptr) ? nullptr : search(root, k);
    }

    // Função para inserir uma chave
    void insert(T k) {
        // Se a raiz estiver cheia, a árvore cresce em altura
        if (root->n == 2 * t - 1) {
            // Aloca nova raiz
            BTreeNode<T>* s = new BTreeNode<T>(false);

            // Faz a antiga raiz ser filha da nova
            s->children[0] = root;

            // Divide a antiga raiz e move uma chave para a nova raiz
            splitChild(s, 0);

            // Nova raiz tem dois filhos. Decide qual vai ter a nova chave
            int i = 0;
            if (s->keys[0] < k) {
                i++;
            }
            insertNonFull(s->children[i], k);

            // Troca a raiz
            root = s;
        } else {
            // Se a raiz não estiver cheia, insere diretamente
            insertNonFull(root, k);
        }
    }

    // Função para remover uma chave
    void remove(T k) {
        if (!root) {
            cout << "A árvore está vazia\n";
            return;
        }

        // Chama a função para remover
        remove(root, k);

        // Se a raiz ficar sem chaves
        if (root->n == 0) {
            BTreeNode<T>* tmp = root;
            if (root->leaf) {
                root = nullptr;
            } else {
                root = root->children[0];
            }

            // Libera a antiga raiz
            tmp->children[0] = nullptr;  // Evita double-free
            delete tmp;
        }
    }

    // Verifica se a árvore está vazia
    bool isEmpty() const {
        return root == nullptr || root->n == 0;
    }

    // Limpa toda a árvore
    void clear() {
        if (root != nullptr) {
            delete root;
            root = new BTreeNode<T>(true);
        }
    }

    // Imprime a árvore em níveis (útil para depuração)
    void printTree() {
        if (isEmpty()) {
            cout << "Árvore vazia." << endl;
            return;
        }

        cout << "Estrutura da árvore B:" << endl;
        printLevel(root, 0);
        cout << endl;
    }
    /**
 * Salva a árvore B em um arquivo
 * @param filename Nome do arquivo para salvar
 * @return true se salvou com sucesso, false caso contrário
 */
    bool saveToFile(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary | std::ios::trunc);
        if (!file) {
            std::cerr << "Erro ao abrir arquivo para escrita: " << filename << std::endl;
            return false;
        }

        // Salva informações da árvore (grau mínimo)
        file.write(reinterpret_cast<const char*>(&t), sizeof(t));

        // Realiza um percurso em largura para salvar todos os nós
        if (root != nullptr) {
            std::queue<BTreeNode<T>*> nodeQueue;
            nodeQueue.push(root);

            // Contador para atribuir IDs temporários aos nós
            int nodeId = 0;
            std::unordered_map<BTreeNode<T>*, int> nodeIds;

            // Primeiro passo: atribui IDs a todos os nós
            std::queue<BTreeNode<T>*> idQueue;
            idQueue.push(root);

            while (!idQueue.empty()) {
                BTreeNode<T>* node = idQueue.front();
                idQueue.pop();

                nodeIds[node] = nodeId++;

                if (!node->leaf) {
                    for (int i = 0; i <= node->n; i++) {
                        if (node->children[i] != nullptr) {
                            idQueue.push(node->children[i]);
                        }
                    }
                }
            }

            // Segundo passo: escreve o número total de nós
            int totalNodes = nodeIds.size();
            file.write(reinterpret_cast<const char*>(&totalNodes), sizeof(totalNodes));

            // Terceiro passo: salva cada nó
            while (!nodeQueue.empty()) {
                BTreeNode<T>* node = nodeQueue.front();
                nodeQueue.pop();

                // Escreve ID do nó
                int id = nodeIds[node];
                file.write(reinterpret_cast<const char*>(&id), sizeof(id));

                // Escreve se é folha
                bool isLeaf = node->leaf;
                file.write(reinterpret_cast<const char*>(&isLeaf), sizeof(isLeaf));

                // Escreve número de chaves
                file.write(reinterpret_cast<const char*>(&node->n), sizeof(node->n));

                // Escreve as chaves
                for (int i = 0; i < node->n; i++) {
                    file.write(reinterpret_cast<const char*>(&node->keys[i]), sizeof(T));
                }

                // Escreve IDs dos filhos (se não for folha)
                if (!node->leaf) {
                    for (int i = 0; i <= node->n; i++) {
                        int childId = (node->children[i] != nullptr) ?
                                      nodeIds[node->children[i]] : -1;
                        file.write(reinterpret_cast<const char*>(&childId), sizeof(childId));

                        if (node->children[i] != nullptr) {
                            nodeQueue.push(node->children[i]);
                        }
                    }
                }
            }
        } else {
            // Árvore vazia - escreve 0 nós
            int totalNodes = 0;
            file.write(reinterpret_cast<const char*>(&totalNodes), sizeof(totalNodes));
        }

        file.close();
        return true;
    }

/**
 * Carrega a árvore B de um arquivo
 * @param filename Nome do arquivo para carregar
 * @return true se carregou com sucesso, false caso contrário
 */
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Erro ao abrir arquivo para leitura: " << filename << std::endl;
            return false;
        }

        // Limpa a árvore atual
        clear();

        // Lê o grau mínimo
        file.read(reinterpret_cast<char*>(&t), sizeof(t));

        // Lê número total de nós
        int totalNodes;
        file.read(reinterpret_cast<char*>(&totalNodes), sizeof(totalNodes));

        if (totalNodes == 0) {
            // Árvore vazia
            file.close();
            return true;
        }

        // Cria todos os nós primeiro
        std::vector<BTreeNode<T>*> nodes(totalNodes);
        std::vector<std::vector<int>> childIds(totalNodes);

        for (int i = 0; i < totalNodes; i++) {
            // Lê ID do nó
            int id;
            file.read(reinterpret_cast<char*>(&id), sizeof(id));

            // Lê se é folha
            bool isLeaf;
            file.read(reinterpret_cast<char*>(&isLeaf), sizeof(isLeaf));

            // Cria o nó
            nodes[id] = new BTreeNode<T>(isLeaf);

            // Lê número de chaves
            file.read(reinterpret_cast<char*>(&nodes[id]->n), sizeof(nodes[id]->n));

            // Lê as chaves
            for (int j = 0; j < nodes[id]->n; j++) {
                file.read(reinterpret_cast<char*>(&nodes[id]->keys[j]), sizeof(T));
            }

            // Lê IDs dos filhos (se não for folha)
            if (!isLeaf) {
                childIds[id].resize(nodes[id]->n + 1);
                for (int j = 0; j <= nodes[id]->n; j++) {
                    int childId;
                    file.read(reinterpret_cast<char*>(&childId), sizeof(childId));
                    childIds[id][j] = childId;
                }
            }
        }

        // Conecta os nós (estabelece relações pai-filho)
        for (int i = 0; i < totalNodes; i++) {
            if (!nodes[i]->leaf) {
                for (int j = 0; j <= nodes[i]->n; j++) {
                    int childId = childIds[i][j];
                    if (childId != -1) {
                        nodes[i]->children[j] = nodes[childId];
                    } else {
                        nodes[i]->children[j] = nullptr;
                    }
                }
            }
        }

        // Atribui a raiz
        root = nodes[0];

        file.close();
        return true;
    }

private:
    // Função auxiliar para imprimir a árvore em níveis
    void printLevel(BTreeNode<T>* node, int level) {
        if (node == nullptr) return;

        cout << "Nível " << level << ": ";
        for (int i = 0; i < node->n; i++) {
            cout << node->keys[i] << " ";
        }
        cout << endl;

        if (!node->leaf) {
            for (int i = 0; i <= node->n; i++) {
                printLevel(node->children[i], level + 1);
            }
        }
    }


};


#endif // BTREE_H