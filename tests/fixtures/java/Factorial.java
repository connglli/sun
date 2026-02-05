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
        // Warm up
        for (int i = 0; i < 10000; i++) {
            compute(5);
        }

        // Test cases
        System.out.println("0! = " + compute(0));   // 1
        System.out.println("1! = " + compute(1));   // 1
        System.out.println("5! = " + compute(5));   // 120
        System.out.println("10! = " + compute(10)); // 3628800
    }
}
