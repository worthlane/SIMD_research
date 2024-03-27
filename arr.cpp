#include <SFML/Graphics.hpp>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

static const int ARR_SIZE = 8;

#define FOR_ARRAY(code)     for (int i = 0; i < ARR_SIZE; i++)      \
                            {                                       \
                                code                                \
                            }

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

inline void mm_set_ps (float mm[ARR_SIZE], const float val0, const float val1, const float val2, const float val3,
                                           const float val4, const float val5, const float val6, const float val7)
{
    mm[0] = val0; mm[1] = val1; mm[2] = val2; mm[3] = val3; mm[4] = val4; mm[5] = val5; mm[6] = val6; mm[7] = val7;
}

inline void mm_set_ps1 (float mm[ARR_SIZE], const float val)                { FOR_ARRAY( mm[i] = val; ) }
inline void mm_cpy_ps  (float dest[ARR_SIZE], const float src[ARR_SIZE])    { FOR_ARRAY( dest[i] = src[i]; ) }

inline void mm_add_ps  (float dest[ARR_SIZE], const float mm1[ARR_SIZE], const float mm2[ARR_SIZE])
{ FOR_ARRAY( dest[i] = mm1[i] + mm2[i]; ) }

inline void mm_sub_ps  (float dest[ARR_SIZE], const float mm1[ARR_SIZE], const float mm2[ARR_SIZE])
{ FOR_ARRAY( dest[i] = mm1[i] - mm2[i]; ) }

inline void mm_mul_ps  (float dest[ARR_SIZE], const float mm1[ARR_SIZE], const float mm2[ARR_SIZE])
{ FOR_ARRAY( dest[i] = mm1[i] * mm2[i]; ) }

inline void mm_add_epi32  (int dest[ARR_SIZE], const int mm1[ARR_SIZE], const float mm2[ARR_SIZE])
{ FOR_ARRAY( dest[i] = mm1[i] + (int) mm2[i]; ) }

inline void mm_cmple_ps  (float cmp[ARR_SIZE], const float mm1[ARR_SIZE], const float mm2[ARR_SIZE])
{ FOR_ARRAY
  (
    if (mm1[i] <= mm2[i])
        { cmp[i] = 1; }
    else
        { cmp[i] = 0; }
  )
}

inline int mm_movemask_ps (const float cmp[ARR_SIZE])
{
    int mask = 0;
    FOR_ARRAY( mask |= ((int) cmp[i] << i); )
    return mask;
}

// =====================================

void GenerateColor(u_int8_t* points, const int x, const int y, const int N);
void CalcMandelbrot(sf::RenderWindow* window, u_int8_t* points, const float x_shift, const float y_shift, const float scale,
                    const float R2_MAX[ARR_SIZE], const float SHIFT[ARR_SIZE]);
void HandleKeyboard(sf::Event* event, float* x_shift, float* y_shift, float* scale);
void DumpPoints(sf::RenderWindow* window, u_int8_t* points);

// =========================================================================

int main()
{
    sf::RenderWindow window(sf::VideoMode(LENGTH, WIDTH), "Mandelbrot");

    float x_shift = 0.f;
    float y_shift = 0.f;
    float scale   = 1.00;

    float    R2_MAX[ARR_SIZE] = {}; mm_set_ps1(R2_MAX, 10000.f);
    float _01234567[ARR_SIZE] = {}; mm_set_ps(_01234567, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f);

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
        CalcMandelbrot(&window, points, x_shift, y_shift, scale, R2_MAX, _01234567);

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

void CalcMandelbrot(sf::RenderWindow* window, u_int8_t* points, const float x_shift, const float y_shift, const float scale,
                    const float R2_MAX[ARR_SIZE], const float SHIFT[ARR_SIZE])
{
    for (int y = 0; y < WIDTH; y++)
    {
        float x0 = ((-(float) LENGTH / 2) * dx) * scale + x_shift + X_PRE_SHIFT;
        float y0 = (((float) y - (float) WIDTH / 2) * dy) * scale + y_shift + Y_PRE_SHIFT;

        for (int x = 0; x < LENGTH; x += ARR_SIZE, x0 += ARR_SIZE * dx * scale)
        {
            float DX[ARR_SIZE] = {}; mm_set_ps1(DX, dx); mm_mul_ps(DX, DX, SHIFT);

            float X0[ARR_SIZE] = {}; mm_set_ps1(X0, x0); mm_add_ps(X0, X0, DX);
            float Y0[ARR_SIZE] = {}; mm_set_ps1(Y0, y0);

            float  X[ARR_SIZE] = {}; mm_cpy_ps(X, X0);
            float  Y[ARR_SIZE] = {}; mm_cpy_ps(Y, Y0);

            int    N[ARR_SIZE] = { 0, 0, 0, 0 };

            for (int n = 0; n < N_MAX; n++)
            {
                float x2[ARR_SIZE] = {};  mm_mul_ps(x2, X, X);
                float y2[ARR_SIZE] = {};  mm_mul_ps(y2, Y, Y);
                float xy[ARR_SIZE] = {};  mm_mul_ps(xy, X, Y);

                float r2[ARR_SIZE] = {};  mm_add_ps(r2, x2, y2);

                float cmp[ARR_SIZE] = {}; mm_cmple_ps(cmp, r2, R2_MAX);

                int mask = mm_movemask_ps(cmp);
                if (!mask) break;

                mm_add_epi32(N, N, cmp);
                mm_sub_ps(X, x2, y2); mm_add_ps(X, X, X0);
                mm_add_ps(Y, xy, xy); mm_add_ps(Y, Y, Y0);
            }

            FOR_ARRAY ( GenerateColor(points, x + i, y, N[i]); )
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
