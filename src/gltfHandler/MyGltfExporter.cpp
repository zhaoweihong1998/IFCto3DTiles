#include "MyGltfExporter.h"

static inline Ref<Accessor> ExportData(Asset& a, std::string& meshName, Ref<Buffer>& buffer,
    size_t count, void* data, AttribType::Value typeIn, AttribType::Value typeOut, ComponentType compType, bool isIndices = false) {
    if (!count || !data) {
        return Ref<Accessor>();
    }

    unsigned int numCompsIn = AttribType::GetNumComponents(typeIn);
    unsigned int numCompsOut = AttribType::GetNumComponents(typeOut);
    unsigned int bytesPerComp = ComponentTypeSize(compType);

    size_t offset = buffer->byteLength;
    // make sure offset is correctly byte-aligned, as required by spec
    size_t padding = offset % bytesPerComp;
    offset += padding;
    size_t length = count * numCompsOut * bytesPerComp;
    buffer->Grow(length + padding);

    // bufferView
    Ref<BufferView> bv = a.bufferViews.Create(a.FindUniqueID(meshName, "view"));
    bv->buffer = buffer;
    bv->byteOffset = offset;
    bv->byteLength = length; //! The target that the WebGL buffer should be bound to.
    bv->byteStride = 0;
    bv->target = isIndices ? BufferViewTarget_ELEMENT_ARRAY_BUFFER : BufferViewTarget_ARRAY_BUFFER;

    // accessor
    Ref<Accessor> acc = a.accessors.Create(a.FindUniqueID(meshName, "accessor"));
    acc->bufferView = bv;
    acc->byteOffset = 0;
    acc->componentType = compType;
    acc->count = count;
    acc->type = typeOut;

    // calculate min and max values
    {
        // Allocate and initialize with large values.
        float float_MAX = 10000000000000.0f;
        for (unsigned int i = 0; i < numCompsOut; i++) {
            acc->min.push_back(float_MAX);
            acc->max.push_back(-float_MAX);
        }

        // Search and set extreme values.
        float valueTmp;
        for (unsigned int i = 0; i < count; i++) {
            for (unsigned int j = 0; j < numCompsOut; j++) {
                if (numCompsOut == 1) {
                    valueTmp = static_cast<unsigned short*>(data)[i];
                    //cout << valueTmp << ';';
                }
                else {
                    valueTmp = static_cast<aiVector3D*>(data)[i][j];
                }
                if (valueTmp < acc->min[j]) {
                    acc->min[j] = valueTmp;
                }
                if (valueTmp > acc->max[j]) {
                    acc->max[j] = valueTmp;
                }
            }
        }
    }

    // copy the data
    acc->WriteData(count, data, numCompsIn * bytesPerComp);

    return acc;
}
static int track = 0;
void MyGltfExporter::exportMeshes()
{
    
    Ref<Buffer> b = mAsset->GetBodyBuffer();
    if (!b) {
        b = mAsset->buffers.Create(bufferName);
    }
    
    for (unsigned int idx_mesh = 0; idx_mesh < meshes.size(); ++idx_mesh) {
        
        
        const MyMeshInfo mesh = meshes[idx_mesh];
        
        std::string name = mesh.myMesh->name;
        std::string meshId = mAsset->FindUniqueID(name,"mesh");
        
        Ref<Mesh> m = mAsset->meshes.Create(meshId);
        m->primitives.resize(1);
        Mesh::Primitive& p = m->primitives.back();
        m->name = name;
        p.material = mAsset->materials.Get(idx_mesh);
        
        p.batchId = mesh.myMesh->batchId;
        
        float* pos = (float*)malloc(sizeof(float) * mesh.myMesh->vn * 3);
        float* ind_pos = pos;
        /*unsigned int* bacthID = (unsigned int*)malloc(sizeof(unsigned int) * mesh.myMesh->vn);
        unsigned int* index_batchId = bacthID;*/
        float* nor = nullptr;
        float* ind_nor = nullptr;
        
        nor = (float*)malloc(sizeof(float) * mesh.myMesh->vn * 3);
        ind_nor = nor;
        
        unsigned int* index = (unsigned int*)malloc(sizeof(unsigned int) * mesh.myMesh->fn * 3);
        unsigned int* ind_index = index;
        unsigned int ind = 0;

        unsigned int BatchId = mesh.myMesh->batchId;
        m_vertexUintMap.clear();
        for (std::vector<MyVertex>::iterator it = mesh.myMesh->vert.begin(); it != mesh.myMesh->vert.end(); ++it)
        {
            if (it->IsD())
            {
                continue;
            }

            for (int i = 0; i < 3; ++i)
            {
                *ind_pos = (it->P()[i]);
              
                *ind_nor = (it->N()[i]);

                ind_nor++;
                ind_pos++;
            }
            m_vertexUintMap.insert(std::make_pair(&(*it), ind));
            ind++;
            
        }
        
        for (std::vector<MyFace>::iterator it = mesh.myMesh->face.begin(); it != mesh.myMesh->face.end(); ++it)
        {
            if (it->IsD())
            {
                continue;
            }
            for (int i = 0; i < 3; ++i)
            {
                *ind_index = (m_vertexUintMap.at(it->V(i)));
                
                ind_index++;
            }
        }
        
        Ref<Accessor> v = ExportData(*mAsset, meshId, b, mesh.myMesh->vn, pos, AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
        //Ref<Accessor> batch = ExportData(*mAsset, meshId, b, mesh.myMesh->vn, bacthID, AttribType::SCALAR, AttribType::SCALAR, ComponentType_UNSIGNED_INT);
        if (mesh.myMesh[0].vert[0].normalExist) {
            Ref<Accessor> n = ExportData(*mAsset, meshId, b, mesh.myMesh->vn, nor, AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
            if (n) p.attributes.normal.push_back(n);
        }
        if (v) p.attributes.position.push_back(v);
        //if (batch)p.attributes.batch_id.push_back(batch);
        p.indices = ExportData(*mAsset, meshId, b, mesh.myMesh->fn * 3, index, AttribType::SCALAR, AttribType::SCALAR, ComponentType_UNSIGNED_INT, true);
        p.mode = PrimitiveMode_TRIANGLES;
        if (pos != nullptr) {
            free(pos);
        }
        /*if (bacthID != nullptr) {
            free(bacthID);
        }*/
        if (index != nullptr) {
            free(index);
        }
    }
}

inline void SetSamplerWrap(SamplerWrap& wrap, aiTextureMapMode map)
{
    switch (map) {
    case aiTextureMapMode_Clamp:
        wrap = SamplerWrap::Clamp_To_Edge;
        break;
    case aiTextureMapMode_Mirror:
        wrap = SamplerWrap::Mirrored_Repeat;
        break;
    case aiTextureMapMode_Wrap:
    case aiTextureMapMode_Decal:
    default:
        wrap = SamplerWrap::Repeat;
        break;
    };
}

void MyGltfExporter::GetTexSampler(const aiMaterial* mat, Ref<Texture> texture, aiTextureType tt, unsigned int slot)
{
    aiString aId;
    std::string id;
    if (aiGetMaterialString(mat, AI_MATKEY_GLTF_MAPPINGID(tt, slot), &aId) == AI_SUCCESS) {
        id = aId.C_Str();
    }

    if (Ref<Sampler> ref = mAsset->samplers.Get(id.c_str())) {
        texture->sampler = ref;
    }
    else {
        id = mAsset->FindUniqueID(id, "sampler");

        texture->sampler = mAsset->samplers.Create(id.c_str());

        aiTextureMapMode mapU, mapV;
        SamplerMagFilter filterMag;
        SamplerMinFilter filterMin;

        if (aiGetMaterialInteger(mat, AI_MATKEY_MAPPINGMODE_U(tt, slot), (int*)&mapU) == AI_SUCCESS) {
            SetSamplerWrap(texture->sampler->wrapS, mapU);
        }

        if (aiGetMaterialInteger(mat, AI_MATKEY_MAPPINGMODE_V(tt, slot), (int*)&mapV) == AI_SUCCESS) {
            SetSamplerWrap(texture->sampler->wrapT, mapV);
        }

        if (aiGetMaterialInteger(mat, AI_MATKEY_GLTF_MAPPINGFILTER_MAG(tt, slot), (int*)&filterMag) == AI_SUCCESS) {
            texture->sampler->magFilter = filterMag;
        }

        if (aiGetMaterialInteger(mat, AI_MATKEY_GLTF_MAPPINGFILTER_MIN(tt, slot), (int*)&filterMin) == AI_SUCCESS) {
            texture->sampler->minFilter = filterMin;
        }

        aiString name;
        if (aiGetMaterialString(mat, AI_MATKEY_GLTF_MAPPINGNAME(tt, slot), &name) == AI_SUCCESS) {
            texture->sampler->name = name.C_Str();
        }
    }
}

void MyGltfExporter::GetMatTexProp(const aiMaterial* mat, unsigned int& prop, const char* propName, aiTextureType tt, unsigned int slot)
{
    std::string textureKey = std::string(_AI_MATKEY_TEXTURE_BASE) + "." + propName;

    mat->Get(textureKey.c_str(), tt, slot, prop);
}

void MyGltfExporter::GetMatTexProp(const aiMaterial* mat, float& prop, const char* propName, aiTextureType tt, unsigned int slot)
{
    std::string textureKey = std::string(_AI_MATKEY_TEXTURE_BASE) + "." + propName;

    mat->Get(textureKey.c_str(), tt, slot, prop);
}

void MyGltfExporter::GetMatTex(const aiMaterial* mat, Ref<Texture>& texture, aiTextureType tt, unsigned int slot = 0)
{

    if (mat->GetTextureCount(tt) > 0) {
        aiString tex;

        if (mat->Get(AI_MATKEY_TEXTURE(tt, slot), tex) == AI_SUCCESS) {
            std::string path = tex.C_Str();

            if (path.size() > 0) {
                std::map<std::string, unsigned int>::iterator it = mTexturesByPath.find(path);
                if (it != mTexturesByPath.end()) {
                    texture = mAsset->textures.Get(it->second);
                }

                if (!texture) {
                    std::string texId = mAsset->FindUniqueID("", "texture");
                    texture = mAsset->textures.Create(texId);
                    mTexturesByPath[path] = texture.GetIndex();

                    std::string imgId = mAsset->FindUniqueID("", "image");
                    texture->source = mAsset->images.Create(imgId);

                    if (path[0] == '*') { // embedded
                        aiTexture* tex = mScene->mTextures[atoi(&path[1])];

                        uint8_t* data = reinterpret_cast<uint8_t*>(tex->pcData);
                        texture->source->SetData(data, tex->mWidth, *mAsset);

                        if (tex->achFormatHint[0]) {
                            std::string mimeType = "image/";
                            mimeType += (memcmp(tex->achFormatHint, "jpg", 3) == 0) ? "jpeg" : tex->achFormatHint;
                            texture->source->mimeType = mimeType;
                        }
                    }
                    else {
                        texture->source->uri = path;
                    }

                    GetTexSampler(mat, texture, tt, slot);
                }
            }
        }
    }
}

void MyGltfExporter::GetMatTex(const aiMaterial* mat, TextureInfo& prop, aiTextureType tt, unsigned int slot = 0)
{
    Ref<Texture>& texture = prop.texture;

    GetMatTex(mat, texture, tt, slot);

    if (texture) {
        GetMatTexProp(mat, prop.texCoord, "texCoord", tt, slot);
    }
}

void MyGltfExporter::GetMatTex(const aiMaterial* mat, NormalTextureInfo& prop, aiTextureType tt, unsigned int slot = 0)
{
    Ref<Texture>& texture = prop.texture;

    GetMatTex(mat, texture, tt, slot);

    if (texture) {
        GetMatTexProp(mat, prop.texCoord, "texCoord", tt, slot);
        GetMatTexProp(mat, prop.scale, "scale", tt, slot);
    }
}

void MyGltfExporter::GetMatTex(const aiMaterial* mat, OcclusionTextureInfo& prop, aiTextureType tt, unsigned int slot = 0)
{
    Ref<Texture>& texture = prop.texture;

    GetMatTex(mat, texture, tt, slot);

    if (texture) {
        GetMatTexProp(mat, prop.texCoord, "texCoord", tt, slot);
        GetMatTexProp(mat, prop.strength, "strength", tt, slot);
    }
}

aiReturn MyGltfExporter::GetMatColor(const aiMaterial* mat, vec4& prop, const char* propName, int type, int idx)
{
    aiColor4D col;
    aiReturn result = mat->Get(propName, type, idx, col);

    if (result == AI_SUCCESS) {
        prop[0] = col.r; prop[1] = col.g; prop[2] = col.b; prop[3] = col.a;
    }

    return result;
}

aiReturn MyGltfExporter::GetMatColor(const aiMaterial* mat, vec3& prop, const char* propName, int type, int idx)
{
    aiColor3D col;
    aiReturn result = mat->Get(propName, type, idx, col);

    if (result == AI_SUCCESS) {
        prop[0] = col.r; prop[1] = col.g; prop[2] = col.b;
    }

    return result;
}

void MyGltfExporter::exportMaterial()
{
    aiString aiName;
    int i = 0;
    for (int i = 0; i < meshes.size(); ++i) {
        const aiMaterial* mat = mScene->mMaterials[meshes[i].material];

        std::string id = "material_" + to_string(i);

        Ref<Material> m = mAsset->materials.Create(id);

        std::string name;
        if (mat->Get(AI_MATKEY_NAME, aiName) == AI_SUCCESS) {
            name = aiName.C_Str();
        }
        name = mAsset->FindUniqueID(name, "material");

        m->name = name;

        GetMatTex(mat, m->pbrMetallicRoughness.baseColorTexture, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE);

        if (!m->pbrMetallicRoughness.baseColorTexture.texture) {
            //if there wasn't a baseColorTexture defined in the source, fallback to any diffuse texture
            GetMatTex(mat, m->pbrMetallicRoughness.baseColorTexture, aiTextureType_DIFFUSE);
        }

        GetMatTex(mat, m->pbrMetallicRoughness.metallicRoughnessTexture, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE);

        if (GetMatColor(mat, m->pbrMetallicRoughness.baseColorFactor, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR) != AI_SUCCESS) {
            // if baseColorFactor wasn't defined, then the source is likely not a metallic roughness material.
            //a fallback to any diffuse color should be used instead
            GetMatColor(mat, m->pbrMetallicRoughness.baseColorFactor, AI_MATKEY_COLOR_DIFFUSE);
        }

        if (mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, m->pbrMetallicRoughness.metallicFactor) != AI_SUCCESS) {
            //if metallicFactor wasn't defined, then the source is likely not a PBR file, and the metallicFactor should be 0
            m->pbrMetallicRoughness.metallicFactor = 0;
        }

        // get roughness if source is gltf2 file
        if (mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, m->pbrMetallicRoughness.roughnessFactor) != AI_SUCCESS) {
            // otherwise, try to derive and convert from specular + shininess values
            aiColor4D specularColor;
            ai_real shininess;

            if (
                mat->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) == AI_SUCCESS &&
                mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS
                ) {
                // convert specular color to luminance
                float specularIntensity = specularColor[0] * 0.2125f + specularColor[1] * 0.7154f + specularColor[2] * 0.0721f;
                //normalize shininess (assuming max is 1000) with an inverse exponentional curve
                float normalizedShininess = std::sqrt(shininess / 1000);

                //clamp the shininess value between 0 and 1
                normalizedShininess = std::min(std::max(normalizedShininess, 0.0f), 1.0f);
                // low specular intensity values should produce a rough material even if shininess is high.
                normalizedShininess = normalizedShininess * specularIntensity;

                m->pbrMetallicRoughness.roughnessFactor = 1 - normalizedShininess;
            }
        }

        GetMatTex(mat, m->normalTexture, aiTextureType_NORMALS);
        GetMatTex(mat, m->occlusionTexture, aiTextureType_LIGHTMAP);
        GetMatTex(mat, m->emissiveTexture, aiTextureType_EMISSIVE);
        GetMatColor(mat, m->emissiveFactor, AI_MATKEY_COLOR_EMISSIVE);

        mat->Get(AI_MATKEY_TWOSIDED, m->doubleSided);
        mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, m->alphaCutoff);

        aiString alphaMode;

        if (mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
            m->alphaMode = alphaMode.C_Str();
        }
        else {
            float opacity;

            if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
                if (opacity < 1) {
                    m->alphaMode = "BLEND";
                    m->pbrMetallicRoughness.baseColorFactor[3] *= opacity;
                }
            }
        }

        bool hasPbrSpecularGlossiness = false;
        mat->Get(AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS, hasPbrSpecularGlossiness);

        if (hasPbrSpecularGlossiness) {

            if (!mAsset->extensionsUsed.KHR_materials_pbrSpecularGlossiness) {
                mAsset->extensionsUsed.KHR_materials_pbrSpecularGlossiness = true;
            }

            PbrSpecularGlossiness pbrSG;

            GetMatColor(mat, pbrSG.diffuseFactor, AI_MATKEY_COLOR_DIFFUSE);
            GetMatColor(mat, pbrSG.specularFactor, AI_MATKEY_COLOR_SPECULAR);

            if (mat->Get(AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS_GLOSSINESS_FACTOR, pbrSG.glossinessFactor) != AI_SUCCESS) {
                float shininess;

                if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                    pbrSG.glossinessFactor = shininess / 1000;
                }
            }

            GetMatTex(mat, pbrSG.diffuseTexture, aiTextureType_DIFFUSE);
            GetMatTex(mat, pbrSG.specularGlossinessTexture, aiTextureType_SPECULAR);

            m->pbrSpecularGlossiness = Nullable<PbrSpecularGlossiness>(pbrSG);
        }

        bool unlit;
        if (mat->Get(AI_MATKEY_GLTF_UNLIT, unlit) == AI_SUCCESS && unlit) {
            mAsset->extensionsUsed.KHR_materials_unlit = true;
            m->unlit = true;
        }
    }
    
    
}

