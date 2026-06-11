# Chinese Font Usage Guide

This document explains how to prepare and use Simplified Chinese fonts with this project, including:

- where to get `ttf/otf` source font files
- how to convert them into device-usable `.cpfont` files
- where to place the converted font files on the SD card
- how to enable the font on the device

## 1. Understand the required format

The device cannot use `ttf` or `otf` directly.

This project reads a custom `.cpfont` format. The expected font directory layout is:

```text
/.fonts/<Family>/
```

It also supports:

```text
/fonts/<Family>/
```

Example:

```text
/.fonts/SourceHanSansSC/SourceHanSansSC_12.cpfont
/.fonts/SourceHanSansSC/SourceHanSansSC_14.cpfont
/.fonts/SourceHanSansSC/SourceHanSansSC_16.cpfont
/.fonts/SourceHanSansSC/SourceHanSansSC_18.cpfont
```

`/.fonts` is the recommended location.

## 2. Get Chinese font source files

Recommended official open-source Chinese fonts:

- Source Han Sans SC
  - Releases: https://github.com/adobe-fonts/source-han-sans/releases
  - Project: https://github.com/adobe-fonts/source-han-sans
- Noto CJK
  - Project: https://github.com/notofonts/noto-cjk
  - Official Noto usage page: https://notofonts.github.io/noto-docs/website/use/

Recommended starting point for Simplified Chinese sans-serif:

- `SourceHanSansSC-Regular.otf`
- `SourceHanSansSC-Bold.otf`

If you want a serif family instead, you can also use:

- `Noto Serif CJK SC`
- `Source Han Serif SC`

## 3. Install Python dependencies

The repository already includes the conversion script:

```text
lib/EpdFont/scripts/fontconvert_sdcard.py
```

Its dependency list is here:

```text
lib/EpdFont/scripts/requirements.txt
```

From the repository root, run:

```powershell
python -m pip install --user -r lib\EpdFont\scripts\requirements.txt
```

## 4. Convert the font

### 4.1 Do not use `cjk` alone

If you convert with:

```text
--intervals cjk
```

you will usually get Chinese and related CJK characters, but not always full English letters, digits, and common western punctuation. That means Chinese filenames may render while English filenames still miss glyphs.

Use this instead:

```text
--intervals latin-ext,cjk
```

That includes:

- Simplified Chinese
- English uppercase and lowercase letters
- digits
- common western punctuation

### 4.2 Recommended sizes

Generate these sizes:

```text
12,14,16,18
```

Why:

- reader body text uses the selected reading size
- the current firmware also uses a fixed `12pt` font for user-content metadata such as book titles, authors, filenames, and chapter names

### 4.3 Example conversion command for `SourceHanSansSC`

If your source files are located in:

```text
D:\dgx\source\Font\OTF\SimplifiedChinese
```

run this from the repository root:

```powershell
python lib\EpdFont\scripts\fontconvert_sdcard.py `
  --intervals latin-ext,cjk `
  --sizes 12,14,16,18 `
  --name SourceHanSansSC `
  --regular "D:\dgx\source\Font\OTF\SimplifiedChinese\SourceHanSansSC-Regular.otf" `
  --bold "D:\dgx\source\Font\OTF\SimplifiedChinese\SourceHanSansSC-Bold.otf" `
  --output-dir "Other\generated-fonts\SourceHanSansSC"
```

What each option does:

- `--name SourceHanSansSC`
  - sets the font family name
- `--regular`
  - points to the regular style font file
- `--bold`
  - points to the bold style font file
- `--sizes 12,14,16,18`
  - generates all required sizes in one run
- `--intervals latin-ext,cjk`
  - includes both English and Chinese coverage

### 4.4 If you do not have italic or bold-italic

That is fine.

Using only `regular + bold` is enough for normal reading. The current firmware gracefully falls back when italic or bold-italic files are not available.

## 5. What the conversion produces

The command above writes output to:

```text
Other/generated-fonts/SourceHanSansSC/
```

Typical output files:

- `SourceHanSansSC_12.cpfont`
- `SourceHanSansSC_14.cpfont`
- `SourceHanSansSC_16.cpfont`
- `SourceHanSansSC_18.cpfont`

This repository already contains a ready-to-use generated set at:

```text
Other/generated-fonts/SourceHanSansSC/
```

## 6. Copy the font to the SD card

Copy the generated `.cpfont` files into:

```text
/.fonts/SourceHanSansSC/
```

Complete example:

```text
/.fonts/SourceHanSansSC/SourceHanSansSC_12.cpfont
/.fonts/SourceHanSansSC/SourceHanSansSC_14.cpfont
/.fonts/SourceHanSansSC/SourceHanSansSC_16.cpfont
/.fonts/SourceHanSansSC/SourceHanSansSC_18.cpfont
```

If the directories do not exist, create them manually:

- `/.fonts`
- `/.fonts/SourceHanSansSC`

## 7. You can also upload fonts through the web UI

If you do not want to copy files manually to the SD card, you can:

1. connect the device to Wi-Fi
2. open the device web UI
3. go to `/fonts`
4. upload the `.cpfont` files from the same family

Important:

- the upload page accepts `.cpfont`
- not `ttf`
- not `otf`

## 8. Enable the font on the device

After the files are on the SD card, go to:

```text
Settings -> Reader -> Reader Font Family
```

Then select:

```text
SourceHanSansSC
```

The font management entry is:

```text
Settings -> Reader -> Manage Fonts
```

## 9. Recommended workflow

Use this sequence:

1. get `SourceHanSansSC-Regular.otf` and `SourceHanSansSC-Bold.otf`
2. install the Python dependencies
3. run the conversion command to generate `12/14/16/18` `.cpfont` files
4. copy them to `/.fonts/SourceHanSansSC/`
5. select `SourceHanSansSC` in `Reader Font Family`
6. restart the device, or re-enter the font selection page to confirm the font has loaded

## 10. Common issues

### 10.1 Chinese displays, but English filenames do not

This usually means the font was generated with:

```text
--intervals cjk
```

Fix:

regenerate it with:

```text
--intervals latin-ext,cjk
```

### 10.2 I copied `ttf/otf` files to the SD card, but the device does not see them

That is expected.

The device only reads `.cpfont`, so you must convert first.

### 10.3 I generated only `14/16/18`, and metadata text still looks wrong

Add `12pt` as well. The current firmware uses a fixed `12pt` user-content font for metadata UI text.

### 10.4 The generated font files are large

That is normal for CJK fonts. Chinese coverage includes a large number of glyphs, so `.cpfont` output is much larger than Latin-only fonts.

## 11. A currently verified example

This repository has already been used to generate a working Chinese font pack from:

- `D:\dgx\source\Font\OTF\SimplifiedChinese\SourceHanSansSC-Regular.otf`
- `D:\dgx\source\Font\OTF\SimplifiedChinese\SourceHanSansSC-Bold.otf`

Generated output location:

```text
Other/generated-fonts/SourceHanSansSC/
```

If you just want to use a ready-made set, you can take that generated pack directly and copy it to the SD card.
