//
//  geometry_manager.h
//
//  Created by Jonathan Tompson on 11/2/12.
//
//  Singleton class managed by Renderer class.
//
//  Geometry IO and resource Managment. Uses ASSIMP to load in 3D models in 
//  many possible formats (includeing: 3ds, x, Collada, etc).  However, while
//  this library is flexible it is slow.  Also uses freeimage for textures.
//
//  I have implemented a custom "jbin" format, which is a very simple 
//  but very fast to load compressed binary format.
//
//  NOTE: WHEN GEOMETRY IS ADDED TO THE SCENE GRAPH ROOT, THE MEMORY OWNERSHIP
//        IS TRANSFERED TO THE GeometryManager CLASS (IT WILL HANDLE 
//        DESTRUCTION)
//

#pragma once

#include "jtil/math/math_types.h"
#include "jtil/renderer/geometry/geometry.h"  // For GeometryType
#include "jtil/renderer/texture/texture.h"  // For TEXTURE_WRAP_MODE

struct aiScene;
struct aiNode;
struct aiBone;
struct aiMesh;
struct aiMaterial;

namespace Assimp { class Importer; }

#define GEOMETRY_MANAGER_N_RAINBOW_COL 9
#define AABBOX_CUBE_NAME "aabboxcube"

namespace jtil {
namespace data_str {template <typename TFirst, typename TSecond> class Pair;}
namespace data_str {template <class TKey, class TValue> class HashMapManaged;}
namespace data_str {template <class TKey, class TValue> class HashMap;}
namespace data_str {template <typename T> class VectorManaged;}
namespace data_str {template <typename T> class Vector;}

namespace renderer {

  class Texture;
  class GeometryInstance;
  struct Bone;
  struct Material;
  class Renderer;

  class GeometryManager {
  public:
    GeometryManager(Renderer* renderer);
    ~GeometryManager();

    // Unlocked data access
    inline GeometryInstance* scene_root() { return scene_root_; }

    // synchronization
    void lockData();
    void unlockData();

    // loadModelFromFile - Slow.  Uses ASSIMP library to parse many format 
    // types.  This Loads the geometry and creates an instance.  IT DOES NOT
    // ADD IT TO THE SCENE GRAPH.
    GeometryInstance* loadModelFromFile(const std::string& path, 
      const std::string& filename, const bool smooth_normals = false,
      const bool mesh_optimiation = false, const bool flip_uv_coords = true);
    // load/saveModelFromJBinFile - Fast.  Binary file that is compressed
    // on disk.  Save uses up to 2x MEM temp space and load uses up to 1x MEM.
    void saveModelToJBinFile(const std::string& path, 
      const std::string& filename, const GeometryInstance* root);
    GeometryInstance* loadModelFromJBinFile(
      const std::string& path, const std::string& filename);

    // loadTexture - Load a texture from file (only once)
    Texture* loadTexture(const std::string& path_filename,
      const TextureWrapMode wrap = TEXTURE_CLAMP,
      const TextureFilterMode filter = TEXTURE_NEAREST, 
      const bool mip_map = false);

    // findGeometryByName O(1) -> Find Geometry in the global database
    Geometry* findGeometryByName(const std::string& name);
    // findGeometryInstanceByName O(n) -> Find Geometry in scene graph
    // Returns the first instance reached by DFS
    static GeometryInstance* findGeometryInstanceByName(const std::string& name,
      GeometryInstance* root);

    // To create custom geometry use createDynamicGeometry
    GeometryInstance* createDynamicGeometry(const std::string& name);

    // Render Stack inteface methods --> NONE OF THESE ARE THREAD SAFE!
    void renderStackReset();  // NOT THREAD SAFE!
    GeometryInstance* renderStackPop();  // NOT THREAD SAFE!
    bool renderStackEmpty();  // NOT THREAD SAFE!

    // Methods for creating generic geometry instances
    GeometryInstance* makeDispQuad();
    GeometryInstance* makeCube(const math::Float3& col);
    GeometryInstance* makeCubeRainbow();
    GeometryInstance* makePyramid(const math::Float3& col);
    GeometryInstance* makePyramidRainbow();
    GeometryInstance* makeTorusKnot(const math::Float3& col, 
      const uint32_t turns, const uint32_t slices, const uint32_t stacks);
    GeometryInstance* makeSphere(const uint32_t n_stacks, 
      const uint32_t n_slices, const float inside_radius, 
      const math::Float3& col);
    GeometryInstance* makeCone(const uint32_t n_slices, 
      const float height, const float inside_radius, const math::Float3& col);
    GeometryInstance* makeCylinder(const uint32_t n_slices, 
      const float height, const float base_inside_radius, 
      const float top_inside_radius, const math::Float3& col);

    // methods for creating generic meshes (just pos, nor, ind)
    // DOES NOT ADD THIS GEOMETRY TO THE SCENE GRAPH!  For this, use the 
    // GeometryInstance methods above.
    Geometry* makeQuadGeometry(const std::string& name,
      const bool sync = true);
    Geometry* makeDispQuadGeometry(const std::string& name,
      const bool sync = true);
    Geometry* makeCubeGeometry(const std::string& name,
      const bool sync = true);
    Geometry* makePyramidGeometry(const std::string& name,
      const bool sync = true);
    Geometry* makeTorusKnotGeometry(const std::string& name,
      const uint32_t turns, const uint32_t slices, const uint32_t stacks, 
      const bool sync = true);
    Geometry* makeSphereGeometry(const std::string& name,
      const uint32_t n_stacks, const uint32_t n_slices, 
      const float inside_radius, const bool sync = true);
    Geometry* makeConeGeometry(const std::string& name,
      const uint32_t n_slices, const float height, const float inside_radius, 
      const bool sync = true);
    Geometry* makeCylinderGeometry(const std::string& name,
      const uint32_t n_slices, const float height, 
      const float base_inside_radius, const float top_inside_radius, 
      const bool sync = true);

