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

        // Warm up
        for (int i = 0; i < 1000; i++) {
            compute(a, b);
        }

        // Test case
        int[][] result = compute(a, b);
        System.out.println("Result:");
        for (int i = 0; i < result.length; i++) {
            for (int j = 0; j < result[0].length; j++) {
                System.out.print(result[i][j] + " ");
            }
            System.out.println();
        }
        // Expected:
        // 19 22
        // 43 50
    }
}
