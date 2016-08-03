#ifndef MESH_INDEX_HPP
#define MESH_INDEX_HPP

/*!
 * \brief type used to reference all mesh primitives (edges, faces, vertices)
 */
typedef unsigned int mesh_index;

/*!
 * \brief value representing an invalid mesh index
 */
const mesh_index inv_index(-1);

/*!
 * \brief checks whether an index is valid (not equal to inv_ind)
 */
inline bool is_valid(mesh_index index)
{
    return index != inv_index;
}

#endif // MESH_INDEX_HPP
