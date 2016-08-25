#ifndef MESH_DECIMATOR_HPP
#define MESH_DECIMATOR_HPP

#include <vector>
#include <unordered_map>
#include <functional>

#include <QObject>
#include <QMutex>

#include "util.hpp"
#include "mesh_index.hpp"

class Mesh;

class MeshDecimator : public QObject
{
    struct VertexPair
    {
        mesh_index m_v0, m_v1;
        glm::vec3 m_newPos;
        float m_cost;
        bool m_removed;

        VertexPair(mesh_index v0, mesh_index v1) : m_v0(v0), m_v1(v1), m_removed(false) { }

        bool isValid() const { return is_valid(m_v0) && is_valid(m_v1); }
        void invalidate() { m_v0 = m_v1 = inv_index; }

        bool isRemoved() const { return m_removed; }
        void remove() { m_removed = true; }
        void unremove() { m_removed = false; }
    };

    struct VertexPairCostComparer
    {
        const std::vector<VertexPair>* m_pairs;

        VertexPairCostComparer(const std::vector<VertexPair>& pairs) : m_pairs(&pairs) { }

        bool operator()(std::size_t lhs, std::size_t rhs) const {
            return (*m_pairs)[lhs].m_cost < (*m_pairs)[rhs].m_cost;
        }
    };

    Q_OBJECT

private:
    Mesh * m_mesh;
    unsigned int m_targetFaceCount, m_oldFaceCount, m_currentFaceCount, m_lastAttemptFaceCount;

    bool m_abort;

    std::vector<Quadric> m_quadrics;
    std::vector<VertexPair> m_pairs;
    std::vector<std::size_t> m_pairsByCost;
    std::unordered_multimap<mesh_index, std::size_t> m_pairsByVertex;

    VertexPairCostComparer m_costComparer;

    mutable QMutex m_mutex;

    MeshDecimator(const MeshDecimator& other) = delete;
    MeshDecimator& operator=(const MeshDecimator& other) = delete;
    MeshDecimator& operator=(MeshDecimator&& other) = delete;

    void computeQuadrics();
    void computePairCost(std::size_t p);
    void initPairs();
    void cleanupPairs();
    void initHelpers();

    bool isPairContractable(const VertexPair& pair) const;

    bool iterate();

    void updateProgress();

public:
    MeshDecimator(Mesh * mesh, unsigned int targetFaceCount);
    ~MeshDecimator();

    int progress() const;
    bool isAborting() const;

public slots:
    void start();
    void abort();

signals:
    void finished();
    void progressChanged(int value);
    void error(QString msg);
};

#endif // MESH_DECIMATOR_HPP
