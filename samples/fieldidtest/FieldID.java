class A {
    public int a;
    public void seta(int b) {
        a = b;
    }
    public int geta() {
        return a;
    }
}

class B extends A {
    public void seta(int b) {
        a = b + 1;
    }
}


public class FieldID {
    public static void main(String[] args) {
        B obj = new B();
        obj.seta(3);
        int num = obj.geta();
        System.out.println(num);
    }
}

