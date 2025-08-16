package com.example.nfcreader;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import androidx.core.view.ViewCompat;

/* loaded from: classes3.dex */
public class DitherProcessor {
    public static Bitmap applyFloydSteinbergDither(Bitmap original, int ditherStrength) {
        if (original == null) {
            return null;
        }
        Bitmap grayBitmap = convertToGrayscale(original);
        int width = grayBitmap.getWidth();
        int height = grayBitmap.getHeight();
        Bitmap result = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        int[] pixels = new int[width * height];
        grayBitmap.getPixels(pixels, 0, width, 0, 0, width, height);
        float[] grayValues = new float[pixels.length];
        for (int i = 0; i < pixels.length; i++) {
            grayValues[i] = (pixels[i] >> 16) & 255;
        }
        applyDithering(grayValues, width, height, ditherStrength);
        for (int i2 = 0; i2 < pixels.length; i2++) {
            int gray = (int) grayValues[i2];
            int pixel = (gray << 16) | ViewCompat.MEASURED_STATE_MASK | (gray << 8) | gray;
            pixels[i2] = pixel;
        }
        result.setPixels(pixels, 0, width, 0, 0, width, height);
        grayBitmap.recycle();
        return result;
    }

    private static Bitmap convertToGrayscale(Bitmap original) {
        Bitmap grayBitmap = Bitmap.createBitmap(original.getWidth(), original.getHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(grayBitmap);
        Paint paint = new Paint();
        ColorMatrix colorMatrix = new ColorMatrix();
        colorMatrix.setSaturation(0.0f);
        paint.setColorFilter(new ColorMatrixColorFilter(colorMatrix));
        canvas.drawBitmap(original, 0.0f, 0.0f, paint);
        return grayBitmap;
    }

    private static void applyDithering(float[] pixels, int width, int height, int ditherStrength) {
        float strength = ditherStrength / 100.0f;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = (y * width) + x;
                float oldPixel = pixels[index];
                float newPixel = oldPixel < 128.0f ? 0.0f : 255.0f;
                pixels[index] = newPixel;
                float error = (oldPixel - newPixel) * strength;
                if (x + 1 < width) {
                    int i = index + 1;
                    pixels[i] = pixels[i] + ((7.0f * error) / 16.0f);
                }
                if (x - 1 >= 0 && y + 1 < height) {
                    int i2 = (index + width) - 1;
                    pixels[i2] = pixels[i2] + ((3.0f * error) / 16.0f);
                }
                if (y + 1 < height) {
                    int i3 = index + width;
                    pixels[i3] = pixels[i3] + ((5.0f * error) / 16.0f);
                }
                if (x + 1 < width && y + 1 < height) {
                    int i4 = index + width + 1;
                    pixels[i4] = pixels[i4] + ((1.0f * error) / 16.0f);
                }
            }
        }
    }

    public static Bitmap scaleImage(Bitmap original, float scaleFactor) {
        if (original == null) {
            return null;
        }
        int newWidth = (int) (original.getWidth() * scaleFactor);
        int newHeight = (int) (original.getHeight() * scaleFactor);
        return Bitmap.createScaledBitmap(original, newWidth, newHeight, true);
    }
}
