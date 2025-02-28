#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include "BTree.h"

using namespace std;

// Função para testar a funcionalidade de salvar e carregar
void testBTreePersistence() {
    cout << "=== TESTE DE PERSISTÊNCIA DA ÁRVORE B ===" << endl;

    // Criando árvore e inserindo valores
    BTree<int> originalTree;
    vector<int> values = {50, 30, 70, 20, 40, 60, 80, 15, 25, 35, 45, 55, 65, 75, 85};

    cout << "Inserindo valores na árvore original..." << endl;
    for (int val : values) {
        originalTree.insert(val);
    }

    // Exibindo estrutura original
    cout << "\nEstrutura da árvore original:" << endl;
    originalTree.printTree();

    // Salvando em arquivo
    string filename = "btree_test.dat";
    cout << "\nSalvando árvore em arquivo: " << filename << endl;
    if (originalTree.saveToFile(filename)) {
        cout << "Árvore salva com sucesso!" << endl;
    } else {
        cout << "Erro ao salvar árvore em arquivo." << endl;
        return;
    }

    // Criando nova árvore e carregando do arquivo
    BTree<int> loadedTree;
    cout << "\nCarregando árvore do arquivo..." << endl;
    if (loadedTree.loadFromFile(filename)) {
        cout << "Árvore carregada com sucesso!" << endl;
    } else {
        cout << "Erro ao carregar árvore do arquivo." << endl;
        return;
    }

    // Exibindo estrutura carregada
    cout << "\nEstrutura da árvore carregada:" << endl;
    loadedTree.printTree();

    // Verificando se todas as chaves estão presentes
    cout << "\nVerificando consistência das chaves..." << endl;
    bool allKeysFound = true;
    for (int val : values) {
        if (!loadedTree.search(val)) {
            cout << "Erro: Chave " << val << " não encontrada na árvore carregada!" << endl;
            allKeysFound = false;
        }
    }

    if (allKeysFound) {
        cout << "Todas as chaves foram encontradas corretamente na árvore carregada!" << endl;
    }

    // Testando operações adicionais na árvore carregada
    cout << "\nRealizando operações adicionais na árvore carregada..." << endl;

    // Inserção
    int newValue = 42;
    cout << "Inserindo valor " << newValue << "..." << endl;
    loadedTree.insert(newValue);

    // Busca
    if (loadedTree.search(newValue)) {
        cout << "Valor " << newValue << " encontrado com sucesso!" << endl;
    } else {
        cout << "Erro: Valor " << newValue << " não encontrado após inserção!" << endl;
    }

    // Remoção
    int valueToRemove = 30;
    cout << "Removendo valor " << valueToRemove << "..." << endl;
    loadedTree.remove(valueToRemove);

    if (!loadedTree.search(valueToRemove)) {
        cout << "Valor " << valueToRemove << " removido com sucesso!" << endl;
    } else {
        cout << "Erro: Valor " << valueToRemove << " ainda presente após remoção!" << endl;
    }

    // Estrutura final
    cout << "\nEstrutura final da árvore após operações:" << endl;
    loadedTree.printTree();

    cout << "\n=== TESTE DE PERSISTÊNCIA CONCLUÍDO COM SUCESSO ===" << endl;
}

int main() {
    try {
        testBTreePersistence();
        return 0;
    } catch (const exception& e) {
        cerr << "Erro durante o teste: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "Erro desconhecido durante o teste" << endl;
        return 1;
    }
}