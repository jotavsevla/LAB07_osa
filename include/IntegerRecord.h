#ifndef INTEGER_RECORD_H
#define INTEGER_RECORD_H

#include <string>
#include <cstring>

// Classe simples que representa um registro de inteiro
class IntegerRecord {
public:
    int value;

    // Método de serialização - converte o inteiro em uma sequência de bytes
    string pack() const {
        int size = sizeof(int);  // Tamanho do dado (4 bytes para um int)

        // Buffer para armazenar o tamanho + o valor
        string buffer;
        buffer.resize(sizeof(int));  // Primeiro int armazena o tamanho
        memcpy(&buffer[0], &size, sizeof(int));

        // Adiciona o valor inteiro ao buffer
        buffer.resize(buffer.size() + sizeof(int));
        memcpy(&buffer[sizeof(int)], &value, sizeof(int));

        return buffer;
    }

    // Método de desserialização - converte bytes de volta para o inteiro
    bool unpack(const string& buffer) {
        if (buffer.size() < sizeof(int) * 2) {
            return false;  // Buffer muito pequeno
        }

        // Lê o valor a partir do offset após o tamanho
        memcpy(&value, buffer.data() + sizeof(int), sizeof(int));
        return true;
    }
};

#endif // INTEGER_RECORD_H