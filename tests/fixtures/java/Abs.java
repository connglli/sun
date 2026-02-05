/**
 * Absolute value (loop-free).
 */
public class Abs {
    public static int compute(int n) {
        if (n < 0) {
            return -n;
        } else {
            return n;
        }
    }

    public static void main(String[] args) {
        compute(-5);
    }
}
