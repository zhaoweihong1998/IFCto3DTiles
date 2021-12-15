public class MyIfc {
    static
    {
        System.loadLibrary("MyIfc");
    }
    long nativeIfc;
    public MyIfc()
    {
        nativeIfc = createNativeIfc();
    }
    private native long createNativeIfc();
    public native void readFile(long addr,String filename);
    public native void to3DTiles(long addr);
    public long getAddr(){
        return nativeIfc;
    }
}
