#ifndef FHE_RESIZE_H
#define FHE_RESIZE_H

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <fstream>

#include "seal/seal.h"
#include <opencv2/opencv.hpp>

#define BILINEAR 0
#define BICUBIC 1

using namespace seal;
using namespace cv;


void approximated_step(Ciphertext &amplitude, Ciphertext &b1, Ciphertext &b2) {

} 

/* We want to find the Fourier decomposition of a step function at b_1 and b_2, 
 * and then 0 everywhere else from 0 to 64. 
 * 1             ________
 *              |        |
 *              |        |
 * 0  __________|        |_____________________
 *   0         b1        b2                   64
 * We will shift the origin to be at (b1 + b2)/2 and then calculate the Fourier 
 * decomposition there for the coefficients. 
 * 
 * Returns a vector of coefficients and sine stuff
 * First term is b/64
 * Other terms will be (2/(k Pi)) Sin(k b pi/64) Cos(k x pi/64) 
 * Degree will just be the number of terms of approximation
 * Will return the coefficients without the B!
 */
void calculate_coefficients(std::vector<double> &coeff,
                            std::vector<double> &sincoeff,
                            std::vector<double> &coscoeff,
                            int degree) {
    for (int i = 1; i <= degree; i++) {
        coeff.push_back(2.0/(i * M_PI));
        sincoeff.push_back(i * M_PI/64.0);
        coscoeff.push_back(i * M_PI/64.0);
    }
}

/* We will approximate with the center at x = 3pi/2, because our values of b
 * will be between 1 to 64 at max, which means that with the coefficients
 * we will get better approximations because of range of B. The coefficients will be
 * -1 + 1/2 (x - (3 \[Pi])/2)^2 - 1/24 (x - (3 \[Pi])/2)^4 + 
 * 1/720 (x - (3 \[Pi])/2)^6 - (x - (3 \[Pi])/2)^8/40320
 */
Ciphertext homomorphic_sine(Ciphertext &x,
                            Evaluator &evaluator, 
                            FractionalEncoder &encoder, 
                            Encryptor &encryptor) {
    Ciphertext res;
    encryptor.encrypt(encoder.encode(0), res);

    Ciphertext shifted_x(x);
    evaluator.sub_plain(shifted_x, encoder.encode(3 * M_PI));

    Ciphertext power2(shifted_x);
    Ciphertext power4(shifted_x);
    Ciphertext power6(shifted_x);
    Ciphertext power8(shifted_x);

    // 2nd order term
    evaluator.square(power2);
    evaluator.multiply_plain(power2, encoder.encode(0.5));

    // 4th order term
    evaluator.square(power4);
    evaluator.square(power4);
    evaluator.multiply_plain(power4, encoder.encode(-1.0/24.0));

    // 6th order term 
    evaluator.square(power6);
    evaluator.square(power6);
    evaluator.multiply(power6, shifted_x);     
    evaluator.multiply(power6, shifted_x);     
    evaluator.multiply_plain(power6, encoder.encode(1.0/720.0));

    // 8th order term
    evaluator.square(power8);
    evaluator.square(power8);
    evaluator.square(power8);
    evaluator.multiply_plain(power8, encoder.encode(-1.0/40320.0));

    // Add 0th order term and everything up 
    evaluator.add_plain(res, encoder.encode(-1.0));
    evaluator.add(res, power2);
    evaluator.add(res, power4);
    evaluator.add(res, power6);
    evaluator.add(res, power8);

    return res;
}

#endif