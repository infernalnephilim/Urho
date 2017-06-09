#include <iostream>
#include <string>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/UIEvents.h>

#include <Urho3D/DebugNew.h>

#include "Character.h"
#include "MainScene.h"
#include "Touch.h"

URHO3D_DEFINE_APPLICATION_MAIN(MainScene)

MainScene::MainScene(Context* context) :
	App(context), time_(0)
{
	// Register factory and attributes for the Character component so it can be created via CreateComponent, and loaded / saved
	Character::RegisterObject(context);
}

MainScene::~MainScene()
{
}

void MainScene::Start()
{
	App::Start();

	if (touchEnabled_)
		touch_ = new Touch(context_, TOUCH_SENSITIVITY);

	CreateScene();
	
	CreateCharacter();
	
	SubscribeToEvents();
	
	App::InitMouseMode(MM_RELATIVE);

	UpdateText();

}
void MainScene::UpdateText()
{	
	// CZAS
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	GetSubsystem<UI>()->GetRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
	// Let's create some text to display.
	text_ = new Text(context_);
	// Text will be updated later in the E_UPDATE handler. Keep readin'.
	text_->SetText("Score ");
	// If the engine cannot find the font, it comes with Urho3D.
	// Set the environment variables URHO3D_HOME, URHO3D_PREFIX_PATH or
	// change the engine parameter "ResourcePrefixPath" in the Setup method.
	text_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 20);
	text_->SetColor(Color(0, 0, 0));
	text_->SetHorizontalAlignment(HA_LEFT);
	text_->SetVerticalAlignment(VA_TOP);
	GetSubsystem<UI>()->GetRoot()->AddChild(text_);
	
	// POZYCJA BOHATERA
	
	text2_ = new Text(context_);
	// Text will be updated later in the E_UPDATE handler. Keep readin'.
	text2_->SetText("POS ");
	// If the engine cannot find the font, it comes with Urho3D.
	// Set the environment variables URHO3D_HOME, URHO3D_PREFIX_PATH or
	// change the engine parameter "ResourcePrefixPath" in the Setup method.
	text2_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 20);
	text2_->SetColor(Color(1, 0, 0));
	text2_->SetHorizontalAlignment(HA_RIGHT);
	text2_->SetVerticalAlignment(VA_BOTTOM);
	GetSubsystem<UI>()->GetRoot()->AddChild(text2_);
}
void MainScene::CreateScene()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	scene_ = new Scene(context_);

	// Create scene subsystem components
	scene_->CreateComponent<Octree>();
	scene_->CreateComponent<PhysicsWorld>();
	scene_->CreateComponent<DebugRenderer>();

	// Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside the scene,
	// so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
	cameraNode_ = new Node(context_);
	Camera* camera = cameraNode_->CreateComponent<Camera>();
	camera->SetFarClip(300.0f);
	GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

	// Create static scene content. First create a zone for ambient lighting and fog control
	Node* zoneNode = scene_->CreateChild("Zone");
	Zone* zone = zoneNode->CreateComponent<Zone>();
	zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
	zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
	zone->SetFogStart(100.0f);
	zone->SetFogEnd(300.0f);
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

	// Create a directional light with cascaded shadow mapping
	Node* lightNode = scene_->CreateChild("DirectionalLight");
	lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
	Light* light = lightNode->CreateComponent<Light>();
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
	light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
	light->SetSpecularIntensity(0.5f);

	// SKY
	Node* skyNode = scene_->CreateChild("Sky");
	skyNode->SetScale(500.0f); // The scale actually does not matter
	Skybox* skybox = skyNode->CreateComponent<Skybox>();
	skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

	// Create the floor object
	Node* floorNode = scene_->CreateChild("Floor");
	floorNode->SetPosition(Vector3(0.0f, -0.5f, 100.0f));
	floorNode->SetScale(Vector3(100.0f, 1.0f, 200.0f));
	StaticModel* object = floorNode->CreateComponent<StaticModel>();
	object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	object->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));

	RigidBody* body = floorNode->CreateComponent<RigidBody>();
	// Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
	// inside geometry
	body->SetCollisionLayer(2);
	CollisionShape* shape = floorNode->CreateComponent<CollisionShape>();
	shape->SetBox(Vector3::ONE);

	Node* leftWallNode = scene_->CreateChild("LeftWall");
	leftWallNode->SetPosition(Vector3(-4.5f, 2.0f, 100.0f));
	leftWallNode->SetScale(Vector3(1.0f, 4.0f, 200.0f));
	StaticModel* object2 = leftWallNode->CreateComponent<StaticModel>();
	object2->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	object2->SetMaterial(cache->GetResource<Material>("Materials/GreenTransparent.xml"));

	RigidBody* body2 = leftWallNode->CreateComponent<RigidBody>();
	body2->SetCollisionLayer(2);
	CollisionShape* shape2 = leftWallNode->CreateComponent<CollisionShape>();
	shape2->SetBox(Vector3::ONE);

	Node* rightWallNode = scene_->CreateChild("RightWall");
	rightWallNode->SetPosition(Vector3(4.5f, 2.0f, 100.0f));
	rightWallNode->SetScale(Vector3(1.0f, 4.0f, 200.0f));
	StaticModel* object3 = rightWallNode->CreateComponent<StaticModel>();
	object3->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	object3->SetMaterial(cache->GetResource<Material>("Materials/GreenTransparent.xml"));

	RigidBody* body3 = rightWallNode->CreateComponent<RigidBody>();
	body3->SetCollisionLayer(2);
	CollisionShape* shape3 = rightWallNode->CreateComponent<CollisionShape>();
	shape3->SetBox(Vector3::ONE);



	const unsigned NUM_BOXES = 30;
	for (unsigned i = 0; i < NUM_BOXES; ++i)
	{
		std::cout <<int(Random(3.0f));
		Node* objectNode = scene_->CreateChild("Box");
		objectNode->SetPosition(Vector3((int(Random(3.0f) - 1.0f))*3.0f - 3.0f, 0.75f, (Random(90.0f) + 5.0f)*4));
		objectNode->SetRotation(Quaternion(0.0f, 0.0f,0.0f));
		objectNode->SetScale(1.5f);
		StaticModel* object5 = objectNode->CreateComponent<StaticModel>();
		object5->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		object5->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
		object5->SetCastShadows(true);

		RigidBody* body5 = objectNode->CreateComponent<RigidBody>();
		body5->SetCollisionLayer(2);
		// Bigger boxes will be heavier and harder to move
		//body5->SetMass(20.0f);
		CollisionShape* shape5 = objectNode->CreateComponent<CollisionShape>();
		shape5->SetBox(Vector3::ONE);
	}

	const unsigned NUM_CARROTS = 30;
	for (unsigned i = 0; i < NUM_CARROTS; ++i)
	{
		std::cout << int(Random(3.0f));
		Node* carrotNode = scene_->CreateChild("Carrot");
		carrotNode->SetPosition(Vector3((int(Random(3.0f) - 1.0f))*3.0f - 3.0f, 2.0f, (Random(90.0f) + 5.0f) * 4));
		carrotNode->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
		carrotNode->SetScale(1.5f);
		StaticModel* carrot = carrotNode->CreateComponent<StaticModel>();
		carrot->SetModel(cache->GetResource<Model>("Models/TeaPot.mdl"));
		carrot->SetMaterial(cache->GetResource<Material>("Models/marchew/material.xml"));
		carrot->SetCastShadows(true);

		RigidBody* carrotBody = carrotNode->CreateComponent<RigidBody>();
		carrotBody->SetCollisionLayer(2);
		// Bigger boxes will be heavier and harder to move
		//body5->SetMass(20.0f);
		CollisionShape* carrotShape = carrotNode->CreateComponent<CollisionShape>();
		carrotShape->SetBox(Vector3::ONE);
	}
	/*
	RigidBody* ch = character_->GetComponent<RigidBody>();

	if (ch->GetPosition().y_ > 10.0f)
	{
		Node* objectNode2 = scene_->CreateChild("Box");
		objectNode2->SetPosition(Vector3((int(Random(3.0f) - 1.0f))*3.0f - 3.0f, 0.75f, (Random(90.0f) + 5.0f) * 4));
		objectNode2->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
		objectNode2->SetScale(2.5f);
		StaticModel* object52 = objectNode2->CreateComponent<StaticModel>();
		object52->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		object52->SetMaterial(cache->GetResource<Material>("Materials/Water.xml"));
		object52->SetCastShadows(true);
	}
	*/
}

