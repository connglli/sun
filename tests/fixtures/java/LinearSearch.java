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
        compute(arr, 70);
    }
}
