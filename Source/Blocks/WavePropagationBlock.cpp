/**
 * @file
 * This file is part of SWE.
 *
 * @author Alexander Breuer (breuera AT in.tum.de, http://www5.in.tum.de/wiki/index.php/Dipl.-Math._Alexander_Breuer)
 * @author Sebastian Rettenberger (rettenbs AT in.tum.de,
 * http://www5.in.tum.de/wiki/index.php/Sebastian_Rettenberger,_M.Sc.)
 * @author Michael Bader (bader AT in.tum.de, http://www5.in.tum.de/wiki/index.php/Michael_Bader)
 *
 * @section LICENSE
 *
 * SWE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SWE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SWE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * @section DESCRIPTION
 *
 * Implementation of Blocks::Block that uses solvers in the wave propagation formulation.
 */

#include "WavePropagationBlock.hpp"

#include <iostream>
#include <immintrin.h>





Blocks::WavePropagationBlock::WavePropagationBlock(int nx, int ny, RealType dx, RealType dy):
  Block(nx, ny, dx, dy),
  hNetUpdatesLeft_(nx + 1, ny),
  hNetUpdatesRight_(nx + 1, ny),
  huNetUpdatesLeft_(nx + 1, ny),
  huNetUpdatesRight_(nx + 1, ny),
  hNetUpdatesBelow_(nx, ny + 1),
  hNetUpdatesAbove_(nx, ny + 1),
  hvNetUpdatesBelow_(nx, ny + 1),
  hvNetUpdatesAbove_(nx, ny + 1) {}

Blocks::WavePropagationBlock::WavePropagationBlock(
  int nx, int ny, RealType dx, RealType dy,
  Tools::Float2D<RealType>& h,
  Tools::Float2D<RealType>& hu,
  Tools::Float2D<RealType>& hv
):
  Block(nx, ny, dx, dy, h, hu, hv),
  hNetUpdatesLeft_(nx + 1, ny),
  hNetUpdatesRight_(nx + 1, ny),
  huNetUpdatesLeft_(nx + 1, ny),
  huNetUpdatesRight_(nx + 1, ny),
  hNetUpdatesBelow_(nx, ny + 1),
  hNetUpdatesAbove_(nx, ny + 1),
  hvNetUpdatesBelow_(nx, ny + 1),
  hvNetUpdatesAbove_(nx, ny + 1) {}




