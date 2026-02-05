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
        // Warm up
        for (int i = 0; i < 10000; i++) {
            compute(48, 18);
        }

        // Test cases
        System.out.println("gcd(48, 18) = " + compute(48, 18));   // 6
        System.out.println("gcd(100, 35) = " + compute(100, 35)); // 5
        System.out.println("gcd(17, 13) = " + compute(17, 13));   // 1
        System.out.println("gcd(56, 98) = " + compute(56, 98));   // 14
    }
}
