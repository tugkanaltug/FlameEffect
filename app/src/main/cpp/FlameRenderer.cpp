#include "FlameRenderer.h"
#include <GLES3/gl3.h>
#include <jni.h>
#include <android/log.h>
#include <cmath>
#include <cstdlib> // rand() ve RAND_MAX için

#define LOG_TAG "FlameRenderer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Tam Ekran Quad Vertex Koordinatları (Konum ve Texture Koordinatları)
const GLfloat QUAD_VERTICES[] = {
        // x, y, u, v
        -1.0f, -1.0f, 0.0f, 0.0f, // Sol Alt
        1.0f, -1.0f, 1.0f, 0.0f, // Sağ Alt
        -1.0f, 1.0f, 0.0f, 1.0f, // Sol Üst
        1.0f, 1.0f, 1.0f, 1.0f, // Sağ Üst
};

// Basit Vertex Shader (Tam ekran çizimi için)
const char *VERTEX_SHADER_SRC =
        R"(#version 300 es
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_texCoord;
out vec2 v_texCoord;
void main() {
    gl_Position = a_position;
    v_texCoord = a_texCoord;
})";

// Hesaplama Fragment Shader'ı (Alev Efekti Mantığı: Yükselme, Sönümleme ve Tohumlama)
const char *COMPUTE_FRAGMENT_SHADER_SRC =
        R"(#version 300 es
precision highp float;

uniform sampler2D u_bufferTexture;
uniform vec2 u_resolution;
in vec2 v_texCoord;
out vec4 outColor;

// Orijinal görünüme yakın yüksek sönümlenme oranı (Hızlı sönmeyi sağlar)
const float DECAY_RATE = 0.01;
// En alt satırda kullanılacak tohumlama olasılığı
const float BASE_FIRE_PROBABILITY = 0.4; // %40 olasılıkla alev başlat

// Basit rastgele sayı üreteci
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 offset = 1.0 / u_resolution; // 1 piksel ofseti

    // Orijinal J2ME Diffüzyon Çekirdeği
    vec2 coord_d1 = v_texCoord - vec2(0.0, offset.y);
    vec2 coord_d2 = v_texCoord - vec2(0.0, offset.y * 2.0);

    // Komşu Yoğunluklarını Oku
    float p_d2_c = texture(u_bufferTexture, coord_d2).r;
    float p_d1_l = texture(u_bufferTexture, coord_d1 - vec2(offset.x, 0.0)).r;
    float p_d1_c = texture(u_bufferTexture, coord_d1).r;
    float p_d1_r = texture(u_bufferTexture, coord_d1 + vec2(offset.x, 0.0)).r;

    // Ortalama alma ve Sönümleme
    float avg = (p_d2_c + p_d1_l + p_d1_c + p_d1_r) / 4.0;
    float new_intensity = max(0.0, avg - DECAY_RATE);

    // En alt satırı sürekli besle
    // Sadece en alt 1 piksel satırını (offset.y * 1.0) hedefle
    if (v_texCoord.y < offset.y * 1.0) {
        float rand_val = random(gl_FragCoord.xy);

        if (rand_val < BASE_FIRE_PROBABILITY) {
            // Yüksek enerji (yeni alev tohumu)
            new_intensity = 1.0;
        } else {
            // Aradaki pikselleri siyah yap (alev kaynağında boşluklar oluşturur)
            new_intensity = 0.0;
        }
    }

    // Yeni yoğunluğu R kanalına yaz.
    outColor = vec4(new_intensity, 0.0, 0.0, 1.0);
})";

// Çizim Fragment Shader'ı (Alev Renk Paleti)
const char *RENDER_FRAGMENT_SHADER_SRC =
        R"(#version 300 es
precision highp float;
uniform sampler2D u_bufferTexture;
in vec2 v_texCoord;
out vec4 outColor;
void main() {
    float intensity = texture(u_bufferTexture, v_texCoord).r;
    vec3 color;
    if (intensity < 0.1) {
        color = vec3(0.0);
    } else if (intensity < 0.4) {
        color = vec3(intensity * 2.0, intensity * 0.5, 0.0); // Koyu Kırmızı
    } else if (intensity < 0.7) {
        color = vec3(1.0, intensity * 1.5, 0.0); // Turuncu/Kırmızı
    } else {
        color = vec3(1.0, 1.0, intensity * 0.5); // Sarı/Beyaz
    }
    outColor = vec4(color, 1.0);
})";

