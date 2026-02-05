/**
 * Max of two integers (loop-free).
 */
public class Max {
    public static int compute(int a, int b) {
        if (a > b) {
            return a;
        } else {
            return b;
        }
    }

    public static void main(String[] args) {
        compute(5, 10);
    }
}
