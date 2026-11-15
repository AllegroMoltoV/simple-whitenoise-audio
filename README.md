# Simple White Noise

[English](#english) | [日本語](#日本語)

Listed on [Audio Plugin Soko-kun](https://audio-soko-kun.allegromoltov.jp/plugins/allegromoltov/simple-whitenoise-audio?lang=en), which links only to releases that passed its checks. / 音のプラグイン倉庫くんの[掲載ページ](https://audio-soko-kun.allegromoltov.jp/plugins/allegromoltov/simple-whitenoise-audio)から、検査を通った版を入手できます。

## English

Simple White Noise is an instrument plugin that generates stereo white noise, in VST® 3 and CLAP formats. It has a single parameter, Volume.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/VST_Compatible_Logo_Steinberg_negative.png">
  <img src="docs/images/VST_Compatible_Logo_Steinberg.png" alt="VST Compatible" width="120">
</picture>

### Background

This plugin was made to try the whole flow of building a small audio plugin and publishing it with verifiable releases: the archives are built from source by GitHub Actions, checked with [clap-validator](https://github.com/free-audio/clap-validator) and pluginval, and signed with a build provenance attestation. The plugin is written as a CLAP plugin, and clap-wrapper builds the VST 3 plugin from the same code.

### Features

- Formats: VST 3, CLAP
- Platforms: Windows x64, Linux x86_64
- Output: one stereo output. The left and right channels carry independent noise.
- Parameter: Volume, from -60 dB to 0 dB. The default is -18 dB. Changes are smoothed over about 10 ms.
- The plugin does not use the network and does not read or write files.

Lower the volume of your monitors before you insert the plugin. White noise at 0 dB is loud.

### Installation

1. Download the archive for your platform from the [releases](https://github.com/AllegroMoltoV/simple-whitenoise-audio/releases): `simple-whitenoise-<version>-windows-x64.zip` or `simple-whitenoise-<version>-linux-x86_64.zip`.
2. Verify the archive (see below).
3. For VST 3 hosts, extract the folder `simple-whitenoise.vst3` and copy the whole folder to the standard VST 3 folder:
   - Windows: `C:\Program Files\Common Files\VST3`
   - Linux: `~/.vst3`
4. For CLAP hosts, extract `simple-whitenoise.clap` and copy it to the standard CLAP folder:
   - Windows: `C:\Program Files\Common Files\CLAP`
   - Linux: `~/.clap`
5. Rescan plugins in your DAW. The plugin appears as an instrument named Simple White Noise by AllegroMoltoV.

The CLAP file keeps the name and the plugin ID of 1.0.x, so copying it over the old file updates the plugin.

### Verifying the download

Every archive has a build provenance attestation created by the release workflow of this repository. With the [GitHub CLI](https://cli.github.com/), run:

```sh
gh attestation verify simple-whitenoise-1.1.0-windows-x64.zip --repo AllegroMoltoV/simple-whitenoise-audio
```

You can also compare the SHA-256 hash shown on the release page. On Windows, `Get-FileHash` in PowerShell prints it; on Linux, use `sha256sum`.

### Building from source

Requirements: CMake 3.21 or later and a C++17 compiler (Visual Studio 2022 on Windows, GCC on Linux). CMake downloads CLAP 1.2.10, clap-wrapper 0.17.0, and the VST 3 SDK that clap-wrapper pins, at fixed commits and tags.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build --config Release --prefix package
```

The last command copies the plugins, the license texts, and `README.txt` to `package/` in the layout of the release archive. The option `-DSWN_SANITIZE=ON` builds only the CLAP plugin with AddressSanitizer and UndefinedBehaviorSanitizer on GCC and Clang.

### Contents

| Path | Contents |
| --- | --- |
| `src/plugin.cpp` | The plugin |
| `src/plugin-entry.cpp` | The CLAP entry point, compiled into both the CLAP and the VST 3 plugin |
| `CMakeLists.txt` | Build configuration and release layout |
| `packaging/README.txt` | Installation notes included in the archive |
| `.github/workflows/build.yaml` | Build, validation, and release workflow |
| `.github/scripts/check-archive.py` | Check of the archive contents before release |
| `.audio-soko-kun/` | Display name, category, archives, and logo for Audio Plugin Soko-kun |
| `docs/images/` | VST Compatible logo |
| `LICENSE` | MIT License |

### AI disclosure

The source code, workflow, documentation, and logo of this plugin were written with Anthropic Claude Code. The author decided the requirements, reviewed the logo, and approved each external operation.

### License

MIT License. See [LICENSE](LICENSE).

The VST Compatible logo images in `docs/images/` are trademarks of Steinberg Media Technologies GmbH and are not covered by the MIT License.

The archives include the following third-party software. Their license texts are in `THIRD_PARTY_LICENSES` in each archive.

| Software | Use | License |
| --- | --- | --- |
| CLAP | CLAP format | MIT |
| clap-wrapper | Building the VST 3 plugin from the CLAP plugin | MIT |
| {fmt} | Used by clap-wrapper | MIT |
| VST 3 SDK | VST 3 format | MIT |

VST is a registered trademark of Steinberg Media Technologies GmbH.

## 日本語

Simple White Noise は、ステレオのホワイトノイズを発生させる音源のプラグインです。VST® 3 と CLAP の形式で配布します。パラメーターは音量 (Volume) の 1 つだけです。

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/VST_Compatible_Logo_Steinberg_negative.png">
  <img src="docs/images/VST_Compatible_Logo_Steinberg.png" alt="VST Compatible" width="120">
</picture>

### 背景

小さなオーディオプラグインを作り、検証できる形でリリースするまでの流れを試すために作りました。配布物は GitHub Actions でソースからビルドし、[clap-validator](https://github.com/free-audio/clap-validator) と pluginval で検証し、build provenance の証明を付けます。プラグインは CLAP のプラグインとして書いています。VST 3 のプラグインは、同じコードから clap-wrapper で生成します。

### 機能

- 規格: VST 3、CLAP
- 対応環境: Windows x64、Linux x86_64
- 出力: ステレオの出力が 1 つです。左右のチャンネルに独立したノイズを出します。
- パラメーター: Volume。範囲は -60 dB から 0 dB、既定値は -18 dB です。変更は約 10 ms かけてなめらかに反映します。
- ネットワークを使いません。ファイルを読み書きしません。

プラグインを挿す前に、モニターの音量を下げてください。0 dB のホワイトノイズは大きな音です。

### 導入

1. [リリース](https://github.com/AllegroMoltoV/simple-whitenoise-audio/releases)から、環境に合うアーカイブをダウンロードします。`simple-whitenoise-<版>-windows-x64.zip` か `simple-whitenoise-<版>-linux-x86_64.zip` です。
2. アーカイブを検証します。手順は次の節にあります。
3. VST 3 に対応した DAW では、`simple-whitenoise.vst3` フォルダーを取り出し、フォルダーごと VST 3 の標準のフォルダーへコピーします。
   - Windows: `C:\Program Files\Common Files\VST3`
   - Linux: `~/.vst3`
4. CLAP に対応した DAW では、`simple-whitenoise.clap` を取り出し、CLAP の標準のフォルダーへコピーします。
   - Windows: `C:\Program Files\Common Files\CLAP`
   - Linux: `~/.clap`
5. DAW でプラグインを再スキャンします。AllegroMoltoV の Simple White Noise という名前の音源として表示されます。

CLAP のファイルの名前とプラグインの ID は 1.0.x から変わりません。古いファイルに上書きすると更新できます。

### ダウンロードしたファイルの検証

すべてのアーカイブに、このリポジトリのリリースのワークフローが作った build provenance の証明があります。[GitHub CLI](https://cli.github.com/) で次を実行します。

```sh
gh attestation verify simple-whitenoise-1.1.0-windows-x64.zip --repo AllegroMoltoV/simple-whitenoise-audio
```

リリースのページに表示される SHA-256 のハッシュと照合することもできます。Windows では PowerShell の `Get-FileHash`、Linux では `sha256sum` で表示できます。

### ソースからのビルド

CMake 3.21 以降と、C++17 のコンパイラーが必要です。Windows では Visual Studio 2022、Linux では GCC を使います。CMake は、CLAP 1.2.10、clap-wrapper 0.17.0、clap-wrapper が指定する版の VST 3 SDK を、コミットとタグを固定して取得します。ビルドのコマンドは英語の節と同じです。`cmake --install` を実行すると、プラグインとライセンス文と `README.txt` を、配布するアーカイブと同じ構成で `package/` にコピーします。

### AI 利用開示

このプラグインのソースコード、ワークフロー、文書、ロゴは Anthropic Claude Code で作成しました。要件の決定、ロゴのレビュー、外部への操作の承認は作者が行いました。

### ライセンス

MIT License です。[LICENSE](LICENSE) を参照してください。

`docs/images/` の VST Compatible ロゴの画像は Steinberg Media Technologies GmbH の商標で、MIT License の対象外です。

配布するアーカイブには、次の第三者のソフトウェアを含みます。いずれもアーカイブの `THIRD_PARTY_LICENSES` にライセンス文を同梱しています。

| ソフトウェア | 用途 | ライセンス |
| --- | --- | --- |
| CLAP | CLAP の形式 | MIT |
| clap-wrapper | CLAP から VST 3 の生成 | MIT |
| {fmt} | clap-wrapper の依存 | MIT |
| VST 3 SDK | VST 3 の形式 | MIT |

VST is a registered trademark of Steinberg Media Technologies GmbH.

## Changelog

### 1.1.0

- Added the VST 3 plugin for Windows x64 and Linux x86_64, built from the CLAP plugin with clap-wrapper. The CLAP file name and the plugin ID are unchanged.
- The archives now include the VST 3 bundle `simple-whitenoise.vst3`, `README.txt`, and the third-party license texts in `THIRD_PARTY_LICENSES`.
- The release workflow validates the VST 3 plugin with pluginval at strictness level 10 on both platforms.

### 1.0.1

- Declared the category and the archives in `.audio-soko-kun/config.json`. The plugin itself is unchanged.

### 1.0.0

- First release. Stereo white noise with one volume parameter, for Windows x64 and Linux x86_64.
