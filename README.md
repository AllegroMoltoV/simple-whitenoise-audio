# Simple White Noise

[English](#english) | [日本語](#日本語)

## English

Simple White Noise is a CLAP plugin that generates stereo white noise. It has a single parameter, Volume.

### Background

This plugin was made to try the whole flow of building a small audio plugin and publishing it with verifiable releases: the archives are built from source by GitHub Actions, checked with [clap-validator](https://github.com/free-audio/clap-validator), and signed with a build provenance attestation.

### Features

- Format: CLAP
- Platforms: Windows x64, Linux x86_64
- Output: one stereo output. The left and right channels carry independent noise.
- Parameter: Volume, from -60 dB to 0 dB. The default is -18 dB. Changes are smoothed over about 10 ms.
- The plugin does not use the network and does not read or write files.

Lower the volume of your monitors before you insert the plugin. White noise at 0 dB is loud.

### Installation

1. Download the archive for your platform from the [releases](https://github.com/AllegroMoltoV/simple-whitenoise-audio/releases): `simple-whitenoise-<version>-windows-x64.zip` or `simple-whitenoise-<version>-linux-x86_64.zip`.
2. Verify the archive (see below).
3. Extract `simple-whitenoise.clap` and copy it to the standard CLAP folder:
   - Windows: `C:\Program Files\Common Files\CLAP`
   - Linux: `~/.clap`
4. Rescan plugins in your DAW. The plugin appears as an instrument named Simple White Noise by AllegroMoltoV.

### Verifying the download

Every archive has a build provenance attestation created by the release workflow of this repository. With the [GitHub CLI](https://cli.github.com/), run:

```sh
gh attestation verify simple-whitenoise-1.0.0-windows-x64.zip --repo AllegroMoltoV/simple-whitenoise-audio
```

You can also compare the SHA-256 hash shown on the release page. On Windows, `Get-FileHash` in PowerShell prints it; on Linux, use `sha256sum`.

### Building from source

Requirements: CMake 3.21 or later and a C++17 compiler (Visual Studio 2022 on Windows, GCC on Linux). CMake downloads the CLAP headers, pinned to the commit of CLAP 1.2.10.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The plugin is written to `build/Release/simple-whitenoise.clap` on Windows and `build/simple-whitenoise.clap` on Linux. The option `-DSWN_SANITIZE=ON` builds with AddressSanitizer and UndefinedBehaviorSanitizer on GCC and Clang.

### Contents

| Path | Contents |
| --- | --- |
| `src/plugin.cpp` | The plugin |
| `CMakeLists.txt` | Build configuration |
| `.github/workflows/build.yaml` | Build, validation, and release workflow |
| `.audio-soko-kun/` | Display name and logo for Audio Plugin Soko-kun |
| `LICENSE` | MIT License |

### AI disclosure

The source code, workflow, documentation, and logo of this plugin were written with Anthropic Claude Code. The author decided the requirements, reviewed the logo, and approved each external operation.

### License

MIT License. See [LICENSE](LICENSE). CLAP is distributed under the MIT License by the CLAP project.

## 日本語

Simple White Noise は、ステレオのホワイトノイズを発生させる CLAP のプラグインです。パラメーターは音量 (Volume) の 1 つだけです。

### 背景

小さなオーディオプラグインを作り、検証できる形でリリースするまでの流れを試すために作りました。配布物は GitHub Actions でソースからビルドし、[clap-validator](https://github.com/free-audio/clap-validator) で検証し、build provenance の証明を付けます。

### 機能

- 規格: CLAP
- 対応環境: Windows x64、Linux x86_64
- 出力: ステレオの出力が 1 つです。左右のチャンネルに独立したノイズを出します。
- パラメーター: Volume。範囲は -60 dB から 0 dB、既定値は -18 dB です。変更は約 10 ms かけてなめらかに反映します。
- ネットワークを使いません。ファイルを読み書きしません。

プラグインを挿す前に、モニターの音量を下げてください。0 dB のホワイトノイズは大きな音です。

### 導入

1. [リリース](https://github.com/AllegroMoltoV/simple-whitenoise-audio/releases)から、環境に合うアーカイブをダウンロードします。`simple-whitenoise-<版>-windows-x64.zip` か `simple-whitenoise-<版>-linux-x86_64.zip` です。
2. アーカイブを検証します。手順は次の節にあります。
3. `simple-whitenoise.clap` を取り出し、CLAP の標準のフォルダーへコピーします。
   - Windows: `C:\Program Files\Common Files\CLAP`
   - Linux: `~/.clap`
4. DAW でプラグインを再スキャンします。AllegroMoltoV の Simple White Noise という名前の音源として表示されます。

### ダウンロードしたファイルの検証

すべてのアーカイブに、このリポジトリのリリースのワークフローが作った build provenance の証明があります。[GitHub CLI](https://cli.github.com/) で次を実行します。

```sh
gh attestation verify simple-whitenoise-1.0.0-windows-x64.zip --repo AllegroMoltoV/simple-whitenoise-audio
```

リリースのページに表示される SHA-256 のハッシュと照合することもできます。Windows では PowerShell の `Get-FileHash`、Linux では `sha256sum` で表示できます。

### ソースからのビルド

CMake 3.21 以降と、C++17 のコンパイラーが必要です。Windows では Visual Studio 2022、Linux では GCC を使います。CLAP のヘッダーは、CMake が CLAP 1.2.10 のコミットに固定して取得します。ビルドのコマンドは英語の節と同じです。

### AI 利用開示

このプラグインのソースコード、ワークフロー、文書、ロゴは Anthropic Claude Code で作成しました。要件の決定、ロゴのレビュー、外部への操作の承認は作者が行いました。

### ライセンス

MIT License です。[LICENSE](LICENSE) を参照してください。CLAP は、CLAP のプロジェクトが MIT License で配布しています。

## Changelog

### 1.0.1

- Declared the category and the archives in `.audio-soko-kun/config.json`. The plugin itself is unchanged.

### 1.0.0

- First release. Stereo white noise with one volume parameter, for Windows x64 and Linux x86_64.
