#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include "BTree.h"
#include "BTreePersistence.h"

using namespace std;

class BTreeTester {
private:
    BTreePersistence<int>* tree;
    vector<int> insertedValues;

public:
    BTreeTester(int degree = 3, const string& filename = "btree_test.dat") {
        tree = new BTreePersistence<int>(degree, filename);
    }

    ~BTreeTester() {
        if (tree) {
            delete tree;
        }
    }

    // Gera valores aleatórios únicos para inserção
    void generateRandomValues(int count, int minValue = 1, int maxValue = 10000) {
        insertedValues.clear();

        // Configura o gerador de números aleatórios
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> distrib(minValue, maxValue);

        // Gera valores únicos
        while (insertedValues.size() < count) {
            int value = distrib(gen);
            if (find(insertedValues.begin(), insertedValues.end(), value) == insertedValues.end()) {
                insertedValues.push_back(value);
            }
        }
    }

    // Insere os valores gerados na árvore
    void insertValues() {
        cout << "Inserindo " << insertedValues.size() << " valores..." << endl;

        auto start = chrono::high_resolution_clock::now();

        for (int value : insertedValues) {
            tree->insert(value);
        }

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duration = end - start;

        cout << "Inserção concluída em " << duration.count() << " ms" << endl;
    }

    // Verifica se todos os valores inseridos podem ser encontrados
    bool verifySearch() {
        cout << "Verificando busca..." << endl;

        auto start = chrono::high_resolution_clock::now();

        bool allFound = true;
        for (int value : insertedValues) {
            if (!tree->search(value)) {
                cerr << "Erro: Valor " << value << " não encontrado!" << endl;
                allFound = false;
                break;
            }
        }

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duration = end - start;

        cout << "Busca " << (allFound ? "bem-sucedida" : "falhou") << " em " << duration.count() << " ms" << endl;
        return allFound;
    }

    // Testa a remoção de alguns valores
    bool testRemoval(int count) {
        if (count > insertedValues.size()) {
            count = insertedValues.size();
        }

        cout << "Testando remoção de " << count << " valores..." << endl;

        // Cria uma cópia dos primeiros 'count' valores para remoção
        vector<int> valuesToRemove(insertedValues.begin(), insertedValues.begin() + count);

        auto start = chrono::high_resolution_clock::now();

        for (int value : valuesToRemove) {
            tree->remove(value);
        }

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duration = end - start;

        // Verifica se os valores removidos não estão mais na árvore
        bool allRemoved = true;
        for (int value : valuesToRemove) {
            if (tree->search(value)) {
                cerr << "Erro: Valor " << value << " ainda está na árvore após remoção!" << endl;
                allRemoved = false;
                break;
            }
        }

        cout << "Remoção " << (allRemoved ? "bem-sucedida" : "falhou") << " em " << duration.count() << " ms" << endl;

        // Atualiza a lista de valores inseridos
        insertedValues.erase(insertedValues.begin(), insertedValues.begin() + count);

        return allRemoved;
    }

    // Verifica a integridade da árvore
    bool checkIntegrity() {
        cout << "Verificando integridade da árvore..." << endl;
        bool isValid = tree->verifyIntegrity();
        cout << "Integridade " << (isValid ? "válida" : "inválida") << endl;
        return isValid;
    }

    // Obtém estatísticas da árvore
    void showStats() {
        cout << "\nEstatísticas da árvore:" << endl;
        tree->getStats();
    }

    // Imprime a estrutura da árvore
    void printTree() {
        cout << "\nEstrutura da árvore:" << endl;
        tree->printTree();
    }

    // Salva a árvore em um formato visual
    void visualizeTree(const string& filename = "btree_visual.dot") {
        cout << "Gerando visualização da árvore..." << endl;
        tree->generateDotVisualization(filename);
    }

    // Executa uma bateria completa de testes
    void runFullTest(int insertCount = 1000, int removeCount = 100) {
        cout << "=== Iniciando teste completo da BTreePersistence ===" << endl;

        // Gera valores aleatórios
        generateRandomValues(insertCount);

        // Insere valores
        insertValues();

        // Verifica busca
        if (!verifySearch()) {
            cerr << "Teste de busca falhou!" << endl;
            return;
        }

        // Verifica integridade
        if (!checkIntegrity()) {
            cerr << "Verificação de integridade falhou!" << endl;
            return;
        }

        // Mostra estatísticas
        showStats();

        // Testa remoção
        if (!testRemoval(removeCount)) {
            cerr << "Teste de remoção falhou!" << endl;
            return;
        }

        // Verifica integridade novamente
        if (!checkIntegrity()) {
            cerr << "Verificação de integridade após remoção falhou!" << endl;
            return;
        }

        // Mostra estatísticas novamente
        showStats();

        // Gera visualização se o número de nós for razoável
        if (insertCount - removeCount < 100) {
            visualizeTree();
        }

        cout << "=== Teste completo concluído com sucesso ===" << endl;
    }
};

#endif // BTREE_TESTER_H