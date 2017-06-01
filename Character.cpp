#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "Character.h"

Character::Character(Context* context) :
	LogicComponent(context),
	onGround_(false),
	okToJump_(true),
	inAirTimer_(0.0f)
{
	// Only the physics update event is needed: unsubscribe from the rest for optimization
	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void Character::RegisterObject(Context* context)
{
	context->RegisterFactory<Character>();

	// These macros register the class attributes to the Context for automatic load / save handling.
	// We specify the Default attribute mode which means it will be used both for saving into file, and network replication
	URHO3D_ATTRIBUTE("Controls Yaw", float, controls_.yaw_, 0.0f, AM_DEFAULT);
	URHO3D_ATTRIBUTE("Controls Pitch", float, controls_.pitch_, 0.0f, AM_DEFAULT);
	URHO3D_ATTRIBUTE("On Ground", bool, onGround_, false, AM_DEFAULT);
	URHO3D_ATTRIBUTE("OK To Jump", bool, okToJump_, true, AM_DEFAULT);
	URHO3D_ATTRIBUTE("On Left Lane", bool, onLeftLane_, false, AM_DEFAULT);
	URHO3D_ATTRIBUTE("On Middle Lane", bool, onMiddleLane_, true, AM_DEFAULT);
	URHO3D_ATTRIBUTE("On Right Lane", bool, onRightLane_, false, AM_DEFAULT);
	URHO3D_ATTRIBUTE("In Air Timer", float, inAirTimer_, 0.0f, AM_DEFAULT);
}

void Character::Start()
{
	// Component has been inserted into its scene node. Subscribe to events now
	SubscribeToEvent(GetNode(), E_NODECOLLISION, URHO3D_HANDLER(Character, HandleNodeCollision));
}

void Character::FixedUpdate(float timeStep)
{

	/////////////////////////////
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();

	// Construct new Text object, set string to display and font to use
	Text* instructionText = ui->GetRoot()->CreateChild<Text>();
	instructionText->SetText(
		"\t\t\t\tTEXT\t\t\t\t"
	);
	instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
	// The text has multiple rows. Center them in relation to each other
	instructionText->SetTextAlignment(HA_CENTER);

	// Position the text relative to the screen center
	instructionText->SetHorizontalAlignment(HA_CENTER);
	instructionText->SetVerticalAlignment(VA_CENTER);
	instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
	////////////////////////////////


	/// \todo Could cache the components for faster access instead of finding them each frame
	RigidBody* body = GetComponent<RigidBody>();
	AnimationController* animCtrl = GetComponent<AnimationController>();

	// Update the in air timer. Reset if grounded
	if (!onGround_)
		inAirTimer_ += timeStep;
	else
		inAirTimer_ = 0.0f;
	// When character has been in air less than 1/10 second, it's still interpreted as being on ground
	bool softGrounded = inAirTimer_ < INAIR_THRESHOLD_TIME;

	// Update movement & animation
	const Quaternion& rot = node_->GetRotation();
	Vector3 moveDir = Vector3::ZERO;
	const Vector3& velocity = body->GetLinearVelocity();
	// Velocity on the XZ plane
	Vector3 planeVelocity(velocity.x_, 0.0f, velocity.z_);

	if (controls_.IsDown(CTRL_FORWARD))
		moveDir += Vector3::FORWARD;
	/*if (controls_.IsDown(CTRL_BACK))
		moveDir += Vector3::BACK;*/
	if (controls_.IsDown(CTRL_RIGHT))
	{
		moveDir += Vector3::RIGHT;
		//body->SetPosition((body->GetPosition()) + Vector3::RIGHT * 2);
		/*if ((body->GetPosition()).x_ >= 2.0f)
		{
			instructionText->SetPosition(70.0f, ui->GetRoot()->GetHeight() / 4);
		}
		else if ((body->GetPosition()).x_ < 2.0f && (body->GetPosition()).x_ > -2.0f)
		{
			body->SetPosition((body->GetPosition()) + Vector3::RIGHT * 2);
		}
		else {
			body->SetPosition((body->GetPosition()) + Vector3::RIGHT * 2);
		}
*/
		/*if (onMiddleLane_)
		{
			onMiddleLane_ = false;
			moveDir += Vector3::RIGHT * 15;
			onRightLane_ = true;
			
		}
		else if (onLeftLane_)
		{
			onLeftLane_ = false;
			moveDir += Vector3::RIGHT * 15;
			onMiddleLane_ = true;
			
		}*/
	}

	if (controls_.IsDown(CTRL_LEFT))
	{
		moveDir += Vector3::LEFT;
		//body->SetPosition((body->GetPosition()) + Vector3::LEFT * 2);
		//if ((body->GetPosition()).x_ <= 2.0f)
		//{
		//	
		//	//instructionText->SetPosition(70.0f, ui->GetRoot()->GetHeight() / 4);
		//}
		//else if ((body->GetPosition()).x_ < 2.0f && (body->GetPosition()).x_ > -2.0f)
		//{
		//	//body->ApplyImpulse(Vector3::LEFT * 2 * MOVE_FORCE);
		//	body->SetPosition((body->GetPosition()) + Vector3::LEFT * 2);
		//	//moveDir += Vector3::LEFT * 15;
		//}
		//else {
		//	body->SetPosition((body->GetPosition()) + Vector3::LEFT * 2);
		//}
		/*if (onLeftLane_)
		{
			instructionText->SetPosition(-70.0f, ui->GetRoot()->GetHeight() / 4);
		}
		if(onMiddleLane_)
		{
			onMiddleLane_ = false;
			moveDir += Vector3::LEFT*15;
			onLeftLane_ = true;
			
		}
		else if (onRightLane_)
		{
			onRightLane_ = false;
			moveDir += Vector3::LEFT * 15;
			onMiddleLane_ = true;
			
		}*/

		
	}
	

	// Normalize move vector so that diagonal strafing is not faster
	if (moveDir.LengthSquared() > 0.0f)
		moveDir.Normalize();

	// If in air, allow control, but slower than when on ground
	body->ApplyImpulse(rot * moveDir * (softGrounded ? MOVE_FORCE : INAIR_MOVE_FORCE));

	if (softGrounded)
	{
		// When on ground, apply a braking force to limit maximum ground velocity
		Vector3 brakeForce = -planeVelocity * BRAKE_FORCE;
		body->ApplyImpulse(brakeForce);

		// Jump. Must release jump control inbetween jumps
		if (controls_.IsDown(CTRL_JUMP))
		{
			if (okToJump_)
			{
				body->ApplyImpulse(Vector3::UP * JUMP_FORCE);
				okToJump_ = false;
			}
		}
		else
			okToJump_ = true;
	}

	// Play walk animation if moving on ground, otherwise fade it out
	if (softGrounded && !moveDir.Equals(Vector3::ZERO))
		animCtrl->PlayExclusive("Models/Jack_Walk.ani", 0, true, 0.2f);
	else
		animCtrl->Stop("Models/Jack_Walk.ani", 0.2f);
	// Set walk animation speed proportional to velocity
	animCtrl->SetSpeed("Models/Jack_Walk.ani", planeVelocity.Length() * 0.3f);

	// Reset grounded flag for next frame
	onGround_ = false;
}

void Character::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
	// Check collision contacts and see if character is standing on ground (look for a contact that has near vertical normal)
	using namespace NodeCollision;

	MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());

	while (!contacts.IsEof())
	{
		Vector3 contactPosition = contacts.ReadVector3();
		Vector3 contactNormal = contacts.ReadVector3();
		/*float contactDistance = */contacts.ReadFloat();
		/*float contactImpulse = */contacts.ReadFloat();

		// If contact is below node center and mostly vertical, assume it's a ground contact
		if (contactPosition.y_ < (node_->GetPosition().y_ + 1.0f))
		{
			float level = Abs(contactNormal.y_);
			if (level > 0.75)
				onGround_ = true;
		}
	}
}