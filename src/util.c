//
// Created by Matej on 14. 06. 2016.
//

#include "util.h"

int32_t max(int32_t a, int32_t b) {
    return a > b ? a : b;
}

int32_t min(int32_t a, int32_t b) {
    return a < b ? a : b;
}

int32_t divCeil(int32_t dividend, int32_t divisor) {
    return (dividend + divisor - 1) / divisor;
}





