#include "include\3DEngine.h"

#include <assimp\Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class TestGame : public Game
{
public:
	TestGame() {}

	virtual void Init(const Window& window);
protected:
private:
	TestGame(const TestGame& other) {}
	void operator=(const TestGame& other) {}
};

Entity* traverse(aiNode* node, aiMesh** meshes, aiMaterial** materials)
{
	Entity* e = new Entity();
	
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = meshes[node->mMeshes[i]];

		std::vector<Vector3f> positions;
		std::vector<Vector2f> texCoords;
		std::vector<Vector3f> normals;
		std::vector<Vector3f> tangents;
		std::vector<unsigned int> indices;

		const aiVector3D aiZeroVector(0.0f, 0.0f, 0.0f);
		for (unsigned int j = 0; j < mesh->mNumVertices; j++)
		{
			const aiVector3D pos = node->mTransformation * mesh->mVertices[j];
			const aiVector3D normal = (node->mTransformation * mesh->mNormals[j]).Normalize();
			const aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][j] : aiZeroVector;
			const aiVector3D tangent = mesh->HasTangentsAndBitangents() ? mesh->mTangents[j] : aiZeroVector;

			positions.push_back(Vector3f(pos.x, pos.y, pos.z));
			texCoords.push_back(Vector2f(texCoord.x, texCoord.y));
			normals.push_back(Vector3f(normal.x, normal.y, normal.z));
			tangents.push_back(Vector3f(tangent.x, tangent.y, tangent.z));
		}

		for (unsigned int j = 0; j < mesh->mNumFaces; j++)
		{
			const aiFace& face = mesh->mFaces[j];
			assert(face.mNumIndices == 3);
			indices.push_back(face.mIndices[0]);
			indices.push_back(face.mIndices[2]);
			indices.push_back(face.mIndices[1]);
		}

		aiMaterial* mat = materials[mesh->mMaterialIndex];

		aiString matName;
		mat->Get(AI_MATKEY_NAME, matName);

		e->AddComponent(new MeshRenderer(Mesh(mesh->mName.C_Str(), IndexedModel(indices, positions, texCoords, normals, tangents).Finalize()), Material(matName.C_Str())));
	}

	for (int i = 0; i < node->mNumChildren; i++)
	{
		if (node->mChildren[i]->mNumMeshes > 0)
		{
			Entity* eChild = traverse(node->mChildren[i], meshes, materials);
			e->AddChild(eChild);
		}
	}

	return e;
}

void TestGame::Init(const Window& window)
{
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile("./res/scenes/scene.fbx",
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace);

	if (!scene)
	{
		assert(0 == 0);
	}

	if (scene->HasCameras())
	{
		for(int i = 0; i < scene->mNumCameras; i++)
		{
			//aiNode* cameraNode = scene->mRootNode->FindNode(scene->mCameras[i]->mName);

			aiVector3D camPos = scene->mCameras[i]->mPosition;
			aiVector3D camUp = scene->mCameras[i]->mUp.Normalize();
			aiVector3D camLeft = scene->mCameras[i]->mLookAt.Normalize(); // this is actually left???
			Vector3f up = Vector3f(camUp.x, camUp.y, camUp.z);
			Vector3f forward = Vector3f(camLeft.x, camLeft.y, camLeft.z).Cross(up);

			Matrix4f matrix = Matrix4f().InitRotationFromDirection(forward, up);

			AddToScene((new Entity(Vector3f(camPos.x, camPos.y, camPos.z), Quaternion(matrix)))
						->AddComponent(new CameraComponent(Matrix4f().InitPerspective(
										ToRadians(70.0f), window.GetAspect(), 0.1f, 1000.0f)))
						->AddComponent(new FreeLook(window.GetCenter()))
						->AddComponent(new FreeMove(10.0f)));
		}
	}
	if (scene->HasLights())
	{
		for (int i = 0; i < scene->mNumLights; i++)
		{
			aiLight* light = scene->mLights[i];
			aiNode* lightNode = scene->mRootNode->FindNode(light->mName);

			switch (light->mType)
			{
				case aiLightSource_POINT:
				{
					aiVector3D lightPos = lightNode->mTransformation * light->mPosition;

					AddToScene((new Entity(Vector3f(lightPos.x, lightPos.y, lightPos.z)))
						->AddComponent(new PointLight(Vector3f(1, 1, 1), 1.0f, Attenuation(0, 0, 1))));

					break;
				}
				case aiLightSource_DIRECTIONAL:
				{
					aiVector3D lightDir = -(lightNode->mTransformation * light->mDirection).Normalize();
					Vector3f lightUp = Vector3f(lightDir.x, lightDir.y, lightDir.z).Cross(Vector3f(0, 1, 0));

					Matrix4f lightRot = Matrix4f().InitRotationFromDirection(Vector3f(lightDir.x, lightDir.y, lightDir.z), lightUp);

					AddToScene((new Entity(Vector3f(), Quaternion(lightRot)))
						->AddComponent(new DirectionalLight(Vector3f(1, 1, 1), 0.4f, 10, 80.0f, 1.0f)));

					break;
				}
			}
		}
	}

	std::vector<Material*> materials;
	if (scene->HasMaterials())
	{
		for (int i = 0; i < scene->mNumMaterials; i++)
		{
			aiMaterial* mat = scene->mMaterials[i];

			aiString name;
			mat->Get(AI_MATKEY_NAME, name);

			aiColor3D diffuseColor(0.f, 0.f, 0.f);
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);

			aiColor3D specularColor(0.f, 0.f, 0.f);
			mat->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);

			float shininess = -1.0f;
			mat->Get(AI_MATKEY_SHININESS, shininess);

			aiString textureName;
			mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), textureName);

			if (textureName.length == 0)
			{
				textureName = "default_diffuse.jpg";
			}
			else
			{
				textureName = Util::Split(textureName.C_Str(), '\\').back();
			}

			materials.push_back(new Material(name.C_Str(), Texture(textureName.C_Str()), 1.0f, 0.2f));
		}
	}

	aiNode* root = scene->mRootNode;
	for (int i = 0; i < root->mNumChildren; i++)
	{
		if (root->mChildren[i]->mNumMeshes > 0)
		{
			Entity* e = traverse(root->mChildren[i], scene->mMeshes, scene->mMaterials);
			AddToScene(e);
		}
	}

	for (int i = 0; i < materials.size(); i++)
		delete materials[i];
}

int main()
{
	TestGame game;
	Window window(1280, 720, "3D Game Engine");
	RenderingEngine renderer(window);

	//window.SetFullScreen(true);

	CoreEngine engine(500, &window, &renderer, &game);
	engine.Start();

	return 0;
}