// --- OpenGL Yardımcı Fonksiyonları ---

GLuint FlameRenderer::loadShader(GLenum type, const char *shaderSrc) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        LOGE("glCreateShader başarısız oldu.");
        return 0;
    }
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char *infoLog = (char *) malloc(infoLen);
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            LOGE("Shader derleme hatası:\n%s", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint FlameRenderer::createProgram(const char *vShaderSrc, const char *fShaderSrc) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vShaderSrc);
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fShaderSrc);
    if (vertexShader == 0 || fragmentShader == 0) return 0;

    GLuint program = glCreateProgram();
    if (program == 0) return 0;

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        LOGE("Program link hatası.");
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// --- FlameRenderer Sınıfı Uygulaması ---

FlameRenderer::FlameRenderer() :
        mWidth(0), mHeight(0),
        mCurrentBuffer(0),
        mRenderProgram(0), mComputeProgram(0),
        mQuadVbo(0) {
    mFbo[0] = mFbo[1] = 0;
    mTexture[0] = mTexture[1] = 0;
}

void FlameRenderer::setupFullScreenQuad() {
    // VBO'yu sadece bir kez oluştur
    if (mQuadVbo == 0) {
        glGenBuffers(1, &mQuadVbo);
        glBindBuffer(GL_ARRAY_BUFFER, mQuadVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTICES), QUAD_VERTICES, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

// YENİ: Başlangıçta gürültü tohumlaması eklendi (Siyah ekranı önler)
void FlameRenderer::setupPingPongFBOs() {
    // 1. Önceki FBO ve Texture'ları sil
    if (mFbo[0] != 0) {
        glDeleteFramebuffers(2, mFbo);
        mFbo[0] = mFbo[1] = 0;
    }
    if (mTexture[0] != 0) {
        glDeleteTextures(2, mTexture);
        mTexture[0] = mTexture[1] = 0;
    }

    // 2. FBO'ları oluştur
    glGenFramebuffers(2, mFbo);
    glGenTextures(2, mTexture);

    // 3. Başlangıç gürültüsü için CPU buffer'ı oluştur
    int numPixels = mWidth * mHeight;
    auto *seedData = new float[numPixels];
    for (int i = 0; i < numPixels; ++i) {
        // Efektin sönümlenmiş başlaması için düşük yoğunluklu rastgele gürültü
        seedData[i] = ((float) rand() / RAND_MAX) * 0.1f;
    }

    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, mTexture[i]);

        // R32F (float) formatı kullan
        // İlk buffer'ı (i==0) CPU'dan gelen tohum verisiyle doldur, diğerini (i==1) boş bırak.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mWidth, mHeight, 0, GL_RED, GL_FLOAT,
                     (i == 0) ? seedData : nullptr);

        // Alev/duman için yumuşak geçiş (GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // FBO'ya Texture'ı bağla
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("FBO %d Tamamlanmadı!", i);
        }
    }

    // CPU buffer'ını temizle
    delete[] seedData;

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mCurrentBuffer = 0; // İlk buffer'dan başla
}

// JNI nativeInit() tarafından çağrılır
void FlameRenderer::initialize() {
    LOGD("OpenGL Başlatılıyor/Yeniden Yükleniyor (initialize).");

    // Önceki Shader Programlarını ve VBO'yu sil
    if (mRenderProgram != 0) glDeleteProgram(mRenderProgram);
    if (mComputeProgram != 0) glDeleteProgram(mComputeProgram);
    if (mQuadVbo != 0) glDeleteBuffers(1, &mQuadVbo);
    mRenderProgram = 0;
    mComputeProgram = 0;
    mQuadVbo = 0;

    // Shader'ları derle
    mRenderProgram = createProgram(VERTEX_SHADER_SRC, RENDER_FRAGMENT_SHADER_SRC);
    mComputeProgram = createProgram(VERTEX_SHADER_SRC, COMPUTE_FRAGMENT_SHADER_SRC);

    if (mRenderProgram == 0 || mComputeProgram == 0) {
        LOGE("Shader derlenemedi!");
        return;
    }

    setupFullScreenQuad();
    glDisable(GL_DEPTH_TEST);
}

