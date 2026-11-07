# Derleme / Building

## Türkçe

### Gerekli araçlar

1. Windows x64.
2. Visual Studio 2022 veya 2026, ya da Visual Studio Build Tools.
3. Yükleyicide **C++ ile masaüstü geliştirme / Desktop development with C++**
   iş yükü: MSVC x64 derleyici ve Windows 10/11 SDK seçili olmalı.
4. CMake yolunu kullanacaksanız CMake 3.24 veya üzeri. Betik yolunda CMake gerekmez.

Oyun veya HaulMP derlemek için gerekli değildir. Arayüzü oyun kapalıyken
açabilirsiniz. Deneysel filtre yalnızca README'deki belirli modül sürümünü
kabul eder; derlemenin başarılı olması başka oyun sürümünü desteklemez.

### Yol A — CMake

Kaynak ZIP'ini çıkarın. `CMakeLists.txt` bulunan klasörde terminal açın:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\veyra.exe
```

Oluşan uygulama: `build\Release\veyra.exe`.
Varsayılan Visual Studio kurulumunu CMake seçer; derleyici nesli için
makinenize özel bir klasör yolunu kaynaklara yazmanız gerekmez.

### Yol B — CMake olmadan

Başlat menüsünden Visual Studio'nun **x64 Native Tools Command Prompt**
penceresini açın. Kaynak klasörüne gidip çalıştırın:

```bat
cd /d "KAYNAKLARI_CIKARDIGINIZ_KLASOR"
build-msvc.cmd
build\veyra.exe
```

Klasör örneğini kendi yolunuzla değiştirin. Betik uygulamayı ve iki testi
derler, testler başarısızsa hata koduyla durur. Çıktı `build\veyra.exe`.

### Dosya yapısı

```text
src/main.cpp                  Arayüz, dil seçimi, kısayollar
src/local_pass_patch.hpp      Sürüm kontrollü deneysel filtre
src/core.hpp                  Süreç adı/desteklenmeyen özellik durumu
tests/core_tests.cpp           Süreç adı ve durum kontrolleri
tests/local_pass_fixture.cpp   Yalnız kendi belleğinde yazma/geri alma testi
CMakeLists.txt                CMake hedefleri ve CTest
build-msvc.cmd                MSVC ile doğrudan derleme
.github/workflows/build.yml   Windows üzerinde CI derleme/testi
```

### Sık sorunlar

- **cl bulunamıyor:** normal terminal yerine x64 Native Tools penceresini kullanın.
- **windows.h bulunamıyor:** Windows SDK ve C++ iş yükünü yükleyin.
- **EXE değiştirilemiyor:** önce çalışan eski uygulamayı normal şekilde kapatın.
- **Unsupported module / Code mismatch:** sürüm uyumsuzluğu veya başka yama var;
  sürüm/bağlam kontrolünü kaldırmak çözüm değildir.
- **Hız alanı boş:** telemetri sağlayıcısı henüz entegre değil.
- Windows 10'da Windows 11 pencere köşesi/başlık rengi seçenekleri sistem
  tarafından desteklenmeyebilir; uygulama bunlar olmadan devam eder.

### GitHub'da paylaşma

Bu klasörün içeriğini depo kökü yapın. `build/` klasörünü, kişisel ayarları,
oyun dosyalarını, dökümleri veya günlükleri kaynak depoya koymayın.
`.gitignore` derleme çıktılarını hariç tutar. Hazır EXE isterseniz ayrı bir
GitHub Release eki olabilir. `.github/workflows/build.yml` push ve pull
request üzerinde derleyip test eder; otomatik yayın yapmaz.

Yayın öncesi README'deki özellik/sürüm sınırlarını koruyun ve kullanmak
istediğiniz lisansı ekleyin. Bu paket lisans seçmez. GitHub'a otomatik
yükleme veya depo oluşturma yapılmadı.

## English

Install Visual Studio 2022/2026 or Build Tools with **Desktop development
with C++**, an x64 MSVC toolset and Windows SDK. No external UI dependencies
are required. CMake 3.24+ is optional when using the direct MSVC script.

From the repository root, use the CMake commands above. The Release binary
is `build\Release\veyra.exe`. Alternatively open an **x64 Native
Tools Command Prompt**, enter the source folder and run `build-msvc.cmd`.
That route outputs `build\veyra.exe` and runs both tests.

Close older copies before rebuilding or launching. TR/EN switches the UI
and stored activity immediately; the preference is saved under
`%LOCALAPPDATA%\Veyra\settings.ini`. Windows 11 provides the rounded
outer window; unsupported DWM styling is optional on older systems.

validate game stability, remote physics effects or other module versions.
The memory fixture modifies only its own process, never the game.

Upload source files as the repository root. Keep generated binaries,
private settings, logs, dumps and game files out of source control. Choose
a license before publishing. The CI workflow builds and tests; it does not
publish a release. No repository has been created automatically.

## Verification

### Icon resources

`app.rc` embeds the bundled `assets/veyra.ico` and version information.
The MSVC script invokes Windows SDK `rc.exe` before linking; CMake compiles
the RC source directly. If `rc` is missing, install/select the Windows SDK.
No icon generation step is required for a normal build.

To regenerate the original multi-size icon from its matching vector design,
run the bundled drawing script on Windows:

```powershell
powershell -NoProfile -File .\tools\create-icon.ps1
```

The icon contains 16, 24, 32, 48, 64, 128 and 256 pixel PNG images.
Both the window icon and the visible panel mark use the embedded resource.
The SVG and drawing script are original assets included with this project.

Local: MSVC x64 / C++20 /W4 /WX, both tests passed. CMake was unavailable
locally; its commands and the GitHub workflow have not been executed here.
UI changes do not modify the filter implementation.
