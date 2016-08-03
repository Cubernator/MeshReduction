#ifndef UTIL_HPP
#define UTIL_HPP

#include <utility>

/*!
 * \brief hash combining function
 * (implementation is based on boost: http://www.boost.org/doc/libs/1_53_0/doc/html/hash/reference.html#boost.hash_combine)
 */
template<typename T>
void hash_combine(size_t& seed, T const& v)
{
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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

#endif // UTIL_HPP
