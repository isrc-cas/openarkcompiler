class A {
  public int first() {
    return 0;
  }
}

class B extends A {
  public void foo() {
  }

  public int first() {
    return 1;
  }
}

class C extends A {
  public void bar() {
  }

  public int first() {
    return 2;
  }
}

public class VTable {
  public static void main(String [] args) {
    A x = new B();
    x.first();
    A y = new C();
    y.first();
  }

  public void add(A x) {
    x.first();
  }
}
