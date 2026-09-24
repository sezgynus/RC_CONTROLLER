# RC Controller

Bluetooth gamepad ile bir RC aracın motorunu, direksiyon servosunu ve aydınlatmasını kontrol eden ESP32 firmware’i. Bluepad32 girdileri sürüş mantığına dönüştürülür; motor/servo PWM çıkışları ve aktif-düşük aydınlatma pinleri ayrı bir donanım sürücüsünde yönetilir.

## Özellikler

- İleri/geri sürüş, iki tetik birlikte basıldığında fren komutu.
- Dört elektronik vites: motor ölçekleri %60, %70, %80 ve %100.
- Direksiyon trimi ve Preferences üzerinden kalıcı kayıt.
- Kısa/uzun far, selektör, sinyal, dörtlü ve geri vites ışığı.
- Direksiyon dönüşü sonrası sinyallerin otomatik iptali.
- Gamepad renk LED’i ve titreşim geri bildirimi çağrıları.
- 500 ms giriş zaman aşımında motor komutunu kesen failsafe.

## Donanım

Bluetooth Classic/Bluepad32 ile uyumlu ESP32, desteklenen gamepad, EN/IN1/IN2 girişli motor sürücüsü, direksiyon servosu ve uygun aydınlatma sürücü devreleri gerekir. Motor ve servo GPIO’dan beslenmez.

| İşlev | GPIO |
|---|---|
| Motor EN (PWM), IN1, IN2 | 12, 14, 15 |
| Direksiyon servosu | 4 |
| Sol / sağ sinyal | 5 / 17 |
| Kısa far / fren lambası | 33 / 32 |
| Uzun far / geri vites lambası | 2 / 0 |

Aydınlatma çıkışları **aktif düşük** çalışır. Motor PWM’i 20 kHz/8 bit, servo PWM’i 50 Hz/16 bittir. Servo darbe aralığı trim öncesi 900–2000 µs; sürücü değişmeyen komuttan 500 ms sonra servo PWM’ini serbest bırakır. Bu ayarları gerçek mekanik sınırlarla karşılaştırın.

## Derleme ve ilk çalıştırma

1. Bluepad32 ve `uni.h` sağlayan uyumlu Arduino/ESP32 geliştirme ortamını kurun.
2. Bütün kaynakları `RC_CONTROLLER.ino` ile aynı sketch klasöründe tutun.
3. Kodun kullandığı `ledcSetup` ve `ledcAttachPin` API’lerini destekleyen çekirdek sürümünü seçin; kesin sürüm depoda sabitlenmemiştir.
4. Gerçek ESP32 kartını seçerek derleyip yükleyin. Seri tanılama **115200 baud**.
5. İlk kontrolü tekerlekler zeminden kaldırılmışken yapın; yön, servo merkezi, ışık polaritesi ve bağlantı kaybındaki davranışı doğrulayın.

Bluetooth allowlist açılışta etkinleştirilir. Bağlanan denetleyicinin adresi listeye eklenir; temiz bir cihazdaki ilk eşleştirme, kullanılan Bluepad32 sürümünün allowlist/provisioning davranışına bağlıdır. Bağlantı kurulamıyorsa bu akışı kontrol edin.

## Kumanda eşlemesi

Aşağıdaki isimler Bluepad32’nin mantıksal tuş adlarıdır; fiziksel semboller gamepad modeline göre değişebilir.

| Girdi | İşlev |
|---|---|
| `throttle()` | İleri gaz |
| `brake()` | Tek başına geri sürüş; gazla birlikte fren |
| Sol analog X (`axisX`) | Direksiyon |
| X | Kapalı → kısa far → uzun far → kapalı |
| A, basılı tutulurken | Selektör; bırakınca önceki uzun far durumu |
| L1 / R1 | Sol / sağ sinyali aç-kapat |
| Y | Dörtlü aç-kapat |
| D-pad yukarı / aşağı | Vites büyüt / küçült |
| `miscButtons() & 0x04` + D-pad sol/sağ | Trimi −5 / +5 µs değiştir |
| `miscButtons() & 0x02` | OTA erişim noktasını aç-kapat |

İkinci vitesten sonraki vites büyütme için de `0x04` yardımcı tuşu gerekir. Vites değişiminde gamepad LED’i sırasıyla yeşil, sarı, turuncu ve kırmızı olur.

## Failsafe ve model sınırları

Girişler 500 ms güncellenmezse gaz, fren, direksiyon ve motor komutları sıfırlanır; dörtlü durumu etkinleştirilir. Bu davranış aktif mekanik fren garantisi değildir. Sürüş sınıfındaki hız ve ivme değerleri yazılımsal tahmindir; mevcut ana akışta fiziksel hız sensörü okunmaz.

Motor sürücüsündeki fren profili son hareket yönünün tersine sürüş uygular. Gerçek motor, sürücü ve mekanikle uyumu ayrıca değerlendirilmelidir.

## Kaynak düzeni

- `RC_CONTROLLER.ino`: Bluepad32 bağlantıları, tuş eşlemesi ve ana döngü.
- `RcCarController.*`: sürüş durumu, vitesler, trim, aydınlatma ve failsafe.
- `RcHardwareDriver.*`: pinler, PWM, servo ve lamba çıkışları.
- `RcController.h`: ortak başlıklar ve tuş kenarı algılama.
- `RcInputAdapter.*`: giriş adaptörü kaynakları.
- `RumbleManager.*`: gamepad titreşim bileşeni.
- `OTA.h`: isteğe bağlı kablosuz güncelleme bileşeni.
