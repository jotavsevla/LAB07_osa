# LAB07 OSA — segunda iteração da Árvore B

Repositório acadêmico em C++11 com uma implementação de Árvore B para pares `chave → endereço`. O conteúdo atual é praticamente a mesma linha de implementação do `LAB06_osa`; ele **não contém o motor de busca** que algumas descrições antigas atribuíam ao LAB07.

## Estado da implementação

**Snapshot educacional, ainda não seguro para uso como índice.** Há defeitos conhecidos de manipulação de vetores e propriedade de nós que podem causar acesso fora dos limites, ponteiros pendentes e dupla liberação.

## Operações e complexidade pretendida

| Operação | Complexidade |
| --- | ---: |
| busca | `O(log_t n)` |
| inserção | `O(log_t n)` |
| remoção | `O(log_t n)` |
| split/merge local | `O(t)` |
| memória total | `O(n)` |

Esses limites pressupõem que as invariantes da Árvore B sejam mantidas: ocupação mínima/máxima, filhos ordenados e todas as folhas na mesma profundidade.

## Auditoria técnica

- `splitChild` acessa a chave mediana depois de reduzir o vetor que a continha.
- O destrutor de `Node` apaga filhos recursivamente e o destrutor de `BTree` também faz uma travessia recursiva: ambos podem liberar os mesmos nós.
- `merge` move ponteiros de filhos, mas apaga o irmão ainda se considerando dono deles.
- a promoção de um filho para nova raiz é seguida da exclusão do antigo pai, cujo destrutor pode apagar a raiz promovida.
- operações de empréstimo escrevem em índices ainda inexistentes do `vector`.
- não há testes automatizados de invariantes, nem comparação com uma implementação de referência.

Por isso, a complexidade assintótica descreve o **algoritmo desejado**, não uma garantia da versão atual.

A execução do exemplo com AddressSanitizer reproduz um `container-overflow` em `splitChild` durante as inserções.

## Compilar

```bash
make
./bin/btree
make clean
```

Para reproduzir falhas de memória:

```bash
make clean
make CXXFLAGS="-Wall -Wextra -std=c++11 -Iinclude -fsanitize=address,undefined"
./bin/btree
```

## Próximos passos

1. definir propriedade exclusiva com `std::unique_ptr`;
2. corrigir a captura da mediana e todos os redimensionamentos;
3. escrever um verificador de invariantes;
4. gerar sequências aleatórias de operações e comparar com `std::map`;
5. decidir se LAB06 e LAB07 representam versões distintas; se não, consolidar ou arquivar uma delas.

## Relação com os outros módulos

O motor de busca e o índice invertido estão atualmente no repositório [`LAB05_osa`](https://github.com/jotavsevla/LAB05_osa). O mapa correto dos módulos está em [OSA-projects](https://github.com/jotavsevla/OSA-projects).
