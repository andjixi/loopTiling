#include <stdio.h>

int main() 
{
    int n = 3, m = 3, p = 3;

    int A[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int B[3][3] = {{9, 8, 7}, {6, 5, 4}, {3, 2, 1}};
    int C[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    int i, j, k;

    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            for (k = 0; k < p; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return 0;
}