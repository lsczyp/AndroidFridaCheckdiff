package com.jlxy.javaapi;

public class C {
    public static boolean InitCMoudle(String path){
        try {
            System.load(path);
        }catch (Exception e){
            return false;
        }
        return true;
    }

    public static native long mapload(String libname);
    //map方式导入表是没有装载的，没有效果
    public static native long getImport(long base, String funcname);
    public static native long getExport(long base, String funcname);
    public static native long getCode(long base, String funcname);
}
