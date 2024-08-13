#include <stdio.h>

int main() 
{
    int n = 3;

    int A[3] = {1, 2, 3};
    int B[3] = {2, 3, 4};
    int C[3][3];

    int i, j;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = A[i] * B[j];
        }
    }

    // for (i = 0; i < 3; i++) {
    //     for (j = 0; j < 3; j++) {
    //         printf("%d ", C[i][j]);
    //     }
    //     printf("\n");
    // }

    return 0;
}