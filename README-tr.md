# RC Controller

[English](README.md) | [Türkçe](README-tr.md)

**RC Controller, Bluetooth gamepad ile bir RC aracı sürmek için geliştirilmiş ESP32 firmware'idir.** Bluepad32 üzerinden alınan gamepad girdilerini sürüş komutlarına dönüştürür; araç durum mantığını, motor/servo/aydınlatma donanım sürücüsünü, titreşim geri bildirimini ve opsiyonel OTA güncellemeyi ayrı bileşenlerde tutar.

Mevcut firmware şu işlevleri kontrol eder:

- ileri ve geri motor sürüşü;
- gaz ve fren birlikte basıldığında aktif frenleme;
- direksiyon servosu ve kalıcı direksiyon trimi;
- dört yazılımsal vites seviyesi;
- kısa far, uzun far, selektör, fren lambası, geri vites lambası, sinyaller ve dörtlü;
- gamepad renk LED'i ve titreşim geri bildirimi;
- 500 ms komut failsafe'i;
- Arduino OTA güncellemesi için opsiyonel Wi-Fi erişim noktası.

## Mimari

```text
Bluetooth gamepad
       |
       v
    Bluepad32
       |
       v
RC_CONTROLLER.ino
  girdi eşleme / bağlantı yönetimi
       |
       v
RcCarController
  araç durumu / vites / ışıklar / failsafe
       |
       +--------------------+
       |                    |
       v                    v
RcHardwareDriver       RumbleManager
motor / servo /        gamepad titreşimi
aydınlatma çıkışları   sanal dinamiklerden
       |
       v
 RC araç donanımı
```

`OTA.h`, sürüş mantığından bağımsız opsiyonel Wi-Fi AP ve ArduinoOTA yolunu sağlar.

## Donanım

Firmware, Bluetooth Classic / Bluepad32 ile uyumlu bir ESP32 ortamını hedefler. Desteklenen bir gamepad, EN/IN1/IN2 kontrollü motor sürücüsü, direksiyon servosu ve uygun aydınlatma sürücü devreleri gerekir.

**Motoru veya direksiyon servosunu doğrudan ESP32 GPIO pinlerinden beslemeyin.** Gerçek motor, servo ve lambalar için uygun güç ve sürücü donanımı kullanın.

### Pin Atamaları

| İşlev | GPIO |
|---|---:|
| Motor EN / PWM | 12 |
| Motor IN1 | 14 |
| Motor IN2 | 15 |
| Direksiyon servosu | 4 |
| Sol sinyal | 5 |
| Sağ sinyal | 17 |
| Kısa far | 33 |
| Fren lambası | 32 |
| Uzun far | 2 |
| Geri vites lambası | 0 |

Tüm aydınlatma çıkışları **aktif düşük** çalışır: aktif lamba LOW, pasif lamba HIGH yazılarak sürülür.

## PWM Yapılandırması

### Motor

```text
Frekans: 20 kHz
Çözünürlük: 8 bit
PWM kanalı: 2
```

Motor komutu işaretlidir:

- pozitif: ileri;
- negatif: geri;
- sıfır: motor PWM yok.

Sıfırdan farklı sürüş komutunda donanım sürücüsü PWM aralığını yeniden map eder; alt çıkış 127'den başlarken üst sınır seçili vites ölçeğine göre belirlenir.

### Direksiyon Servosu

```text
Frekans: 50 Hz
Çözünürlük: 16 bit
PWM kanalı: 1
Temel pulse aralığı: 900–2000 us
```

Direksiyon komutu `-1.0 ... +1.0` aralığından servo pulse aralığına map edilir ve ardından kayıtlı trim değeri eklenir.

İstenen duty değişmeyi bıraktığında mevcut implementasyon servo PWM'ini 500 ms daha aktif tutar ve ardından servoyu serbest bırakmak için duty değerini sıfıra çeker.

Pulse aralığı ve trim, kullanımdan önce gerçek mekanik direksiyon sınırlarıyla karşılaştırılmalıdır.

