#include <jni.h>
#include <vector>
#include <cassert>
#include <memory>
#include <cmath>
#include <tuple>

#include <android/log.h>

#define TAG "gaussian-jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__); // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__); // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__); // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__); // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__); // 定义LOGF类型

double gaussian(int x, int y, double sigma) {
    double sigma2 = pow(sigma, 2);
    return 0.5 * M_1_PI / sigma2 * pow(M_E, -(pow(x, 2) + pow(y, 2)) / (2 * sigma2));
}

[[nodiscard]]
std::vector<double> gaussian_kernel(int matrix_size, double sigma) {
    std::vector<double> res(matrix_size * matrix_size, 0);
    const int translation{(matrix_size - 1) / 2};
    double sum{};

    for (size_t row{0}; row < matrix_size; ++row) {
        for (size_t column{0}; column < matrix_size; ++column) {
            res[row * matrix_size + column] = gaussian(
                    static_cast<int>(row) - translation,
                    static_cast<int>(column) - translation,
                    sigma);
            sum += res[row * matrix_size + column];
        }
    }

    for (auto &num: res) {
        num /= sum;
    }

    return res;
}

std::tuple<int, int, int, int> int_argb8888(int color) {
    auto color_unsigned = static_cast<unsigned>(color);
    int a = static_cast<int>(color_unsigned >> 24) & 0xff;
    int r = static_cast<int>(color_unsigned >> 16) & 0xff;
    int g = static_cast<int>(color_unsigned >> 8) & 0xff;
    int b = static_cast<int>(color_unsigned)      & 0xff;

    return std::make_tuple(a, r, g, b);
}

int argb8888_int(int a, int r, int g, int b) {
    int res{};
    res |= a << 24;
    res |= r << 16;
    res |= g << 8;
    res |= b;

    return res;
}

int *get_fill_boundary(JNIEnv *env, jintArray pixels, jint width, jint height, jint kernel_size) {
    const int translate{(kernel_size - 1) / 2};
    const int new_height{height + 2 * translate};
    const int new_width{width + 2 * translate};
    std::unique_ptr<jint[]> c_new_pixels{
            new jint[new_width * new_height]};

    for (size_t row{}; row < height; ++row) {
        env->GetIntArrayRegion(
                pixels,
                static_cast<int>(row) * width, width,
                c_new_pixels.get() + (row + translate) * new_width + translate);

        for (size_t translate_index{}; translate_index < translate; ++translate_index) {
            // fill left
            c_new_pixels[
                    ((row + translate) * new_width + translate) - translate_index - 1
            ] = c_new_pixels[
                    ((row + translate) * new_width + translate) + translate_index
            ];
            // fill right
            c_new_pixels[
                    ((row + translate) * new_width + translate) + width + translate_index
            ] = c_new_pixels[
                    ((row + translate) * new_width + translate) + width - translate_index - 1
            ];
        }
    }

    // fill the top and bottom
    for (size_t translate_index{}; translate_index < translate; ++translate_index) {
        for (size_t col{}; col < new_width; ++col) {
            // fill top
            c_new_pixels[
                    (translate - translate_index - 1) * new_width + col
            ] = c_new_pixels[
                    (translate + translate_index) * new_width + col
            ];

            // fill bottom
            c_new_pixels[
                    (height + translate + translate_index) * new_width + col
            ] = c_new_pixels[
                    (height + translate - translate_index - 1) * new_width + col
            ];
        }
    }

    return c_new_pixels.release();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ziyuanrenjianjinhao_gaussianblur_MainActivity_gaussianBlur(JNIEnv *env, jobject thiz,
                                                                    jintArray pixels, jint width,
                                                                    jint height, jint kernel_size,
                                                                    jdouble sigma) {
    assert(kernel_size > 0 && kernel_size % 2);
    assert(sigma > 0);
    LOGD("call create gaussian kernel")
    auto kernel{gaussian_kernel(kernel_size, sigma)};
    LOGD("call create gaussian kernel: success")
    jsize pixels_len = env->GetArrayLength(pixels);
    assert(pixels_len == width * height);
//    jsize pixels_len = width * height;
    LOGD("call get fill boundary")
    std::unique_ptr<jint[]> c_pixels{get_fill_boundary(env, pixels, width, height, kernel_size)};
    LOGD("call get fill boundary: success")
//    env->GetIntArrayRegion(pixels, 0, pixels_len, c_pixels.get());

    // 填充边缘
    std::unique_ptr<jint[]> c_new_pixels{new jint[pixels_len]};

    const int translate{(kernel_size - 1) / 2};
    const int new_width{width + 2 * translate};


    for (size_t row{}; row < height; ++row) {
        for (size_t col{}; col < width; ++col) {
            double alpha_weighted_mean{};
            double red_weighted_mean{};
            double green_weighted_mean{};
            double blue_weighted_mean{};
            // 计算子层核半径内的区域
            for (size_t macro_row{}; macro_row < kernel_size; ++macro_row) {
                for (size_t macro_col{}; macro_col < kernel_size; ++macro_col) {
                    auto[a, r, g, b] =
                    int_argb8888(c_pixels[
                                         (row + macro_row) * new_width
                                         + (col + macro_col)]);
                    alpha_weighted_mean += a * kernel[macro_row * kernel_size + macro_col];
                    red_weighted_mean += r * kernel[macro_row * kernel_size + macro_col];
                    green_weighted_mean += g * kernel[macro_row * kernel_size + macro_col];
                    blue_weighted_mean += b * kernel[macro_row * kernel_size + macro_col];
                }
            }

            /*c_new_pixels[row * width + col] = argb8888_int(
                    static_cast<int>(alpha_weighted_mean > 255 ? 255 : alpha_weighted_mean < 0 ? 0 : alpha_weighted_mean),
                    static_cast<int>(red_weighted_mean > 255 ? 255 : red_weighted_mean < 0 ? 0 : red_weighted_mean),
                    static_cast<int>(green_weighted_mean > 255 ? 255 : green_weighted_mean < 0 ? 0 : green_weighted_mean),
                    static_cast<int>(blue_weighted_mean > 255 ? 255 : blue_weighted_mean < 0 ? 0 : blue_weighted_mean)
                    );*/

            c_new_pixels[row * width + col] = argb8888_int(
                    static_cast<int>(alpha_weighted_mean),
                    static_cast<int>(red_weighted_mean),
                    static_cast<int>(green_weighted_mean),
                    static_cast<int>(blue_weighted_mean)
            );
        }
    }

    LOGD("变化前数值：%d, 变化后数值：%d", c_pixels[600 * new_width + 600], c_new_pixels[(600 - translate) * width + 600 - translate])
    LOGD("变化前数值：%d, 变化后数值：%d", c_pixels[800 * new_width + 800], c_new_pixels[(800 - translate) * width + 800 - translate])
    env->SetIntArrayRegion(pixels, 0, pixels_len, c_new_pixels.release());

    LOGD("call gaussian blur: success")

    return (void) 0;
}


