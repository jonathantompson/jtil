#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/data_str/vector.h"
#include "jtil/data_str/vector_managed.h"
#include "jtil/data_str/pair.h"
#include "jtil/data_str/circular_buffer.h"
#include "jtil/data_str/hash_map.h"
#include "jtil/data_str/hash_set.h"
#include "jtil/data_str/hash_map_managed.h"
#include "jtil/data_str/hash_funcs.h"  // For HashString
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/geometry_instance.h"
#include "jtil/renderer/geometry/bone.h"
#include "jtil/renderer/colors/colors.h"
#include "jtil/renderer/texture/texture.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/renderer/gl_include.h"
#include <assimp/Importer.hpp>      // C++ importer interface
#include "assimp/scene.h"           // Output data structure
#include "assimp/postprocess.h"     // Post processing flags
#include "assimp/DefaultLogger.hpp"
#include "jtil/math/math_types.h"
#include "jtil/string_util/string_util.h"
#include "jtil/ucl/ucl_helper.h"
#include "jtil/fastlz/fastlz_helper.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/renderer/renderer.h"

#define WHITE_TEXTURE "resource_files/white.tga"

using std::cout;
using std::stringstream;
using std::endl;
using std::string;
using std::wruntime_error;

namespace jtil {

using math::Float2;
using math::Float3;
using math::Float4;
using math::Float4x4;
using math::Int4;
using math::FloatQuat;
using data_str::Vector;
using data_str::VectorManaged;
using data_str::Pair;
using data_str::HashMapManaged;
using data_str::HashMap;
using data_str::HashSet;
using ucl::UCLHelper;
using fastlz::FastlzHelper;

#define GM_START_HM_SIZE 211  // Starting hash-map size.  Best if it is prime.
// #define IMPORT_DISPLACEMENT_MAPS

#define SAFE_DELETE(target) \
  if (target != NULL) { \
    delete target; \
    target = NULL; \
  }

namespace renderer {
  GeometryManager::GeometryManager(Renderer* renderer) {
    data_lock_.lock();
    renderer_ = renderer;
    
    tex_ = new HashMapManaged<string, Texture*>(GM_START_HM_SIZE, 
      &data_str::HashString);
    geom_ = new HashMapManaged<string, Geometry*>(GM_START_HM_SIZE, 
      &data_str::HashString);
    bone_name_to_index_ = new HashMap<string, uint32_t>(GM_START_HM_SIZE, 
      &data_str::HashString);
    bones_ = new VectorManaged<Bone*>();
    render_stack_ = new Vector<GeometryInstance*>();
    scene_root_ = new GeometryInstance;
    
    addGeometry(makeCubeGeometry(AABBOX_CUBE_NAME));

    white_tex_ = loadTexture(WHITE_TEXTURE);
    data_lock_.unlock();
  }

  GeometryManager::~GeometryManager() {
    data_lock_.lock();
    // Note HashMapManaged will clear all heap memory for us.
    SAFE_DELETE(tex_);
    SAFE_DELETE(geom_);
    SAFE_DELETE(bones_);
    SAFE_DELETE(bone_name_to_index_);
    SAFE_DELETE(render_stack_);

    // recursively delete the scene graph:
    SAFE_DELETE(scene_root_);
    data_lock_.unlock();
  }

  GeometryInstance* GeometryManager::loadModelFromFile(const string& path, 
    const string& filename, bool smooth_normals, const bool mesh_optimiation,
    const bool flip_uv_coords) {
    // Note Assimp interface is singleton so we can only load one model at
    // a time.
    data_lock_.lock();

    Assimp::Importer* importer;
    const aiScene* scene;
    createAssimpImporter(importer, scene, path, filename, smooth_normals, 
      mesh_optimiation, flip_uv_coords);

    // Create a new model container class
    GeometryInstance* model_root = new GeometryInstance();

    // Import the meshes (what I call Geometry), into the global mesh database:
    cout << "Converting ASSIMP Geoemtry from " << filename << "..." << endl;
    cout << "..." << endl;
    loadAssimpSceneMeshes(path, filename, scene);

    // Now populate the model instance
    cout << "Converting ASSIMP Scene Graph from " << filename << "..." << endl;
    populateModelInstance(path, filename, scene, model_root);

    // Connect the Bone nodes to the GeometryInstance nodes that use them.
    associateBoneTransforms(model_root);
    
    // Clean up the assimp structures.
    delete importer;

    cout << "Finished loading " << filename << endl;

    data_lock_.unlock();
    return model_root;
  }

void GeometryManager::createAssimpImporter(Assimp::Importer*& importer,
    const aiScene*& scene, const string& path, const string& filename, 
    bool smooth_normals, const bool mesh_optimiation,
    const bool flip_uv_coords) {
    // ASSUMPTION: data_lock_.lock() already called
    // Add a slash at the end of the pathname if it doesn't exist.
    string full_path;
    if (path.at(path.length()-1) != '/' && path.at(path.length()-1) != '\\') {
      full_path = path + string("/");
    } else {
      full_path = path;
    }
    cout << "Loading scene from " << full_path << filename << "..." << endl;
    
     // Create an instance of the Importer class
    importer = new Assimp::Importer();

    importer->SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, MAX_BONES);
    if (importer->GetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS) != 
      MAX_BONES) {
      throw wruntime_error(L"GeometryManager::loadFromFile() - ERROR: "
        L"Couldn't set the max bone weight property!");
    }

    // And have it read the given file with some example postprocessing
    // List of post-processing options:
    // http://assimp.sourceforge.net/lib_html/postprocess_8h.html
    uint32_t flags = 0;
    flags |= aiProcess_Triangulate;  // In case faces have > 3 vertices
    flags |= aiProcess_FindInvalidData;
    if (flip_uv_coords) {
      flags |= aiProcess_FlipUVs;
    }
    if (mesh_optimiation) {
      flags |= aiProcess_ImproveCacheLocality;  // O(n)
      flags |= aiProcess_OptimizeMeshes;
      flags |= aiProcess_OptimizeGraph;
    }
    // flags |= aiProcess_GenUVCoords;  // Convert sph or cyl to uv mappings
    flags |= aiProcess_LimitBoneWeights;  // Small weights are ignored and then
                                          // weights are renormalized!
    flags |= aiProcess_JoinIdenticalVertices;  // Remove redundant data
    if (smooth_normals) {
      flags |= aiProcess_GenSmoothNormals;  // ignored if normals exist
    } else {
      flags |= aiProcess_GenNormals;
    }

    if (!importer->ValidateFlags(flags)) {
      throw wruntime_error(L"GeometryManager::loadFromFile() - ERROR:"
        L" One or more post-processing flag is not supported!");
    }

    Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);

    scene = importer->ReadFile(full_path + filename, flags);

    Assimp::DefaultLogger::kill();

    // If the import failed, report it
    if (!scene) {
      stringstream ss;
      ss << "GeometryManager::createAssimpImporter() - ERROR: Assimp importer";
      ss << " failed with error string - " << importer->GetErrorString();
      throw wruntime_error(ss.str());
    }
  }

  void GeometryManager::loadAssimpSceneMeshes(const string& path, 
    const string& filename, const aiScene* scene) {
    // ASSUMPTION: data_lock_.lock() already called
    for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
      aiMesh* mesh = scene->mMeshes[i];
      string mesh_name = calcAssimpMeshName(path, filename, mesh, i);
      Geometry* geom = findGeometryByName(mesh_name);
      if (geom == NULL) {
        // See if the file geometry type is supported
        GeometryType type = calcAssimpMeshGeometryType(scene, mesh);
        if (!getGeometryTypeSupport(type)) {
          stringstream ss;
          ss << "GeometryManager::loadAssimpSceneMeshes() - WARNING: ";
          ss << "GeometryType " << (int)type << " is not yet supported for";
          ss << "mesh in " << filename << std::endl << "--> Dropping";
          ss << " unsuported vertex attributes (however it still might not ";
          ss << "render correctly or at all).";
          std::cout << ss.str() << std::endl;
        }

        // Each mesh has one and only one material
        aiMaterial* assimp_mtrl = scene->mMaterials[mesh->mMaterialIndex];

        // Only load geometry with triangle faces (since that's all the 
        // renderer supports for now).
        bool tri_geometry = checkAssimpTriFaces(geom, mesh);
        if ( tri_geometry ) {
          // Create a new Geometry element and fill the vertex attributes
          geom = new Geometry(mesh_name);

          // Extract data from the assimp mesh (if it exists)
          extractAssimpPositions(geom, mesh);
          extractAssimpNormals(geom, mesh);
          extractAssimpVertexColors(geom, mesh);
          extractAssimpTangents(geom, mesh);
          extractAssimpTextureCoords(geom, mesh);
          extractAssimpRGBTexture(geom, mesh, assimp_mtrl, path);
          extractAssimpDispTexture(geom, mesh, assimp_mtrl, path);
          extractAssimpBones(geom, mesh, path, filename);
          extractAssimpFaces(geom, mesh);
        
          if (geom->hasVertexAttribute(VERTATTR_DISP_TEX) && 
            !geom->hasVertexAttribute(VERTATTR_TANGENT)) {
            geom->calcTangentVectors();  // O(n)
          }

          cout << "   Loaded geometry - type: " << (uint32_t)geom->type();
          cout << ", faces: " << geom->ind().size() / 3;
          cout << ", name: " << mesh_name << std::endl;

          // Finally sync it with openGL add it to the geometry pool
          geom->sync();
          addGeometry(geom);
        } else {
          cout << "   Couldn't load geometry - name: " << mesh_name << " since"
            " it contains non-triangle faces!" << std::endl;
        }
      }
    }
  }

  void GeometryManager::extractAssimpPositions(Geometry* geom, 
    const aiMesh* mesh) {
    if (mesh->HasPositions()) {
      geom->addVertexAttribute(VERTATTR_POS);
      geom->pos().capacity(mesh->mNumVertices);
      geom->pos().resize(mesh->mNumVertices);
      for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        geom->pos()[i].set(&mesh->mVertices[i][0]);
      }
    }
  }

  void GeometryManager::extractAssimpNormals(Geometry* geom, 
    const aiMesh* mesh) {
    if (mesh->HasNormals()) {
      geom->addVertexAttribute(VERTATTR_NOR);
      geom->nor().capacity(mesh->mNumVertices);
      geom->nor().resize(mesh->mNumVertices);
      for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        geom->nor()[i].set(&mesh->mNormals[i][0]);
        geom->nor()[i].normalize();
      }
    }
  }

  void GeometryManager::extractAssimpVertexColors(Geometry* geom, 
    const aiMesh* mesh) {
    if (mesh->HasVertexColors(0)) {
      geom->addVertexAttribute(VERTATTR_COL);
      geom->col().capacity(mesh->mNumVertices);
      geom->col().resize(mesh->mNumVertices);
      for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        geom->col()[i].set(&mesh->mColors[0][i][0]);
      }
    }
  }

  void GeometryManager::extractAssimpTangents(Geometry* geom, 
    const aiMesh* mesh) {
    static_cast<void>(geom);
    static_cast<void>(mesh);
#ifdef IMPORT_DISPLACEMENT_MAPS
    if (mesh->HasTangentsAndBitangents()) {
      geom->addVertexAttribute(VERTATTR_TANGENT);
      geom->tangent().capacity(mesh->mNumVertices);
      geom->tangent().resize(mesh->mNumVertices);
      for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        geom->tangent()[i].set(&mesh->mTangents[i][0]);
        geom->tangent()[i].normalize();
      }
    }