void MainScene::CreateCharacter() {
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	Node* objectNode = scene_->CreateChild("Jack");
	objectNode->SetPosition(Vector3(0.0f, 1.1f, 0.0f));
	//objectNode->SetScale(Vector3(1.0f, 1.0f, 1.0f));
	//objectNode->SetRotation(Quaternion(90, Vector3(0, 1, 0)));

	// Create the rendering component + animation controller
	AnimatedModel* object = objectNode->CreateComponent<AnimatedModel>();
	object->SetModel(cache->GetResource<Model>("Models/Mutant/Mutant.mdl"));
	object->SetMaterial(cache->GetResource<Material>("Models/Mutant/Materials/mutant_M.xml"));
	object->SetCastShadows(true);
	objectNode->CreateComponent<AnimationController>();

	// Set the head bone for manual control
	object->GetSkeleton().GetBone("Mutant:Head")->animated_ = false;

	// Create rigidbody, and set non-zero mass so that the body becomes dynamic
	RigidBody* body = objectNode->CreateComponent<RigidBody>();
	body->SetCollisionLayer(1);
	body->SetMass(1.0f);

	// Set zero angular factor so that physics doesn't turn the character on its own.
	// Instead we will control the character yaw manually
	body->SetAngularFactor(Vector3::ZERO);

	// Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
	body->SetCollisionEventMode(COLLISION_ALWAYS);

	// Set a capsule shape for collision
	CollisionShape* shape = objectNode->CreateComponent<CollisionShape>();
	shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f,0.9f, 0.0f));

	// Create the character logic component, which takes care of steering the rigidbody
	// Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
	// and keeps it alive as long as it's not removed from the hierarchy
	character_ = objectNode->CreateComponent<Character>();
	//////////////////
}


