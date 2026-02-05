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
        // Warm up
        for (int i = 0; i < 10000; i++) {
            compute(2, 10);
        }

        // Test cases
        System.out.println("2^0 = " + compute(2, 0));   // 1
        System.out.println("2^10 = " + compute(2, 10)); // 1024
        System.out.println("3^4 = " + compute(3, 4));   // 81
        System.out.println("5^3 = " + compute(5, 3));   // 125
    }
}
