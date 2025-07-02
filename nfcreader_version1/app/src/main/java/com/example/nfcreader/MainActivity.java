package com.example.nfcreader;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

import android.app.PendingIntent;
import android.util.Log;

public class MainActivity extends AppCompatActivity {
    
    private static final int REQUEST_IMAGE_PICK = 1001;
    private static final int REQUEST_CAMERA_PERMISSION = 1002;
    private static final int REQUEST_STORAGE_PERMISSION = 1003;
    
    private ImageView imagePreview;
    private Button btnSelectImage;
    private Button btnSendEsp32;
    private SeekBar seekBarDither;
    private SeekBar seekBarScale;
    private TextView tvStatus;
    private EditText editEsp32Ip;
    
    private Handler mainHandler;
    
    private String esp32Ip = "192.168.4.1"; // 可在UI中让用户输入
    
    private Bitmap originalBitmap;
    private Bitmap processedBitmap;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        initViews();
        setupListeners();
        
        mainHandler = new Handler(Looper.getMainLooper());
        editEsp32Ip = findViewById(R.id.editEsp32Ip);
    }
    
    private void initViews() {
        imagePreview = findViewById(R.id.imagePreview);
        btnSelectImage = findViewById(R.id.btnSelectImage);
        btnSendEsp32 = findViewById(R.id.btnSendEsp32);
        seekBarDither = findViewById(R.id.seekBarDither);
        seekBarScale = findViewById(R.id.seekBarScale);
        tvStatus = findViewById(R.id.tvStatus);
    }
    
    private void setupListeners() {
        btnSelectImage.setOnClickListener(v -> selectImage());
        btnSendEsp32.setOnClickListener(v -> sendImageToESP32());
        
        seekBarDither.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser && originalBitmap != null) {
                    processImage();
                }
            }
            
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}
            
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        
        seekBarScale.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser && originalBitmap != null) {
                    processImage();
                }
            }
            
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}
            
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
    }
    
    private void selectImage() {
        if (checkPermissions()) {
            showImageSelectionDialog();
        } else {
            requestPermissions();
        }
    }
    
    private boolean checkPermissions() {
        boolean hasStoragePermission;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Android 13+ 使用新的存储权限
            hasStoragePermission = ContextCompat.checkSelfPermission(this, Manifest.permission.READ_MEDIA_IMAGES) 
                    == PackageManager.PERMISSION_GRANTED;
        } else {
            // Android 13以下使用旧的存储权限
            hasStoragePermission = ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) 
                    == PackageManager.PERMISSION_GRANTED;
        }
        
        return hasStoragePermission &&
               ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) 
                == PackageManager.PERMISSION_GRANTED;
    }
    
    private void requestPermissions() {
        String[] permissions;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Android 13+ 使用新的存储权限
            permissions = new String[]{
                Manifest.permission.READ_MEDIA_IMAGES,
                Manifest.permission.CAMERA
            };
        } else {
            // Android 13以下使用旧的存储权限
            permissions = new String[]{
                Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.CAMERA
            };
        }
        
        ActivityCompat.requestPermissions(this, permissions, REQUEST_STORAGE_PERMISSION);
    }
    
    private void showImageSelectionDialog() {
        Intent intent = new Intent(Intent.ACTION_PICK, MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
        startActivityForResult(intent, REQUEST_IMAGE_PICK);
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        
        if (requestCode == REQUEST_IMAGE_PICK && resultCode == RESULT_OK && data != null) {
            Uri imageUri = data.getData();
            if (imageUri != null) {
                loadImageFromUri(imageUri);
            }
        }
    }
    
    private void loadImageFromUri(Uri uri) {
        try {
            // 加载图像
            originalBitmap = MediaStore.Images.Media.getBitmap(getContentResolver(), uri);
            
            // 处理图像
            processImage();
            
            // 启用发送按钮
            btnSendEsp32.setEnabled(true);
            tvStatus.setText(getString(R.string.image_loaded));
            
        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(this, getString(R.string.image_processing_failed), Toast.LENGTH_SHORT).show();
        }
    }
    
    private void processImage() {
        if (originalBitmap == null) return;
        
        // 在后台线程处理图像
        new Thread(() -> {
            try {
                // 获取缩放因子 (0.1 - 1.0)
                float scaleFactor = 0.1f + (seekBarScale.getProgress() / 100.0f) * 0.9f;
                
                // 缩放图像
                Bitmap scaledBitmap = DitherProcessor.scaleImage(originalBitmap, scaleFactor);
                
                // 应用抖动算法
                int ditherStrength = seekBarDither.getProgress();
                processedBitmap = DitherProcessor.applyFloydSteinbergDither(scaledBitmap, ditherStrength);
                
                // 强制缩放到水墨屏确切尺寸 (128x296)
                Bitmap finalBitmap = Bitmap.createScaledBitmap(processedBitmap, 128, 296, true);
                if (processedBitmap != finalBitmap) {
                    processedBitmap.recycle();
                }
                processedBitmap = finalBitmap;
                
                // 在UI线程更新预览
                mainHandler.post(() -> {
                    imagePreview.setImageBitmap(processedBitmap);
                    tvStatus.setText(getString(R.string.image_processing_complete) + " - Size: " + processedBitmap.getWidth() + "x" + processedBitmap.getHeight());
                });
                
                // 清理资源
                if (scaledBitmap != originalBitmap) {
                    scaledBitmap.recycle();
                }
                
            } catch (Exception e) {
                e.printStackTrace();
                mainHandler.post(() -> {
                    Toast.makeText(MainActivity.this, getString(R.string.image_processing_failed), Toast.LENGTH_SHORT).show();
                });
            }
        }).start();
    }
    
    private void sendImageToESP32() {
        if (processedBitmap == null) {
            Toast.makeText(this, getString(R.string.select_image_first), Toast.LENGTH_SHORT).show();
            return;
        }
        
        // 验证图片尺寸
        int width = processedBitmap.getWidth();
        int height = processedBitmap.getHeight();
        
        if (width != 128 || height != 296) {
            Toast.makeText(this, getString(R.string.image_size_error) + ": " + width + "x" + height + ", " + getString(R.string.expected_size) + " 128x296", Toast.LENGTH_LONG).show();
            return;
        }
        
        String ipInput = editEsp32Ip.getText().toString().trim();
        if (!ipInput.isEmpty()) {
            esp32Ip = ipInput;
        }
        
        // 获取像素数据
        int[] pixels = new int[width * height];
        processedBitmap.getPixels(pixels, 0, width, 0, 0, width, height);
        
        // 转换为字节数据
        byte[] imageData = new byte[4736];
        int byteIndex = 0, bitIndex = 0;
        int blackPixels = 0, whitePixels = 0;
        
        for (int i = 0; i < pixels.length && byteIndex < 4736; i++) {
            int pixel = pixels[i];
            int gray = (pixel >> 16) & 0xFF;
            boolean isBlack = gray < 128;
            
            if (isBlack) {
                imageData[byteIndex] |= (1 << (7 - bitIndex));  // 黑色像素设为1
                blackPixels++;
            } else {
                // 白色像素设为0
                whitePixels++;
            }
            
            bitIndex++;
            if (bitIndex >= 8) { 
                bitIndex = 0; 
                byteIndex++; 
            }
        }
        
        // 显示统计信息
        String stats = String.format("Black pixels: %d, White pixels: %d, Total bytes: %d", 
                                   blackPixels, whitePixels, byteIndex);
        tvStatus.setText(getString(R.string.uploading_image) + " " + stats);
        
        new Thread(() -> {
            try {
                java.net.URL url = new java.net.URL("http://" + esp32Ip + "/upload");
                java.net.HttpURLConnection conn = (java.net.HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");
                conn.setDoOutput(true);
                
                // 生成boundary
                String boundary = "----WebKitFormBoundary" + System.currentTimeMillis();
                conn.setRequestProperty("Content-Type", "multipart/form-data; boundary=" + boundary);
                
                java.io.OutputStream os = conn.getOutputStream();
                java.io.PrintWriter writer = new java.io.PrintWriter(new java.io.OutputStreamWriter(os, "UTF-8"), true);
                
                // 写入表单数据
                writer.append("--").append(boundary).append("\r\n");
                writer.append("Content-Disposition: form-data; name=\"image\"; filename=\"image.bin\"").append("\r\n");
                writer.append("Content-Type: application/octet-stream").append("\r\n");
                writer.append("\r\n");
                writer.flush();
                
                // 写入图片数据
                os.write(imageData);
                os.flush();
                
                // 写入结束标记
                writer.append("\r\n");
                writer.append("--").append(boundary).append("--").append("\r\n");
                writer.flush();
                
                os.close();
                int responseCode = conn.getResponseCode();
                String msg = responseCode == 200 ? getString(R.string.image_upload_success) : (getString(R.string.upload_failed) + ": " + responseCode);
                runOnUiThread(() -> tvStatus.setText(msg + " - " + stats));
                conn.disconnect();
            } catch (Exception e) {
                runOnUiThread(() -> tvStatus.setText(getString(R.string.upload_failed) + ": " + e.getMessage()));
            }
        }).start();
    }
    
    @Override
    protected void onPause() {
        super.onPause();
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (originalBitmap != null && !originalBitmap.isRecycled()) {
            originalBitmap.recycle();
        }
        if (processedBitmap != null && !processedBitmap.isRecycled()) {
            processedBitmap.recycle();
        }
    }
    
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        
        if (requestCode == REQUEST_STORAGE_PERMISSION) {
            boolean allGranted = true;
            for (int result : grantResults) {
                if (result != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }
            
            if (allGranted) {
                selectImage();
            } else {
                Toast.makeText(this, getString(R.string.permission_message), Toast.LENGTH_LONG).show();
                // 显示对话框引导用户去设置
                showPermissionDialog();
            }
        }
    }
    
    private void showPermissionDialog() {
        new androidx.appcompat.app.AlertDialog.Builder(this)
            .setTitle(getString(R.string.permission_required))
            .setMessage(getString(R.string.permission_message))
            .setPositiveButton(getString(R.string.go_to_settings), (dialog, which) -> {
                // 打开应用设置页面
                Intent intent = new Intent(android.provider.Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                intent.setData(Uri.fromParts("package", getPackageName(), null));
                startActivity(intent);
            })
            .setNegativeButton(getString(R.string.cancel), null)
            .show();
    }
} 