void Blocks::WavePropagationBlock::computeNumericalFluxes() {
  // Maximum (linearized) wave speed within one iteration
  RealType maxWaveSpeed = RealType(0.0);
//_mm256_loadu_pd(&hu_.data_[(i-1)*(nx_+2)+j])
  // Compute the net-updates for the vertical edges
  for (int i = 1; i < nx_ + 2; i++) {
    for (int j = 1; j < ny_ + 1; j+=4) {
      __m256d maxEdgeSpeed = _mm256_setzero_pd();
      __m256d hleft_simd = _mm256_loadu_pd(&h_.data_[(i-1)*(nx_+2)+j]);
      __m256d hright_simd = _mm256_loadu_pd(&h_.data_[i*(nx_+2)+j]);
      __m256d huleft_simd = _mm256_loadu_pd(&hu_.data_[(i-1)*(nx_+2)+j]);
      __m256d huright_simd = _mm256_loadu_pd(&hu_.data_[i*(nx_+2)+j]);
      __m256d bleft_simd = _mm256_loadu_pd(&b_.data_[(i-1)*(nx_+2)+j]);
      __m256d bright_simd = _mm256_loadu_pd(&b_.data_[i*(nx_+2)+j]);
      __m256d hNetUpdatesLeft_simd = _mm256_loadu_pd(&hNetUpdatesLeft_.data_[(i-1)*(nx_+2)+j-1]);
      __m256d hNetUpdatesRight_simd = _mm256_loadu_pd(&hNetUpdatesRight_.data_[(i-1)*(nx_+2)+j-1]);
      __m256d huNetUpdatesLeft_simd = _mm256_loadu_pd(&huNetUpdatesLeft_.data_[(i-1)*(nx_+2)+j-1]);
      __m256d huNetUpdatesRight_simd = _mm256_loadu_pd(&huNetUpdatesRight_.data_[(i-1)*(nx_+2)+j-1]);
      wavePropagationSolver_.computeNetUpdates(
        hleft_simd,
        hright_simd,
        huleft_simd,
        huright_simd,
        bleft_simd,
        bright_simd,
        hNetUpdatesLeft_simd,
        hNetUpdatesRight_simd,
        huNetUpdatesLeft_simd,
        huNetUpdatesRight_simd,
        maxEdgeSpeed
      );
      alignas(32) double vecArr[4];_mm256_storeu_pd(vecArr, maxEdgeSpeed);
      double maximum = vecArr[0];
      for(int i = 0;i<4;i++)maximum = std::max(maximum,vecArr[i]);
      // Update the thread-local maximum wave speed
      maxWaveSpeed = std::max(maxWaveSpeed, maximum);
    }
  }

  // Compute the net-updates for the horizontal edges
  for (int i = 1; i < nx_ + 1; i++) {
    for (int j = 1; j < ny_ + 2; j+=4) {
      __m256d maxEdgeSpeed = _mm256_setzero_pd();
      __m256d hleft_simd = _mm256_loadu_pd(&h_.data_[i*(nx_+2)+j-1]);
      __m256d hright_simd = _mm256_loadu_pd(&h_.data_[i*(nx_+2)+j]);
      __m256d hvleft_simd = _mm256_loadu_pd(&hv_.data_[i*(nx_+2)+j-1]);
      __m256d hvright_simd = _mm256_loadu_pd(&hv_.data_[i*(nx_+2)+j]);
      __m256d bleft_simd = _mm256_loadu_pd(&b_.data_[i*(nx_+2)+j-1]);
      __m256d bright_simd = _mm256_loadu_pd(&b_.data_[i*(nx_+2)+j]);
      __m256d hNetUpdatesBelow_simd = _mm256_loadu_pd(&hNetUpdatesBelow_.data_[(i-1)*(nx_+2)+j-1]);
      __m256d hNetUpdatesAbove_simd = _mm256_loadu_pd(&hNetUpdatesAbove_.data_[(i-1)*(nx_+2)+j-1]);
      __m256d hvNetUpdatesBelow_simd = _mm256_loadu_pd(&hvNetUpdatesBelow_.data_[(i-1)*(nx_+2)+j-1]);
      __m256d hvNetUpdatesAbove_simd = _mm256_loadu_pd(&hvNetUpdatesAbove_.data_[(i-1)*(nx_+2)+j-1]);

      wavePropagationSolver_.computeNetUpdates(
        hleft_simd,
        hright_simd,
        hvleft_simd,
        hvright_simd,
        bleft_simd,
        bright_simd,
        hNetUpdatesBelow_simd,
        hNetUpdatesAbove_simd,
        hvNetUpdatesBelow_simd,
        hvNetUpdatesAbove_simd,
        maxEdgeSpeed
      );
      alignas(32) double vecArr[4];_mm256_storeu_pd(vecArr, maxEdgeSpeed);
      double maximum = vecArr[0];
      for(int i = 0;i<4;i++)maximum = std::max(maximum,vecArr[i]);
      // Update the thread-local maximum wave speed
      maxWaveSpeed = std::max(maxWaveSpeed, maximum);
    }
  }

  if (maxWaveSpeed > 0.00001) {
    // Compute the time step width
    maxTimeStep_ = std::min(dx_ / maxWaveSpeed, dy_ / maxWaveSpeed);

    // Reduce maximum time step size by "safety factor"
    maxTimeStep_ *= RealType(0.4); // CFL-number = 0.5
  } else {
    // Might happen in dry cells
    maxTimeStep_ = std::numeric_limits<RealType>::max();
  }
}

void Blocks::WavePropagationBlock::updateUnknowns(RealType dt) {
  // Update cell averages with the net-updates
  for (int i = 1; i < nx_ + 1; i++) {
    for (int j = 1; j < ny_ + 1; j++) {
      h_[i][j] -= dt / dx_ * (hNetUpdatesRight_[i - 1][j - 1] + hNetUpdatesLeft_[i][j - 1])
                  + dt / dy_ * (hNetUpdatesAbove_[i - 1][j - 1] + hNetUpdatesBelow_[i - 1][j]);
      hu_[i][j] -= dt / dx_ * (huNetUpdatesRight_[i - 1][j - 1] + huNetUpdatesLeft_[i][j - 1]);
      hv_[i][j] -= dt / dy_ * (hvNetUpdatesAbove_[i - 1][j - 1] + hvNetUpdatesBelow_[i - 1][j]);

      if (h_[i][j] < 0) {
#ifndef NDEBUG
        // Only print this warning when debug is enabled
        // Otherwise we cannot vectorize this loop
        if (h_[i][j] < -0.1) {
          std::cerr << "Warning, negative height: (i,j)=(" << i << "," << j << ")=" << h_[i][j] << std::endl;
          std::cerr << "         b: " << b_[i][j] << std::endl;
        }
#endif

        // Zero (small) negative depths
        h_[i][j] = hu_[i][j] = hv_[i][j] = RealType(0.0);
      } else if (h_[i][j] < 0.1) {             // dryTol
        hu_[i][j] = hv_[i][j] = RealType(0.0); // No water, no speed!
      }
    }
  }
}
