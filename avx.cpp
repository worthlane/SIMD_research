#include <SFML/Graphics.hpp>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <immintrin.h>

static const int LENGTH = 400;
static const int WIDTH  = 200;

static const int N_MAX  = 256;

static const int RGB_MAX = 256;

static const float LIN_SHIFT   = 0.10;
static const float SCALE_SHIFT = 0.1;           // 10 percent size increase/decrease
static const float X_PRE_SHIFT = -1.325;
static const float Y_PRE_SHIFT = 0.f;

static const float dx = ((float) 1 / (float) LENGTH);
static const float dy = ((float) 1 / (float) WIDTH);

extern "C" u_int64_t _GetTick();

static const size_t TEST_AMT = 300;
#define TEST_RUN 1

// =====================================

void GenerateColor(u_int8_t* points, const int x, const int y, const int N);
void CalcMandelbrot(sf::RenderWindow* window, u_int8_t* points, const float x_shift, const float y_shift, const float scale);
void HandleKeyboard(sf::Event* event, float* x_shift, float* y_shift, float* scale);
void DumpPoints(sf::RenderWindow* window, u_int8_t* points);

// =========================================================================

int main()
{
    sf::RenderWindow window(sf::VideoMode(LENGTH, WIDTH), "Mandelbrot");

    float x_shift = 0.f;
    float y_shift = 0.f;
    float scale   = 1.00;

    u_int64_t time = 0;
    size_t    run  = 0;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            HandleKeyboard(&event, &x_shift, &y_shift, &scale);
        }

        window.clear();

        u_int64_t start = _GetTick();

        u_int8_t points[WIDTH * LENGTH * 4] = {};
        CalcMandelbrot(&window, points, x_shift, y_shift, scale);

        u_int64_t end   = _GetTick();

        time += end - start;
        run++;

        #if TEST_RUN
        if (run >= TEST_AMT)
            window.close();

        printf("%ld/%ld\n", run, TEST_AMT);
        #else
        DumpPoints(&window, points);
        #endif
    }

    #if TEST_RUN
    printf("RUNS: %ld, TIME: %ld, TIME PER RUN: %ld\n", run, time, time / run);
    #endif

    return 0;
}

// -------------------------------------------------------

void GenerateColor(u_int8_t* points, const int x, const int y, const int N)
{
    assert(points);

    float I = sqrtf( sqrtf( (float) N / (float) N_MAX )) * 255.f;
    u_int8_t color = (u_int8_t) I;

    size_t point_pos = (y * LENGTH + x) * 4;

    points[point_pos]     = RGB_MAX - color;
    points[point_pos + 1] = RGB_MAX - color;
    points[point_pos + 2] = color;
    points[point_pos + 3] = 255;
}

// -------------------------------------------------------

void CalcMandelbrot(sf::RenderWindow* window, u_int8_t* points, const float x_shift, const float y_shift, const float scale)
{
    static const __m256 R2_MAX    = _mm256_set1_ps(10000.f);
    static const __m256 _01234567 = _mm256_set_ps(7.f, 6.f, 5.f, 4.f, 3.f, 2.f, 1.f, 0.f);

    for (int y = 0; y < WIDTH; y++)
    {
        float x0 = ((-(float) LENGTH / 2) * dx) * scale + x_shift + X_PRE_SHIFT;
        float y0 = (((float) y - (float) WIDTH / 2) * dy) * scale + y_shift + Y_PRE_SHIFT;

        for (int x = 0; x < LENGTH; x += 8, x0 += 8 * dx * scale)
        {
            __m256 DX = _mm256_mul_ps(_mm256_set1_ps(dx),
                                      _mm256_set_ps(7.f, 6.f, 5.f, 4.f,
                                                    3.f, 2.f, 1.f, 0.f));

            __m256 X0 = _mm256_add_ps(_mm256_set1_ps(x0), DX);
            __m256 Y0 = _mm256_set1_ps(y0);

            __m256 X = X0;
            __m256 Y = Y0;

            __m256i N = _mm256_setzero_si256();

            for (int n = 0; n < N_MAX; n++)
            {
                __m256 x2 = _mm256_mul_ps(X, X);
                __m256 y2 = _mm256_mul_ps(Y, Y);
                __m256 xy = _mm256_mul_ps(X, Y);

                __m256 r2 = _mm256_add_ps(x2, y2);

                __m256 cmp = _mm256_cmp_ps(r2, R2_MAX, _CMP_LE_OQ);

                int mask = _mm256_movemask_ps(cmp);
                if (!mask) break;

                N = _mm256_add_epi32(N, _mm256_castps_si256(cmp));
                X = _mm256_add_ps(_mm256_sub_ps(x2, y2), X0);
                Y = _mm256_add_ps(_mm256_add_ps(xy, xy), Y0);
            }

            float* n_iters = (float*) (&N);
            for (int i = 0; i < 8; i++)
                GenerateColor(points, x + i, y, n_iters[i]);
        }
    }
}

// -------------------------------------------------------

void HandleKeyboard(sf::Event* event, float* x_shift, float* y_shift, float* scale)
{
    assert(event);
    assert(x_shift);
    assert(y_shift);
    assert(scale);

    if (event->type == sf::Event::KeyReleased)
    {
        switch (event->key.code)
        {
            case sf::Keyboard::A:
                *x_shift -= LIN_SHIFT * *scale;
                break;

            case sf::Keyboard::D:
                *x_shift += LIN_SHIFT * *scale;
                break;

            case sf::Keyboard::W:
                *y_shift -= LIN_SHIFT * *scale;
                break;

            case sf::Keyboard::S:
                *y_shift += LIN_SHIFT * *scale;
                break;

            case sf::Keyboard::Up:
                *scale += SCALE_SHIFT * *scale;
                break;

            case sf::Keyboard::Down:
                *scale -= SCALE_SHIFT * *scale;
                break;

            default:
                break;
        }
    }
}

// -------------------------------------------------------

void DumpPoints(sf::RenderWindow* window, u_int8_t* points)
{
    assert(points);
    assert(window);

    sf::Texture texture;
    texture.create(LENGTH, WIDTH);
    sf::Sprite sprite;

    texture.update(points);
    sprite.setTexture(texture);

    window->draw(sprite);

    window->display();
}
