# MoneyUnderBed DeskPet (unofficial)

[中文](README.md) | English

An unofficial fan-made desktop pet featuring the heroine of the Chinese game
《床下有罐钱》 (MoneyUnderBed). She walks around your desktop, rests and says a few
lines on her own, and you can click, drag and feed her.

**[Download the latest release](https://github.com/HZ-TYZQ/MoneyUnderBed-DeskPet/releases/latest)**

## Features

- Four-direction movement with pixel-art animation.
- Click, drag and ice cream feeding interactions.
- Short lines and conversations true to the original character.
- Adjustable activity, dialogue, animation, display scale and window behavior.
- Hide to the tray, bring her back by launching the program again, and recovery
  after screen lock and sleep.
- The interface follows the system language: Simplified Chinese on a Chinese
  system, English everywhere else. The character's lines are always in Chinese.

## Download and run

### Windows 11

Download the `windows-x86_64` ZIP, extract all of it and run
`money-under-bed-deskpet.exe`.

The program is not signed yet. Windows may show a SmartScreen warning, and Smart
App Control or enterprise policies may block it.

### Linux

Download the `x86_64.AppImage`, make it executable and run it:

```bash
chmod +x MoneyUnderBed-DeskPet-*.AppImage
./MoneyUnderBed-DeskPet-*.AppImage
```

KDE Plasma on XCB/XWayland is officially supported. The program uses the Qt XCB
backend; niri and the native Qt Wayland backend are not supported.

## Support

| Environment | Status | Verified |
| --- | --- | --- |
| Windows 11 x86-64 | Supported | 100% / 125% / 150% / 200% scaling, Explorer restart, screen lock and sleep recovery |
| KDE Plasma + XCB/XWayland | Supported | Fedora 44, Plasma 6.7.4, 125% scaling; the release candidate ran for three hours straight |
| GNOME | Experimental | GNOME 50.5 tested OK (1.2.0) |
| Multiple monitors, hot-plugging, mixed DPI | Best effort | Not tested yet |
| niri, native Qt Wayland | Not supported | — |

The full acceptance record is in the [1.1.0 checklist](docs/ReleaseChecklist-1.1.md)
(in Chinese).

## Unofficial notice and licenses

This is an **unofficial, non-commercial fan project**. It is not affiliated with,
published by, or endorsed by the developers of MoneyUnderBed.

The character assets come from the
[fan-creation asset pack](https://www.bilibili.com/video/BV1XwhV6TEXQ/) published
by `_U5B_` and may be used for fan creations only: **no commercial use, no R18
content, no use for AI training**. The character assets are not covered by the
GPL; see [assets/LICENSE.md](assets/LICENSE.md) (in Chinese) for the full terms.

| Content | License |
| --- | --- |
| Program code and project documentation | [GPL-3.0-or-later](LICENSE) |
| Ark Pixel dialogue font | [OFL-1.1](third_party/ark-pixel-font/OFL.txt) |
| Character assets | [The author's fan-creation terms](assets/LICENSE.md) |

Not everything in this repository is under the same license; see
[packaging/LICENSES.md](packaging/LICENSES.md) (in Chinese) for the full picture.

## Building

Requires CMake `3.21+`, Ninja and Qt `6.11` (including the Qt Linguist tools,
which compile the UI translations). CI uses Qt `6.11.2`.

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Product decisions, development notes and release acceptance records are in
Chinese: [Decisions](docs/Decisions.md) (the original version is archived as
[legacy/Decisions](docs/legacy/Decisions.md)),
[DevelopmentStatus-1.1](docs/Plans/DevelopmentStatus-1.1.md) and
[ReleaseChecklist-1.1](docs/ReleaseChecklist-1.1.md). Known issues that are not
fixed yet are listed in [known_issue](docs/known_issue.md).
