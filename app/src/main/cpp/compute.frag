#version 300 es
precision highp float;

// Input: Bir önceki frame'in durumu
uniform sampler2D u_bufferTexture;
// Input: Ekran boyutları
uniform vec2 u_resolution;
// Output
out vec4 outColor;

// Orijinal kodun maxColorValue'su
const float MAX_COLOR_VALUE = 255.0; // Basitlik için bir değer

void main() {
    // Mevcut pikselin ekran koordinatı
    vec2 coord = gl_FragCoord.xy;
    // Mevcut pikselin texture koordinatı
    vec2 texCoord = coord / u_resolution;

    // Komşu Piksellerin Koordinatları (1 piksel ofset)
    vec2 offset = 1.0 / u_resolution;

    // Yoğunluk okuma: Texture'dan okunan R, G, B, A değerlerinin ilk kanalı (R) yoğunluğu taşır.
    float self = texture(u_bufferTexture, texCoord).r * MAX_COLOR_VALUE;
    float up = texture(u_bufferTexture, texCoord + vec2(0.0, offset.y)).r * MAX_COLOR_VALUE;
    float down = texture(u_bufferTexture, texCoord - vec2(0.0, offset.y)).r * MAX_COLOR_VALUE;
    float left = texture(u_bufferTexture, texCoord - vec2(offset.x, 0.0)).r * MAX_COLOR_VALUE;
    float right = texture(u_bufferTexture, texCoord + vec2(offset.x, 0.0)).r * MAX_COLOR_VALUE;

    // Dört temel komşunun ortalaması
    float avg = (up + down + left + right) / 4.0;

    // Sönümleme (Decay): Orijinal koddaki avg > 0 ? avg - 1 : 0 mantığı
    float new_intensity = max(0.0, avg - 1.0);

    // --- Renklendirme Mantığı (Palette) ---
    vec3 color;

    // Normalleştirilmiş yoğunluk (0.0 - 1.0)
    float norm_intensity = new_intensity / MAX_COLOR_VALUE;

    // Orijinal renk geçişi (Sarı -> Yeşil -> Parlak) mantığı
    if (norm_intensity > 0.5) {
        // Yüksek Yoğunluk: Parlak Beyaz/Yeşil
        color = vec3(norm_intensity, 1.0, norm_intensity);
    } else if (norm_intensity > 0.25) {
        // Orta Yoğunluk: Yeşil/Sarı
        color = vec3(norm_intensity, norm_intensity, 0.0);
    } else {
        // Düşük Yoğunluk: Siyah/Kırmızı
        color = vec3(norm_intensity * 2.0, 0.0, 0.0);
    }

    // Yoğunluk verisini bir sonraki hesaplama için R kanalına yaz.
    // Görünür rengi diğer kanallara yaz.
    outColor = vec4(new_intensity / MAX_COLOR_VALUE, color);
}