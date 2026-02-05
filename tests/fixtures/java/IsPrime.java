/**
 * Primality test.
 * Tests: loops with square root bound, early return, modulo
 */
public class IsPrime {
    public static boolean compute(int n) {
        if (n <= 1) {
            return false;
        }
        if (n <= 3) {
            return true;
        }
        if (n % 2 == 0 || n % 3 == 0) {
            return false;
        }

        // Check divisors up to sqrt(n)
        for (int i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) {
                return false;
            }
        }

        return true;
    }

    public static void main(String[] args) {
        // Warm up
        for (int i = 0; i < 10000; i++) {
            compute(97);
        }

        // Test cases
        System.out.println("isPrime(2) = " + compute(2));     // true
        System.out.println("isPrime(17) = " + compute(17));   // true
        System.out.println("isPrime(97) = " + compute(97));   // true
        System.out.println("isPrime(100) = " + compute(100)); // false
        System.out.println("isPrime(4) = " + compute(4));     // false
    }
}
