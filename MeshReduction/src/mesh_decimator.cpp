#include "mesh_decimator.hpp"
#include "mesh.hpp"

#include <unordered_set>
#include <QDebug>


void MeshDecimator::computeQuadrics()
{
    m_quadrics.resize(m_mesh->vertexCount());

    const float boundaryPenalty = 100.0f;

    // compute error quadrics (Q-matrix) for every vertex
    for (mesh_index v = 0; v < m_quadrics.size(); ++v) {
        glm::vec3 vpos = m_mesh->vPosition(v);

        // iterate over edge fan to get all planes intersecting at v
        for (mesh_index e : m_mesh->vEdgeFan(v)) {
            mesh_index be = inv_index;

            if (m_mesh->eIsBoundary(e)) {
                be = m_mesh->eOpposite(e);
            } else {
                if (m_mesh->eIsBoundary(m_mesh->eOpposite(e))) {
                    be = e;
                }
                mesh_index f = m_mesh->eFace(e);
                glm::vec3 n = m_mesh->fNormal(f);

                m_quadrics[v] += Quadric(n, vpos);
            }

            if (is_valid(be)) { // is there a boundary edge?
                mesh_index of = m_mesh->eFace(be);
                glm::vec3 on = m_mesh->fNormal(of);
                glm::vec3 ev = m_mesh->eVector(be);

                // get normal of imaginary plane intersecting the edge and perpendicular to the face
                glm::vec3 cpn = glm::normalize(glm::cross(ev, on));

                // weight this quadric by a penalty
                m_quadrics[v] += Quadric(cpn, vpos) * boundaryPenalty;
            }
        }
    }
}

void MeshDecimator::computePairCost(std::size_t p)
{
    VertexPair& pair = m_pairs[p];
    float oldCost = pair.m_cost;

    Quadric Q = m_quadrics[pair.m_v0] + m_quadrics[pair.m_v1];

    if (!Q.optimum(&pair.m_newPos, &pair.m_cost)) {

        glm::vec3 vp0 = m_mesh->vPosition(pair.m_v0), vp1 = m_mesh->vPosition(pair.m_v1),
                vm = (vp0 + vp1) * 0.5f;

        pair.m_cost = std::numeric_limits<float>::max();
        for (glm::vec3 pos : {vp0, vp1, vm}) {
            float cost = Q(pos);
            if (cost < pair.m_cost) {
                pair.m_cost = cost;
                pair.m_newPos = pos;
            }
        }
    }

    if (!pair.isRemoved()) { // fix priority queue
        if (pair.m_cost < oldCost)
            m_pairsByCost.decrease(pair.m_handle);
        else if (pair.m_cost > oldCost)
            m_pairsByCost.increase(pair.m_handle);
    }
}

void MeshDecimator::initPairs()
{
    // reserve memory to avoid reallocations and/or rehashes
    m_pairs.reserve(m_mesh->edgeCount());

    std::unordered_set<mesh_index> skipEdges;
    for (mesh_index e = 0; e < m_mesh->halfedgeCount(); ++e) {
        if (skipEdges.count(e)) continue;

        mesh_index eo = m_mesh->eOpposite(e);
        skipEdges.insert(eo);

        mesh_index v0 = m_mesh->eStartVertex(e), v1 = m_mesh->eEndVertex(e);

        VertexPair pair(v0, v1);

        std::size_t p = m_pairs.size();
        m_pairs.push_back(pair);

        computePairCost(p);
    }
}

void MeshDecimator::cleanupPairs()
{
    auto it = std::remove_if(m_pairs.begin(), m_pairs.end(), [] (const VertexPair& pair) {
        return !pair.isValid();
    });

    m_pairs.erase(it, m_pairs.end());
}

void MeshDecimator::initHelpers()
{
    m_pairsByCost.clear();

    m_pairsByVertex.clear();
    m_pairsByVertex.reserve(m_pairs.size());

    for (std::size_t p = 0; p < m_pairs.size(); ++p) {
        VertexPair& pair = m_pairs[p];

        if (!pair.isValid()) continue;

        m_pairsByVertex.emplace(pair.m_v0, p);
        m_pairsByVertex.emplace(pair.m_v1, p);

        pair.m_handle = m_pairsByCost.push(p);
        pair.unremove();
    }
}

bool MeshDecimator::isPairContractable(const MeshDecimator::VertexPair &pair) const
{
    return m_mesh->isPairContractable(pair.m_v0, pair.m_v1, pair.m_newPos);
}

