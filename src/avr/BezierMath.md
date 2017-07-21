# Cubic Bezier

    f(x) = A(1 - x)^3 + 3B(1 - x)^2 x + 3C(1 - x) x^2 + Dx^3

     -Ax^3 + 3Ax^2 - 3Ax + A
     3Bx^3 - 6Bx^2 + 3Bx
    -3Cx^3 + 3Cx^2
      Dx^3


    f(x) = (-A + 3B -3C + D)x^3 + (3A - 6B + 3C)x^2 + (-3A + 3B)x + A

      a =  -A + 3B - 3C + D
      b =  3A - 6B + 3C
      c = -3A + 3B
      d =   A

    f(x) = ax^3 + bx^2 + cx + d

    integral f(x) dx = a/4 x^4 + b/3 x^3 + c/2 x^2 + dx + E

    = (-A + 3B - 3C + D)/4 x^4 + (A - 2B + B) x^3 + 3/2 (B - A) x^2 + Ax + E

# Quintic Bezier

    A(1 - x)^5 + 5A(1 - x)^4 x + 10A(1 - x)^3 x^2 + 10B(1 - x)^2 x^3 +
    5B(1 - x)x^4 + Bx^5

    (-6A + 6B)x^5 + (15A - 15B)x^4 + (-10A + 10B)x^3 + A

    6(B - A)x^5 + 15(A - B)x^4 + 10(B - A)x^3 + A

    x^3 (6(B - A)x^2 + 15(A - B)x + 10(B - A)) + A

    a =   6(B - A)
    b = -15(B - A)
    c =  10(B - A)
    d = A

    f(x) = ax^5 + bx^4 + cx^3 + d

    f(x) = (ax^2 + bx + c)x^3 + d


    integral f(x) = a/6 x^6 + b/5 x^5 + c/4 x^4 + dx + e

    = (B - A)x^6 - 3(B - A)x^5 + 5/2(B - A)x^4 + Ax + e

    = (B - A)x^4 (x^2 - 3x + 5/2) + Ax + e

    A = 0
    B = 1
    e = 0

    f(x) = 6x^5 -15x^4 + 10x^3
    int f(x) dx = x^6 - 3x^5 + 5/2x^4 + C
