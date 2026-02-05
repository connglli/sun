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
        // Warm up
        for (int i = 0; i < 10000; i++) {
            compute(10);
        }

        // Test cases
        System.out.println("fib(0) = " + compute(0));   // 0
        System.out.println("fib(1) = " + compute(1));   // 1
        System.out.println("fib(5) = " + compute(5));   // 5
        System.out.println("fib(10) = " + compute(10)); // 55
        System.out.println("fib(15) = " + compute(15)); // 610
    }
}
