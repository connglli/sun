/**
 * Iterative Fibonacci computation.
 * Tests: loops, phi nodes, simple arithmetic
 */
public class Fibonacci {
    public static int compute(int n) {
        if (n <= 1) {
            return n;
        }

        int a = 0;
        int b = 1;

        for (int i = 2; i <= n; i++) {
            int tmp = a + b;
            a = b;
            b = tmp;
        }

        return b;
    }

    public static void main(String[] args) {
        // Force C2 compilation with -Xcomp flag
        compute(10);
    }
}
