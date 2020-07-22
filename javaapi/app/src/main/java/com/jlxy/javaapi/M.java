package com.jlxy.javaapi;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class M {

    public static void mkdirs(String path){
        File dirs = new File(path.substring(0,path.lastIndexOf('/')));
        dirs.mkdirs();
    }

    public static boolean copyFileUsingFileStreams(String from, String to) {
        boolean b = false;
        InputStream input = null;
        OutputStream output = null;
        File source = new File(from);
        File dest = new File(to);

        try {
            input = new FileInputStream(source);
            output = new FileOutputStream(dest);
            byte[] buf = new byte[1024];
            int bytesRead;
            while ((bytesRead = input.read(buf)) > 0) {
                output.write(buf, 0, bytesRead);
            }
            b = true;
        } catch (IOException e){
            e.printStackTrace();
        }
        finally {
            try {
                if(input != null)
                    input.close();
                if(output != null)
                    output.close();
            }catch (IOException e){

            }
        }
        return b;
    }

    private static final String COMMAND_SU = "su";
    private static final String COMMAND_SH = "sh";

    public static String execCommand(String cmd, boolean isRoot) {
        StringBuilder result = new StringBuilder();
        DataOutputStream dos = null;
        DataInputStream dis = null;

        try {
            Process p = Runtime.getRuntime().exec(isRoot?COMMAND_SU:COMMAND_SH);// 经过Root处理的android系统即有su命令
            dos = new DataOutputStream(p.getOutputStream());
            dis = new DataInputStream(p.getInputStream());

            dos.writeBytes(cmd + "\n");
            dos.flush();
            dos.writeBytes("exit\n");
            dos.flush();
            p.waitFor();

            String line = null;
            while ((line = dis.readLine()) != null) {
                result.append(line);
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (dos != null) {
                try {
                    dos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (dis != null) {
                try {
                    dis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return result.toString();
    }
}
