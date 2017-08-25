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
			const aiVector3D normal = mesh->mNormals[j];
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
			if (light->mType == aiLightSource_POINT)
			{
				aiVector3D lightPos = lightNode->mTransformation * light->mPosition;

				AddToScene((new Entity(Vector3f(lightPos.x, lightPos.y, lightPos.z)))
					->AddComponent(new PointLight(Vector3f(1, 1, 1), 1.0f, Attenuation(0, 0, 1))));
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

	/*if (scene->HasMeshes())
	{
		for (int i = 0; i < scene->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[i];
			const char* meshName = mesh->mName.C_Str();
			aiNode* meshNode = scene->mRootNode->FindNode(meshName);

			std::vector<Vector3f> positions;
			std::vector<Vector2f> texCoords;
			std::vector<Vector3f> normals;
			std::vector<Vector3f> tangents;
			std::vector<unsigned int> indices;

			const aiVector3D aiZeroVector(0.0f, 0.0f, 0.0f);
			for (unsigned int j = 0; j < mesh->mNumVertices; j++)
			{
				const aiVector3D pos = meshNode->mTransformation * mesh->mVertices[j];
				const aiVector3D normal = mesh->mNormals[j];
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

			aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

			aiString matName;
			mat->Get(AI_MATKEY_NAME, matName);

			AddToScene((new Entity())
						->AddComponent(new MeshRenderer(Mesh(mesh->mName.C_Str(), IndexedModel(indices, positions, texCoords, normals, tangents).Finalize()), Material(matName.C_Str()))));
		}
	}*/
	for (int i = 0; i < materials.size(); i++)
		delete materials[i];

	/*AddToScene((new Entity())
	->AddComponent(new CameraComponent(Matrix4f().InitPerspective(
	ToRadians(70.0f), window.GetAspect(), 0.1f, 1000.0f)))
	->AddComponent(new FreeLook(window.GetCenter()))
	->AddComponent(new FreeMove(10.0f)));*/


	//Material bricks("bricks", Texture("bricks.jpg"), 0.1f, 0,
	//	Texture("bricks_normal.jpg"), Texture("bricks_disp.png"), 0.03f, -0.5f);
	/*Material bricks2("bricks2", Texture("bricks2.jpg"), 0.0f, 0,
		Texture("bricks2_normal.png"), Texture("bricks2_disp.jpg"), 0.04f, -1.0f);*/

	//IndexedModel square;
	//{
	//square.AddVertex(1.0f, -1.0f, 0.0f);  square.AddTexCoord(Vector2f(1.0f, 1.0f));
	//square.AddVertex(1.0f, 1.0f, 0.0f);   square.AddTexCoord(Vector2f(1.0f, 0.0f));
	//square.AddVertex(-1.0f, -1.0f, 0.0f); square.AddTexCoord(Vector2f(0.0f, 1.0f));
	//square.AddVertex(-1.0f, 1.0f, 0.0f);  square.AddTexCoord(Vector2f(0.0f, 0.0f));
	//square.AddFace(0, 1, 2); square.AddFace(2, 1, 3);
	//}
	//Mesh customMesh("square", square.Finalize());

	/*AddToScene((new Entity(Vector3f(0, -50, 5), Quaternion(), 32.0f))
		->AddComponent(new MeshRenderer(Mesh("terrain02.obj"), Material("bricks2"))));*/
	//		
	///*AddToScene((new Entity(Vector3f(7,0,7)))
	//	->AddComponent(new PointLight(Vector3f(0,1,0), 0.4f, Attenuation(0,0,1))));*/
	//	
	/*AddToScene(new Entity(Vector3f(15,-5.0f,5), Quaternion(Vector3f(1,0,0), ToRadians(-60.0f)) * Quaternion(Vector3f(0,1,0), ToRadians(90.0f)))
		->AddComponent(new SpotLight(Vector3f(0,1,1), 1.0f, Attenuation(0,0,0.02f), ToRadians(91.1f), 7, 1.0f, 0.5f)));*/
	//	
	/*AddToScene((new Entity(Vector3f(), Quaternion(Vector3f(1,0,0), ToRadians(-45))))
		->AddComponent(new DirectionalLight(Vector3f(1,1,1), 0.4f, 10, 80.0f, 1.0f)));*/
	//	
	///*AddToScene((new Entity(Vector3f(0, 2, 0), Quaternion(Vector3f(0,1,0), 0.4f), 1.0f))
	//	->AddComponent(new MeshRenderer(Mesh("plane3.obj"), Material("bricks2")))
	//	->AddChild((new Entity(Vector3f(0, 0, 25)))
	//		->AddComponent(new MeshRenderer(Mesh("plane3.obj"), Material("bricks2")))
	//		->AddChild((new Entity())
	//			->AddComponent(new CameraComponent(Matrix4f().InitPerspective(ToRadians(70.0f), window.GetAspect(), 0.1f, 1000.0f)))
	//			->AddComponent(new FreeLook(window.GetCenter()))
	//			->AddComponent(new FreeMove(10.0f)))));*/
	//	
	//AddToScene((new Entity(Vector3f(24,-12,5), Quaternion(Vector3f(0,1,0), ToRadians(30.0f))))
	//	->AddComponent(new MeshRenderer(Mesh("sphere.obj"), Material("bricks"))));
	//		
	///*AddToScene((new Entity(Vector3f(0,0,7), Quaternion(), 1.0f))
	//	->AddComponent(new MeshRenderer(Mesh("square"), Material("bricks2"))));*/
	////
	////
	////	//TODO: Temporary Physics Engine Code!
	////	PhysicsEngine physicsEngine;
	////	
	////	physicsEngine.AddObject(PhysicsObject(
	////			new BoundingSphere(Vector3f(0.0f, 0.0f, 0.0f), 1.0f),
	////		   	Vector3f(0.0f, 0.0f, 1.141f/2.0f)));
	////
	////	physicsEngine.AddObject(PhysicsObject(
	////			new BoundingSphere(Vector3f(1.414f/2.0f * 7.0f, 0.0f, 1.414f/2.0f * 7.0f), 1.0f),
	////			Vector3f(-1.414f/2.0f, 0.0f, -1.414f/2.0f))); 
	////
	////
	////	PhysicsEngineComponent* physicsEngineComponent 
	////		= new PhysicsEngineComponent(physicsEngine);
	////
	////	for(unsigned int i = 0; 
	////		i < physicsEngineComponent->GetPhysicsEngine().GetNumObjects(); 
	////		i++)
	////	{
	////		
	////		AddToScene((new Entity(Vector3f(0,0,0), Quaternion(), 
	////					1.0f))
	////			->AddComponent(new PhysicsObjectComponent(
	////					&physicsEngineComponent->GetPhysicsEngine().GetObject(i)))
	////			->AddComponent(new MeshRenderer(Mesh("sphere.obj"), Material("bricks"))));
	////	}
	////
	////	AddToScene((new Entity())
	////		->AddComponent(physicsEngineComponent));

	///*AddToScene((new Entity(Vector3f(), Quaternion(Vector3f(1,1,1), ToRadians(-45))))
	//	->AddComponent(new DirectionalLight(Vector3f(1,1,1), 0.4f, 10, 80.0f, 1.0f)));*/


	//static const int CUBE_SIZE = 3;

	/*AddToScene((new Entity())
		->AddComponent(new PointLight(Vector3f(1, 1, 1),
		(CUBE_SIZE * CUBE_SIZE) * 2, Attenuation(0, 0, 1))));*/

	/*for (int i = -CUBE_SIZE; i <= CUBE_SIZE; i++)
	{
	for (int j = -CUBE_SIZE; j <= CUBE_SIZE; j++)
	{
	for (int k = -CUBE_SIZE; k <= CUBE_SIZE; k++)
	{
	if (i == -CUBE_SIZE || i == CUBE_SIZE ||
	j == -CUBE_SIZE || j == CUBE_SIZE ||
	k == -CUBE_SIZE || k == CUBE_SIZE)
	{
	if (i == 0 || j == 0 || k == 0)
	{
	AddToScene((new Entity(Vector3f(i * 2, j * 2, k * 2)))
	->AddComponent(new MeshRenderer(Mesh("sphere.obj"),
	Material("bricks"))));
	}
	else
	{
	AddToScene((new Entity(Vector3f(i * 2, j * 2, k * 2)))
	->AddComponent(new MeshRenderer(Mesh("cube.obj"),
	Material("bricks2"))));
	}

	}
	}
	}
	}*/


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