## Gamepad Kontrolleri

Aşağıdaki isimler Bluepad32'nin mantıksal girdileridir; fiziksel tuş sembolleri gamepad modeline göre değişebilir.

| Girdi | İşlev |
|---|---|
| `throttle()` | İleri gaz |
| `brake()` | Tek başına geri sürüş; gazla birlikte tam fren isteği |
| Sol analog X / `axisX()` | Direksiyon |
| X | Kısa far → uzun far → kapalı döngüsü |
| A, basılı tutulurken | Selektör / geçici uzun far |
| L1 | Sol sinyali aç-kapat |
| R1 | Sağ sinyali aç-kapat |
| Y | Dörtlüyü aç-kapat |
| D-pad Yukarı | Vites büyüt |
| D-pad Aşağı | Vites küçült |
| `miscButtons() & 0x04` + D-pad Sağ | Direksiyon trimi +5 us |
| `miscButtons() & 0x04` + D-pad Sol | Direksiyon trimi -5 us |
| `miscButtons() & 0x02` | OTA erişim noktasını aç-kapat |

Firmware birinci viteste başlar. İkinci vitesten daha yukarı çıkmak için D-pad Yukarı ile birlikte `miscButtons() & 0x04` basılı olmalıdır.

### Vites Geri Bildirimi

| Vites | Motor ölçeği | Gamepad LED |
|---|---:|---|
| 1 | %60 | Yeşil |
| 2 | %70 | Sarı |
| 3 | %80 | Turuncu |
| 4 | %100 | Kırmızı |

LED rengi, D-pad vites değiştirme tuşu bırakıldığında güncellenir.

## Direksiyon Trimi

Trim, ESP32 `Preferences` ile saklanır:

```text
namespace: rc_cfg
key: steer_trim
```

Her trim komutu kayıtlı değeri 5 µs değiştirir. Değer açılışta yüklenir ve üretilen servo pulse genişliğine doğrudan eklenir.

Kaynak kod trim değerini şu anda sınırlandırmaz; bu nedenle çok sayıda ardışık ayar, oluşan pulse değerini nominal 900–2000 µs aralığının dışına taşıyabilir.

## Aydınlatma

Sinyaller ortak 400 ms toggle periyodu kullanır. Dörtlü modu, tekil sol/sağ sinyallerden daha yüksek önceliğe sahiptir.

Tekil sinyal açıkken direksiyon hareketi sinyali otomatik iptal edebilir:

```text
aktivasyon eşiği: eşleşen yönde |steering| > 0.35
iptal eşiği:      aktivasyondan sonra |steering| < 0.10
```

Sol sinyal açıldığında sağ sinyal, sağ sinyal açıldığında sol sinyal kapatılır.

Fren ve geri vites lambalarının durumları da mevcut sürüş/fren komutuna göre motor donanım sürücüsü tarafından güncellenir.

## Sürüş ve Fren Mantığı

`RcCarController::computeMotorOutput()` iki analog tetik girdisini şu şekilde yorumlar:

| Gaz | Fren | Sonuç |
|---|---|---|
| Aktif | Pasif | İleri sürüş |
| Pasif | Aktif | Geri sürüş |
| Aktif | Aktif | Tam fren komutu |
| Pasif | Pasif | Serbest sürüş / sıfır motor komutu |

Her iki tetik için aktiflik eşiği `0.05` değeridir.

### Fren Profili

Tam fren komutu aktif olduğunda donanım sürücüsü motoru son hareket yönünün tersine sürer.

Tanımlı PWM profilleri:

| Vites | Başlangıç PWM | Tutma PWM |
|---|---:|---:|
| 1 | 180 | 120 |
| 2 | 200 | 120 |
| 3 | 220 | 120 |
| 4 | 240 | 120 |

Ramp süresi sabiti 8000 ms'dir. Sanal hız 10'un üzerindeyken ve ramp süresi dolmamışken PWM, vitese özgü başlangıç değerinden 120'ye doğru azalır; aksi halde tutma değeri kullanılır.

