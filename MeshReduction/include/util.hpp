#ifndef UTIL_HPP
#define UTIL_HPP

#include <utility>
#include <array>
#include "glm/glm.hpp"

/*!
 * \brief hash combining function
 * (implementation is based on boost: http://www.boost.org/doc/libs/1_53_0/doc/html/hash/reference.html#boost.hash_combine)
 */
template<typename T>
void hash_combine(size_t& seed, T const& v)
{
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline glm::vec3 triangleCross(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    return glm::cross(p1 - p0, p2 - p0);
}

inline glm::vec3 triangleNormal(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    return glm::normalize(triangleCross(p0, p1, p2));
}

inline float triangleArea(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    return glm::length(triangleCross(p0, p1, p2)) * 0.5f;
}

namespace std
{
    template<typename T, typename U>
    struct hash<pair<T,U>>
    {
        typedef pair<T,U> argument_type;
        typedef size_t result_type;

        result_type operator() (const argument_type& v) const {
            result_type result = std::hash<T>()(v.first);
            hash_combine(result, v.second);
            return result;
        }
    };
}

struct sym_mat3
{
    std::array<float, 6> m_data;

    sym_mat3();
    sym_mat3(float m11, float m12, float m13, float m22, float m23, float m33);

    sym_mat3& operator+=(const sym_mat3& other);
    sym_mat3 operator+(const sym_mat3& other) const;

    sym_mat3& operator*=(float factor);
    sym_mat3 operator*(float factor) const;

    glm::vec3 operator*(const glm::vec3& v) const;

    explicit operator glm::mat3() const;
};

struct Quadric
{
    sym_mat3 m_A;
    glm::vec3 m_b;
    float m_c;

    Quadric();
    Quadric(const glm::vec3& n, float d);
    Quadric(const glm::vec3& n, const glm::vec3& p);

    Quadric& operator+=(const Quadric& other);
    Quadric operator+(const Quadric& other) const;

    Quadric& operator*=(float factor);
    Quadric operator*(float factor) const;

    float operator()(const glm::vec3& v) const;

    bool optimum(glm::vec3* v, float* cost) const;
};

#endif // UTIL_HPP
