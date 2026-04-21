#include <stdio.h>

void saltoSapo(int p[], int n, int delta) {
    int u[100];      // Conjunto das pedras onde o sapo parou
    int u_idx = 0;   // Índice para o conjunto u
    
    // 1. u <- {p[1]} (Em C, o primeiro índice é 0)
    u[u_idx++] = p[0];
    
    // 2. ultima_pos <- p[1]
    int ultima_pos = p[0];
    
    // 3. para i = 2 até n faça (ajustado para índices 0 a n-1 em C)
    for (int i = 1; i < n; i++) {
        // 4. se p[i] - ultima_pos > delta então
        if (p[i] - ultima_pos > delta) {
            // 5. ultima_pos <- p[i-1]
            ultima_pos = p[i - 1];
            
            // 6. u <- u U {p[i-1]}
            u[u_idx++] = p[i - 1];
            
            // Verificação de segurança: se mesmo voltando uma pedra ainda for longe demais
            if (p[i] - ultima_pos > delta) {
                printf("O sapo nao consegue alcancar a proxima pedra!\n");
                return;
            }
        }
    }
    
    // 7. u <- u U {p[n]} (Adiciona a última pedra/destino)
    // Evita duplicar se a última pedra já for a última_pos
    if (u[u_idx - 1] != p[n - 1]) {
        u[u_idx++] = p[n - 1];
    }

    // 8. retorne u (Exibindo os saltos realizados)
    printf("Caminho percorrido (pedras): ");
    for (int j = 0; j < u_idx; j++) {
        printf("%d ", u[j]);
    }
    printf("\nTotal de paradas: %d\n", u_idx);
}

int main() {
    // Exemplo: Pedras nas posições 0, 10, 25, 40, 50, 65 e delta de 20
    int pedras[] = {0, 10, 25, 40, 50, 65};
    int n = sizeof(pedras) / sizeof(pedras[0]);
    int delta = 20;

    printf("Iniciando travessia com delta = %d...\n", delta);
    saltoSapo(pedras, n, delta);

    return 0;
}