Bu, motoru ters yönde aktif sürmeye dayalı bir frenleme stratejisidir; belirli bir fiziksel fren kuvveti garantisi değildir. Gerçek motor sürücüsü ve mekanikle uyumluluğu ayrıca değerlendirilmelidir.

## Sanal Dinamikler ve Titreşim

Firmware yazılımsal `virtualSpeed` ve `virtualAccel` değerleri tutar. Ana uygulama fiziksel bir araç hız sensörü okumaz.

Model:

- sürüş sırasında motor komutuyla orantılı ivmelenme;
- tam frende güçlü modellenmiş yavaşlama;
- serbest sürüşte modellenmiş yuvarlanma direnci;
- `-5.0 ... +5.0` aralığında sınırlandırılmış ivme

kullanır.

Bu değerler `RumbleManager` bileşenine aktarılır.

Mevcut titreşim tetikleri:

| Koşul | Efekt |
|---|---|
| Sanal ivme > 2.5 | Kalkış/patinaj titreşimi |
| Sanal ivme < -2.5 | Fren kayması titreşimi |
| Mutlak sanal ivme > 4.9 | Kısa vites vuruntusu benzeri titreşim |

Titreşim çağrıları arasında en az 40 ms olacak şekilde rate limit uygulanır. Bu efektler ölçülmüş tekerlek kayması veya fiziksel ivmeden değil, sanal modelden türetilir.

## Sinyal Otomatik İptali

Sağ sinyal, direksiyon `+0.35` değerini geçtiğinde; sol sinyal ise `-0.35` altına indiğinde armed duruma gelir. Armed olduktan sonra direksiyonun `±0.10` içine dönmesi ilgili sinyali kapatır.

Bu yöntem fiziksel direksiyon açı sensörü olmadan, direksiyon dönüşünden sonra sinyal iptalini taklit eder.

## Failsafe

Her gaz, fren veya direksiyon güncellemesi girdi zaman damgasını yeniler. Varsayılan failsafe süresi:

```text
500 ms
```

Timeout oluşursa controller:

- gaz ve fren girişlerini sıfırlar;
- yazılımsal direksiyon komutunu merkeze getirir;
- motor ve fren komutlarını sıfırlar;
- failsafe durumunu aktif eder;
- fren lambası durumunu açar;
- dörtlüyü açar.

Böylece donanım motor komutu sıfıra gider. Bu davranış garanti edilmiş mekanik veya aktif elektriksel fren olarak yorumlanmamalıdır.

## Bluetooth Davranışı

Açılışta Bluepad32 / BTstack allowlist, `BP32.setup()` çağrısından önce başlatılır ve etkinleştirilir.

Bir controller bağlandığında Bluetooth adresi allowlist'e eklenir ve ilk boş `BP32_MAX_GAMEPADS` slotuna kaydedilir. Sürüş kodu yalnız gamepad tipindeki controller'ları işler.

Kaynakta Bluetooth anahtarlarını unutma ve yeni bağlantıları etkinleştirme çağrıları yorum satırı durumundadır. Bu nedenle ilk eşleştirme/provisioning davranışı kullanılan Bluepad32 sürümüne ve derleme ortamına da bağlıdır.

`set_max_bt_tx_power()` fonksiyonu kaynakta bulunur ancak `setup()` tarafından çağrılmaz.

## OTA Güncelleme Modu

OTA açılışta pasiftir. İlgili gamepad misc tuşu OTA'yı açıp kapatır.

`OTA::begin()` etkinleştirildiğinde:

1. `RcController-` öneki ile ESP32 Bluetooth adresinin son üç byte'ından hostname üretir;
2. Wi-Fi'yi AP moduna geçirir;
3. SoftAP başlatır;
4. ArduinoOTA'yı yapılandırır;
5. core 0'a pinlenmiş ve sürekli `ArduinoOTA.handle()` çağıran bir FreeRTOS task oluşturur.

