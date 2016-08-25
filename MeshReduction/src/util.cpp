#include "util.hpp"

#include <algorithm>
#include <functional>

sym_mat3::sym_mat3()
{
    m_data.fill(0.0f);
}

sym_mat3::sym_mat3(float m11, float m12, float m13, float m22, float m23, float m33)
                : m_data({m11, m12, m13, m22, m23, m33}) { }

sym_mat3 &sym_mat3::operator+=(const sym_mat3 &other)
{
    std::transform(m_data.begin(), m_data.end(),
                   other.m_data.begin(),
                   m_data.begin(),
                   std::plus<float>());

    return *this;
}

sym_mat3 sym_mat3::operator+(const sym_mat3 &other) const
{
    sym_mat3 result(*this);
    return result += other;
}

sym_mat3 &sym_mat3::operator*=(float factor)
{
    std::transform(m_data.begin(), m_data.end(), m_data.begin(),
                   std::bind(std::multiplies<float>(), std::placeholders::_1, factor));

    return *this;
}

sym_mat3 sym_mat3::operator*(float factor) const
{
    sym_mat3 result(*this);
    return result *= factor;
}

glm::vec3 sym_mat3::operator*(const glm::vec3 &v) const
{
    return glm::vec3(m_data[0] * v.x + m_data[1] * v.y + m_data[2] * v.z,
                     m_data[1] * v.x + m_data[3] * v.y + m_data[4] * v.z,
            m_data[2] * v.x + m_data[4] * v.y + m_data[5] * v.z);
}

sym_mat3::operator glm::mat3() const
{
    return glm::mat3(m_data[0], m_data[1], m_data[2],
                     m_data[1], m_data[3], m_data[4],
                     m_data[2], m_data[4], m_data[5]);
}

Quadric::Quadric() : m_A(), m_b(0.0f, 0.0f, 0.0f), m_c(0.0f) { }

Quadric::Quadric(const glm::vec3& n, float d) :
    m_A(n.x * n.x, n.x * n.y, n.x * n.z,
                   n.y * n.y, n.y * n.z,
                              n.z * n.z),
    m_b(d * n),
    m_c(d * d) { }

Quadric::Quadric(const glm::vec3 &n, const glm::vec3 &p) : Quadric(n, -glm::dot(n, p)) { }

Quadric &Quadric::operator+=(const Quadric &other)
{
    m_A += other.m_A;
    m_b += other.m_b;
    m_c += other.m_c;

    return *this;
}

Quadric Quadric::operator+(const Quadric &other) const
{
    Quadric result(*this);
    return result += other;
}

Quadric &Quadric::operator*=(float factor)
{
    m_A *= factor;
    m_b *= factor;
    m_c *= factor;

    return *this;
}

Quadric Quadric::operator*(float factor) const
{
    Quadric result(*this);
    return result *= factor;
}

float Quadric::operator()(const glm::vec3 &v) const
{
    return glm::dot(v, m_A * v) + 2.0f * glm::dot(m_b, v) + m_c;
}

bool Quadric::optimum(glm::vec3 *v, float *cost) const
{
    glm::mat3 A(m_A);
    float det = glm::determinant(A);

    if (glm::abs(det) < 0.001f) // matrix not invertible or poorly conditioned
        return false;

    glm::mat3 iA = glm::inverse(A);
    glm::vec3 tmp = iA * m_b;
    *v = -tmp;
    *cost = -glm::dot(m_b, tmp) + m_c;

    return true;
}