MeshDecimator::MeshDecimator(Mesh *mesh, unsigned int targetFaceCount) :
    m_mesh(mesh), m_targetFaceCount(targetFaceCount), m_abort(false), m_costComparer(m_pairs), m_pairsByCost(m_costComparer)
{
    m_lastAttemptFaceCount = m_oldFaceCount = m_currentFaceCount = m_mesh->faceCount();
}

bool MeshDecimator::iterate()
{
    if (m_pairsByCost.empty()) { // no pairs left!
        if (m_currentFaceCount == m_lastAttemptFaceCount) {
            // no progress was made since last attempt: abort!
            return false;
        }

        m_lastAttemptFaceCount = m_currentFaceCount;

        // otherwise: clean up data and try again!
        cleanupPairs();
        initHelpers();
    }

    std::size_t p = m_pairsByCost.top();
    m_pairsByCost.pop();

    VertexPair& curPair = m_pairs[p];
    curPair.remove();

    if (!(curPair.isValid() && isPairContractable(curPair))) {
        return true;
    }

    mesh_index v0 = curPair.m_v0, v1 = curPair.m_v1;
    mesh_index collEdge = m_mesh->vConnectingEdge(v0, v1);

    m_currentFaceCount -= m_mesh->collapseEdge(collEdge, curPair.m_newPos);

    auto v1Range = m_pairsByVertex.equal_range(v1);
    for (auto it = v1Range.first; it != v1Range.second; ++it) {
        std::size_t p1 = it->second;
        if (p1 == p) continue;

        VertexPair& pair = m_pairs[p1];
        if (!pair.isValid()) continue;

        m_pairsByVertex.emplace(v0, p1);

        if (pair.m_v0 == v1)
            pair.m_v0 = v0;
        else
            pair.m_v1 = v0;
    }

    m_pairsByVertex.erase(v1Range.first, v1Range.second);

    curPair.invalidate();

    m_quadrics[v0] += m_quadrics[v1];

    std::unordered_set<mesh_index> duplicateHelper;

    auto v0Range = m_pairsByVertex.equal_range(v0);
    for (auto it = v0Range.first; it != v0Range.second; ++it) {
        std::size_t pi = it->second;
        VertexPair& pair = m_pairs[pi];
        mesh_index ov = (pair.m_v0 == v0) ? pair.m_v1 : pair.m_v0;

        if (!pair.isValid())
            continue;

        if (duplicateHelper.count(ov)) {
            pair.invalidate();
        } else {
            duplicateHelper.insert(ov);
            computePairCost(pi);
        }

        auto ovRange = m_pairsByVertex.equal_range(ov);
        for (auto it = ovRange.first; it != ovRange.second; ++it) {
            std::size_t vp = it->second;
            VertexPair& vpair = m_pairs[vp];
            if (vpair.isValid() && vpair.isRemoved()) {
                if (isPairContractable(vpair)) {
                    vpair.m_handle = m_pairsByCost.push(vp); // a recently removed pair has become valid again! re-add it to the heap
                    vpair.unremove();
                }
            }
        }
    }

    if (m_currentFaceCount <= m_targetFaceCount)
        return false;

    return true;
}

void MeshDecimator::updateProgress()
{
    emit progressChanged(progress());
}

MeshDecimator::~MeshDecimator()
{
    m_mesh->cleanupData();
    m_mesh->recomputeNormals();
}

float MeshDecimator::progress() const
{
    unsigned int startDiff = m_oldFaceCount - m_targetFaceCount;
    unsigned int diff = m_currentFaceCount - m_targetFaceCount;
    if (m_currentFaceCount < m_targetFaceCount) diff = 0;

    float p = 1.0f - (float(diff) / float(startDiff));
    return p;
}

bool MeshDecimator::isAborting() const
{
    QMutexLocker ml(&m_mutex);

    return m_abort;
}

void MeshDecimator::abort()
{
    QMutexLocker ml(&m_mutex);

    m_abort = true;
}

void MeshDecimator::start()
{
    QMutexLocker ml(m_mesh->mutex());

    try {
        computeQuadrics();
        initPairs();
        initHelpers();

        while (true) {
            if (isAborting() || !iterate())
                break;

            updateProgress();
        }
    } catch (std::runtime_error& e) {
        emit error(QString(e.what()));
    }

    emit finished();
}