Ana sketch'teki mevcut çağrı AP'yi şu SSID ile başlatır:

```text
SSID: RcController
```

Kimlik doğrulama değerleri mevcut kaynak kodda doğrudan bulunmaktadır. OTA erişiminin önemli olduğu bir ortamda firmware'i kullanmadan önce bu bilgileri inceleyin ve değiştirin.

OTA kapatıldığında OTA task'ı silinir, ArduinoOTA durdurulur, SoftAP kapatılır ve Wi-Fi tamamen kapatılır.

## Derleme

Depo kesin ESP32 core veya Bluepad32 sürümlerini sabitlemez.

Derlemek için:

1. Bluepad32 ve kaynakta kullanılan `uni.h` API'sini sağlayan Arduino uyumlu bir ESP32 geliştirme ortamı kullanın.
2. Tüm proje dosyalarını `RC_CONTROLLER.ino` ile aynı Arduino sketch klasöründe tutun.
3. Donanım sürücüsündeki `ledcSetup()` ve `ledcAttachPin()` API'leriyle uyumlu bir ESP32 core kullanın.
4. Gerçek ESP32 kartını seçip sketch'i derleyin ve yükleyin.
5. Seri tanılama **115200 baud** kullanır.

İlk testte sürülen tekerlekleri zeminden kaldırın; aracı sürmeden önce motor yönünü, direksiyon merkezini/uç noktalarını, ışık polaritesini, fren davranışını ve failsafe'i doğrulayın.

## Mevcut Sınırlamalar

- Ana uygulama fiziksel hız sensörü okumaz.
- Sanal hız/ivme değerleri modeldir, ölçüm değildir.
- Direksiyon triminde yazılımsal sınır yoktur.
- Fren implementasyonu motoru aktif olarak ters yönde sürer.
- Aydınlatma çıkışları aktif-düşük harici devre varsayar.
- Firmware sabit GPIO atamaları kullanır.
- Kesin Bluepad32 / ESP32 bağımlılık sürümleri sabitlenmemiştir.
- OTA kimlik bilgileri kaynakta gömülüdür.
- `RcInputAdapter.cpp` ve `RcInputAdapter.h` mevcut depoda boş placeholder dosyalardır.
- Firmware birden fazla controller slotu tutabilir ancak bağlı gamepad'lerden gelen sürüş girdileri aynı tek araç controller durumunu işler.

## Kaynak Dosya Haritası

| Dosya | Görevi |
|---|---|
| `RC_CONTROLLER.ino` | Açılış, Bluepad32 callback'leri, controller saklama, tuş eşleme, ana döngü ve OTA toggle |
| `RcCarController.h/.cpp` | Araç durumu, vites ölçekleri, direksiyon trimi, aydınlatma durumu, sanal dinamikler ve failsafe |
| `RcHardwareDriver.h/.cpp` | GPIO atamaları, motor PWM/yön, aktif fren, servo PWM ve ışık çıkışları |
| `RumbleManager.h/.cpp` | Sanal ivmeden üretilen gamepad titreşim efektleri |
| `OTA.h` | Opsiyonel SoftAP + ArduinoOTA servisi ve task'ı |
| `RcController.h` | Ortak include'lar ve button-edge yardımcı yapısı |
| `RcInputAdapter.h/.cpp` | Mevcut depoda boş placeholder dosyalar |
| `.github/workflows/sign-commits.yml` | Depo commit imzalama workflow'u |

## Depo Yapısı

```text
RC_CONTROLLER/
├── .github/
│   └── workflows/
│       └── sign-commits.yml
├── OTA.h
├── RC_CONTROLLER.ino
├── README.md
├── README-tr.md
├── RcCarController.cpp
├── RcCarController.h
├── RcController.h
├── RcHardwareDriver.cpp
├── RcHardwareDriver.h
├── RcInputAdapter.cpp
├── RcInputAdapter.h
├── RumbleManager.cpp
└── RumbleManager.h
```
