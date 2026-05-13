# em68030-guest-linux

[English documentation (README.md)](README.md)

Em68030 MC68030/MVME147 エミュレータ
([Em68030_WinUI3Cpp](https://github.com/hha0x617/Em68030_WinUI3Cpp),
[Em68030_CsWPF](https://github.com/hha0x617/Em68030_CsWPF))
用の Linux ゲスト側カーネルモジュール、パッチ、設定ファイルです。

*リポジトリ固有のアセット (`drivers/` 以下の `em68030fb` /
`em68030input` ドライバ、`patches/` のカーネルパッチ、`configs/` の
カーネル設定、`docker/` および `build.sh` / `build.ps1` のビルド
スクリプト、GitHub Actions ワークフロー) は
[Claude Code](https://docs.anthropic.com/en/docs/claude-code) との
vibe coding で開発しています。上流の Linux カーネルソース本体は
AI 生成ではありません — ビルド時に kernel.org からそのまま取得して
います。*

## ライセンス

本リポジトリは GPL-2.0-only でライセンスされています。詳細は [LICENSE](LICENSE) を参照してください。

エミュレータ本体は別リポジトリで Apache License 2.0 のもとライセンスされています
([Em68030_WinUI3Cpp](https://github.com/hha0x617/Em68030_WinUI3Cpp),
[Em68030_CsWPF](https://github.com/hha0x617/Em68030_CsWPF))。

## ビルド済みバイナリ

ビルド済みカーネルイメージとモジュールは
[Releases](https://github.com/hha0x617/Em68030-Guest-Linux/releases) ページで入手できます。

リリースアセットのファイル名にはビルドを識別するサフィックスが付いています
（例: `em68030fb-abc1234.ko`, `em68030input-abc1234.ko`）。
**インストール前に `.ko` ファイルを元の名前にリネームしてください:**

```bash
mv em68030fb-*.ko em68030fb.ko
mv em68030input-*.ko em68030input.ko
```

`vmlinux` イメージはエミュレータで直接ロードできます (File → Open ELF)。

## 対応カーネルバージョン

ビルド基盤は `KERNEL_VERSION` でパラメタ化されており、Linux 6.12.17
(デフォルト — 現行 CI 成果物のベース) と Linux 7.0.x の両方を受け付け
ます。メジャー成分が kernel.org の URL (`pub/linux/kernel/v6.x/` vs
`v7.x/`) と `patches/linux-6/` / `patches/linux-7/` 配下のパッチ
ディレクトリ選択の両方に使われます。

| ステータス | バージョン |
|---|---|
| デフォルト (CI 成果物) | **6.12.17** |
| 検証済 (KPI + パッチ apply) | **7.0.x** — フルクロスビルドおよびエミュレータ実機 boot は 11.0 後追従と同じく後続作業 |

7.0.x を選ぶ場合は `--kernel-version 7.0.6` (build.sh)、
`-KernelVersion 7.0.6` (build.ps1)、もしくは
`workflow_dispatch` の `kernel_version: 7.0.6` を渡してください。

## Docker によるビルド

Docker のみ必要です。クロスコンパイラのインストールは不要です。

**Linux / macOS / WSL:**
```bash
./build.sh                                # デフォルト 6.12.17
./build.sh --kernel-version 7.0.6         # Linux 7.0.6 でビルド
```

**Windows (PowerShell):**
```powershell
.\build.ps1                                    # デフォルト 6.12.17
.\build.ps1 -KernelVersion 7.0.6               # Linux 7.0.6 でビルド
```

カーネルソースは kernel.org から自動的にダウンロードされます。
出力ファイルは `output/` に配置されます。

## ドキュメント

| ガイド | 説明 |
|--------|------|
| [フレームバッファディスプレイ セットアップ](docs/setup_framebuffer_ja.md) | Linux ゲストでの fbcon と X Window System のセットアップ |
| [Framebuffer Display Setup](docs/setup_framebuffer.md) | English version |

## 内容

### drivers/em68030fb/

EMFB 制御レジスタ (`$FFFE8000`) からフレームバッファパラメータを読み取り、
`simple-framebuffer` プラットフォームデバイスを登録するローダブルカーネルモジュールです。

エミュレータ設定で構成された任意の解像度/BPP で動作します。ハードコードされた値はありません。

**前提条件:**
- `CONFIG_FB_SIMPLE=y`（または `=m`）でビルドされたカーネル
- `CONFIG_TRIM_UNUSED_KSYMS` が無効 — このオプションはエクスポートされていないシンボル
  (`ioremap`, `platform_device_register` 等) を削除し、`insmod` が
  "Unknown symbol in module" で失敗する原因になります
- 確認: `zcat /proc/config.gz | grep FB_SIMPLE`

**ホスト上でのクロスコンパイル:**

エミュレートされた m68k ゲスト上でネイティブにカーネルモジュールをビルドするのは実用的ではありません。
カーネルビルドシステムがホストアーキテクチャ用のツール（`scripts/` 内）を必要とし、
ゲスト上では実行できないためです。代わりにホスト上でクロスコンパイルしてください。

**ホスト**（クロスビルド環境）上で、実行中のカーネルのビルドに使用したカーネルソースツリーを使用します。
カーネルは先に完全ビルド済み（`vmlinux`）である必要があります。
```bash
# カーネルを先にビルド（vmlinux が既にある場合はスキップ）
cd /path/to/linux-${KERNEL_VERSION}
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- vmlinux -j$(nproc)

# カーネル 6.12 以降はエクスポートシンボルを vmlinux.symvers に書き込みますが、
# ツリー外モジュールビルドは Module.symvers を読みます。古い/空の場合はコピー。
cp vmlinux.symvers Module.symvers

# モジュールをクロスコンパイル
cd /path/to/em68030-guest-linux/drivers/em68030fb
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- -C /path/to/linux-${KERNEL_VERSION} M=$(pwd) modules
```

生成された `em68030fb.ko` をゲストディスクイメージにコピーしてください。

**ゲスト上でのロード:**
```bash
insmod /path/to/em68030fb.ko
```

**確認:**
```bash
ls -la /dev/fb0
cat /dev/urandom > /dev/fb0   # エミュレータのフレームバッファウィンドウにノイズが表示されるはず
```

**自動ロード用インストール:**

ホスト上でインストールターゲットをクロスコンパイル:
```bash
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- -C /path/to/linux-${KERNEL_VERSION} M=$(pwd) \
  INSTALL_MOD_PATH=/path/to/guest-rootfs modules_install
```

またはゲスト上で手動:
```bash
mkdir -p /lib/modules/$(uname -r)/extra
cp em68030fb.ko /lib/modules/$(uname -r)/extra/

# メタデータファイルが存在しない場合は作成（"make vmlinux" のみではインストールされない）
cd /lib/modules/$(uname -r)
touch modules.order modules.builtin modules.builtin.modinfo

depmod -a

# systemd (Debian, Gentoo systemd):
echo em68030fb > /etc/modules-load.d/em68030fb.conf

# OpenRC (Gentoo OpenRC):
# /etc/conf.d/modules に modules="em68030fb" を追加
```

### drivers/em68030input/

EMKM 制御レジスタ (`$FFFE9000`) からイベントを読み取り、仮想キーボードおよびマウス入力デバイスを
提供するローダブルカーネルモジュールです。

エミュレータホストがフレームバッファウィンドウでキーボードおよびマウスイベントをキャプチャし、
MMIO イベント FIFO にプッシュします。このドライバは 100 Hz で FIFO をポーリングし、
Linux 入力サブシステム (`evdev`) 経由でイベントを報告します。

3 つの入力デバイスを登録します:
- **キーボード** (`/dev/input/event0`): Linux `KEY_*` コード
- **タブレット** (`/dev/input/event1`): X Window System / libinput 用の絶対座標
- **マウス** (`/dev/input/event2`): 絶対座標の差分から計算した相対座標（gpm によるfbcon テキスト選択用）

**前提条件:**
- `CONFIG_INPUT=y` および `CONFIG_INPUT_EVDEV=y`（または `=m`）でビルドされたカーネル
- `CONFIG_TRIM_UNUSED_KSYMS` が無効
- エミュレータ設定でフレームバッファが有効（EMKM デバイスはフレームバッファがアクティブな場合のみ存在）

**ホスト上でのクロスコンパイル:**
```bash
cd /path/to/em68030-guest-linux/drivers/em68030input
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- -C /path/to/linux-${KERNEL_VERSION} M=$(pwd) modules
```

生成された `em68030input.ko` をゲストディスクイメージにコピーしてください。

**ゲスト上でのロード:**
```bash
insmod /path/to/em68030input.ko
```

**確認:**
```bash
# "Em68030 Virtual Keyboard" と "Em68030 Virtual Mouse" が表示されるはず
cat /proc/bus/input/devices
```

**自動ロード用インストール:**
```bash
mkdir -p /lib/modules/$(uname -r)/extra
cp em68030input.ko /lib/modules/$(uname -r)/extra/
depmod -a

# systemd (Debian, Gentoo systemd):
echo em68030input > /etc/modules-load.d/em68030input.conf

# OpenRC (Gentoo OpenRC):
# /etc/conf.d/modules に modules="em68030fb em68030input" を追加
```

### patches/

Em68030 固有の MVME147 サポート用カーネルソースパッチです。カーネル
メジャーバージョンごとに `patches/linux-6/` / `patches/linux-7/` に
分かれており、ビルド基盤は `KERNEL_VERSION` のメジャー成分に対応する
ディレクトリを自動的に選択します。

#### `mvme147-uart-16550.patch`

MVME147 ボード構成 (`arch/m68k/mvme147/config.c`) に仮想 16550A UART プラットフォーム
デバイスを `$FFFE2000` で登録します。実際の MVME147 はシリアル I/O に Z8530 SCC を使用しますが、
アップストリーム Linux カーネルには MVME147 向けの Z8530 ベース tty ドライバがありません。
このパッチにより、広くサポートされている `8250/16550` シリアルドライバでユーザースペース
コンソール (`/dev/ttyS0`) を提供できるようになります。

このパッチがない場合、カーネルは起動し `earlyprintk` は動作しますが、ユーザースペースに
コンソールデバイスがありません (`Warning: unable to open an initial console`)。

Linux 7.0 は MVME147 config の include 群に `linux/platform_device.h`
を取り込み、`linux/rtc.h` を `linux/rtc/m48t59.h` に置き換えたため、
パッチを 6 系と 7 系で別々に保持しています
([`linux-6/`](patches/linux-6/) / [`linux-7/`](patches/linux-7/))。

**適用方法:**
```bash
cd /path/to/linux-${KERNEL_VERSION}
KERNEL_MAJOR=${KERNEL_VERSION%%.*}
patch -p1 < /path/to/em68030-guest-linux/patches/linux-${KERNEL_MAJOR}/mvme147-uart-16550.patch
```

**必要なカーネル設定:**
- `CONFIG_SERIAL_8250=y`
- `CONFIG_SERIAL_8250_CONSOLE=y`

カーネルビルドの完全な手順については、エミュレータリポジトリの getting started ガイド
(`getting_started_debian.md` / `getting_started_gentoo.md`) も参照してください。

### configs/

`mvme16x_defconfig` ベースに Em68030 固有のオプションを有効化した
保存済み `.config` ファイルです。先頭行の `# Linux/m68k 6.12.17 Kernel
Configuration` は採取時のバージョンを示しており、新しいカーネル
(7.0.x 等) でビルドする際は `make olddefconfig` が差分を自動吸収します。
`menuconfig` を手動で実行する代わりに、カーネルソースツリーに `.config`
としてコピーできます。

| ファイル | 説明 |
|---------|------|
| `.config.20260308_BASIC_CONFIG` | 最小構成: UART + ネットワーク (`CONFIG_MVME147_NET=y`)、フレームバッファなし |
| `.config.20260312_FB_SIMPLE` | フル構成: UART + フレームバッファ + ネットワーク |
| `.config.20260314_FB_CONSOLE_with_INPUT_EVDEV` | フル構成 + fbcon + evdev 入力（キーボード/マウス用） |

**有効化されている主要オプション（全設定共通）:**
- `CONFIG_M68030=y`, `CONFIG_MVME147=y` — CPU およびボードサポート
- `CONFIG_SERIAL_8250=y`, `CONFIG_SERIAL_8250_CONSOLE=y` — 仮想 UART コンソール
- `CONFIG_MVME147_SCSI=y`, `CONFIG_BLK_DEV_SD=y` — SCSI ディスクサポート
- `CONFIG_EXT4_FS=y` — ルートファイルシステム

**使用方法:**
```bash
cp /path/to/em68030-guest-linux/configs/.config.20260312_FB_SIMPLE \
   /path/to/linux-${KERNEL_VERSION}/.config
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- olddefconfig
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- vmlinux -j$(nproc)
```

## 謝辞

このプロジェクトは、Linux/m68k を現代のシステム上で機能させ続けている Linux カーネル
コミュニティと、下流のディストリビューションコミュニティの継続的な活動に依存しています。
以下のプロジェクトとそのメンテナーの方々に、心より感謝申し上げます:

- **Linux カーネルコミュニティ** — 特に m68k サブシステムのメンテナーの方々と
  `linux-m68k` メーリングリストの貢献者の皆様 — に感謝します。このリポジトリの
  out-of-tree モジュールおよび in-tree パッチは、広く Linux カーネルコミュニティに
  よって維持されている各サブシステム (fbdev, evdev, シリアル, SCSI) に直接
  結合しています。http://www.linux-m68k.org/

- **The Debian Project / Debian Ports チーム** — このリポジトリのカーネルモジュールと
  パッチの消費元である Debian/m68k unofficial port を維持していただいていることに
  感謝します。Debian コミュニティ全体が「リリース対象外アーキテクチャも歓迎する」
  という方針を取り続けていただいているからこそ、この活動に実用的な意味があります。
  https://www.ports.debian.org/

- **The Gentoo Project / Gentoo m68k チーム** — m68k 向け stage3 tarball の定期的な
  公開と、portage ツリーの m68k 対応の維持に感謝します。Gentoo はこのリポジトリで
  開発されるドライバのもう一つの下流ターゲットです。
  https://wiki.gentoo.org/wiki/M68k

皆様の継続的な活動によって、このプロジェクトが成立しています。

## 関連プロジェクト

- [Em68030_WinUI3Cpp](https://github.com/hha0x617/Em68030_WinUI3Cpp) — エミュレータ (C++/WinUI3)
- [Em68030_CsWPF](https://github.com/hha0x617/Em68030_CsWPF) — エミュレータ (C#/WPF)
- [Em68030-Guest-NetBSD](https://github.com/hha0x617/Em68030-Guest-NetBSD) — NetBSD ゲストコンポーネント

## 貢献とポリシー

- 貢献ワークフロー: [`CONTRIBUTING.md`](CONTRIBUTING.md)
- 行動規範: [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md)（Contributor Covenant 2.1 準拠）
- セキュリティ: [`SECURITY.md`](SECURITY.md)
