public class App {
    public static void main(String[] args) {
        MyIfc myIfc = new MyIfc();
        long addr = myIfc.getAddr();
        System.out.println(args[0]);

        myIfc.readFile(addr,args[0]);

        myIfc.to3DTiles(addr);
    }
}
