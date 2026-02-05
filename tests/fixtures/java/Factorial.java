/**
 * Iterative factorial computation.
 * Tests: loops, multiplication, simple conditions
 */
public class Factorial {
    public static int compute(int n) {
        int result = 1;
        for (int i = 2; i <= n; i++) {
            result *= i;
        }
        return result;
    }

    public static void main(String[] args) {
        compute(10);
    }
}
