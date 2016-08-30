#include "mesh_iterators.hpp"
#include "mesh.hpp"

#ifdef TEST_EFAN_ITER
#include <stdexcept>
#include <QDebug>
#endif


edge_fan_iterator::edge_fan_iterator(const Mesh *mesh, mesh_index eStart, mesh_index eCurrent) :
    m_mesh(mesh), m_eStart(eStart), m_eCurrent(eCurrent)
#ifdef TEST_EFAN_ITER
    , m_iterations(0)
#endif
{ }

edge_fan_iterator::edge_fan_iterator() :
    edge_fan_iterator(nullptr, inv_index, inv_index) { }

edge_fan_iterator::edge_fan_iterator(const Mesh *mesh, mesh_index eStart) :
    edge_fan_iterator(mesh, eStart, eStart) { }

edge_fan_iterator::reference edge_fan_iterator::operator*() const
{
    return m_eCurrent;
}

edge_fan_iterator &edge_fan_iterator::operator++()
{
    mesh_index o = m_mesh->eOpposite(m_eCurrent);

    bool terminate = false;

    if (m_mesh->eIsBoundary(o)) {
        terminate = true; // stop at boundary edge
    } else {
        m_eCurrent = m_mesh->eNext(o);

        if (m_eCurrent == m_eStart) {
            terminate = true; // stop when we reached the beginning again
        }
    }

    if (terminate) {
        m_eCurrent = inv_index; // this marks the iterator as "past the end"
    }

    // this debugging-test is intended to detect infinite loops caused by corrupted mesh structures
#ifdef TEST_EFAN_ITER
    ++m_iterations;
    if (m_iterations > MAX_EFAN_ITERATIONS) {
        qDebug() << "edge_fan_iterator:";
        qDebug() << " - m_eStart:" << m_eStart;
        qDebug() << " - m_eCurrent:" << m_eCurrent;
        throw std::runtime_error("maximum iterations reached!");
    }
#endif

    return *this;
}

edge_fan_iterator edge_fan_iterator::operator++(int)
{
    edge_fan_iterator copy = *this;

    this->operator++();

    return copy;
}

bool edge_fan_iterator::operator==(const edge_fan_iterator &other) const
{
    return other.m_mesh == m_mesh
            && other.m_eStart == m_eStart
            && other.m_eCurrent == m_eCurrent;
}

bool edge_fan_iterator::operator!=(const edge_fan_iterator &other) const
{
    return other.m_mesh != m_mesh
            || other.m_eStart != m_eStart
            || other.m_eCurrent != m_eCurrent;
}
