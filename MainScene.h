#pragma once

#include "App.h"

namespace Urho3D
{
	class Node;
	class Scene;
}

class Character;
class Touch;

class MainScene : public App 
{
	URHO3D_OBJECT(MainScene, App);

public:
	SharedPtr<Text> text_;
	SharedPtr<Text> text2_;
	float time_;
	float characterPositionX;
	float characterPositionZ;

	MainScene(Context* context);
	~MainScene();

	virtual void Start();

private:
	// Utworzenie sceny
	void CreateScene();
	// Utworzenie bohatera
	void CreateCharacter();
	
	void UpdateText();

	void SubscribeToEvents();
	
	/// Handle application update. Set controls to character.
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	/// Handle application post-update. Update camera position after character has moved.
	void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
	void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

	/// Touch utility object.
	SharedPtr<Touch> touch_;
	/// The controllable character component.
	WeakPtr<Character> character_;
};