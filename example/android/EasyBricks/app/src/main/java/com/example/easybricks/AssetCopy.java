package com.example.easybricks;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.content.res.AssetManager;
import android.util.Log;

public class AssetCopy {
    public static void AssetToSD(AssetManager asset, String assetpath, String SDpath) {
        Log.i("AssetCopy", "Copy " + assetpath + " to " + SDpath);
        String[] filenames = null;
        FileOutputStream out = null;
        InputStream in = null;
        try {
            filenames = asset.list(assetpath);
            if(filenames.length > 0) {
                // Directory
                SDpath = SDpath + assetpath;
                File sd_file = new File(SDpath);
                if (!sd_file.exists()) {
                    sd_file.mkdir();
                }
                for (String fileName : filenames) {
                    AssetToSD(asset, assetpath + "/" + fileName, SDpath + "/" + fileName);
                }
            } else {
                in = asset.open(assetpath);

                // File
                File sd_file = new File(SDpath);
                if(!sd_file.exists()) {
                    sd_file.createNewFile();
                }
                // Write sd file
                in = asset.open(assetpath);
                out = new FileOutputStream(sd_file);
                byte[] buffer = new byte[1024];
                int byteCount = 0;
                while ((byteCount = in.read(buffer)) != -1) {
                    out.write(buffer, 0, byteCount);
                }
                out.flush();
            }
        } catch (IOException e) {
            Log.e("AssetCopy", e.toString());
        } finally {
            try {
                if (in != null) {
                    in.close();
                }
                if (out != null) {
                    out.close();
                }
            } catch (IOException e) {
                Log.e("AssetCopy", e.toString());
            }
        }
    }
}
