package com.example.nfcreader;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;

public class DitherProcessor {
    
    /**
     * 使用Floyd-Steinberg抖动算法处理图像
     * @param original 原始图像
     * @param ditherStrength 抖动强度 (0-100)
     * @return 处理后的黑白图像
     */
    public static Bitmap applyFloydSteinbergDither(Bitmap original, int ditherStrength) {
        if (original == null) return null;
        
        // 首先转换为灰度图
        Bitmap grayBitmap = convertToGrayscale(original);
        
        // 获取图像尺寸
        int width = grayBitmap.getWidth();
        int height = grayBitmap.getHeight();
        
        // 创建输出图像
        Bitmap result = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        
        // 创建像素数组
        int[] pixels = new int[width * height];
        grayBitmap.getPixels(pixels, 0, width, 0, 0, width, height);
        
        // 转换为灰度值数组
        float[] grayValues = new float[pixels.length];
        for (int i = 0; i < pixels.length; i++) {
            grayValues[i] = (pixels[i] >> 16) & 0xFF; // 取红色分量作为灰度值
        }
        
        // 应用Floyd-Steinberg抖动算法
        applyDithering(grayValues, width, height, ditherStrength);
        
        // 转换回像素数组
        for (int i = 0; i < pixels.length; i++) {
            int gray = (int) grayValues[i];
            int pixel = (255 << 24) | (gray << 16) | (gray << 8) | gray;
            pixels[i] = pixel;
        }
        
        result.setPixels(pixels, 0, width, 0, 0, width, height);
        
        // 清理资源
        grayBitmap.recycle();
        
        return result;
    }
    
    /**
     * 转换为灰度图
     */
    private static Bitmap convertToGrayscale(Bitmap original) {
        Bitmap grayBitmap = Bitmap.createBitmap(original.getWidth(), original.getHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(grayBitmap);
        Paint paint = new Paint();
        
        ColorMatrix colorMatrix = new ColorMatrix();
        colorMatrix.setSaturation(0); // 设置为0使图像变为灰度
        paint.setColorFilter(new ColorMatrixColorFilter(colorMatrix));
        
        canvas.drawBitmap(original, 0, 0, paint);
        return grayBitmap;
    }
    
    /**
     * 应用Floyd-Steinberg抖动算法
     */
    private static void applyDithering(float[] pixels, int width, int height, int ditherStrength) {
        // 调整抖动强度
        float strength = ditherStrength / 100.0f;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = y * width + x;
                float oldPixel = pixels[index];
                
                // 二值化
                float newPixel = (oldPixel < 128) ? 0 : 255;
                pixels[index] = newPixel;
                
                // 计算误差
                float error = (oldPixel - newPixel) * strength;
                
                // 误差扩散到相邻像素
                if (x + 1 < width) {
                    pixels[index + 1] += error * 7.0f / 16.0f;
                }
                if (x - 1 >= 0 && y + 1 < height) {
                    pixels[index + width - 1] += error * 3.0f / 16.0f;
                }
                if (y + 1 < height) {
                    pixels[index + width] += error * 5.0f / 16.0f;
                }
                if (x + 1 < width && y + 1 < height) {
                    pixels[index + width + 1] += error * 1.0f / 16.0f;
                }
            }
        }
    }
    
    /**
     * 缩放图像
     */
    public static Bitmap scaleImage(Bitmap original, float scaleFactor) {
        if (original == null) return null;
        
        int newWidth = (int) (original.getWidth() * scaleFactor);
        int newHeight = (int) (original.getHeight() * scaleFactor);
        
        return Bitmap.createScaledBitmap(original, newWidth, newHeight, true);
    }
} 