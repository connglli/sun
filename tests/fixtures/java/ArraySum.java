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
        int[] test = {1, 2, 3, 4, 5};
        compute(test);
    }
}
