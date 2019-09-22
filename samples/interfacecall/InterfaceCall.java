interface A{
  public int add();
  public int minus();
}

interface B{
  public int mult();
  public int div();
}

public class InterfaceCall implements A, B {
    public static void main(String[]args) {
    }

    public void test(B x) {
      x.mult();
    }

    public int add() {
      return 6 + 3;
    }

    public int minus() {
      return 6 - 3;
    }

    public int mult() {
      return 6 * 3;
    }

    public int div() {
      return 6 / 3;
    }
}
