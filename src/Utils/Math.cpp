#include "Math.hpp"

#include "Modules/Console.hpp"
#include "SDK.hpp"

#include <cmath>
#include <float.h>
#include <random>

float Math::AngleNormalize(float angle) {
	angle = fmodf(angle, 360.0f);
	if (angle > 180) {
		angle -= 360;
	}
	if (angle < -180) {
		angle += 360;
	}
	return angle;
}
float Math::VectorNormalize(Vector &vec) {
	auto radius = sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	auto iradius = 1.f / (radius + FLT_EPSILON);

	vec.x *= iradius;
	vec.y *= iradius;
	vec.z *= iradius;

	return radius;
}
void Math::AngleVectors(const QAngle &angles, Vector *forward) {
	float sp, sy, cp, cy;

	Math::SinCos(DEG2RAD(angles.y), &sy, &cy);
	Math::SinCos(DEG2RAD(angles.x), &sp, &cp);

	forward->x = cp * cy;
	forward->y = cp * sy;
	forward->z = -sp;
}
void Math::AngleVectors(const QAngle &angles, Vector *forward, Vector *right, Vector *up) {
	float sr, sp, sy, cr, cp, cy;

	Math::SinCos(DEG2RAD(angles.y), &sy, &cy);
	Math::SinCos(DEG2RAD(angles.x), &sp, &cp);
	Math::SinCos(DEG2RAD(angles.z), &sr, &cr);

	if (forward) {
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}

	if (right) {
		right->x = -1 * sr * sp * cy + -1 * cr * -sy;
		right->y = -1 * sr * sp * sy + -1 * cr * cy;
		right->z = -1 * sr * cp;
	}

	if (up) {
		up->x = cr * sp * cy + -sr * -sy;
		up->y = cr * sp * sy + -sr * cy;
		up->z = cr * cp;
	}
}
void Math::VectorAngles(Vector& forward, Vector& pseudoup, QAngle* angles) {
	if (!angles) return;
	Vector left = pseudoup.Cross(forward).Normalize();

	float xyDist = forward.Length2D();

	if (xyDist > 0.001f) {
		angles->y = RAD2DEG(atan2f(forward.y, forward.x));
		angles->x = RAD2DEG(atan2f(-forward.z, xyDist));
		float up_z = (left.y * forward.x) - (left.x * forward.y);
		angles->z = RAD2DEG(atan2f(left.z, up_z));
	} else {
		angles->y = RAD2DEG(atan2f(-left.x, left.y));
		angles->x = RAD2DEG(atan2f(-forward.z, xyDist));
		angles->z = 0;
	}
}
float Math::RandomNumber(const float &min, const float &max) {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<float> dist_pitch(min, std::nextafter(max, FLT_MAX));

	return dist_pitch(mt);
}
int Math::RandomNumber(const int &min, const int &max) {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> dist_pitch(min, max);

	return dist_pitch(mt);
}
bool Math::LUdecomposition(const double matrix[4][4], unsigned char outP[4], unsigned char outQ[4], double outL[4][4], double outU[4][4]) {
	const double *nMatrix[4] = {matrix[0], matrix[1], matrix[2], matrix[3]};
	double *nOutL[4] = {outL[0], outL[1], outL[2], outL[3]};
	double *nOutU[4] = {outU[0], outU[1], outU[2], outU[3]};

	for (int i = 0; i < 4; ++i) {
		outP[i] = i;
		outQ[i] = i;

		for (int j = 0; j < 4; ++j) {
			outU[i][j] = matrix[i][j];
		}
	}

	for (int n = 0; n < 4 - 1; ++n) {
		int t = -1;
		double maxVal = 0;
		for (int i = n; i < 4; ++i) {
			double tabs = abs(outU[i][n]);
			if (maxVal < tabs) {
				t = i;
				maxVal = tabs;
			}
		}

		if (t < 0)
			return false;

		if (n != t) {
			unsigned char ucTmp = outP[n];
			outP[n] = outP[t];
			outP[t] = ucTmp;

			for (int i = 0; i < 4; ++i) {
				double dTmp = outU[n][i];
				outU[n][i] = outU[t][i];
				outU[t][i] = dTmp;
			}
		}

		for (int i = n + 1; i < 4; ++i) {
			outU[i][n] = outU[i][n] / outU[n][n];
			for (int j = n + 1; j < 4; ++j) {
				outU[i][j] = outU[i][j] - outU[i][n] * outU[n][j];
			}
		}
	}

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < i; ++j) {
			outL[i][j] = outU[i][j];
			outU[i][j] = 0;
		}
		outL[i][i] = 1;
		for (int j = i + 1; j < 4; ++j) {
			outL[i][j] = 0;
		}
	}

	return true;
}
void Math::SolveWithLU(const double L[4][4], const double U[4][4], const unsigned char P[4], const unsigned char Q[4], const double b[4], double outX[4]) {
	double y[4];
	const double *nL[4] = {L[0], L[1], L[2], L[3]};
	const double *nU[4] = {U[0], U[1], U[2], U[3]};

	for (int i = 0; i < 4; i++) {
		double sum = 0;
		for (int k = 0; k < i; k++)
			sum += L[i][k] * y[k];

		y[i] = 1.0 / L[i][i] * (b[P[i]] - sum);
	}

	for (int i = 4 - 1; i >= 0; i--) {
		double sum = 0;
		for (int k = i + 1; k < 4; k++)
			sum += U[i][k] * outX[Q[k]];

		outX[Q[i]] = 1.0 / U[i][i] * (y[i] - sum);
	}
}
void Math::ConcatTransforms(const matrix3x4_t &in1, const matrix3x4_t &in2, matrix3x4_t &out) {
	out.m_flMatVal[0][0] = in1.m_flMatVal[0][0] * in2.m_flMatVal[0][0] + in1.m_flMatVal[0][1] * in2.m_flMatVal[1][0] + in1.m_flMatVal[0][2] * in2.m_flMatVal[2][0];
	out.m_flMatVal[0][1] = in1.m_flMatVal[0][0] * in2.m_flMatVal[0][1] + in1.m_flMatVal[0][1] * in2.m_flMatVal[1][1] + in1.m_flMatVal[0][2] * in2.m_flMatVal[2][1];
	out.m_flMatVal[0][2] = in1.m_flMatVal[0][0] * in2.m_flMatVal[0][2] + in1.m_flMatVal[0][1] * in2.m_flMatVal[1][2] + in1.m_flMatVal[0][2] * in2.m_flMatVal[2][2];
	out.m_flMatVal[0][3] = in1.m_flMatVal[0][0] * in2.m_flMatVal[0][3] + in1.m_flMatVal[0][1] * in2.m_flMatVal[1][3] + in1.m_flMatVal[0][2] * in2.m_flMatVal[2][3] + in1.m_flMatVal[0][3];
	out.m_flMatVal[1][0] = in1.m_flMatVal[1][0] * in2.m_flMatVal[0][0] + in1.m_flMatVal[1][1] * in2.m_flMatVal[1][0] + in1.m_flMatVal[1][2] * in2.m_flMatVal[2][0];
	out.m_flMatVal[1][1] = in1.m_flMatVal[1][0] * in2.m_flMatVal[0][1] + in1.m_flMatVal[1][1] * in2.m_flMatVal[1][1] + in1.m_flMatVal[1][2] * in2.m_flMatVal[2][1];
	out.m_flMatVal[1][2] = in1.m_flMatVal[1][0] * in2.m_flMatVal[0][2] + in1.m_flMatVal[1][1] * in2.m_flMatVal[1][2] + in1.m_flMatVal[1][2] * in2.m_flMatVal[2][2];
	out.m_flMatVal[1][3] = in1.m_flMatVal[1][0] * in2.m_flMatVal[0][3] + in1.m_flMatVal[1][1] * in2.m_flMatVal[1][3] + in1.m_flMatVal[1][2] * in2.m_flMatVal[2][3] + in1.m_flMatVal[1][3];
	out.m_flMatVal[2][0] = in1.m_flMatVal[2][0] * in2.m_flMatVal[0][0] + in1.m_flMatVal[2][1] * in2.m_flMatVal[1][0] + in1.m_flMatVal[2][2] * in2.m_flMatVal[2][0];
	out.m_flMatVal[2][1] = in1.m_flMatVal[2][0] * in2.m_flMatVal[0][1] + in1.m_flMatVal[2][1] * in2.m_flMatVal[1][1] + in1.m_flMatVal[2][2] * in2.m_flMatVal[2][1];
	out.m_flMatVal[2][2] = in1.m_flMatVal[2][0] * in2.m_flMatVal[0][2] + in1.m_flMatVal[2][1] * in2.m_flMatVal[1][2] + in1.m_flMatVal[2][2] * in2.m_flMatVal[2][2];
	out.m_flMatVal[2][3] = in1.m_flMatVal[2][0] * in2.m_flMatVal[0][3] + in1.m_flMatVal[2][1] * in2.m_flMatVal[1][3] + in1.m_flMatVal[2][2] * in2.m_flMatVal[2][3] + in1.m_flMatVal[2][3];
}
void Math::AngleMatrix(const QAngle &angles, matrix3x4_t &mat) {
	float sr, sp, sy, cr, cp, cy;

	SinCos(DEG2RAD(angles.y), &sy, &cy);
	SinCos(DEG2RAD(angles.x), &sp, &cp);
	SinCos(DEG2RAD(angles.z), &sr, &cr);

	mat.m_flMatVal[0][0] = cp * cy;
	mat.m_flMatVal[1][0] = cp * sy;
	mat.m_flMatVal[2][0] = -sp;

	float crcy = cr * cy;
	float crsy = cr * sy;
	float srcy = sr * cy;
	float srsy = sr * sy;
	mat.m_flMatVal[0][1] = sp * srcy - crsy;
	mat.m_flMatVal[1][1] = sp * srsy + crcy;
	mat.m_flMatVal[2][1] = sr * cp;

	mat.m_flMatVal[0][2] = (sp * crcy + srsy);
	mat.m_flMatVal[1][2] = (sp * crsy - srcy);
	mat.m_flMatVal[2][2] = cr * cp;

	mat.m_flMatVal[0][3] = 0.0f;
	mat.m_flMatVal[1][3] = 0.0f;
	mat.m_flMatVal[2][3] = 0.0f;
}
void Math::AngleMatrix(const QAngle& angles, const Vector& position, matrix3x4_t& mat) {
	AngleMatrix(angles, mat);
	MatrixSetColumn(position, 3, mat);
}
void Math::MatrixSetColumn(const Vector &in, int column, matrix3x4_t &out) {
	out.m_flMatVal[0][column] = in.x;
	out.m_flMatVal[1][column] = in.y;
	out.m_flMatVal[2][column] = in.z;
}


