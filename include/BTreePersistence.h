//
// Created by João on 27/02/25.
//
#include "BTree.h"
using namespace std;
#ifndef LAB07_OSA_BTREEPERSISTENCE_H
#define LAB07_OSA_BTREEPERSISTENCE_H
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
 * @brief Remove uma chave da árvore
 * @param k Chave a ser removida
 *
 * Nota: Esta implementação não está completa e é fornecida como referência.
 * Para uma implementação completa, seria necessário adaptar os métodos de
 * remoção da classe BTree original para trabalhar com persistência.
 */
void remove(T k) {
    cout << "Método de remoção não implementado completamente na versão persistente." << endl;
    // A implementação de remoção seria análoga à da classe BTree original,
    // mas com a adição de chamadas a ensureChildrenLoaded e marcação de
    // nós modificados como "sujos" para posterior salvamento.
}

/**
 * @brief Retorna estatísticas da árvore
 */
void getStats() {
    int totalNodes = 0;
    int totalKeysInMemory = 0;
    int maxHeight = 0;

    // Função recursiva para calcular estatísticas
    function<void(PersistentBTreeNode<T>*, int)> calculateStats;
    calculateStats = [&](PersistentBTreeNode<T>* node, int height) {
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
}
};

#endif //LAB07_OSA_BTREEPERSISTENCE_H
