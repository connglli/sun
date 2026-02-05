/**
 * Sum all elements in an array.
 * Tests: array allocation, array access, loops with arrays
 */
public class ArraySum {
    public static int compute(int[] arr) {
        int sum = 0;
        for (int i = 0; i < arr.length; i++) {
            sum += arr[i];
        }
        return sum;
    }

    public static void main(String[] args) {
        int[] test1 = {1, 2, 3, 4, 5};
        int[] test2 = {10, 20, 30, 40, 50};
        int[] test3 = {-5, 10, -3, 8, 2};

        // Warm up
        for (int i = 0; i < 10000; i++) {
            compute(test1);
        }

        // Test cases
        System.out.println("sum([1,2,3,4,5]) = " + compute(test1));      // 15
        System.out.println("sum([10,20,30,40,50]) = " + compute(test2)); // 150
        System.out.println("sum([-5,10,-3,8,2]) = " + compute(test3));   // 12
    }
}
