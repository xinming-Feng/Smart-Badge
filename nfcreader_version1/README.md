# NFC图像上传器

一个简单的Android应用，可以将图片通过NFC传输到2.9英寸水墨屏显示。

## 功能特性

- 📱 选择图片（从图库或相机）
- 🎨 Floyd-Steinberg抖动算法处理
- 📏 图像缩放功能
- 📡 NFC数据传输
- 👀 实时预览效果

## 技术实现

### 抖动算法
使用Floyd-Steinberg抖动算法将彩色图像转换为高质量的黑白图像，特别适合水墨屏显示。

### NFC通信
- 支持NDEF格式的NFC标签
- 自动检测NFC设备
- 图像数据压缩传输

### 图像处理
- 实时灰度转换
- 可调节抖动强度
- 图像缩放适配屏幕

## 使用方法

1. 启动应用
2. 点击"选择图片"按钮选择要处理的图片
3. 调整抖动强度和缩放参数
4. 点击"发送到NFC"按钮
5. 将手机靠近NFC芯片完成传输

## 系统要求

- Android 5.0 (API 21) 或更高版本
- 支持NFC的Android设备
- 相机和存储权限

## 开发环境

- Android Studio
- Gradle 7.4.2
- Android SDK 33
- Java 8

## 项目结构

```
app/
├── src/main/
│   ├── java/com/example/nfcreader/
│   │   ├── MainActivity.java          # 主活动
│   │   └── DitherProcessor.java       # 抖动算法处理
│   ├── res/
│   │   ├── layout/activity_main.xml   # 主界面布局
│   │   ├── values/strings.xml         # 字符串资源
│   │   └── xml/nfc_tech_filter.xml    # NFC技术过滤器
│   └── AndroidManifest.xml            # 应用清单
└── build.gradle                       # 模块构建配置
```

## 构建和运行

1. 克隆项目到本地
2. 在Android Studio中打开项目
3. 连接支持NFC的Android设备
4. 点击运行按钮

## 注意事项

- 确保设备NFC功能已启用
- 需要相机和存储权限
- 建议使用较小的图片以获得更好的处理效果
- NFC传输可能需要几秒钟时间

## 许可证

MIT License 