MyGltfExporter::MyGltfExporter(vector<MyMeshInfo> meshes, char* buffername, const aiScene* mScene,bool setBinary, IOSystem* io)
{
	this->meshes = meshes;
	this->bufferName = buffername;
	this->setBinary = setBinary;
	this->mScene = mScene;
	mAsset = new Asset(io);
    if (setBinary) {
        mAsset->SetAsBinary();
    }
}

void MyGltfExporter::constructAsset()
{
    exportMaterial();
    exportMeshes();
    Ref<Scene> scene = mAsset->scenes.Create("scene");
    Ref<Node> root = mAsset->nodes.Create("root");
    root->name = "root";
    unordered_map<int, vector<int>> node_children;
    for (int i = 0; i < mAsset->meshes.Size(); ++i) {
        auto cur_mesh = mAsset->meshes.Get(i);
        auto node_id = cur_mesh->primitives[0].batchId;
        auto it = node_children.find(node_id);
        if (it != node_children.end()) {
            it->second.push_back(cur_mesh->index);
        }
        else {
            node_children[node_id] = {};
            node_children[node_id].push_back(cur_mesh->index);
        }
    }

    for (auto v : node_children) {
        string id = "node_" + to_string(v.first);
        Ref<Node> node = mAsset->nodes.Create(id);
        node->name = id;
        for (auto i : v.second) {
            node->meshes.push_back(mAsset->meshes.Get(i));
        }
        root->children.push_back(node);
    }

    for (unsigned int n = 0; n < mAsset->nodes.Size(); ++n) {
        Ref<Node> node = mAsset->nodes.Get(n);

        unsigned int nMeshes = static_cast<unsigned int>(node->meshes.size());

        //skip if it's 1 or less meshes per node
        if (nMeshes > 0) {
            Ref<Mesh> firstMesh = node->meshes.at(0);
            node->name = firstMesh->name;
            //loop backwards to allow easy removal of a mesh from a node once it's merged
            for (unsigned int m = nMeshes - 1; m >= 1; --m) {
                Ref<Mesh> mesh = node->meshes.at(m);

                //append this mesh's primitives to the first mesh's primitives
                firstMesh->primitives.insert(
                    firstMesh->primitives.end(),
                    mesh->primitives.begin(),
                    mesh->primitives.end()
                );

                //remove the mesh from the list of meshes
                unsigned int removedIndex = mAsset->meshes.Remove(mesh->id.c_str());

                //find the presence of the removed mesh in other nodes
                for (unsigned int nn = 0; nn < mAsset->nodes.Size(); ++nn) {
                    Ref<Node> node = mAsset->nodes.Get(nn);

                    for (unsigned int mm = 0; mm < node->meshes.size(); ++mm) {
                        Ref<Mesh>& meshRef = node->meshes.at(mm);
                        unsigned int meshIndex = meshRef.GetIndex();

                        if (meshIndex == removedIndex) {
                            node->meshes.erase(node->meshes.begin() + mm);
                        }
                        else if (meshIndex > removedIndex) {
                            Ref<Mesh> newMeshRef = mAsset->meshes.Get(meshIndex - 1);

                            meshRef = newMeshRef;
                        }
                    }
                }
            }

            //since we were looping backwards, reverse the order of merged primitives to their original order
            std::reverse(firstMesh->primitives.begin() + 1, firstMesh->primitives.end());
        }
    }
    //end

    scene->nodes.push_back(root);
    mAsset->asset.version = "2.0";
    mAsset->scene = scene;
}


void MyGltfExporter::write()
{
    string filename = bufferName;

    //string dir = prefix + "\\output\\" + filename;
    string dir = "output\\"+filename;
    /*cout << dir << endl;*/
    //string dir = bufferName;
	AssetWriter writer(*mAsset);
	if (setBinary) {
        dir += ".b3dm";
		writer.WriteGLBFile(dir.c_str());
	}
	else {
        dir += ".gltf";
		writer.WriteFile(dir.c_str());
	}
}
