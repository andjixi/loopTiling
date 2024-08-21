#include <stdio.h>

int main() 
{
    int n = 3, m = 3;

    int A[3] = {1, 2, 3};
    int B[3] = {2, 3, 4};
    int C[3][3];

    int i, j;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            C[i][j] = A[i] * B[j];
        }
    }

    return 0;
}