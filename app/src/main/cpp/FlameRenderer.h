#ifndef FLAME_RENDERER_H
#define FLAME_RENDERER_H

#include <GLES3/gl3.h>

class FlameRenderer {
public:
    /**
     * Yapılandırıcı (Constructor).
     * Tüm üye değişkenleri varsayılan değerlerine (0) ayarlar.
     */
    FlameRenderer();

    /**
     * OpenGL bağlamı oluşturulduğunda çağrılır.
     * Shader programlarını ve VBO gibi boyuttan bağımsız kaynakları oluşturur.
     */
    void initialize();

    /**
     * Ekran boyutu değiştiğinde veya bağlam yeniden oluşturulduğunda çağrılır.
     * FBO'lar ve Texture'lar gibi boyuta bağımlı kaynakları ayarlar.
     */
    void resize(int width, int height);

    /**
     * Her karede (frame) çağrılır.
     * 1. Hesaplama (Compute) shader'ını çalıştırır (Efektin durumunu günceller).
     * 2. Çizim (Render) shader'ını çalıştırır (Sonucu ekrana çizer).
     */
    void renderFrame();

    /**
     * OpenGL bağlamı yok edildiğinde çağrılır.
     * Tüm ayrılmış GPU kaynaklarını (Programlar, FBO'lar, VBO'lar, Texture'lar) temizler.
     */
    void shutdown();

private:
    // --- OpenGL Kaynak ID'leri ---

    // Shader Programları
    GLuint mRenderProgram;  // Ekrana renkli çizim yapan program
    GLuint mComputeProgram; // Efektin durumunu (yoğunluk) hesaplayan program

    // Ping-Pong Framebuffer'lar (Hesaplama için)
    GLuint mFbo[2]{};     // Framebuffer Object ID'leri
    GLuint mTexture[2]{}; // FBO'lara bağlı Texture ID'leri

    // Vertex Buffer Object (Tam ekran dikdörtgeni için)
    GLuint mQuadVbo;

    // --- Durum (State) Değişkenleri ---
    int mWidth;
    int mHeight;
    int mCurrentBuffer; // Ping-Pong için mevcut buffer indisi (0 veya 1)

    // --- Yardımcı Metotlar ---

    /**
     * Verilen kaynak koddan bir Shader derler (Vertex veya Fragment).
     */
    static GLuint loadShader(GLenum type, const char *shaderSrc);

    /**
     * Verilen Vertex ve Fragment shader'larını kullanarak bir GL Programı oluşturur ve linkler.
     */
    static GLuint createProgram(const char *vShaderSrc, const char *fShaderSrc);

    /**
     * mWidth ve mHeight boyutlarına göre Ping-Pong FBO'ları ve Texture'ları ayarlar.
     */
    void setupPingPongFBOs();

    /**
     * mQuadVbo'yu (tam ekran VBO) oluşturur ve verisini yükler.
     */
    void setupFullScreenQuad();
};

#endif //FLAME_RENDERER_H