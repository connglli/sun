/**
 * Euclidean algorithm for GCD.
 * Tests: while loops, modulo operation, complex control flow
 */
public class GCD {
    public static int compute(int a, int b) {
        while (b != 0) {
            int tmp = b;
            b = a % b;
            a = tmp;
        }
        return a;
    }

    public static void main(String[] args) {
        compute(48, 18);
    }
}
