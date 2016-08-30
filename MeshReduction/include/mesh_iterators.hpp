#ifndef MESH_ITERATORS_HPP
#define MESH_ITERATORS_HPP

#include "mesh_index.hpp"

#include <iterator>

// comment this out to disable iteration tests in debug mode
#define TEST_EFAN_ITER_DEBUG

#if defined(_DEBUG) && defined(TEST_EFAN_ITER_DEBUG)
#define TEST_EFAN_ITER
#endif

#ifdef TEST_EFAN_ITER
#define MAX_EFAN_ITERATIONS 1000
#endif

class Mesh;

/*!
 * \brief custom iterator class, which can iterate over an edge fan (all halfedges originating from a single vertex)
 */
class edge_fan_iterator : public std::iterator<std::forward_iterator_tag, mesh_index,
                                               std::ptrdiff_t, const mesh_index*, const mesh_index&>
{
private:
    const Mesh* m_mesh;
    mesh_index m_eStart, m_eCurrent;

#ifdef TEST_EFAN_ITER
    unsigned int m_iterations;
#endif

public:
    edge_fan_iterator();
    edge_fan_iterator(const Mesh* mesh, mesh_index eStart, mesh_index eCurrent);
    edge_fan_iterator(const Mesh* mesh, mesh_index eStart);

    reference operator*() const;

    edge_fan_iterator& operator++();
    edge_fan_iterator operator++(int);

    bool operator==(const edge_fan_iterator& other) const;
    bool operator!=(const edge_fan_iterator& other) const;
};

/*!
 * \brief helper class to enable for-each loop syntax
 */
class edge_fan
{
public:
    typedef edge_fan_iterator iterator;

private:
    const Mesh* m_mesh;
    mesh_index m_eStart;

public:
    edge_fan() : m_mesh(nullptr), m_eStart(inv_index) { }
    edge_fan(const Mesh* mesh, mesh_index eStart) : m_mesh(mesh), m_eStart(eStart) { }

    iterator begin() const { return iterator(m_mesh, m_eStart, m_eStart); }
    iterator end() const { return iterator(m_mesh, m_eStart, inv_index); }
};

#endif // MESH_ITERATORS_HPP
