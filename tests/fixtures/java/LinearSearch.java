/**
 * Linear search in an array.
 * Tests: array access, loops, early return
 */
public class LinearSearch {
    public static int compute(int[] arr, int target) {
        for (int i = 0; i < arr.length; i++) {
            if (arr[i] == target) {
                return i;
            }
        }
        return -1;  // Not found
    }

    public static void main(String[] args) {
        int[] arr = {10, 23, 45, 70, 11, 15};

        // Warm up
        for (int i = 0; i < 10000; i++) {
            compute(arr, 70);
        }

        // Test cases
        System.out.println("search(arr, 70) = " + compute(arr, 70));  // 3
        System.out.println("search(arr, 10) = " + compute(arr, 10));  // 0
        System.out.println("search(arr, 15) = " + compute(arr, 15));  // 5
        System.out.println("search(arr, 99) = " + compute(arr, 99));  // -1
    }
}
