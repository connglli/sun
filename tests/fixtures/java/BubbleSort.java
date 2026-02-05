/**
 * Bubble sort algorithm.
 * Tests: nested loops, array access, array mutations
 */
public class BubbleSort {
    public static void compute(int[] arr) {
        int n = arr.length;
        for (int i = 0; i < n - 1; i++) {
            for (int j = 0; j < n - i - 1; j++) {
                if (arr[j] > arr[j + 1]) {
                    // Swap
                    int temp = arr[j];
                    arr[j] = arr[j + 1];
                    arr[j + 1] = temp;
                }
            }
        }
    }

    public static void main(String[] args) {
        // Warm up
        for (int i = 0; i < 1000; i++) {
            int[] warmup = {5, 2, 8, 1, 9};
            compute(warmup);
        }

        // Test case
        int[] arr = {64, 34, 25, 12, 22, 11, 90};
        System.out.print("Before: ");
        for (int x : arr) System.out.print(x + " ");
        System.out.println();

        compute(arr);

        System.out.print("After: ");
        for (int x : arr) System.out.print(x + " ");
        System.out.println();
        // Expected: 11 12 22 25 34 64 90
    }
}
