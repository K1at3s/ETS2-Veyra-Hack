# Veyra 0.9

Native C++20 / Windows x64 panel with Turkish and English UI.
Independent project; not affiliated with HaulMP or SCS Software.

[Türkçe](#türkçe) · [English](#english) · [Detailed guide](BUILDING.md)

## Türkçe

### Özellikler

- **F6:** deneysel yerel geçiş filtresini aç/kapat.
- **F8:** pencereyi üstte tutmayı aç/kapat.
- Otomatik ETS2 süreç algılama, TR/EN dil seçimi ve işlem günlüğü.
- Bu sürümde hızlandırma, speedhack veya hız sınırı kaldırma bulunmaz.

### Kurulum ve kullanım

1. GitHub'da **Code → Download ZIP** seçin ve kaynakları bir klasöre çıkarın.
2. Kaynak kod doğrudan çalışmaz; aşağıdaki yöntemlerden biriyle derleyin.
3. Önceki Veyra/trainer kopyalarını kapatın; aynı kısayollar çakışabilir.
4. Derlediğiniz **veyra.exe** dosyasını çalıştırın. Süreç algılama otomatiktir.
5. Sağ üstten TR/EN seçin. Filtre başlangıçta **kapalıdır**; F6 veya düğmesiyle kontrol edilir.
6. Çıkarken normal kapatma yolunu kullanın. Geri yükleme hatası varsa gösterilen mesajı inceleyin.

Oyun kapalıyken arayüz açılabilir. Deneysel filtre yalnızca aşağıda belirtilen
incelenmiş HaulMP sürümünü kabul eder. Sunucu adı kontrolü yoktur.
Yönetici yetkisi otomatik istenmez; erişim hataları arayüzde gösterilir.

### Neyle geliştirildi ve derlendi?

| Bileşen | Araç |
| --- | --- |
| Dil / hedef | C++20 / Windows x64 |
| Yerel derleyici | Visual Studio 2026, MSVC 14.51, /W4 /WX |
| Windows SDK | 10.0.26100.0 |
| Arayüz | Win32, GDI+, DWM |
| Simge / sürüm kaynağı | Windows Resource Compiler, rc.exe |
| Derleme | MSVC betiği veya CMake 3.24+ |

Visual Studio 2022/2026 veya Build Tools yükleyicisinde **C++ ile masaüstü
geliştirme / Desktop development with C++**, x64 MSVC araçları ve Windows
10/11 SDK seçili olmalıdır. CMake yalnızca CMake yöntemi için gereklidir.
Standart derleme için Python veya üçüncü taraf UI kütüphanesi gerekmez.

### Yöntem 1 — CMake olmadan

Başlat menüsünden **x64 Native Tools Command Prompt for Visual Studio** açın.
Kaynak klasörüne gidin; örnek yolu kendi klasörünüzle değiştirin:

~~~bat
cd /d "KAYNAK_KLASORUNUZ"
build-msvc.cmd
build\veyra.exe
~~~

Betik uygulamayı, simge kaynağını ve iki testi derler; testleri çalıştırır.
Uygulama çıktısı: **build\veyra.exe**.

### Yöntem 2 — CMake

~~~powershell
git clone https://github.com/K1at3s/ETS2-Veyra-Hack.git
cd ETS2-Veyra-Hack
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\veyra.exe
~~~

ZIP'ten çıkardıysanız ilk iki komutu atlayıp kaynak kökünde çalışın.
Bu yöntemin uygulama çıktısı: **build\Release\veyra.exe**.

### Ayarlar ve sorun giderme

- Dil tercihi: %LOCALAPPDATA%\Veyra\settings.ini.
- cl bulunamıyorsa x64 Native Tools penceresini kullanın.
- windows.h veya rc bulunamıyorsa C++ iş yükünü ve Windows SDK'yı kontrol edin.
- Kısayol kullanımda mesajı varsa diğer panel/trainer kopyalarını kapatın.
- Modül/kod eşleşmiyorsa yazma reddedilir; sürüm kontrolünü kaldırmayın.
- Normal kapanış orijinal baytı geri yüklemeyi dener ve doğrular. Zorla kapatmada geri alma garanti değildir.
- Geçiş kullanıcı tarafından bildirildi; kararlılık ve karşı istemci sonuçları bağımsız doğrulanmadı.
  Önceki, farklı bir deneysel yamada çökme bildirildi.

## English

### Features and installation

**F6** toggles the experimental local pass-through filter. **F8** pins/unpins
the window. Automatic process detection, TR/EN selection and activity history
are included. There is no acceleration, speedhack or speed-limit bypass.

Download **Code → Download ZIP**, extract the sources and build them. A source
ZIP is not an installer. Close older panels, launch the compiled executable,
choose a language and check game detection. The filter starts disabled.
Close normally to attempt verified restoration.

The UI can run without the game. Only the inspected HaulMP build below is
accepted. There is no server-name check or automatic elevation.

### Requirements and build

Install Visual Studio 2022/2026 or Build Tools with **Desktop development with
C++**, an x64 MSVC toolset and Windows SDK. CMake 3.24+ is optional. The
application uses Windows' built-in Win32, GDI+ and DWM; no third-party UI
runtime or Python is required.

From an **x64 Native Tools Command Prompt**, enter the source root and run:

~~~bat
build-msvc.cmd
build\veyra.exe
~~~

Alternatively, from a terminal in the source root:

~~~powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\veyra.exe
~~~

The direct MSVC output is **build\veyra.exe**. The CMake Release output is
**build\Release\veyra.exe**. Local verification used Visual Studio 2026,
MSVC 14.51, Windows SDK 10.0.26100.0, C++20 and /W4 /WX. Both tests passed.
CMake was unavailable locally; its workflow has not been verified by a local
CMake run. The memory fixture modifies only its own process, never ETS2.

### Settings and limitations

Language preference: %LOCALAPPDATA%\Veyra\settings.ini. Low-level backend
errors retain their original English text. Missing cl, rc or Windows headers
usually indicates the wrong terminal or missing SDK/tools. Close older panels
if hotkeys are already registered.

Pass-through was user-reported; stability and remote effects were not
independently verified. A previous, different experimental patch had a crash
report. Forced termination cannot guarantee restoration. A successful build
does not establish game compatibility.

## Supported experimental build / Desteklenen sürüm

Inspected HaulMP **868**, haulmp_stub.dll, 9,697,264 bytes, SHA-256:

~~~text
b0facd6fe0b1eee0f44addd7bda8aea33f17d1bb85ce0d86c01f21cf05c16cc5
~~~

The module hash and loaded instruction context must match. The legacy NCZ
override must be absent. No game binaries, private logs, dumps or third-party
trainer binaries are bundled.

## Source layout / Kaynak yapısı

| Path | Purpose |
| --- | --- |
| src/main.cpp | UI, language selection, hotkeys |
| src/local_pass_patch.hpp | Version-checked experimental filter |
| src/core.hpp | Process-name matching |
| tests/ | Local process-name and memory-operation tests |
| assets/ | Original Veyra SVG and multi-size ICO |
| app.rc | Embedded icon and version resources |
| tools/create-icon.ps1 | Optional icon regeneration |
| build-msvc.cmd | Direct MSVC build and tests |
| CMakeLists.txt | CMake targets and CTest |
| .github/workflows/build.yml | Windows build/test workflow |
| publish-github.ps1 | Explicit source publication script |

## Publish / GitHub'a gönderme

Install Git for Windows and sign in through Git's authentication flow when
requested. You need push permission for the repository. Do not put passwords
or tokens in this script.

From this source folder:

~~~powershell
.\publish-github.ps1 -WhatIf
.\publish-github.ps1
~~~

The first command previews the action. The second clones the repository into
a fresh staging folder, copies only the listed project sources, commits and
pushes normally. No force-push is used and unrelated remote files are not
deleted. Concurrent remote changes may cause a rejected push; the script stops.

Türkçe: Betik kaynakları ayrı bir klasörde hazırlar ve normal commit/push
yapar. Derleme çıktıları, kişisel ayarlar, günlükler ve dökümler gönderilmez.
Git kimlik doğrulamasını Git'in giriş akışında tamamlayın. Eksik commit kimliği
için betik açıklayıcı hata verir. Hedef:

https://github.com/K1at3s/ETS2-Veyra-Hack.git

A redistribution license has not been selected. Choose one before release.
Further notes: [BUILDING.md](BUILDING.md).