Matrix::Matrix(int rows, int cols, const double init)
	: rows(rows)
	, cols(cols) {
	this->mat.resize(rows);
	for (auto &it : this->mat) {
		it.resize(cols, init);
	}
}

Matrix &Matrix::operator=(const Matrix &rhs) {
	if (&rhs == this)
		return *this;

	auto new_rows = rhs.rows;
	auto new_cols = rhs.cols;

	this->mat.resize(new_rows);
	for (auto &it : this->mat) {
		it.resize(new_cols);
	}

	for (unsigned int i = 0; i < new_rows; ++i)
		for (unsigned int j = 0; j < new_cols; ++j)
			mat[i][j] = rhs(i, j);

	this->rows = new_rows;
	this->cols = new_cols;

	return *this;
}

Matrix Matrix::operator+(const Matrix &rhs) {
	Matrix result(this->rows, this->cols, 0);

	for (unsigned int i = 0; i < this->rows; ++i)
		for (unsigned int j = 0; j < this->cols; ++j)
			result(i, j) = this->mat[i][j] + rhs(i, j);

	return result;
}

Matrix &Matrix::operator+=(const Matrix &rhs) {
	auto new_rows = rhs.rows;
	auto new_cols = rhs.cols;

	for (unsigned int i = 0; i < new_rows; ++i)
		for (unsigned int j = 0; j < new_cols; ++j)
			this->mat[i][j] += rhs(i, j);

	return *this;
}