// JNI nativeResize() tarafından çağrılır
void FlameRenderer::resize(int width, int height) {
    LOGD("OpenGL Yeniden Boyutlandırıldı: %dx%d", width, height);
    mWidth = width;
    mHeight = height;

    // FBO'ları ve Texture'ları bu yeni boyuta göre yeniden oluştur (ve tohumla)
    setupPingPongFBOs();
}

void FlameRenderer::renderFrame() {
    // Güvenlik kontrolü
    if (mWidth <= 0 || mHeight <= 0 || mComputeProgram == 0 || mRenderProgram == 0 ||
        mFbo[0] == 0) {
        return;
    }

    // --- AŞAMA 1: HESAPLAMA (GPU Üzerinde State Güncelleme) ---

    glBindFramebuffer(GL_FRAMEBUFFER, mFbo[1 - mCurrentBuffer]);
    glViewport(0, 0, mWidth, mHeight);
    glUseProgram(mComputeProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture[mCurrentBuffer]);
    glUniform1i(glGetUniformLocation(mComputeProgram, "u_bufferTexture"), 0);
    glUniform2f(glGetUniformLocation(mComputeProgram, "u_resolution"), (float) mWidth,
                (float) mHeight);

    glBindBuffer(GL_ARRAY_BUFFER, mQuadVbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void *) nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          (const void *) (2 * sizeof(GLfloat)));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Buffer'ı takas et
    mCurrentBuffer = 1 - mCurrentBuffer;

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // --- AŞAMA 2: ÇİZİM (Ekrana Renklendirme) ---

    glViewport(0, 0, mWidth, mHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(mRenderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture[mCurrentBuffer]); // En son hesaplanan buffer'ı çiz
    glUniform1i(glGetUniformLocation(mRenderProgram, "u_bufferTexture"), 0);

    glBindBuffer(GL_ARRAY_BUFFER, mQuadVbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void *) nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          (const void *) (2 * sizeof(GLfloat)));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FlameRenderer::shutdown() {
    LOGD("OpenGL Kapatılıyor.");
    if (mFbo[0] != 0) glDeleteFramebuffers(2, mFbo);
    if (mTexture[0] != 0) glDeleteTextures(2, mTexture);
    if (mRenderProgram != 0) glDeleteProgram(mRenderProgram);
    if (mComputeProgram != 0) glDeleteProgram(mComputeProgram);
    if (mQuadVbo != 0) glDeleteBuffers(1, &mQuadVbo);

    mFbo[0] = mFbo[1] = 0;
    mTexture[0] = mTexture[1] = 0;
    mRenderProgram = 0;
    mComputeProgram = 0;
    mQuadVbo = 0;
}


// --- JNI Bağlantıları ---
static FlameRenderer *gRenderer = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_flame_effect_NativeRenderer_nativeInit(JNIEnv *env, jobject thiz) {
    if (gRenderer == nullptr) {
        gRenderer = new FlameRenderer();
    }
    gRenderer->initialize();
}

extern "C" JNIEXPORT void JNICALL
Java_flame_effect_NativeRenderer_nativeResize(JNIEnv *env, jobject thiz, jint width, jint height) {
    if (gRenderer != nullptr) {
        gRenderer->resize(width, height);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_flame_effect_NativeRenderer_nativeRender(JNIEnv *env, jobject thiz) {
    if (gRenderer != nullptr) {
        gRenderer->renderFrame();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_flame_effect_NativeRenderer_nativeShutdown(JNIEnv *env, jobject thiz) {
    if (gRenderer != nullptr) {
        gRenderer->shutdown();
        delete gRenderer;
        gRenderer = nullptr;
        LOGD("Native Renderer Kapatıldı.");
    }
}