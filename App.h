#pragma once

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>

namespace Urho3D
{
	class Node;
	class Scene;
	class Sprite;
	
}

using namespace Urho3D;

const float TOUCH_SENSITIVITY = 2.0f;

class App : public Application
{
	URHO3D_OBJECT(App, Application);

public:
	App(Context* context);
	
	virtual void Setup();
	virtual void Start();
	virtual void Stop();
	
protected:
	virtual String GetScreenJoystickPatchString() const { return String::EMPTY; }
	// Inicjalizacja sterowania myszka
	void InitMouseMode(MouseMode mode);
	// Inicjalizacja sterowania dotykiem
	void InitTouchInput();

	// obrot kamery w poziomie
	float yaw_;
	// obrot kamery w pionie
	float pitch_;

	bool touchEnabled_;
	MouseMode useMouseMode_;

	// scena
	SharedPtr<Scene> scene_;
	// kamera
	SharedPtr<Node> cameraNode_;

private:
	bool paused_;

	unsigned screenJoystickIndex_;
	/// Screen joystick index for settings (mobile platforms only).
	unsigned screenJoystickSettingsIndex_;

	// Ustawienie tytulu okna i ikonki
	void SetWindowTitleAndIcon();
	void CreateConsoleAndDebugHud();

	void HandleMouseModeRequest(StringHash eventType, VariantMap& eventData);
	void HandleMouseModeChange(StringHash eventType, VariantMap& eventData);

	void HandleKeyDown(StringHash eventType, VariantMap& eventData);

	// Update kamery
	void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
	

	/// Handle touch begin event to initialize touch input on desktop platform.
	void HandleTouchBegin(StringHash eventType, VariantMap& eventData);
};

#include "App.inl"