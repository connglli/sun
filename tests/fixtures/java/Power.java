/**
 * Iterative power computation (base^exp).
 * Tests: loops, multiplication accumulation
 */
public class Power {
    public static int compute(int base, int exp) {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }

    public static void main(String[] args) {
        compute(2, 10);
    }
}
