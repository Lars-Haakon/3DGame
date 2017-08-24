#include "../../include/core/math3d.h"

Vector3f Vector3f::Rotate(const Quaternion& rotation) const
{
	Quaternion conjugateQ = rotation.Conjugate();
	Quaternion w = rotation * (*this) * conjugateQ;

	return Vector3f(w.GetX(), w.GetY(), w.GetZ());
}