/**
 * Binary search in a sorted array.
 * Tests: complex control flow, while loops, array access
 */
public class BinarySearch {
    public static int compute(int[] arr, int target) {
        int left = 0;
        int right = arr.length - 1;

        while (left <= right) {
            int mid = (left + right) / 2;

            if (arr[mid] == target) {
                return mid;
            } else if (arr[mid] < target) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }

        return -1;  // Not found
    }

    public static void main(String[] args) {
        int[] arr = {2, 5, 8, 12, 16, 23, 38, 45, 56, 67, 78};

        // Warm up
        for (int i = 0; i < 10000; i++) {
            compute(arr, 23);
        }

        // Test cases
        System.out.println("binarySearch(arr, 23) = " + compute(arr, 23));  // 5
        System.out.println("binarySearch(arr, 2) = " + compute(arr, 2));    // 0
        System.out.println("binarySearch(arr, 78) = " + compute(arr, 78));  // 10
        System.out.println("binarySearch(arr, 99) = " + compute(arr, 99));  // -1
    }
}
