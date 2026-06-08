# H743 Face Assets

把要显示的表情图像按下面的结构放在这个目录：

```text
assets/
  happy/
    40.png
    65.png
    80.png
  sad/
    50.png
  angry/
    80.png
  surprised/
    60.png
  thinking/
    70.png
  neutral/
    50.png
```

要求：

- 目录名必须是 `happy / sad / angry / surprised / thinking / neutral`
- 文件名必须是 `0-100` 的整数，表示强度
- 支持 `png / bmp / jpg / jpeg`
- 图片会被转换成 `128x64` 的单色位图并编译进 STM32H743 的程序 Flash

生成资源代码：

```bash
cd stm32_h743_uart_face_display
python -m pip install Pillow
python tools/generate_face_assets.py
```

生成后会覆盖：

- `inc/generated_face_assets.h`
- `src/generated_face_assets.c`
