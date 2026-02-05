/**
 * Matrix multiplication (2D arrays).
 * Tests: triple nested loops, 2D array access, complex indexing
 */
public class MatrixMultiply {
    public static int[][] compute(int[][] a, int[][] b) {
        int m = a.length;
        int n = b[0].length;
        int p = a[0].length;

        int[][] result = new int[m][n];

        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                int sum = 0;
                for (int k = 0; k < p; k++) {
                    sum += a[i][k] * b[k][j];
                }
                result[i][j] = sum;
            }
        }

        return result;
    }

    public static void main(String[] args) {
        int[][] a = {{1, 2}, {3, 4}};
        int[][] b = {{5, 6}, {7, 8}};
        compute(a, b);
    }
}
