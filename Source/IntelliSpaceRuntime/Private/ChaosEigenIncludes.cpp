// ChaosEigenIncludes.cpp
// This TU exists only to ensure Eigen's solver headers are compiled
// with the same configuration/macros as the rest of the module.

#include "CoreMinimal.h"

// Match Epic's Eigen usage (usually set in Build.cs, but harmless here too)
#ifndef EIGEN_MPL2_ONLY
#define EIGEN_MPL2_ONLY 1
#endif

// Keep third-party warnings isolated the UE way
THIRD_PARTY_INCLUDES_START
#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>
THIRD_PARTY_INCLUDES_END

// No executable code here. Just including the headers ensures consistent
// template visibility & macros across iOS builds that touch Chaos/Eigen.