Matrix Matrix::operator*(const Matrix &rhs) {
	if (this->cols != rhs.rows) {
		console->Print("Error: rows != cols\n");
		return Matrix(1, 1, 0);
	}

	Matrix result(this->rows, rhs.cols, 0);

	int sum;
	for (unsigned int i = 0; i < this->rows; ++i) {
		for (unsigned int j = 0; j < rhs.cols; ++j) {
			sum = 0;
			for (unsigned int k = 0; k < this->rows; ++k) {
				sum += this->mat[i][k] * rhs(k, j);
			}
			result(i, j) += sum;
		}
	}

	return result;
}

Matrix &Matrix::operator*=(const Matrix &rhs) {
	Matrix result = (*this) * rhs;
	(*this) = result;
	return *this;
}

Vector Matrix::operator*(const Vector &rhs) {
	if (this->cols != 3) {
		console->Print("Error: rows != cols\n");
		return Vector(0, 0, 0);
	}

	Vector result(0, 0, 0);

	for (unsigned int i = 0; i < this->rows; ++i) {
		for (unsigned int j = 0; j < 3; ++j) {
			result[i] += this->mat[i][j] * rhs[j];
		}
	}

	return result;
}

Vector Matrix::operator*=(const Vector &rhs) {
	return (*this) * rhs;
}

void Matrix::Print() {
	for (unsigned int i = 0; i < this->rows; ++i) {
		for (unsigned int j = 0; j < this->cols; ++j) {
			console->Print("%d ", this->mat[i][j]);
		}
		console->Print("\n");
	}
}