void MainScene::SubscribeToEvents()
{
	// Subscribe to Update event for setting the character controls before physics simulation
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(MainScene, HandleUpdate));

	// Subscribe to PostUpdate event for updating the camera position after physics simulation
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(MainScene, HandlePostUpdate));
	SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(MainScene, HandlePostRenderUpdate));
	// Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
	UnsubscribeFromEvent(E_SCENEUPDATE);
}

void MainScene::HandleUpdate(StringHash eventType, VariantMap& eventData)
{

	using namespace Update;

	Input* input = GetSubsystem<Input>();

	if (character_)
	{
		// Clear previous controls
		character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

		// Update controls using touch utility class
		if (touch_)
			touch_->UpdateTouches(character_->controls_);
		
		
	

		// Update controls using keys
		UI* ui = GetSubsystem<UI>();
		if (!ui->GetFocusElement())
		{
			if (!touch_ || !touch_->useGyroscope_)
			{
				
			//	character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
				//character_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
				character_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
				character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
			}
			character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));

			// Add character yaw & pitch from the mouse motion or touch input
			if (touchEnabled_)
			{
				for (unsigned i = 0; i < input->GetNumTouches(); ++i)
				{
					TouchState* state = input->GetTouch(i);
					if (!state->touchedElement_)    // Touch on empty space
					{
						Camera* camera = cameraNode_->GetComponent<Camera>();
						if (!camera)
							return;

						Graphics* graphics = GetSubsystem<Graphics>();
						//character_->controls_.yaw_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.x_;
						//character_->controls_.pitch_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.y_;
					}
				}
			}
			else
			{
				//character_->controls_.yaw_ += (float)input->GetMouseMoveX() * YAW_SENSITIVITY;
				//character_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;
			}
			
			character_->controls_.Set(CTRL_FORWARD);
			
			// Limit pitch
			//character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
			// Set rotation already here so that it's updated every rendering frame instead of every physics frame
			character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));


			// Turn on/off gyroscope on mobile platform
			if (touch_ && input->GetKeyPress(KEY_G))
				touch_->useGyroscope_ = !touch_->useGyroscope_;

			// Check for loading / saving the scene
			if (input->GetKeyPress(KEY_F5))
			{
				File saveFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/CharacterDemo.xml", FILE_WRITE);
				scene_->SaveXML(saveFile);
			}
			if (input->GetKeyPress(KEY_F7))
			{
				File loadFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/CharacterDemo.xml", FILE_READ);
				scene_->LoadXML(loadFile);
				// After loading we have to reacquire the weak pointer to the Character component, as it has been recreated
				// Simply find the character's scene node by name as there's only one of them
				Node* characterNode = scene_->GetChild("Jack", true);
				if (characterNode)
					character_ = characterNode->GetComponent<Character>();
			}
		}
		
	}
}


void MainScene::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	if (!character_)
		return;

	Node* characterNode = character_->GetNode();

	// update wyswietlanego score
	time_ += 0.01;
	std::string str;
	str.append("Score: ");
	str.append(std::to_string(int(time_ * 10)));
	String s(str.c_str(), str.size());
	text_->SetText(s);

	// update wyswietlanej pozycji bohatera
	std::string str2;
	str2.append("posX: ");
	str2.append(std::to_string(int(characterNode->GetPosition().x_)));
	str2.append(" posY: ");
	str2.append(std::to_string(int(characterNode->GetPosition().y_)));
	str2.append(" posZ: ");
	str2.append(std::to_string(int(characterNode->GetPosition().z_)));
	String s2(str2.c_str(), str2.size());
	text2_->SetText(s2);
	



	// Get camera lookat dir from character yaw + pitch
	Quaternion rot = characterNode->GetRotation();
	Quaternion dir = rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT);

		// Third person camera: position behind the character
		Vector3 aimPoint = characterNode->GetPosition() + rot * Vector3(0.0f, 2.2f, -1.0f);

		// Collide camera ray with static physics objects (layer bitmask 2) to ensure we see the character properly
		Vector3 rayDir = dir * Vector3::BACK;
		float rayDistance = touch_ ? touch_->cameraDistance_ : CAMERA_INITIAL_DIST;
		PhysicsRaycastResult result;
		scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, Ray(aimPoint, rayDir), rayDistance, 2);
		if (result.body_)
			rayDistance = Min(rayDistance, result.distance_);
		rayDistance = Clamp(rayDistance, CAMERA_MIN_DIST, CAMERA_MAX_DIST);

		cameraNode_->SetPosition(aimPoint + rayDir * rayDistance);
		cameraNode_->SetRotation(dir);
	
	
}

void MainScene::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
	GetSubsystem<Renderer>()->DrawDebugGeometry(true);
}