    // Helpers to calculate generic cone and sphere extents
    static float calcSphereOutsideRadius(const uint32_t n_stacks, 
      const uint32_t n_slices, const float inside_radius);
    static float calcConeOutsideRadius(const uint32_t n_slices, 
      const float inside_radius);

    // Blank white texture
    inline Texture* white_tex() { return white_tex_; }
  private:
    Renderer* renderer_;  // Not owned here

    std::recursive_mutex data_lock_;

    // Textures for all GeometryTexturedMesh and GeometryTexturedBonedMesh are
    // stored here (so we can avoid loading in the same texture twice).
    data_str::HashMapManaged<std::string, Texture*>* tex_;
    Texture* white_tex_;

    // Data to help generate basic shapes
    static const math::Float3 pos_cube_[8];
    static const math::Float3 pos_pyramid_[5];
    static const math::Float3 rainbow_colors_[GEOMETRY_MANAGER_N_RAINBOW_COL];
    static const math::Float3 pos_quad_[6];  // Non-IBuffer version

    // The global scene graph and geometry pool used by the renderer
    GeometryInstance* scene_root_;
    data_str::HashMapManaged<std::string, Geometry*>* geom_;
    data_str::HashMap<std::string, uint32_t>* bone_name_to_index_;
    data_str::VectorManaged<Bone*>* bones_;
    data_str::Vector<GeometryInstance*>* render_stack_;

    // Assimp support functions
    void loadAssimpSceneMeshes(const std::string& path, 
      const std::string& filename, const aiScene* scene);
    void populateModelInstance(const std::string& path, 
      const std::string& filename, const aiScene* scene, 
      GeometryInstance* model_root);
    void associateBoneTransforms(GeometryInstance* model_root);
    static GeometryType calcAssimpMeshGeometryType(const aiScene* scene, 
      const aiMesh* mesh);
    static bool getGeometryTypeSupport(const GeometryType type);
    static const std::string calcAssimpMeshName(const std::string& path, 
      const std::string& filename, const aiMesh* mesh,
      const uint32_t mesh_index);
    static const std::string calcAssimpNodeName(const std::string& path, 
      const std::string& filename, const aiNode* mesh);
    static const std::string calcAssimpBoneName(const std::string& path, 
      const std::string& filename, const aiBone* mesh);
    static void extractAssimpMtrl(Material& mtrl, aiMaterial* assimp_mtrl);
    static void extractAssimpPositions(Geometry* geom, const aiMesh* mesh);
    static void extractAssimpNormals(Geometry* geom, const aiMesh* mesh);
    static void extractAssimpVertexColors(Geometry* geom, const aiMesh* mesh);
    static void extractAssimpTangents(Geometry* geom, const aiMesh* mesh);
    static void extractAssimpTextureCoords(Geometry* geom, const aiMesh* mesh);
    void extractAssimpRGBTexture(Geometry* geom, const aiMesh* mesh,
      const aiMaterial* mtrl, const std::string& path);
    void extractAssimpDispTexture(Geometry* geom, const aiMesh* mesh, 
      const aiMaterial* mtrl, const std::string& path);
    void extractAssimpBones(Geometry* geom, const aiMesh* mesh,
      const std::string& path, const std::string& filename);
    static void extractAssimpFaces(Geometry* geom, const aiMesh* mesh);
    static bool checkAssimpTriFaces(Geometry* geom, const aiMesh* mesh);

    // Helpers for geometry creation
    static void SphericalToCartesean(math::Float3& xyz, const float r, 
      const float phi, const float theta);
    static void GetRotatedAxis(math::Float3& vec_out, 
      const math::Float3& vec_in, const double angle, 
      const math::Float3& axis);

    void addGeometry(Geometry* const geom);
    GeometryInstance* createNewInstance(const Geometry* geom);
    Bone* findBoneByName(const std::string& bone_name);
    Bone* findBoneByIndex(const uint32_t index);
    void addBone(Bone* bone);
    
    void saveModelMeshesToJBinFile(std::ofstream& file, 
      const GeometryInstance* model);
    void loadModelMeshesFromJBinFile(std::ifstream& file);
    void saveModelNodesToJBinFile(std::ofstream& file, 
      const GeometryInstance* model);
    GeometryInstance* loadModelNodesFromJBinFile(std::ifstream& file);
    GeometryInstance* loadNodeFromJBinFile(std::ifstream& file);
    void saveModelBonesToJBinFile(std::ofstream& file, 
      const GeometryInstance* model);
    void loadModelBonesFromJBinFile(std::ifstream& file);

    void createAssimpImporter(Assimp::Importer*& importer,
      const aiScene*& scene, const std::string& path, 
      const std::string& filename, bool smooth_normals, 
      const bool mesh_optimiation, const bool flip_uv_coords);

    // Non-copyable, non-assignable.
    GeometryManager(GeometryManager&);
    GeometryManager& operator=(const GeometryManager&);
  };

};  // namespace renderer
};  // namespace jtil