#endif
  }

  void GeometryManager::extractAssimpTextureCoords(Geometry* geom, 
    const aiMesh* mesh) {
    if (mesh->HasTextureCoords(0)) {
      geom->addVertexAttribute(VERTATTR_TEX_COORD);
      geom->tex_coord().capacity(mesh->mNumVertices);
      geom->tex_coord().resize(mesh->mNumVertices);
      for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        geom->tex_coord()[i].set(mesh->mTextureCoords[0][i][0],
          mesh->mTextureCoords[0][i][1]);
      }
    }
  }

  void GeometryManager::extractAssimpRGBTexture(Geometry* geom, 
    const aiMesh* mesh, const aiMaterial* mtrl, const string& path) {
    if (mesh->HasTextureCoords(0)) {
      bool filter = true;
      GET_SETTING("texture_filtering_on", bool, filter);
      TextureFilterMode mode = filter ? TEXTURE_LINEAR : 
        TEXTURE_NEAREST;

      // Load in the RGB textures
      geom->rgb_tex() = NULL;
      geom->addVertexAttribute(VERTATTR_RGB_TEX);
      if (mtrl->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString str;
        if (mtrl->GetTexture(aiTextureType_DIFFUSE, 0, &str, NULL,
          NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            string path_with_slash;
            if (path.at(path.length()-1) != '/' && 
              path.at(path.length()-1) != '\\') {
              path_with_slash = path + string("/");
            } else {
              path_with_slash = path;
            }
            string FullPath = path_with_slash + string(str.data);
            try { 
              geom->rgb_tex() = loadTexture(FullPath, TEXTURE_REPEAT, mode, 
                filter);
            } catch(const wruntime_error &e) {
              static_cast<void>(e);
              cout << "WARNING: Couldn't load texture " << FullPath << endl;
            }
        }
      }
    }
  }

  void GeometryManager::extractAssimpDispTexture(Geometry* geom, 
    const aiMesh* mesh, const aiMaterial* mtrl, const string& path) {
    static_cast<void>(geom);
    static_cast<void>(mesh);
    static_cast<void>(mtrl);
    static_cast<void>(path);
    if (mesh->HasTextureCoords(0)) {
#ifdef IMPORT_DISPLACEMENT_MAPS
      // Load in the DISP textures
      geom->disp_tex() = NULL;
      if (mtrl->GetTextureCount(aiTextureType_HEIGHT) > 0) {
        geom->addVertexAttribute(VERTATTR_DISP_TEX);
        aiString str;
        if (mtrl->GetTexture(aiTextureType_HEIGHT, 0, &str, 
          NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
          string path_with_slash;
          if (path.at(path.length()-1) != '/' && 
            path.at(path.length()-1) != '\\') {
            path_with_slash = path + string("/");
          } else {
            path_with_slash = path;
          }
          string FullPath = path_with_slash + string(str.data);
          try { 
            geom->disp_tex() = loadTexture(FullPath, TEXTURE_REPEAT, mode, 
              false);
          } catch(const wruntime_error &e) {
            static_cast<void>(e);
            cout << "WARNING: Couldn't load texture " << FullPath << endl;
          }
        }
      }
#endif
    }
  }

  void GeometryManager::extractAssimpBones(Geometry* geom, 
    const aiMesh* mesh, const string& path, const string& filename) {
    if (mesh->HasBones()) {
      if (mesh->mNumBones > MAX_BONE_COUNT) {
        throw std::wruntime_error("GeometryManager::extractAssimpBones() - "
          "ERROR: Model number of bones is > MAX_BONE_COUNT");
      }

      geom->addVertexAttribute(VERTATTR_BONEW);
      geom->bonew().capacity(mesh->mNumVertices);
      geom->bonew().resize(mesh->mNumVertices);
      geom->addVertexAttribute(VERTATTR_BONEI);
      geom->bonei().capacity(mesh->mNumVertices);
      geom->bonei().resize(mesh->mNumVertices);

      // Zero the bone ids and contributions
      for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        geom->bonew()[i].set(0, 0, 0, 0);
        geom->bonei()[i].set(0, 0, 0, 0);
      }

      // Now for each bone, find the bone in the global database.  If it
      // doesn't exist, then add it.
      for (uint32_t i = 0; i < mesh->mNumBones; i++) {
        string bone_name = calcAssimpBoneName(path, filename, mesh->mBones[i]);

        // See if the bone exists in the global bone database (multiple meshes
        // may share a bone).
        Bone* bone = findBoneByName(bone_name);
        if (bone == NULL) {
          // Bone doesn't yet exist, so we need to create one and insert it
          cout << "   Loaded bone - name: " << bone_name << std::endl;
          bone = new Bone(bone_name, mesh->mBones[i]->mOffsetMatrix[0]);
          addBone(bone);  // Also, set's the bone index into the global array
        }

        uint32_t local_bone_index = geom->bone_names().size();
        char *name = string_util::cStrCpy(bone->bone_name);
        geom->bone_names().pushBack(name);

        for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights ; j++) {
          uint32_t vertex = mesh->mBones[i]->mWeights[j].mVertexId;
          float weight = mesh->mBones[i]->mWeights[j].mWeight;

          // Use the next avaliable bone attachement (that is zero weight)
          for (uint32_t k = 0; k < MAX_BONES; k++) {
            if (geom->bonew()[vertex].m[k] == 0.0) {
              // Bone not yet attached:
              geom->bonew()[vertex].m[k] = weight;
              geom->bonei()[vertex].m[k] = local_bone_index;
              break;
            }
          }
        }
      }
    }
  }

  void GeometryManager::extractAssimpFaces(Geometry* geom, 
    const aiMesh* mesh) {
    if (mesh->HasFaces()) {
      uint32_t n_pts = 0;
      uint32_t n_lines = 0;
      uint32_t n_tri = 0;
      uint32_t n_quads = 0;
      uint32_t n_others = 0;
      for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        switch (mesh->mFaces[i].mNumIndices) {
        case 1:
          n_pts++;
          break;
        case 2:
          n_lines++;
          break;
        case 3:
          n_tri++;
          break;
        case 4:
          n_quads++;
          break;
        default:
          n_others++;
          break;
        }
      }
      if (n_pts + n_lines + n_quads + n_others != 0) {
        throw wruntime_error("GeometryManager::extractAssimpFaces"
          " - WARNING: model mesh has non-triangle faces. Only triangles "
          "are supported for now.");
      }
      if (n_tri == 0) {
        throw wruntime_error("GeometryManager::extractAssimpFaces"
          " - ERROR: model mesh has no triangle faces.");
      }

      geom->ind().capacity(n_tri*3);
      geom->ind().resize(n_tri*3);
      uint32_t cur_ind = 0;
      for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        if (mesh->mFaces[i].mNumIndices == 3) {
          geom->ind()[cur_ind] = mesh->mFaces[i].mIndices[0];
          cur_ind++;
          geom->ind()[cur_ind] = mesh->mFaces[i].mIndices[2];
          cur_ind++;
          geom->ind()[cur_ind] = mesh->mFaces[i].mIndices[1];
          cur_ind++;
        }
      }
    }
  }

  bool GeometryManager::checkAssimpTriFaces(Geometry* geom, 
    const aiMesh* mesh) {
    if (mesh->HasFaces()) {
      uint32_t n_pts = 0;
      uint32_t n_lines = 0;
      uint32_t n_tri = 0;
      uint32_t n_quads = 0;
      uint32_t n_others = 0;
      for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        switch (mesh->mFaces[i].mNumIndices) {
        case 1:
          n_pts++;
          break;
        case 2:
          n_lines++;
          break;
        case 3:
          n_tri++;
          break;
        case 4:
          n_quads++;
          break;
        default:
          n_others++;
          break;
        }
      }
      if (n_pts + n_lines + n_quads + n_others != 0) {
        return false;
      }
      if (n_tri == 0) {
        return false;
      }
    }
    return true;
  }

  void GeometryManager::extractAssimpMtrl(Material& mtrl, 
    aiMaterial* assimp_mtrl) {
    aiColor4D assimp_color(0.0f, 0.0f, 0.0f, 0.0f);
    if (AI_SUCCESS == assimp_mtrl->Get(AI_MATKEY_COLOR_DIFFUSE, 
      assimp_color)) {
        mtrl.albedo.set(assimp_color.r, assimp_color.g, 
          assimp_color.b);
    }
    float spec_pow;
    if (AI_SUCCESS == assimp_mtrl->Get(AI_MATKEY_SHININESS, spec_pow)) {
      mtrl.spec_power = spec_pow;
    }

    float spec_intens;
    if (AI_SUCCESS == 
      assimp_mtrl->Get(AI_MATKEY_SHININESS_STRENGTH, spec_intens)) {
        mtrl.spec_intensity = spec_intens;
    }
  }

  void GeometryManager::populateModelInstance(const string& path, 
    const string& filename, const aiScene* scene, 
    GeometryInstance* model) {
    // ASSUMPTION: data_lock_.lock() already called
    // We're going to descend the heirachy of nodes recursively (DFS)
    data_str::Vector<aiNode*> nodes;
    data_str::Vector<GeometryInstance*> parents;
    nodes.pushBack(scene->mRootNode);
    parents.pushBack(model);
    aiNode* cur_node;
    GeometryInstance* cur_parent;
    while (nodes.size() > 0) {
      nodes.popBack(cur_node);
      parents.popBack(cur_parent);

      GeometryInstance* cur_instance;
      if (cur_node->mNumMeshes == 1) {
        // A Single mesh --> Just create a single container and fill it with
        // the mesh.  This is a very common case.
        uint32_t mesh_index = cur_node->mMeshes[0];
        aiMesh* mesh = scene->mMeshes[mesh_index];
        string mesh_name = calcAssimpMeshName(path, filename, mesh, 
          mesh_index);
        Geometry* geom = findGeometryByName(mesh_name);
        if (geom == NULL) {
          throw wruntime_error("ERROR: couldn't find geometry for this"
            "aiNode!");
        }
        cur_instance = new GeometryInstance(geom);
        extractAssimpMtrl(cur_instance->mtrl(), 
          scene->mMaterials[mesh->mMaterialIndex]);
      } else if (cur_node->mNumMeshes > 1) {
        // Create an implicit root node and add the mulitple meshes as children
        cur_instance = new GeometryInstance(NULL);
        for (uint32_t i = 0; i < cur_node->mNumMeshes; i++) {
          uint32_t mesh_index = cur_node->mMeshes[i];
          aiMesh* mesh = scene->mMeshes[mesh_index];
          string mesh_name = calcAssimpMeshName(path, filename, mesh, 
            mesh_index);
          Geometry* geom = findGeometryByName(mesh_name);
          GeometryInstance* child_instance = new GeometryInstance(geom);
          extractAssimpMtrl(child_instance->mtrl(), 
            scene->mMaterials[mesh->mMaterialIndex]);
          cur_instance->addChild(child_instance);
        }
      } else {
        // Blank node --> These are often used for bone transforms, and might 
        // be necessary to keep.
        cur_instance = new GeometryInstance();
      }

      if (cur_parent != NULL) {
        cur_parent->addChild(cur_instance);
      }

      cur_instance->name() = calcAssimpNodeName(path, filename, cur_node);

      cout << "   Loaded GeometryInstance, name: " << cur_instance->name();
      if (cur_instance->geom()) {
        cout << ", Geometry: " << cur_instance->geom()->name();
      }
      if (cur_parent) {
        cout << ", parent name: " << cur_parent->name();
      }
      cout << std::endl;

      for (uint32_t i = 0; i < cur_instance->numChildren(); i++) {
        cout << "      Loaded simple child GeometryInstance with Geometry: " << 
          cur_instance->getChild(i)->geom()->name() << std::endl;
      }

      // assimp uses row major layout so we may need to transpose
      const float* mat_assimp = cur_node->mTransformation[0];
      memcpy(cur_instance->mat().m, mat_assimp, 16 * sizeof(mat_assimp[0]));
#ifdef COLUMN_MAJOR
      cur_instance->mat().transpose();
#endif

      // Recurse down the structure
      for (uint32_t i = 0; i < cur_node->mNumChildren; i++) {
        nodes.pushBack(cur_node->mChildren[i]);
        parents.pushBack(cur_instance);
      }
    }
  }

  const string GeometryManager::calcAssimpMeshName(const string& path, 
    const string& filename, const aiMesh* mesh, const uint32_t mesh_index) {
    stringstream ss;
    ss << path << filename << "/" << mesh->mName.C_Str() << "/" << mesh_index;
    return ss.str();
  }

  const string GeometryManager::calcAssimpNodeName(const string& path, 
    const string& filename, const aiNode* node) {
    stringstream ss;
    ss << path << filename << "/" << node->mName.C_Str();
    return ss.str();
  }

  const string GeometryManager::calcAssimpBoneName(const string& path, 
    const string& filename, const aiBone* bone) {
    stringstream ss;
    ss << path << filename << "/" << bone->mName.C_Str();
    return ss.str();
  }

  bool GeometryManager::getGeometryTypeSupport(const GeometryType type) {
    return type == GEOMETRY_COLR_MESH || 
           type == GEOMETRY_COLR_BONED_MESH || 
           type == GEOMETRY_CONST_COLR_MESH ||
           type == GEOMETRY_CONST_COLR_BONED_MESH ||
           type == GEOMETRY_TEXT_MESH || 
           type == GEOMETRY_TEXT_BONED_MESH ||
           type == GEOMETRY_TEXT_DISP_MESH;
  }

  GeometryType GeometryManager::calcAssimpMeshGeometryType(
    const aiScene* scene, const aiMesh* mesh) {
    aiMaterial* assimp_mtrl = scene->mMaterials[mesh->mMaterialIndex];

    std::cout << "  -> Assimp mesh has: ";

    // Figure out if the mesh is textured
    GeometryType type = GEOMETRY_BASE;

    if (mesh->HasTextureCoords(0)) {
      std::cout << "texture coords, ";
      type = static_cast<GeometryType>(type | VERTATTR_TEX_COORD);
    }

    if (mesh->HasBones()) {
      std::cout << "bones, ";
      type = static_cast<GeometryType>(type | VERTATTR_BONEW);
      type = static_cast<GeometryType>(type | VERTATTR_BONEI);
    }

    if (mesh->HasTangentsAndBitangents()) {
      std::cout << "vertex tangents, ";
      type = static_cast<GeometryType>(type | VERTATTR_TANGENT);
    }

    if (mesh->HasVertexColors(0)) {
      std::cout << "vertex colors, ";
      type = static_cast<GeometryType>(type | VERTATTR_COL);
    }

    if (mesh->HasNormals()) {
      std::cout << "vertex normals, ";
      type = static_cast<GeometryType>(type | VERTATTR_NOR);
    }

    if (assimp_mtrl->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
      std::cout << "an RGB texture, ";
      type = static_cast<GeometryType>(type | VERTATTR_RGB_TEX);
    }
    
    if (assimp_mtrl->GetTextureCount(aiTextureType_DISPLACEMENT) > 0) {
      std::cout << "a displacement texture, ";
      type = static_cast<GeometryType>(type | VERTATTR_DISP_TEX);
    }

    if (mesh->HasPositions()) {
      std::cout << "vertex positions";
      type = static_cast<GeometryType>(type | VERTATTR_POS);
    }

    std::cout << std::endl;

    return type;
  }

  void GeometryManager::renderStackReset() {  // NOT THREAD SAFE!
    render_stack_->resize(0);  // empty the stack (without deallocating)
    // Seed the render stack with the root node
    render_stack_->pushBack(scene_root_);
  }

  GeometryInstance* GeometryManager::renderStackPop() {  // NOT THREAD SAFE!
    GeometryInstance* ret = NULL;
    if (render_stack_->size() > 0) {
      render_stack_->popBackUnsafe(ret);  // Remove the last element

      // Now add the children to the geometry stack
      for (uint32_t i = 0; i < ret->numChildren(); i ++) {
        render_stack_->pushBack(ret->getChild(i));
      }
    }
    return ret;
  }

  bool GeometryManager::renderStackEmpty() {  // NOT THREAD SAFE!
    return render_stack_->size() == 0;
  }

  Texture* GeometryManager::loadTexture(const string& path_filename,
    const TextureWrapMode wrap, const TextureFilterMode filter, 
    const bool mip_map) {
    std::lock_guard<std::recursive_mutex> lock(data_lock_);
    // First see if the texture has already been loaded in.
    Texture* ret_tex;
    if (!tex_->lookup(path_filename.c_str(), ret_tex)) {
      ret_tex = new Texture(path_filename, wrap, filter, mip_map);
      tex_->insert(path_filename.c_str(), ret_tex);
    } else {
      if (ret_tex->filter() != filter || ret_tex->wrap() != wrap ||
        ret_tex->mip_map() != mip_map) {
        throw wruntime_error("GeometryManager::loadTexture() - ERROR:"
          " the requested texture has already been loaded but with different"
          " filter, wrap or mip_map settings!");
      }
    }

    return ret_tex;
  }

  void GeometryManager::addGeometry(Geometry* geom) {
    std::lock_guard<std::recursive_mutex> lock(data_lock_);
    if (geom->name() == string("")) {
      throw wruntime_error("GeometryManager::addGeometry() - ERROR:"
        " name string is empty.  Geometry name should be defined.");
    }
    Geometry* geom_lookup;
    if (geom_->lookup(geom->name(), geom_lookup)) {
      throw wruntime_error(string("GeometryManager::addGeometry() - "
        "ERROR: Geometry <") + geom->name() + string("> already exists"));
    }
    geom_->insert(geom->name(), geom);
  }

  Geometry* GeometryManager::findGeometryByName(const string& name) {
    std::lock_guard<std::recursive_mutex> lock(data_lock_);
    Geometry* geom_lookup;
    if (geom_->lookup(name, geom_lookup)) {
      return geom_lookup;
    } else {
      return NULL;
    }
  }

  const Float3 GeometryManager::pos_quad_[6] = {
    Float3(-1.0f, -1.0f, 0.0f),
    Float3( 1.0f, -1.0f, 0.0f),
    Float3(-1.0f,  1.0f, 0.0f),
    Float3(-1.0f,  1.0f, 0.0f),
    Float3( 1.0f, -1.0f, 0.0f),
    Float3( 1.0f,  1.0f, 0.0f),
  };

  const Float3 GeometryManager::pos_cube_[8] = {
    Float3(-1.0f, +1.0f, -1.0f),    // top, left, front
    Float3(-1.0f, +1.0f, +1.0f),    // top, left, back
    Float3(+1.0f, +1.0f, +1.0f),    // top, right, back
    Float3(+1.0f, +1.0f, -1.0f),    // top, right, front
    Float3(-1.0f, -1.0f, -1.0f),    // bottom, left, front
    Float3(-1.0f, -1.0f, +1.0f),    // bottom, left, back
    Float3(+1.0f, -1.0f, +1.0f),    // bottom, right, back
    Float3(+1.0f, -1.0f, -1.0f)     // bottom, right, front
  };

  const Float3 GeometryManager::pos_pyramid_[5] = {
    Float3(+0.0f, +1.0f, +0.0f),    // top
    Float3(-1.0f, -1.0f, -1.0f),    // bottom, left, front
    Float3(-1.0f, -1.0f, +1.0f),    // bottom, left, back
    Float3(+1.0f, -1.0f, +1.0f),    // bottom, right, back
    Float3(+1.0f, -1.0f, -1.0f)     // bottom, right, front
  };

  const Float3 GeometryManager::rainbow_colors_[GEOMETRY_MANAGER_N_RAINBOW_COL]
  = {
    Float3(1.0f, 0.0f, 0.0f),       // red
    Float3(1.0f, 0.5f, 0.0f),       // orange
    Float3(1.0f, 1.0f, 0.0f),       // yellow
    Float3(0.0f, 1.0f, 0.0f),       // green
    Float3(0.0f, 0.0f, 1.0f),       // blue
    Float3(0.435294f, 0.0f, 1.0f),  // indigo
    Float3(0.560784f, 0.0f, 1.0f),  // violet
    Float3(1.0f, 1.0f, 1.0f),       // white
    Float3(0.0f, 0.0f, 0.0f)        // black
  };

  Geometry* GeometryManager::makeQuadGeometry(const string& name,
    const bool sync) {
    Geometry* ret = new Geometry(name);
    ret->addVertexAttribute(VERTATTR_POS);
    for (uint32_t i = 0; i < 6; i++) {
      ret->addPos(pos_quad_[i]);
    }
    if (sync) {
      ret->sync();
    }
    return ret;
  }

  const uint32_t num_tiles_xy = 16;
  Geometry* GeometryManager::makeDispQuadGeometry(const string& name,
    const bool sync) {
    Geometry* ret = new Geometry(name);

    bool filter;
    GET_SETTING("texture_filtering_on", bool, filter);
    TextureFilterMode mode = filter ? TEXTURE_LINEAR : 
      TEXTURE_NEAREST;
    ret->addVertexAttribute(VERTATTR_RGB_TEX);
    ret->rgb_tex() = loadTexture(
      "./models/heightmap_test/rgb.jpg", TEXTURE_REPEAT, mode, filter);
    ret->addVertexAttribute(VERTATTR_DISP_TEX);
    ret->disp_tex() = loadTexture(
      "./models/heightmap_test/disp.jpg", TEXTURE_REPEAT, mode, filter);

    float aspect = (float)ret->disp_tex()->w() / (float)ret->disp_tex()->h();
    ret->addVertexAttribute(VERTATTR_POS);
    ret->addVertexAttribute(VERTATTR_NOR);
    ret->addVertexAttribute(VERTATTR_TEX_COORD);
    ret->pos().capacity((num_tiles_xy + 1) * (num_tiles_xy + 1));
    ret->nor().capacity((num_tiles_xy + 1) * (num_tiles_xy + 1));
    ret->tex_coord().capacity((num_tiles_xy + 1) * (num_tiles_xy + 1));
    static const Float3 norm(0, 1, 0);
    for (uint32_t v = 0; v <= num_tiles_xy; v++) {
      for (uint32_t u = 0; u <= num_tiles_xy; u++) {
        Float3 pos;
        pos[0] = aspect * (-1.0f + 2.0f * (float)u / ((float)num_tiles_xy));
        pos[1] = 0.0f;
        pos[2] = -1.0f + 2.0f * (float)v / ((float)num_tiles_xy);
        Float2 tex;
        tex[0] = 0.0f + 1.0f * (float)u / ((float)num_tiles_xy);
        tex[1] = 0.0f + 1.0f * (float)v / ((float)num_tiles_xy);
        ret->addPos(pos);
        ret->addNor(norm);
        ret->addTexCoord(tex);
      }
    }

    uint32_t num_faces = 2 * num_tiles_xy * num_tiles_xy;
    ret->ind().capacity(3 * num_faces);
    for (uint32_t v = 0; v < num_tiles_xy; v++) {
      for (uint32_t u = 0; u < num_tiles_xy; u++) {
        uint32_t ind = v * (num_tiles_xy+1) + u;
        uint32_t v0 = ind;
        uint32_t v1 = ind + 1;
        uint32_t v2 = v0 + num_tiles_xy + 1;
        uint32_t v3 = v2 + 1;
        ret->ind().pushBack(v0);
        ret->ind().pushBack(v1);
        ret->ind().pushBack(v3);
        ret->ind().pushBack(v0);
        ret->ind().pushBack(v3);
        ret->ind().pushBack(v2);
      }
    }

    ret->calcTangentVectors();

    if (ret->type() != GEOMETRY_TEXT_DISP_MESH) {
      throw wruntime_error("GeometryManager::makeDispQuadGeometry() - Error:"
        " ret->type() != GEOMETRY_TEXT_DISP_MESH!");
    }
    if (sync) {
      ret->sync();
    }
    return ret;
  }

  Geometry* GeometryManager::makeCubeGeometry(const string& name,
    const bool sync) {
    Geometry* ret = new Geometry(name);
    ret->addVertexAttribute(VERTATTR_POS);
    ret->addVertexAttribute(VERTATTR_NOR);

    ret->addTriangle(2, 1, 0, pos_cube_);  // Top face 1
    ret->addTriangle(3, 2, 0, pos_cube_);  // Top face 2
    ret->addTriangle(6, 2, 3, pos_cube_);  // Right face 1
    ret->addTriangle(7, 6, 3, pos_cube_);  // Right face 2
    ret->addTriangle(1, 5, 0, pos_cube_);  // Left face 1
    ret->addTriangle(5, 4, 0, pos_cube_);  // Left face 2
    ret->addTriangle(7, 3, 0, pos_cube_);  // Front face 1
    ret->addTriangle(4, 7, 0, pos_cube_);  // Front face 2
    ret->addTriangle(5, 1, 2, pos_cube_);  // Back face 1
    ret->addTriangle(6, 5, 2, pos_cube_);  // Back face 2
    ret->addTriangle(6, 7, 4, pos_cube_);  // Bottom face 1
    ret->addTriangle(5, 6, 4, pos_cube_);  // Bottom face 2

    if (sync) {
      ret->sync();
    }
    return ret;
  }

  Geometry* GeometryManager::makePyramidGeometry(const string& name,
    const bool sync) {
    Geometry* ret = new Geometry(name);
    ret->addVertexAttribute(VERTATTR_POS);
    ret->addVertexAttribute(VERTATTR_NOR);

    ret->addTriangle(2, 3, 1, pos_pyramid_);  // Bottom face 1
    ret->addTriangle(3, 4, 1, pos_pyramid_);  // Bottom face 2
    ret->addTriangle(1, 4, 0, pos_pyramid_);  // Front Diagonal
    ret->addTriangle(4, 3, 0, pos_pyramid_);  // Right Diagonal
    ret->addTriangle(3, 2, 0, pos_pyramid_);  // Back Diagonal
    ret->addTriangle(2, 1, 0, pos_pyramid_);  // Left Diagonal

    if (sync) {
      ret->sync();
    }
    return ret;
  }
  
  // theta is angle from top [0, pi], phi is angle along slice [0, 2pi]
  // http://en.wikipedia.org/wiki/Spherical_coordinate_system
  void GeometryManager::SphericalToCartesean(Float3& xyz, const float r, 
    const float phi, const float theta) {
    xyz.m[0] = r * sinf(theta) * cosf(phi);
    xyz.m[1] = r * sinf(theta) * sinf(phi);
    xyz.m[2] = r * cosf(theta);
  }

  void GeometryManager::GetRotatedAxis(Float3& vec_out, const Float3& vec_in, 
    const double angle, const Float3& axis) {
    if(angle==0.0) {
      vec_out.set(vec_in);
      return;
    }

    Float3 u(axis);
    u.normalize();

    Float3 rotMatrixRow0, rotMatrixRow1, rotMatrixRow2;

    float sinAngle = static_cast<float>(sin(M_PI * angle / 180.0));
    float cosAngle = static_cast<float>(cos(M_PI * angle / 180.0));
    float oneMinusCosAngle = 1.0f - cosAngle;

    rotMatrixRow0[0] = (u[0]) * (u[0]) + cosAngle*(1-(u[0])*(u[0]));
    rotMatrixRow0[1] = (u[0]) * (u[1])*(oneMinusCosAngle) - sinAngle*u[2];
    rotMatrixRow0[2] = (u[0]) * (u[2])*(oneMinusCosAngle) + sinAngle*u[1];

    rotMatrixRow1[0] = (u[0]) * (u[1])*(oneMinusCosAngle) + sinAngle*u[2];
    rotMatrixRow1[1] = (u[1]) * (u[1]) + cosAngle*(1-(u[1])*(u[1]));
    rotMatrixRow1[2] = (u[1]) * (u[2])*(oneMinusCosAngle) - sinAngle*u[0];

    rotMatrixRow2[0] = (u[0]) * (u[2])*(oneMinusCosAngle) - sinAngle*u[1];
    rotMatrixRow2[1] = (u[1]) * (u[2])*(oneMinusCosAngle) + sinAngle*u[0];
    rotMatrixRow2[2] = (u[2]) * (u[2]) + cosAngle*(1-(u[2])*(u[2]));

    vec_out[0] = Float3::dot(vec_in, rotMatrixRow0);
    vec_out[1] = Float3::dot(vec_in, rotMatrixRow1);
    vec_out[2] = Float3::dot(vec_in, rotMatrixRow2);
  }

  Geometry* GeometryManager::makeTorusKnotGeometry(const string& name,
    const uint32_t turns, const uint32_t slices, const uint32_t stacks,
    const bool sync) {
    Geometry* ret = new Geometry(name);
    ret->addVertexAttribute(VERTATTR_POS);
    ret->addVertexAttribute(VERTATTR_NOR);

    Vector<Float3>* pos = &ret->pos();
    Vector<Float3>* nor = &ret->nor();
    Vector<uint32_t>* ind = &ret->ind();

    // Calculate the total number of vertices and reserve space
    uint32_t num_vertices=stacks*slices;
    pos->capacity(num_vertices);
    pos->resize(num_vertices);

    // Calculate the position of the centre of each ring
    Float3* ring_centres = new Float3[stacks];

    for (int i = 0; i < static_cast<int>(stacks); ++i) {
      // Calculate the value of the parameter t at this point
      double t = i * 2 * (M_PI / stacks);

      // Calculate the position
      ring_centres[i].set(
        static_cast<float>((1.0+0.3*cos(turns*t))*cos(2*t)),
        static_cast<float>((1.0+0.3*cos(turns*t))*0.3f*sin(turns*t)),
        static_cast<float>((1.0+0.3*cos(turns*t))*sin(2*t)));
    }

    // Loop through the rings
    for (int i = 0; i < static_cast<int>(stacks); ++i) {
      // Loop through the vertices making up this ring
      for (int j = 0; j < static_cast<int>(slices); ++j) {
        // Calculate the number of this vertex
        int vertex_number = i * static_cast<int>(slices) + j;

        // Get the vector from the centre of this ring to the centre of the next
        Float3 tangent;
        if (i == (static_cast<int>(stacks) - 1)) {
          Float3::sub(tangent, ring_centres[0], ring_centres[i]);
        } else {
          Float3::sub(tangent, ring_centres[i+1], ring_centres[i]);
        }

        // Calculate the vector perpendicular to the tangent, pointing 
        // approximately in the positive Y direction
        static Float3 yaxis(0.0f, 1.0f, 0.0f);
        Float3 temp1;
        Float3::cross(temp1, yaxis, tangent);
        Float3 temp2;
        Float3::cross(temp2, tangent, temp1);
        temp2.normalize();
        Float3::scale(temp2, 0.2f);

        // Rotate this about the tangent vector to form the ring
        Float3 temp2_rotated;
        GetRotatedAxis(temp2_rotated, temp2, j*360.0f/slices, tangent);

        Float3::add((*pos)[vertex_number], ring_centres[i], temp2_rotated);
      }
    }

    // Calculate the total number of indices and reserve space
    uint32_t num_indices = 6 * stacks * slices;
    ind->capacity(num_indices);
    ind->resize(num_indices);

    // Calculate the indices
    for (uint32_t i = 0; i < stacks; ++i) {
      for (uint32_t j = 0; j < slices; ++j) {
        // Get the index for the 4 vertices around this "quad"
        uint32_t quad_indices[4];

        quad_indices[0] = i * slices + j;

        if (j != slices - 1) {
          quad_indices[1] = i * slices + j + 1;
        } else {
          quad_indices[1] = i * slices;
        }

        if (i != stacks - 1) {
          quad_indices[2] = (i + 1) * slices + j;
        } else {
          quad_indices[2]=j;
        }

        if (i != stacks - 1) {
          if (j != slices - 1) {
            quad_indices[3] = (i + 1) * slices + j + 1;
          } else {
            quad_indices[3] = (i + 1) * slices;
          }
        } else {
          if (j != slices - 1) {
            quad_indices[3] = j + 1;
          } else {
            quad_indices[3] = 0;
          }
        }

        (*ind)[(i*slices+j)*6] = quad_indices[0];
        (*ind)[(i*slices+j)*6+1] = quad_indices[2];
        (*ind)[(i*slices+j)*6+2] = quad_indices[1];

        (*ind)[(i*slices+j)*6+3] = quad_indices[3];
        (*ind)[(i*slices+j)*6+4] = quad_indices[1];
        (*ind)[(i*slices+j)*6+5] = quad_indices[2];
      }
    }

    nor->capacity(num_vertices);
    nor->resize(num_vertices);

    // Clear the normals
    for (uint32_t i = 0; i < num_vertices; ++i) {
      (*nor)[i].zeros();
    }

    // Loop through the triangles adding the normal to each vertex
    Float3 vec1;
    Float3 vec2;
    Float3 cur_normal;
    for (uint32_t i = 0; i < num_indices; i += 3) {
      uint32_t p0 = (*ind)[i];
      uint32_t p1 = (*ind)[i+1];
      uint32_t p2 = (*ind)[i+2];

      // Calculate the normal for this triangle
      Float3::sub(vec1, (*pos)[p0], (*pos)[p1]);
      Float3::sub(vec2, (*pos)[p2], (*pos)[p1]);
      Float3::cross(cur_normal, vec1, vec2);
      cur_normal.normalize();

      // Add this to each of its vertices
      Float3::add((*nor)[p0], (*nor)[p0], cur_normal);
      Float3::add((*nor)[p1], (*nor)[p1], cur_normal);
      Float3::add((*nor)[p2], (*nor)[p2], cur_normal);
    }

    // Normalize the normals
    for (uint32_t i = 0; i < num_vertices; ++i) {
      (*nor)[i].normalize();
    }

    // Delete the ringCentres array
    if (ring_centres) {
      delete[] ring_centres;
      ring_centres = NULL;
    }

    if (sync) {
      ret->sync();
    }
    return ret;
  }

  // This function calculates the outside radius of the geometry for a 
  // sphere with a given number of slices, stacks and inside radius.  Very
  // useful to know how far a discrete sphere extends beyond a perfect sphere.
  float GeometryManager::calcSphereOutsideRadius(const uint32_t n_stacks, 
    const uint32_t n_slices, const float inside_radius) {
    if(n_stacks < 3) {
      throw wruntime_error(L"GeometryManager::calcSphereOutsideRadius() - "
        L"Error: n_stacks < 3");
    }
    if(n_slices < 4) {
      throw wruntime_error(L"GeometryManager::calcSphereOutsideRadius() - "
        L"Error: n_slices < 4");
    }

    float slice_seperation_angle = 2.0f * static_cast<float>(M_PI) / 
        static_cast<float>(n_slices);
    float slice_outside_rad = inside_radius / 
      cosf(slice_seperation_angle * 0.5f); // for vertex points

    float stack_seperation_angle = 1.0f * static_cast<float>(M_PI) / 
      static_cast<float>(n_stacks - 1);
    float stack_outside_radius = inside_radius / 
      cosf(stack_seperation_angle * 0.5f); // for vertex points

    return std::max<float>(slice_outside_rad, stack_outside_radius);
  }

  Geometry* GeometryManager::makeSphereGeometry(const string& name,
    const uint32_t n_stacks, const uint32_t n_slices, 
    const float inside_radius, const bool sync) {
    Geometry* ret = new Geometry(name);
    ret->addVertexAttribute(VERTATTR_POS);
    ret->addVertexAttribute(VERTATTR_NOR);

    Vector<Float3>* pos = &ret->pos();
    Vector<Float3>* nor = &ret->nor();
    Vector<uint32_t>* ind = &ret->ind();

    if(n_stacks < 3) {
      throw wruntime_error(L"GeometryManager::makeSphere() - Error: "
        L"n_stacks < 3");
    }
    if(n_slices < 4) {
      throw wruntime_error(L"GeometryManager::makeSphere() - Error: "
        L"n_slices < 4");
    }

    // bottom and top stacks only have 1 point each
    uint32_t n_vert = std::max<uint32_t>((n_stacks - 2), 
      static_cast<uint32_t>(0)) * n_slices + 2;
    pos->capacity(n_vert);
    pos->resize(n_vert);
    nor->capacity(n_vert);
    nor->resize(n_vert);

    uint32_t n_ind = (2 * n_slices * 3) + std::max<uint32_t>((n_stacks - 3),
      static_cast<uint32_t>(0)) * (2 * n_slices * 3);
    ind->capacity(n_ind);
    ind->resize(n_ind);

    float slice_seperation_angle = 2.0f * static_cast<float>(M_PI) /
      static_cast<float>(n_slices);

    float stack_seperation_angle = 1.0f * static_cast<float>(M_PI) / 
      static_cast<float>(n_stacks - 1);

    float outside_radius = calcSphereOutsideRadius(n_stacks, n_slices, 
      inside_radius);

    // classic spherical coords: - theta = azimuth angle from top
    //                           - phi = zenith angle along slice
    float phi; float theta; 
    uint32_t cur_vertex = 0;

    // top first
    phi = 0; theta = 0;
    SphericalToCartesean((*pos)[cur_vertex], 
      outside_radius, phi, theta); 
    // Just point the normal outwards
    Float3::normalize((*nor)[cur_vertex], (*pos)[cur_vertex]);
    cur_vertex++;

    // Intermediate stacks
    for (uint32_t i = 1; i < (n_stacks - 1); i++) {
      theta = i * stack_seperation_angle;  // [0, pi]
      for (uint32_t j = 0; j < n_slices; j++) {
        phi = j * slice_seperation_angle;  // [0, 2*pi]
        SphericalToCartesean((*pos)[cur_vertex], outside_radius, phi, 
          theta); 
        // Just point the normal outwards
        Float3::normalize((*nor)[cur_vertex], (*pos)[cur_vertex]);
        cur_vertex++;
      }
    }

    // bottom
    phi = 0; theta = static_cast<float>(M_PI);
    SphericalToCartesean((*pos)[cur_vertex], outside_radius, phi, theta); 
    // Just point the normal outwards
    Float3::normalize((*nor)[cur_vertex], (*pos)[cur_vertex]);
    cur_vertex++;

#if defined(DEBUG) || defined(_DEBUG)
    if(cur_vertex != n_vert) {
      throw wruntime_error(L"GeometryManager::makeSphere() -"
        L" Internal Error: Didn't create enough verticies");
    }
#endif

    // Top Indices
    uint32_t cur_index = 0;
    for (uint32_t j = 0; j < (n_slices - 1); j ++) {
      (*ind)[cur_index] = 0; // Top Vertex
      cur_index++;
      (*ind)[cur_index] = j + 2;
      cur_index++;
      // First vertex is the top, so first slice vertex starts at 1
      (*ind)[cur_index] = j + 1; 
      cur_index++;
    }
    (*ind)[cur_index] = 0; // Top Vertex
    cur_index++;
    (*ind)[cur_index] = 1; 
    cur_index++;
    // Back to the first vertex
    (*ind)[cur_index] = n_slices; 
    cur_index++;

    // Intermediate indices
    for (uint32_t i = 1; i < (n_stacks - 2); i++) {
      uint32_t cur_stack_start_index = 1 + (i - 1) * n_slices;
      uint32_t next_stack_start_index = 1 + i * n_slices;
      for (uint32_t j = 0; j < (n_slices - 1); j++) {
        (*ind)[cur_index] = cur_stack_start_index + j;
        cur_index++;
        (*ind)[cur_index] = cur_stack_start_index + j + 1;
        cur_index++;
        (*ind)[cur_index] = next_stack_start_index + j;
        cur_index++;
        (*ind)[cur_index] = cur_stack_start_index + j + 1;
        cur_index++;
        (*ind)[cur_index] = next_stack_start_index + j + 1;
        cur_index++;
        (*ind)[cur_index] = next_stack_start_index + j;
        cur_index++;
      }
      (*ind)[cur_index] = cur_stack_start_index + (n_slices-1);
      cur_index++;
      (*ind)[cur_index] = cur_stack_start_index;  // Back to first vert
      cur_index++;
      (*ind)[cur_index] = next_stack_start_index + (n_slices-1);
      cur_index++;
      (*ind)[cur_index] = cur_stack_start_index;  // Back to first vert
      cur_index++;
      (*ind)[cur_index] = next_stack_start_index;  // Back to first vert
      cur_index++;
      (*ind)[cur_index] = next_stack_start_index + (n_slices-1);
      cur_index++;
    }

    // Bottom Indices
    uint32_t cur_stack_start_index = ( n_stacks - 3 ) * n_slices + 1;
    for (uint32_t j = 0; j < ( n_slices - 1); j++) {
      // First vertex is the top, so first slice vertex starts at 1
      (*ind)[cur_index] = cur_stack_start_index + j; 
      cur_index++;
      (*ind)[cur_index] = cur_stack_start_index + j + 1;
      cur_index++;
      (*ind)[cur_index] = n_vert-1;  // Bottom Vertex
      cur_index++;
    }
    (*ind)[cur_index] = cur_stack_start_index + n_slices - 1;
    cur_index++;
    (*ind)[cur_index] = cur_stack_start_index;  // Back to first vert
    cur_index++;
    (*ind)[cur_index] = n_vert-1; // Top Vertex
    cur_index++;

#if defined(DEBUG) || defined(_DEBUG)
    if(cur_index != n_ind) {
      throw wruntime_error(L"GeometryManager::makeSphere() -"
        L" Internal Error: Didn't create enough indices");
    }
#endif

    if (sync) {
      ret->sync();
    }
    return ret;
  }

  // This function calculates the outside radius of the geometry for a 
  // cone with a given number of slices and inside radius.  Very
  // useful to know how far a discrete cone extends beyond a perfect cone.
  float GeometryManager::calcConeOutsideRadius(const uint32_t n_slices, 
    const float inside_radius) {
    if(n_slices < 3) {
      throw wruntime_error(L"GeometryManager::calcConeOutsideRadius() - "
        L"Error: n_slices < 3");
    }

    float seperation_angle = 2.0f * static_cast<float>(M_PI) / 
      static_cast<float>(n_slices);
    return inside_radius / cosf(seperation_angle * 0.5f);
  }

  Geometry* GeometryManager::makeConeGeometry(const string& name,
    uint32_t n_slices, float height, float inside_radius, bool sync) {
    Geometry* ret = new Geometry(name);
    ret->addVertexAttribute(VERTATTR_POS);
    ret->addVertexAttribute(VERTATTR_NOR);

    Vector<Float3>* pos = &ret->pos();
    Vector<Float3>* nor = &ret->nor();
    Vector<uint32_t>* ind = &ret->ind();

    if(n_slices < 3) {
      throw wruntime_error(L"GeometryManager::makeCone()"
        L"- Error: n_slices < 3");
    }
   

    uint32_t n_vert = n_slices + 1 + // base vertices
                      n_slices * 2;  // Each side needs a top vert for the norm
    pos->capacity(n_vert);
    pos->resize(n_vert);
    nor->capacity(n_vert);
    nor->resize(n_vert);

    uint32_t n_ind = (n_slices * 3) + (n_slices * 3);
    ind->capacity(n_ind);
    ind->resize(n_ind);

    uint32_t cur_vertex = 0;
    float seperation_angle = 2.0f * static_cast<float>(M_PI) / 
      static_cast<float>(n_slices);
    float outside_rad = calcConeOutsideRadius(n_slices, inside_radius);

    // Define the base center vertex
    nor->at(cur_vertex)->set(0.0f, 1.0f, 0.0f);
    pos->at(cur_vertex)->set(0.0f, height, 0.0f);
    cur_vertex ++;

    // Define the base indices
    for (uint32_t i = 0; i < n_slices; i++) {
      float cur_angle = seperation_angle * static_cast<float>(i);
      nor->at(cur_vertex)->set(0.0f, 1.0f, 0.0f);
      pos->at(cur_vertex)->set(outside_rad * cosf(cur_angle), height, 
        outside_rad * sinf(cur_angle));
      cur_vertex ++;
    }

    Float3 cur_base_point, next_base_point, cur_normal, v1, v2;
    Float3 top_point(0, 0, 0);
    float cur_angle, tangent_angle;

    // Define the cone side and top vertices
    for (uint32_t i = 0; i < n_slices; i++) {
      // base
      cur_angle = seperation_angle * static_cast<float>(i);
      cur_base_point.set(outside_rad * cosf(cur_angle), height, 
        outside_rad * sinf(cur_angle));
      pos->at(cur_vertex)->set(cur_base_point);
      tangent_angle = cur_angle + 
        (static_cast<float>(M_PI) / 2.0f);  // Tangent is 90deg away
      v1.set(cosf(tangent_angle), 0.0f, sinf(tangent_angle));  // length 1!
      Float3::normalize(v2, cur_base_point);
      Float3::scale(v2, -1.0f);  // vector from cur position --> Top (0,0,0)
      Float3::cross(cur_normal, v1, v2);
      cur_normal.normalize();
      nor->at(cur_vertex)->set(cur_normal);
      cur_vertex ++;
      
      // next base (there is some re-work here, but that's OK).
      cur_angle = seperation_angle * static_cast<float>(i+1);
      next_base_point.set(outside_rad * cosf(cur_angle), height,
        outside_rad * sinf(cur_angle));

      // Treat the top point as
      pos->at(cur_vertex)->set(top_point);
      Float3::sub(v1, cur_base_point, top_point);
      Float3::sub(v2, next_base_point, top_point);
      v1.normalize();
      v2.normalize();
      Float3::cross(cur_normal, v1, v2);
      cur_normal.normalize();
      nor->at(cur_vertex)->set(cur_normal);
      cur_vertex ++;
    }

#if defined(DEBUG) || defined(_DEBUG)
    if(cur_vertex != n_vert) {
      throw wruntime_error(L"GeometryManager::makeCone() -"
        L" Internal Error: Didn't create enough vertices");
    }
#endif

    // Now define the base indices
    uint32_t cur_index = 0;
    for (uint32_t i = 1; i < n_slices; i++)
    {
      (*ind)[cur_index] = i;
      cur_index ++;
      (*ind)[cur_index] = i + 1;
      cur_index ++;
      (*ind)[cur_index] = 0; // Center
      cur_index ++;
    }
    // Now define the last triangle in the base
    (*ind)[cur_index] = n_slices;
    cur_index ++;
    (*ind)[cur_index] = 1;
    cur_index ++;
    (*ind)[cur_index] = 0; // Center
    cur_index ++;

    // Define the cone side indices
    for (uint32_t i = 0; i < (n_slices-1); i++) {
      (*ind)[cur_index] = n_slices + 1 + (2*i); // base point 1
      cur_index ++;
      (*ind)[cur_index] = n_slices + 1 + (2*i + 1); // top
      cur_index ++;
      (*ind)[cur_index] = n_slices + 1 + (2*i + 2);  // base point 2
      cur_index ++;
    }
    // Now define the last triangle on the side
    (*ind)[cur_index] = n_slices + 1 + (2*(n_slices-1)); // base point 1
    cur_index ++;
    (*ind)[cur_index] = n_slices + 1 + (2*(n_slices-1) + 1); // top
    cur_index ++;
    (*ind)[cur_index] = n_slices + 1; // Back to the start
    cur_index ++;


#if defined(DEBUG) || defined(_DEBUG)
    if(cur_index != n_ind) {
      throw wruntime_error(L"GeometryManager::makeCone() -"
        L" Internal Error: Didn't create enough indices");
    }
#endif

    if (sync) {
      ret->sync();
    }
    return ret;
  }
  
  Geometry* GeometryManager::makeCylinderGeometry(const string& name,
    const uint32_t n_slices, const float height, 
    const float base_inside_radius, const float top_inside_radius, 
    const bool sync) {
    Geometry* ret = new Geometry(name);
    ret->addVertexAttribute(VERTATTR_POS);
    ret->addVertexAttribute(VERTATTR_NOR);

    Vector<Float3>* pos = &ret->pos();
    Vector<Float3>* nor = &ret->nor();
    Vector<uint32_t>* ind = &ret->ind();

    if(n_slices < 3) {
      throw wruntime_error(L"GeometryManager::makeCylinder() - Error: "
        L"n_slices < 3");
    }
    
    uint32_t n_vert = (n_slices + 1 + // base vertices
                       n_slices + 1 + // top vertices
                       n_slices * 2); // Side vertices
    pos->capacity(n_vert);
    pos->resize(n_vert);
    nor->capacity(n_vert);
    nor->resize(n_vert);
    
    uint32_t n_ind = ((n_slices * 3) + // base triangles
                      (n_slices * 3) + // top triangles
                      2 * (n_slices * 3));  // Side triangles
    ind->capacity(n_ind);
    ind->resize(n_ind);
    
    uint32_t cur_vertex = 0;
    float seperation_angle = (2.0f * static_cast<float>(M_PI) /
                              static_cast<float>(n_slices));
    float base_outside_rad = calcConeOutsideRadius(n_slices, 
      base_inside_radius);
    float top_outside_rad = calcConeOutsideRadius(n_slices, top_inside_radius);
    
    // Define the base center vertex
    nor->at(cur_vertex)->set(0.0f, 1.0f, 0.0f);
    pos->at(cur_vertex)->set(0.0f, 0.5f * height, 0.0f);
    cur_vertex ++;
    
    // Define the base radius vertices
    for (uint32_t i = 0; i < n_slices; i++) {
      float cur_angle = seperation_angle * static_cast<float>(i);
      nor->at(cur_vertex)->set(0.0f, 1.0f, 0.0f);
      pos->at(cur_vertex)->set(base_outside_rad * cosf(cur_angle),
        0.5f * height, base_outside_rad * sinf(cur_angle));
      cur_vertex ++;
    }
    
    // Define the top center vertex
    nor->at(cur_vertex)->set(0.0f, -1.0f, 0.0f);
    pos->at(cur_vertex)->set(0.0f, -0.5f * height, 0.0f);
    cur_vertex ++;
    
    // Define the top radius vertices
    for (uint32_t i = 0; i < n_slices; i++) {
      float cur_angle = seperation_angle * static_cast<float>(i);
      nor->at(cur_vertex)->set(0.0f, -1.0f, 0.0f);
      pos->at(cur_vertex)->set(top_outside_rad * cosf(cur_angle),
        -0.5f * height, top_outside_rad * sinf(cur_angle));
      cur_vertex ++;
    }
    
    Float3 cur_base_point, cur_top_point, v1, v2, cur_normal;
    float cur_angle, tangent_angle;
    
    // Define the cone side and top vertices
    for (uint32_t i = 0; i < n_slices; i++) {
      cur_angle = seperation_angle * static_cast<float>(i);
      cur_base_point.set(base_outside_rad * cosf(cur_angle), 0.5f * height,
                         base_outside_rad * sinf(cur_angle));
      cur_top_point.set(top_outside_rad * cosf(cur_angle), -0.5f * height,
                        top_outside_rad * sinf(cur_angle));
      pos->at(cur_vertex)->set(cur_base_point);
      pos->at(cur_vertex+1)->set(cur_top_point);
      
      // Tangent is 90deg away: use it to calculate the normal along the slope
      tangent_angle = cur_angle + (static_cast<float>(M_PI) / 2.0f);  
      v1.set(cosf(tangent_angle), 0.0f, sin(tangent_angle));  // length 1!
      Float3::sub(v2, cur_base_point, cur_top_point);
      v2.normalize();
      Float3::cross(cur_normal, v2, v1 );
      cur_normal.normalize();
      nor->at(cur_vertex)->set(cur_normal);
      nor->at(cur_vertex+1)->set(cur_normal);
      cur_vertex += 2;
    }
    
#if defined(DEBUG) || defined(_DEBUG)
    if(cur_vertex != n_vert) {
      throw wruntime_error(L"GeometryManager::makeCylinder() -"
        L" Internal Error: Didn't create enough vertices");
    }
#endif
    
    // Now define the base indices
    uint32_t cur_index = 0;
    for (uint32_t i = 1; i < n_slices; i++)
    {
      (*ind)[cur_index] = i;
      cur_index ++;
      (*ind)[cur_index] = i + 1;
      cur_index ++;
      (*ind)[cur_index] = 0; // Center
      cur_index ++;
    }
    // Now define the last triangle in the base
    (*ind)[cur_index] = n_slices;
    cur_index ++;
    (*ind)[cur_index] = 1;
    cur_index ++;
    (*ind)[cur_index] = 0; // Center
    cur_index ++;
    
    // Now define the top indices
    uint32_t top_indices_start = n_slices + 1;
    for (uint32_t i = 1; i < n_slices; i++)
    {
      (*ind)[cur_index] = top_indices_start + i;
      cur_index ++;
      (*ind)[cur_index] = top_indices_start + 0; // Center
      cur_index ++;
      (*ind)[cur_index] = top_indices_start + i + 1;
      cur_index ++;
    }
    // Now define the last triangle in the top
    (*ind)[cur_index] = top_indices_start + n_slices;
    cur_index ++;
    (*ind)[cur_index] = top_indices_start + 0; // Center
    cur_index ++;
    (*ind)[cur_index] = top_indices_start + 1;
    cur_index ++;
    
    // Define the cylinder side indices --> Each is a quad patch with 2 tris
    uint32_t side_indices_start = top_indices_start + n_slices + 1;
    for (uint32_t i = 0; i < (n_slices-1); i++) {
      (*ind)[cur_index] = side_indices_start + (2*i); // base
      cur_index ++;
      (*ind)[cur_index] = side_indices_start + (2*i + 1);  // top
      cur_index ++;
      (*ind)[cur_index] = side_indices_start + (2*i + 3); // top
      cur_index ++;
      (*ind)[cur_index] = side_indices_start + (2*i + 2); // base
      cur_index ++;
      (*ind)[cur_index] = side_indices_start + (2*i);  // base
      cur_index ++;
      (*ind)[cur_index] = side_indices_start + (2*i + 3); // top
      cur_index ++;
    }
    // Now define the last 2 triangles on the side
    (*ind)[cur_index] = side_indices_start + (2*(n_slices-1)); // base
    cur_index ++;
    (*ind)[cur_index] = side_indices_start + (2*(n_slices-1) + 1); // top
    cur_index ++;
    (*ind)[cur_index] = side_indices_start + 1; // top
    cur_index ++;
    (*ind)[cur_index] = side_indices_start; // base
    cur_index ++;
    (*ind)[cur_index] = side_indices_start + (2*(n_slices-1)); // top
    cur_index ++;
    (*ind)[cur_index] = side_indices_start + 1; // top
    cur_index ++;
    
#if defined(DEBUG) || defined(_DEBUG)
    if(cur_index != n_ind) {
      throw wruntime_error(L"GeometryManager::makeCylinder() -"
        L" Internal Error: Didn't create enough indices");
    }
#endif
    
    if (sync) {
      ret->sync();
    }
    return ret;
  }

  GeometryInstance* GeometryManager::makeDispQuad() {
    stringstream ss;
    ss << "dispQuad";
    Geometry* geom = findGeometryByName(ss.str());
    if (geom == NULL) {
      geom = makeDispQuadGeometry(ss.str());
      addGeometry(geom);
    }
    GeometryInstance* instance = createNewInstance(geom);
    return instance;
  }

  GeometryInstance* GeometryManager::makeCube(const math::Float3& col) {
    stringstream ss;
    ss << "cube";
    Geometry* geom = findGeometryByName(ss.str());
    if (geom == NULL) {
      geom = makeCubeGeometry(ss.str());
      addGeometry(geom);
    }
    GeometryInstance* instance = createNewInstance(geom);
    instance->mtrl().albedo = col;
    return instance;
  }

  GeometryInstance* GeometryManager::makeCubeRainbow() {
    string name("cube<rainbow>");
    Geometry* geom = findGeometryByName(name);
    if (geom == NULL) {
      geom = makeCubeGeometry(name, false);
      geom->addVertexAttribute(VERTATTR_COL);
      for (uint32_t i = 0; i < geom->pos().size(); i++) {
        geom->addCol(rainbow_colors_[i % GEOMETRY_MANAGER_N_RAINBOW_COL]);
      }
      geom->sync();
      addGeometry(geom);
    }
    return createNewInstance(geom);
  }

  GeometryInstance* GeometryManager::makePyramid(const math::Float3& col) {
    stringstream ss;
    ss << "pyramid<" << col[0] << "," << col[1] << "," << col[2] << ">";
    Geometry* geom = findGeometryByName(ss.str());
    if (geom == NULL) {
      geom = makePyramidGeometry(ss.str());
      addGeometry(geom);
    }
    GeometryInstance* instance = createNewInstance(geom);
    instance->mtrl().albedo = col;
    return instance;
  }

  GeometryInstance* GeometryManager::makePyramidRainbow() {
    string name("pyramid<rainbow>");
    Geometry* geom = findGeometryByName(name);
    if (geom == NULL) {
      geom = makePyramidGeometry(name, false);
      geom->addVertexAttribute(VERTATTR_COL);
      for (uint32_t i = 0; i < geom->pos().size(); i++) {
        geom->addCol(rainbow_colors_[i % GEOMETRY_MANAGER_N_RAINBOW_COL]);
      }
      geom->sync();
      addGeometry(geom);
    }
    return createNewInstance(geom);
  }

  GeometryInstance* GeometryManager::makeTorusKnot(const math::Float3& col, 
    const uint32_t turns, const uint32_t slices, const uint32_t stacks) {
    stringstream ss;
    ss << "torusKnot<" << col[0] << "," << col[1] << "," << col[2] << ",";
    ss << turns << "," << slices << "," << stacks << ">";
    Geometry* geom = findGeometryByName(ss.str());
    if (geom == NULL) {
      geom = makeTorusKnotGeometry(ss.str(), turns, slices, stacks);
      addGeometry(geom);
    }
    GeometryInstance* instance = createNewInstance(geom);
    instance->mtrl().albedo = col;
    return instance;
  }

  GeometryInstance* GeometryManager::makeSphere(const uint32_t n_stacks, 
    const uint32_t n_slices, const float inside_radius, 
    const math::Float3& col) {
    stringstream ss;
    ss << "sphere<" << col[0] << "," << col[1] << "," << col[2] << ",";
    ss << n_stacks << "," << n_slices << "," << inside_radius << ">";
    Geometry* geom = findGeometryByName(ss.str());
    if (geom == NULL) {
      geom = makeSphereGeometry(ss.str(), n_stacks, n_slices, inside_radius);
      addGeometry(geom);
    }
    GeometryInstance* instance = createNewInstance(geom);
    instance->mtrl().albedo = col;
    return instance;
  }

  GeometryInstance* GeometryManager::makeCone(const uint32_t n_slices, 
    const float height, const float inside_radius, const math::Float3& col) {
    stringstream ss;
    ss << "cone<" << col[0] << "," << col[1] << "," << col[2] << ",";
    ss << n_slices << "," << height << inside_radius << ">";
    Geometry* geom = findGeometryByName(ss.str());
    if (geom == NULL) {
      geom = makeConeGeometry(ss.str(), n_slices, height, inside_radius);
      addGeometry(geom);
    }
    GeometryInstance* instance = createNewInstance(geom);
    instance->mtrl().albedo = col;
    return instance;
  }

  GeometryInstance* GeometryManager::makeCylinder(const uint32_t n_slices, 
    const float height, const float base_inside_radius, 
    const float top_inside_radius, const math::Float3& col) {
    stringstream ss;
    ss << "cylinder<" << col[0] << "," << col[1] << "," << col[2] << ",";
    ss << n_slices << "," << height << base_inside_radius << ",";
    ss << top_inside_radius << ">";
    Geometry* geom = findGeometryByName(ss.str());
    if (geom == NULL) {
      geom = makeCylinderGeometry(ss.str(), n_slices, height, 
        base_inside_radius, top_inside_radius);
      addGeometry(geom);
    }
    GeometryInstance* instance = createNewInstance(geom);
    instance->mtrl().albedo = col;
    return instance;
  }

  GeometryInstance* GeometryManager::createNewInstance(const Geometry* geom) {
    GeometryInstance* ret = new GeometryInstance(geom);
    return ret;
  }

  void GeometryManager::saveModelToJBinFile(const string& path, 
    const string& filename, const GeometryInstance* model) {
    // Open a filestream for writing
    string full_path;
    if (path.at(path.length()-1) != '/' && path.at(path.length()-1) != '\\') {
      full_path = path + string("/") + filename;
    } else {
      full_path = path + filename;
    }

    std::ofstream file(full_path.c_str(), std::ios::out | std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error(string("error opening file:") + full_path);
    }

    cout << "Saving model to " << full_path << filename << "..." << endl;

    // Save the meshes first.  
    saveModelMeshesToJBinFile(file, model);

    // Now save the scene graph structure
    saveModelNodesToJBinFile(file, model);

    saveModelBonesToJBinFile(file, model);

    file.close();

    cout << "Finished saving " << full_path << filename << endl;
  }

  void GeometryManager::saveModelMeshesToJBinFile(std::ofstream& file, 
    const GeometryInstance* model) {
    // Collect all used meshes into a vector.
    Vector<const Geometry*> meshes;
    HashSet<string> mesh_names(11, data_str::HashString);
    Vector<const GeometryInstance*> stack;
    stack.pushBack(model);
    while (stack.size() > 0) {
      const GeometryInstance* cur_node;
      stack.popBack(cur_node);
      if (cur_node->geom()) {
        if (!mesh_names.lookup(cur_node->geom()->name())) {
          mesh_names.insert(cur_node->geom()->name());
          meshes.pushBack(cur_node->geom());
        }
      }
      for (uint32_t i = 0; i < cur_node->numChildren(); i++) {
        stack.pushBack(cur_node->getChild(i));
      }
    }

    // Save the number of meshes
    file.write((const char*)(&meshes.size()), sizeof(meshes.size()));
    file.flush();

    // Now save each mesh
    for (uint32_t i = 0; i < meshes.size(); i++) {
      // Convert the geometry to a data array:
      // The pair is 1. the data and 2. the length of the data
      Pair<uint8_t*,uint32_t> cur_data = meshes[i]->saveToArray();

      // Compress the data using UCL
      uint32_t compressed_size = UCLHelper::inPlaceCompress(cur_data.first, 
        cur_data.second, 10);

      // Try FastLZ library and see how it performs
      //uint8_t* compressed_data = 
      //  new uint8_t[FastlzHelper::calcCompressOverhead(cur_data.first)];
      //uint32_t compressed_size2 = FastlzHelper::Compress(
      //  cur_data.first, compressed_data, cur_data.second, 2);

      // Save the data to file as well as it's length
      file.write((const char*)(&compressed_size), sizeof(compressed_size));
      file.write((const char*)(&cur_data.second), sizeof(cur_data.second));
      file.write((const char*)cur_data.first, compressed_size);
      file.flush();

      // Were done with the array data so delete it
      free(cur_data.first);

      cout << "   Saved geometry - type: " << (uint32_t)meshes[i]->type();
      cout << ", faces: " << meshes[i]->ind().size() / 3;
      cout << ", name: " << meshes[i]->name() << std::endl;
    }
  }

  void GeometryManager::saveModelNodesToJBinFile(std::ofstream& file, 
    const GeometryInstance* model) {
    // Count the number of nodes (DFS):
    Vector<const GeometryInstance*> stack;
    stack.pushBack(model);
    uint32_t n_nodes = 0;
    while (stack.size() > 0) {
      n_nodes++;
      const GeometryInstance* cur_node;
      stack.popBack(cur_node);
      for (uint32_t i = 0; i < cur_node->numChildren(); i++) {
        stack.pushBack(cur_node->getChild(i));
      }
    }

    // Write the total number of nodes in the file first
    file.write((const char*)(&n_nodes), sizeof(n_nodes));
    file.flush();

    // Now save the whole heirachy to file BFS and compress each node as we go
    data_str::CircularBuffer<const GeometryInstance*> queue(n_nodes+1);
    queue.write(model);  // Push this node to the back of the empty queue
    while(!queue.empty()) {
      const GeometryInstance* cur_node;
      queue.read(cur_node);
      
      // Add the node's children to the queue for processing later
      for (uint32_t i = 0; i < cur_node->numChildren(); i++) {
        queue.write(cur_node->getChild(i));
      }

      // Convert the node to a data array:
      // The pair is 1. the data and 2. the length of the data
      Pair<uint8_t*,uint32_t> cur_data = cur_node->saveToArray();

      // Compress the data using UCL
      uint32_t compressed_size = UCLHelper::inPlaceCompress(cur_data.first, 
        cur_data.second, 10);

      // Save the data to file as well as it's length
      file.write((const char*)(&compressed_size), sizeof(compressed_size));
      file.write((const char*)(&cur_data.second), sizeof(cur_data.second));
      file.write((const char*)cur_data.first, compressed_size);
      file.flush();

      // Were done with the array data so delete it
      free(cur_data.first);
    }
  }
  
  void GeometryManager::saveModelBonesToJBinFile(std::ofstream& file, 
    const GeometryInstance* model) {
    HashMap<string, uint32_t> bone_name2ind(GM_START_HM_SIZE, 
      data_str::HashString);
    Vector<Bone*> bones_in_tree;
    // Collect all the bones in the file
    Vector<const GeometryInstance*> stack;
    stack.pushBack(model);
    while (stack.size() > 0) {
      const GeometryInstance* cur_node;
      stack.popBack(cur_node);
      if (cur_node->geom()) {
        for (uint32_t i = 0; i < cur_node->geom()->bone_names().size(); i++) {
          std::string cur_id(cur_node->geom()->bone_names()[i]);
          Bone* cur_bone = findBoneByName(cur_id);
          if (cur_bone == NULL) {
            throw std::wruntime_error("GeometryManager::"
              "saveModelBonesToJBinFile() - ERROR: Couldn't find bone by ID!");
          }
          uint32_t ind;
          if (!bone_name2ind.lookup(cur_bone->bone_name, ind)) {
            // Bone hasn't been added yet, add it
            ind = bones_in_tree.size();
            bone_name2ind.insert(cur_bone->bone_name, ind);
            bones_in_tree.pushBack(cur_bone);
          }
        }
      }
      for (uint32_t i = 0; i < cur_node->numChildren(); i++) {
        stack.pushBack(cur_node->getChild(i));
      }
    }
    
    // Now export them to disk
    uint32_t n_nodes = bones_in_tree.size();
    file.write((const char*)(&n_nodes), sizeof(n_nodes));
    file.flush();

    for (uint32_t i = 0; i < bones_in_tree.size(); i++) {
      Bone* cur_bone = bones_in_tree[i];
      Pair<uint8_t*,uint32_t> cur_data = cur_bone->saveToArray();

      // Compress the data using UCL
      uint32_t compressed_size = UCLHelper::inPlaceCompress(cur_data.first, 
        cur_data.second, 10);

      // Save the data to file as well as it's length
      file.write((const char*)(&compressed_size), sizeof(compressed_size));
      file.write((const char*)(&cur_data.second), sizeof(cur_data.second));
      file.write((const char*)cur_data.first, compressed_size);
      file.flush();

      // Were done with the array data so delete it
      free(cur_data.first);
    }
  }

  GeometryInstance* GeometryManager::loadModelFromJBinFile(
    const string& path, const string& filename) {
    // Open a filestream for writing
    string full_path;
    if (path.at(path.length()-1) != '/' && path.at(path.length()-1) != '\\') {
      full_path = path + string("/");
    } else {
      full_path = path;
    }

    string path_filename = full_path + filename;

    std::ifstream file(path_filename.c_str(), std::ios::in | std::ios::binary |
      std::ios::ate);
    if (!file.is_open()) {
      throw wruntime_error(string("loadFromJFile()") + 
        string(": error opening file") + path_filename);
    }
    file.seekg(0, std::ios::beg);

    cout << "Loading model from " << path_filename << "..." << endl;

    // First load back the Geometry
    loadModelMeshesFromJBinFile(file);

    // Now load the scene graph
    GeometryInstance* model_root = loadModelNodesFromJBinFile(file);

    loadModelBonesFromJBinFile(file);

    associateBoneTransforms(model_root);

    file.close();

    cout << "Finished loading model from " << path_filename << endl;

    return model_root;
  }

  void GeometryManager::loadModelMeshesFromJBinFile(std::ifstream& file) {
    uint32_t n_meshes;
    file.read(reinterpret_cast<char*>(&n_meshes), sizeof(n_meshes));

    // Temporary read buffer...  Avoid lots of dynamic memory allocations if
    // there are 1000s of nodes it might be slow.
    Vector<uint8_t> buffer;

    // Now load each mesh
    for (uint32_t i = 0; i < n_meshes; i++) {
      uint32_t compressed_size;
      file.read((char*)(&compressed_size), sizeof(compressed_size));
      uint32_t decompressed_size;
      file.read((char*)(&decompressed_size), sizeof(decompressed_size));

      Pair<uint8_t*,uint32_t> cur_data;
      cur_data.second = decompressed_size;

      // When performing in-place decompression, the data must start back
      // from the beginning of the array (there is some empty space at start)
      uint32_t block_size = 
        UCLHelper::calcInPlaceDecompressSizeRequirement(decompressed_size);

      if (buffer.size() < block_size) {
        buffer.capacity(block_size);
        buffer.resize(block_size);
      }

      cur_data.first = buffer.at(0);
      file.read((char*)(cur_data.first), compressed_size);

      UCLHelper::inPlaceDecompress(cur_data.first, compressed_size, 
        decompressed_size);

      string geom_name = Geometry::loadNameFromArray(cur_data);
      Geometry* cur_geom = findGeometryByName(geom_name);
      if (cur_geom == NULL) {
        cur_geom = Geometry::loadFromArray(this, cur_data);
        addGeometry(cur_geom);
      }

      cout << "   Loaded geometry - type: " << (uint32_t)cur_geom->type();
      cout << ", faces: " << cur_geom->ind().size() / 3;
      cout << ", name: " << cur_geom->name() << std::endl;
    }
  }

  GeometryInstance* GeometryManager::loadModelNodesFromJBinFile(
    std::ifstream& file) {
    // First comes the number of nodes
    uint32_t n_nodes;
    file.read(reinterpret_cast<char*>(&n_nodes), sizeof(n_nodes));

    // Now read in the nodes one, by one.  The nodes are stored BFS.
    GeometryInstance* ret_model = loadNodeFromJBinFile(file);

    data_str::CircularBuffer<GeometryInstance*> queue(n_nodes+1);
    queue.write(ret_model);  // Push this node to the back of the empty queue
    uint32_t n_nodes_read = 0;
    while(!queue.empty()) {
      GeometryInstance* cur_node;
      queue.read(cur_node);
      n_nodes_read++;

      uint32_t num_children = cur_node->children().capacity();
      for (uint32_t i = 0; i < num_children; i++) {
        GeometryInstance* child = loadNodeFromJBinFile(file);
       
        cur_node->addChild(child);
        queue.write(child);
      }
    }

    if (n_nodes_read != n_nodes) {
      throw wruntime_error("GeometryManager::loadModelNodesFromJBinFile()"
        " - INTERNAL ERROR: Incorrect number of nodes read from file!");
    }

#if (defined(WIN32) || defined(_WIN32)) && (defined(_DEBUG) || defined(DEBUG))
     _ASSERTE( _CrtCheckMemory( ) );  // Just in case something went wrong
#endif
    return ret_model;
  }

  GeometryInstance* GeometryManager::loadNodeFromJBinFile(
    std::ifstream& file) {
    uint32_t compressed_size;
    file.read((char*)(&compressed_size), sizeof(compressed_size));
    uint32_t decompressed_size;
    file.read((char*)(&decompressed_size), sizeof(decompressed_size));

    Pair<uint8_t*,uint32_t> cur_data;
    cur_data.second = decompressed_size;

    // When performing in-place decompression, the data must start back
    // from the beginning of the array (there is some empty space at start)
    uint32_t block_size = 
      UCLHelper::calcInPlaceDecompressSizeRequirement(decompressed_size);

    cur_data.first = (uint8_t*)malloc(block_size);
    file.read((char*)(cur_data.first), compressed_size);

    UCLHelper::inPlaceDecompress(cur_data.first, compressed_size, 
      decompressed_size);

    GeometryInstance* node = GeometryInstance::loadFromArray(this, cur_data);

    delete cur_data.first;

    return node;
  }

   void GeometryManager::loadModelBonesFromJBinFile(std::ifstream& file) {
    // First comes the number of nodes
    uint32_t n_bones;
    file.read(reinterpret_cast<char*>(&n_bones), sizeof(n_bones));

    // Temporary read buffer...  Avoid lots of dynamic memory allocations if
    // there are 1000s of nodes it might be slow.
    Vector<uint8_t> buffer;

    // Now read in the bones one, by one
    // Now load each mesh
    for (uint32_t i = 0; i < n_bones; i++) {
      uint32_t compressed_size;
      file.read((char*)(&compressed_size), sizeof(compressed_size));
      uint32_t decompressed_size;
      file.read((char*)(&decompressed_size), sizeof(decompressed_size));

      Pair<uint8_t*,uint32_t> cur_data;
      cur_data.second = decompressed_size;

      // When performing in-place decompression, the data must start back
      // from the beginning of the array (there is some empty space at start)
      uint32_t block_size = 
        UCLHelper::calcInPlaceDecompressSizeRequirement(decompressed_size);

      if (buffer.size() < block_size) {
        buffer.capacity(block_size);
        buffer.resize(block_size);
      }

      cur_data.first = buffer.at(0);
      file.read((char*)(cur_data.first), compressed_size);

      UCLHelper::inPlaceDecompress(cur_data.first, compressed_size, 
        decompressed_size);

      Bone* cur_bone = Bone::loadFromArray(cur_data);
      
      // just double check that a bone by this id doesn't already exist
      if (findBoneByName(cur_bone->bone_name) != NULL) {
        throw std::wruntime_error("GeometryManager::loadModelBonesFromJBinFile"
          "() - ERROR: A bone with this id already exists!");
      }
      addBone(cur_bone);

      cout << "   Loaded bone: " << cur_bone->bone_name << std::endl;
    }

#if (defined(WIN32) || defined(_WIN32)) && (defined(_DEBUG) || defined(DEBUG))
     _ASSERTE( _CrtCheckMemory( ) );  // Just in case something went wrong
#endif
  }

  void GeometryManager::associateBoneTransforms(GeometryInstance* model_root) {
    Vector<GeometryInstance*> stack;
    stack.pushBack(model_root);
    while (stack.size() > 0) {
      GeometryInstance* cur_node;
      stack.popBack(cur_node);
      if (cur_node->geom() && 
        cur_node->geom()->hasVertexAttribute(VERTATTR_BONEW)) {
        // Make sure the root node has a mat_heirachy_inv matrix allocated
        // since we'll need it for bone matrix calculations
        if (model_root->mat_hierarchy_inv() == NULL) {
          model_root->mat_hierarchy_inv() = new Float4x4();
        }

        // The current node points to a geometry object that is boned: we need
        // to find the GeometryInstance nodes that have the same name.
        uint32_t n_bones = cur_node->geom()->bone_names().size();
        cur_node->bone_nodes().capacity(n_bones);
        for (uint32_t i = 0; i < n_bones; i++) {
          string bone_name(cur_node->geom()->bone_names()[i]);
          Bone* cur_bone = findBoneByName(bone_name);
          if (cur_bone == NULL) {
            throw wruntime_error("GeometryManager::associateBoneTransforms() "
              "- ERROR: This GeometryInstance mesh data uses a bone, for which"
              " no bone exists.");
          }
          // Now find the node corresponding to this bone_name
          GeometryInstance* bone_node = findGeometryInstanceByName(bone_name, 
            model_root);
          bone_node->bone() = cur_bone;
          bone_node->bone_root_node() = model_root;
          cur_bone->rest_transform.set(bone_node->mat());
          // Allocate space for the bone transform
          if (bone_node->bone_transform() == NULL) {
            bone_node->bone_transform() = new Float4x4();
          }
          if (bone_node == NULL) {
            throw wruntime_error("GeometryManager::associateBoneTransforms() "
              "- ERROR: This GeometryInstance mesh data uses a bone, for which"
              " no node exists.");
          }
          cur_node->bone_nodes().pushBack(bone_node);
        }  // for (n_bones)
      }  // if (cur_node->geom() && ...)
      for (uint32_t i = 0; i < cur_node->children().size(); i++) {
        stack.pushBack(cur_node->children()[i]);
      }
    }
  }

  GeometryInstance* GeometryManager::findGeometryInstanceByName(
    const string& name, GeometryInstance* root) {
    // Descend the scene graph rooted at root DFS and return the first instance
    // to match name.
    Vector<GeometryInstance*> stack;
    stack.pushBack(root);
    while (stack.size() > 0) {
      GeometryInstance* cur_node;
      stack.popBack(cur_node);
      if (cur_node->name() == name) {
        return cur_node;
      }
      for (uint32_t i = 0; i < cur_node->children().size(); i++) {
        stack.pushBack(cur_node->children()[i]);
      }
    }
    return NULL;
  }

  void GeometryManager::addBone(Bone* bone) {
    bone->bone_index = bones_->size();
    bones_->pushBack(bone);
    bone_name_to_index_->insert(bone->bone_name, bone->bone_index);
  }


  Bone* GeometryManager::findBoneByName(const std::string& bone_name) {
    uint32_t index;
    if (bone_name_to_index_->lookup(bone_name, index)) {
      return (*bones_)[index];
    } else {
      return NULL;
    }
  }

  Bone* GeometryManager::findBoneByIndex(const uint32_t index) {
    return (*bones_)[index];
  }

  GeometryInstance* GeometryManager::createDynamicGeometry(
    const std::string& name) {
    Geometry* geom = findGeometryByName(name);
    if (geom != NULL) {
      throw std::wruntime_error("GeometryManager::createDynamicGeometry() - "
        "ERROR: Geometry name already exists!");
    }
    geom = new Geometry(name, true);  // Geometry is dynamic
    GeometryInstance* instance = createNewInstance(geom);
  }
  
}  // namespace renderer
}  // namespace jtil
