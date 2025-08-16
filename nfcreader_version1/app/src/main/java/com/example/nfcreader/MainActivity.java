package com.example.nfcreader;

import android.app.PendingIntent;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.net.Uri;
import android.nfc.FormatException;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.nfc.tech.Ndef;
import android.nfc.tech.NdefFormatable;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.util.Base64;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Scanner;
import org.json.JSONObject;

/* loaded from: classes3.dex */
public class MainActivity extends AppCompatActivity {
    private static final int REQUEST_CAMERA_PERMISSION = 1002;
    private static final int REQUEST_IMAGE_PICK = 1001;
    private static final int REQUEST_STORAGE_PERMISSION = 1003;
    private Button btnSelectImage;
    private Button btnUploadGithub;
    private Button btnWriteNfc;
    private ImageView imagePreview;
    private Handler mainHandler;
    private NfcAdapter nfcAdapter;
    private Bitmap originalBitmap;
    private PendingIntent pendingIntent;
    private Bitmap processedBitmap;
    private SeekBar seekBarDither;
    private SeekBar seekBarScale;
    private TextView tvStatus;
    private String lastUploadedImageUrl = null;
    private boolean isWaitingForNfcWrite = false;

    @Override // androidx.fragment.app.FragmentActivity, androidx.activity.ComponentActivity, androidx.core.app.ComponentActivity, android.app.Activity
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initViews();
        setupListeners();
        this.mainHandler = new Handler(Looper.getMainLooper());
        NfcAdapter defaultAdapter = NfcAdapter.getDefaultAdapter(this);
        this.nfcAdapter = defaultAdapter;
        if (defaultAdapter == null) {
            Toast.makeText(this, getString(R.string.nfc_not_supported), 1).show();
        } else {
            this.pendingIntent = PendingIntent.getActivity(this, 0, new Intent(this, getClass()).addFlags(536870912), 33554432);
        }
    }

    @Override // androidx.fragment.app.FragmentActivity, android.app.Activity
    protected void onResume() {
        super.onResume();
        NfcAdapter nfcAdapter = this.nfcAdapter;
        if (nfcAdapter != null && nfcAdapter.isEnabled()) {
            this.nfcAdapter.enableForegroundDispatch(this, this.pendingIntent, null, null);
        }
    }

    @Override // androidx.fragment.app.FragmentActivity, android.app.Activity
    protected void onPause() {
        super.onPause();
        NfcAdapter nfcAdapter = this.nfcAdapter;
        if (nfcAdapter != null) {
            nfcAdapter.disableForegroundDispatch(this);
        }
    }

    private void initViews() {
        ImageView imageView = (ImageView) findViewById(R.id.imagePreview);
        this.imagePreview = imageView;
        if (imageView == null) {
            Toast.makeText(this, getString(R.string.image_preview_not_found), 1).show();
        }
        Button button = (Button) findViewById(R.id.btnSelectImage);
        this.btnSelectImage = button;
        if (button == null) {
            Toast.makeText(this, getString(R.string.btn_select_image_not_found), 1).show();
        }
        Button button2 = (Button) findViewById(R.id.btnUploadGithub);
        this.btnUploadGithub = button2;
        if (button2 == null) {
            Toast.makeText(this, getString(R.string.btn_upload_github_not_found), 1).show();
        }
        Button button3 = (Button) findViewById(R.id.btnWriteNfc);
        this.btnWriteNfc = button3;
        if (button3 == null) {
            Toast.makeText(this, getString(R.string.btn_write_nfc_not_found), 1).show();
        }
        SeekBar seekBar = (SeekBar) findViewById(R.id.seekBarDither);
        this.seekBarDither = seekBar;
        if (seekBar == null) {
            Toast.makeText(this, getString(R.string.seek_bar_dither_not_found), 1).show();
        }
        SeekBar seekBar2 = (SeekBar) findViewById(R.id.seekBarScale);
        this.seekBarScale = seekBar2;
        if (seekBar2 == null) {
            Toast.makeText(this, getString(R.string.seek_bar_scale_not_found), 1).show();
        }
        TextView textView = (TextView) findViewById(R.id.tvStatus);
        this.tvStatus = textView;
        if (textView == null) {
            Toast.makeText(this, getString(R.string.tv_status_not_found), 1).show();
        }
    }

    private void setupListeners() {
        this.btnSelectImage.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                selectImageClicked();
            }
        });
        this.btnUploadGithub.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                uploadToGithubClicked();
            }
        });
        this.btnWriteNfc.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                writeNfcClicked();
            }
        });
        this.seekBarDither.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() { // from class: com.example.nfcreader.MainActivity.1
            @Override // android.widget.SeekBar.OnSeekBarChangeListener
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser && MainActivity.this.originalBitmap != null) {
                    MainActivity.this.processImage();
                }
            }

            @Override // android.widget.SeekBar.OnSeekBarChangeListener
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override // android.widget.SeekBar.OnSeekBarChangeListener
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        this.seekBarScale.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() { // from class: com.example.nfcreader.MainActivity.2
            @Override // android.widget.SeekBar.OnSeekBarChangeListener
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser && MainActivity.this.originalBitmap != null) {
                    MainActivity.this.processImage();
                }
            }

            @Override // android.widget.SeekBar.OnSeekBarChangeListener
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override // android.widget.SeekBar.OnSeekBarChangeListener
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
    }

    /* renamed from: lambda$setupListeners$0$com-example-nfcreader-MainActivity, reason: not valid java name */
    /* synthetic */ void m47lambda$setupListeners$0$comexamplenfcreaderMainActivity(View v) {
        selectImage();
    }

    /* renamed from: lambda$setupListeners$1$com-example-nfcreader-MainActivity, reason: not valid java name */
    /* synthetic */ void m48lambda$setupListeners$1$comexamplenfcreaderMainActivity(View v) {
        uploadToGithubClicked();
    }

    /* renamed from: lambda$setupListeners$2$com-example-nfcreader-MainActivity, reason: not valid java name */
    /* synthetic */ void m49lambda$setupListeners$2$comexamplenfcreaderMainActivity(View v) {
        writeNfcClicked();
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
        if (Build.VERSION.SDK_INT >= 33) {
            hasStoragePermission = ContextCompat.checkSelfPermission(this, "android.permission.READ_MEDIA_IMAGES") == 0;
        } else {
            hasStoragePermission = ContextCompat.checkSelfPermission(this, "android.permission.READ_EXTERNAL_STORAGE") == 0;
        }
        return hasStoragePermission && ContextCompat.checkSelfPermission(this, "android.permission.CAMERA") == 0;
    }

    private void requestPermissions() {
        String[] permissions;
        if (Build.VERSION.SDK_INT >= 33) {
            permissions = new String[]{"android.permission.READ_MEDIA_IMAGES", "android.permission.CAMERA"};
        } else {
            permissions = new String[]{"android.permission.READ_EXTERNAL_STORAGE", "android.permission.CAMERA"};
        }
        ActivityCompat.requestPermissions(this, permissions, 1003);
    }

    private void showImageSelectionDialog() {
        Intent intent = new Intent("android.intent.action.PICK", MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
        startActivityForResult(intent, 1001);
    }

    @Override // androidx.fragment.app.FragmentActivity, androidx.activity.ComponentActivity, android.app.Activity
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Uri imageUri;
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 1001 && resultCode == -1 && data != null && (imageUri = data.getData()) != null) {
            loadImageFromUri(imageUri);
        }
    }

    private void loadImageFromUri(Uri uri) {
        try {
            this.originalBitmap = MediaStore.Images.Media.getBitmap(getContentResolver(), uri);
            processImage();
            this.btnUploadGithub.setEnabled(true);
            this.tvStatus.setText(getString(R.string.image_loaded));
        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(this, getString(R.string.image_processing_failed), 0).show();
        }
    }

    /* JADX INFO: Access modifiers changed from: private */
    public void processImage() {
        if (this.originalBitmap == null) {
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                processImageInBackground();
            }
        }).start();
    }

    private void processImageInBackground() {
        try {
            int ditherStrength = this.seekBarDither.getProgress();
            float scaleFactor = this.seekBarScale.getProgress() / 100.0f;
            int previewWidth = (int) (this.originalBitmap.getWidth() * scaleFactor);
            int previewHeight = (int) (this.originalBitmap.getHeight() * scaleFactor);
            Bitmap previewBitmap = Bitmap.createScaledBitmap(this.originalBitmap, previewWidth, previewHeight, true);
            Bitmap ditheredPreview = DitherProcessor.applyFloydSteinbergDither(previewBitmap, ditherStrength);
            Bitmap finalBitmap = Bitmap.createScaledBitmap(ditheredPreview, 296, 128, true);
            previewBitmap.recycle();
            ditheredPreview.recycle();
            this.processedBitmap = finalBitmap;
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    onImageProcessSuccess();
                }
            });
        } catch (Exception e) {
            e.printStackTrace();
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    onImageProcessFailed();
                }
            });
        }
    }

    private void onImageProcessSuccess() {
        this.imagePreview.setImageBitmap(this.processedBitmap);
        String processingCompleteStr = getString(R.string.image_processing_complete) + " - Size: " + this.processedBitmap.getWidth() + "x" + this.processedBitmap.getHeight() + " (Fixed for e-paper)";
        this.tvStatus.setText(processingCompleteStr);
        this.btnUploadGithub.setEnabled(true);
    }

    private void onImageProcessFailed() {
        Toast.makeText(this, getString(R.string.image_processing_failed), 0).show();
    }

    @Override // androidx.appcompat.app.AppCompatActivity, androidx.fragment.app.FragmentActivity, android.app.Activity
    protected void onDestroy() {
        super.onDestroy();
        Bitmap bitmap = this.originalBitmap;
        if (bitmap != null && !bitmap.isRecycled()) {
            this.originalBitmap.recycle();
        }
        Bitmap bitmap2 = this.processedBitmap;
        if (bitmap2 != null && !bitmap2.isRecycled()) {
            this.processedBitmap.recycle();
        }
    }

    @Override // androidx.fragment.app.FragmentActivity, androidx.activity.ComponentActivity, android.app.Activity
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == 1003) {
            boolean allGranted = true;
            int length = grantResults.length;
            int i = 0;
            while (true) {
                if (i >= length) {
                    break;
                }
                int result = grantResults[i];
                if (result == 0) {
                    i++;
                } else {
                    allGranted = false;
                    break;
                }
            }
            if (allGranted) {
                selectImage();
            } else {
                Toast.makeText(this, getString(R.string.permission_message), 1).show();
                showPermissionDialog();
            }
        }
    }

    private void showPermissionDialog() {
        new AlertDialog.Builder(this)
                .setTitle(getString(R.string.permission_required))
                .setMessage(getString(R.string.permission_message))
                .setPositiveButton(getString(R.string.go_to_settings), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        openAppSettings();
                    }
                })
                .setNegativeButton(getString(R.string.cancel), null)
                .show();
    }

    private void openAppSettings() {
        Intent intent = new Intent("android.settings.APPLICATION_DETAILS_SETTINGS");
        intent.setData(Uri.fromParts("package", getPackageName(), null));
        startActivity(intent);
    }

    private String uploadImageToGitHub(byte[] imageBytes, String fileName) throws Exception {
        String path = "images/" + fileName;
        String apiUrl = "https://api.github.com/repos/xinming-Feng/image/contents/" + path;
        String base64Image = Base64.encodeToString(imageBytes, 2);
        JSONObject body = new JSONObject();
        body.put("message", getString(R.string.upload_image_message));
        body.put("content", base64Image);
        body.put("branch", "main");
        URL url = new URL(apiUrl);
        HttpURLConnection conn = (HttpURLConnection) url.openConnection();
        conn.setRequestMethod("PUT");
        conn.setRequestProperty("Authorization", "token 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");
        conn.setRequestProperty("Content-Type", "application/json");
        conn.setDoOutput(true);
        OutputStream os = conn.getOutputStream();
        os.write(body.toString().getBytes(StandardCharsets.UTF_8));
        os.flush();
        os.close();
        int responseCode = conn.getResponseCode();
        if (responseCode == 201 || responseCode == 200) {
            return "https://raw.githubusercontent.com/xinming-Feng/image/main/" + path;
        }
        InputStream err = conn.getErrorStream();
        if (err == null) {
            throw new Exception(getString(R.string.github_upload_failed_status) + ": " + responseCode);
        }
        Scanner s = new Scanner(err).useDelimiter("\\A");
        String error = s.hasNext() ? s.next() : "";
        throw new Exception(getString(R.string.github_upload_failed) + ": " + error);
    }

    private void uploadToGithubClicked() {
        if (this.processedBitmap == null) {
            Toast.makeText(this, getString(R.string.select_process_image_first), 0).show();
            return;
        }
        Matrix matrix = new Matrix();
        matrix.postRotate(90.0f);
        Bitmap bitmap = this.processedBitmap;
        Bitmap rotatedBitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), this.processedBitmap.getHeight(), matrix, true);
        int width = rotatedBitmap.getWidth();
        int height = rotatedBitmap.getHeight();
        int[] pixels = new int[width * height];
        rotatedBitmap.getPixels(pixels, 0, width, 0, 0, width, height);
        final byte[] imageData = new byte[4736];
        for (int i = 0; i < pixels.length; i++) {
            boolean isBlack = (pixels[i] & ViewCompat.MEASURED_SIZE_MASK) < 8421504;
            int byteIndex = i / 8;
            int bitIndex = 7 - (i % 8);
            if (isBlack) {
                imageData[byteIndex] = (byte) ((1 << bitIndex) | imageData[byteIndex]);
            }
        }
        rotatedBitmap.recycle();
        final String fileName = "image_" + System.currentTimeMillis() + ".bin";
        this.tvStatus.setText(getString(R.string.uploading_to_github));
        new Thread(new Runnable() {
            @Override
            public void run() {
                uploadImageToGithubInBackground(imageData, fileName);
            }
        }).start();
    }

    private void uploadImageToGithubInBackground(byte[] imageData, String fileName) {
        try {
            final String url = uploadImageToGitHub(imageData, fileName);
            this.lastUploadedImageUrl = url;
            final String finalUrl = url;
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    onUploadSuccess(finalUrl);
                }
            });
        } catch (Exception e) {
            e.printStackTrace();
            this.tvStatus.setText(getString(R.string.upload_failed_with_error) + ": " + String.valueOf(e.getMessage()));
            Toast.makeText(this, getString(R.string.upload_failed_with_error), 0).show();
        }
    }

    private void onUploadSuccess(String url) {
        this.tvStatus.setText(url != null ? getString(R.string.upload_successful_url_saved) + "\n" + url : getString(R.string.upload_successful_url_saved));
        Toast.makeText(this, getString(R.string.upload_successful), 0).show();
    }

    private void writeNfcClicked() {
        if (this.lastUploadedImageUrl == null) {
            Toast.makeText(this, getString(R.string.upload_image_github_first), 0).show();
            return;
        }
        NfcAdapter nfcAdapter = this.nfcAdapter;
        if (nfcAdapter == null) {
            Toast.makeText(this, getString(R.string.nfc_not_supported), 0).show();
        } else if (!nfcAdapter.isEnabled()) {
            Toast.makeText(this, getString(R.string.please_enable_nfc), 0).show();
        } else {
            this.tvStatus.setText(getString(R.string.bring_nfc_tag_close));
            this.isWaitingForNfcWrite = true;
        }
    }

    @Override // androidx.fragment.app.FragmentActivity, androidx.activity.ComponentActivity, android.app.Activity
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        String action = intent.getAction();
        if (action != null && this.isWaitingForNfcWrite && this.lastUploadedImageUrl != null) {
            if ("android.nfc.action.TAG_DISCOVERED".equals(action) || "android.nfc.action.NDEF_DISCOVERED".equals(action) || "android.nfc.action.TECH_DISCOVERED".equals(action)) {
                Tag tag = (Tag) intent.getParcelableExtra("android.nfc.extra.TAG");
                if (tag != null) {
                    try {
                        writeNdefToTag(tag);
                    } catch (Exception e) {
                        e.printStackTrace();
                        this.tvStatus.setText(getString(R.string.nfc_operation_exception) + ": " + e.getMessage());
                    }
                }
                this.isWaitingForNfcWrite = false;
            }
        }
    }

    private void writeNdefToTag(Tag tag) throws IOException, FormatException {
        try {
            NdefMessage ndefMessage = new NdefMessage(new NdefRecord[]{NdefRecord.createUri(this.lastUploadedImageUrl)});
            Ndef ndef = Ndef.get(tag);
            if (ndef != null) {
                try {
                    ndef.connect();
                    ndef.writeNdefMessage(ndefMessage);
                    ndef.close();
                    this.tvStatus.setText(getString(R.string.nfc_write_successful) + "!");
                    Toast.makeText(this, getString(R.string.nfc_write_successful), 0).show();
                    return;
                } catch (Exception e) {
                    Log.e("NFC", getString(R.string.ndef_write_failed) + ": " + e.getMessage());
                    this.tvStatus.setText(getString(R.string.nfc_write_failed) + ": " + e.getMessage());
                    Toast.makeText(this, getString(R.string.nfc_write_failed), 0).show();
                    return;
                }
            }
            NdefFormatable formatable = NdefFormatable.get(tag);
            if (formatable != null) {
                try {
                    formatable.connect();
                    formatable.format(ndefMessage);
                    formatable.close();
                    this.tvStatus.setText(getString(R.string.nfc_format_write_successful) + "!");
                    Toast.makeText(this, getString(R.string.nfc_format_write_successful), 0).show();
                    return;
                } catch (Exception e2) {
                    Log.e("NFC", getString(R.string.format_failed) + ": " + e2.getMessage());
                    this.tvStatus.setText(getString(R.string.nfc_format_failed) + ": " + e2.getMessage());
                    Toast.makeText(this, getString(R.string.nfc_format_failed), 0).show();
                    return;
                }
            }
            this.tvStatus.setText(getString(R.string.nfc_tag_no_ndef));
            Toast.makeText(this, getString(R.string.nfc_tag_no_ndef), 0).show();
            return;
        } catch (Exception e3) {
            Log.e("NFC", getString(R.string.nfc_operation_exception) + ": " + e3.getMessage());
            this.tvStatus.setText(getString(R.string.nfc_operation_exception) + ": " + e3.getMessage());
            Toast.makeText(this, getString(R.string.nfc_operation_exception), 0).show();
        }
    }

    private void selectImageClicked() {
        if (checkPermissions()) {
            selectImage();
        } else {
            requestPermissions();
        }
    }
}
