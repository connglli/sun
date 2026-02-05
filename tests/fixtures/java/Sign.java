/**
 * Sign function (loop-free).
 * Returns -1 for negative, 0 for zero, 1 for positive.
 */
public class Sign {
    public static int compute(int n) {
        if (n < 0) {
            return -1;
        } else if (n > 0) {
            return 1;
        } else {
            return 0;
        }
    }

    public static void main(String[] args) {
        compute(5);
    }
}
