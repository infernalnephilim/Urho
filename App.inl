#include <Urho3D/Engine/Application.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/Log.h>

App::App(Context* context) :
	Application(context),
	yaw_(0.0f),
	pitch_(0.0f),
	touchEnabled_(false),
	screenJoystickIndex_(M_MAX_UNSIGNED),
	screenJoystickSettingsIndex_(M_MAX_UNSIGNED),
	paused_(false),
	useMouseMode_(MM_ABSOLUTE)
{
}
void App::Setup()
{
	// modyfikacja parametrów startowych silnika
	engineParameters_["WindowTitle"] = GetTypeName();
	engineParameters_["LogName"] = GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs") + GetTypeName() + ".log";
	engineParameters_["FullScreen"] = false;
	engineParameters_["Headless"] = false;
	engineParameters_["Sound"] = true;

	if (!engineParameters_.Contains("ResourcePrefixPaths"))
		engineParameters_["ResourcePrefixPaths"] = ";../share/Resources;../share/Urho3D/Resources";
}
void App::Start()
{
	if (GetPlatform() == "Android" || GetPlatform() == "iOS")
		// On mobile platform, enable touch by adding a screen joystick
		InitTouchInput();
	else if (GetSubsystem<Input>()->GetNumJoysticks() == 0)
		// On desktop platform, do not detect touch when we already got a joystick
		SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(App, HandleTouchBegin));

	SetWindowTitleAndIcon();

	CreateConsoleAndDebugHud();

	SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(App, HandleKeyDown));

	SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(App, HandleSceneUpdate));
}

void App::Stop()
{
	engine_->DumpResources(true);
}

void App::InitTouchInput()
{
	touchEnabled_ = true;

	ResourceCache* cache = GetSubsystem<ResourceCache>();
	Input* input = GetSubsystem<Input>();
	XMLFile* layout = cache->GetResource<XMLFile>("UI/ScreenJoystick_Samples.xml");
	const String& patchString = GetScreenJoystickPatchString();
	if (!patchString.Empty())
	{
		// Patch the screen joystick layout further on demand
		SharedPtr<XMLFile> patchFile(new XMLFile(context_));
		if (patchFile->FromString(patchString))
			layout->Patch(patchFile);
	}
	screenJoystickIndex_ = input->AddScreenJoystick(layout, cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
	input->SetScreenJoystickVisible(screenJoystickSettingsIndex_, true);
}

void App::InitMouseMode(MouseMode mode)
{
	useMouseMode_ = mode;

	Input* input = GetSubsystem<Input>();

	if (GetPlatform() != "Web")
	{
		if (useMouseMode_ == MM_FREE)
			input->SetMouseVisible(true);

		Console* console = GetSubsystem<Console>();
		if (useMouseMode_ != MM_ABSOLUTE)
		{
			input->SetMouseMode(useMouseMode_);
			if (console && console->IsVisible())
				input->SetMouseMode(MM_ABSOLUTE, true);
		}
	}
	else
	{
		input->SetMouseVisible(true);
		SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(App, HandleMouseModeRequest));
		SubscribeToEvent(E_MOUSEMODECHANGED, URHO3D_HANDLER(App, HandleMouseModeChange));
	}
}

void App::SetWindowTitleAndIcon()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	Graphics* graphics = GetSubsystem<Graphics>();
	// Ikonka na pasku tytulu
	Image* icon = cache->GetResource<Image>("Textures/UrhoIcon.png");
	graphics->SetWindowIcon(icon);
	// Tytul okna
	graphics->SetWindowTitle("Temple Run Urho3D");
}

void App::CreateConsoleAndDebugHud()
{
	// Get default style
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

	// Create console
	Console* console = engine_->CreateConsole();
	console->SetDefaultStyle(xmlFile);
	console->GetBackground()->SetOpacity(0.8f);

	// Create debug HUD.
	DebugHud* debugHud = engine_->CreateDebugHud();
	debugHud->SetDefaultStyle(xmlFile);
}





void App::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;
	// Check for pressing ESC. Note the engine_ member variable for convenience access to the Engine object
	int key = eventData[P_KEY].GetInt();
	if (key == KEY_ESCAPE)
		engine_->Exit();
	else if (key == KEY_F1)
		GetSubsystem<Console>()->Toggle();

	// Toggle debug HUD with F2
	else if (key == KEY_F2)
		GetSubsystem<DebugHud>()->ToggleAll();
	else if (!GetSubsystem<UI>()->GetFocusElement())
	{
		Renderer* renderer = GetSubsystem<Renderer>();

		// Preferences / Pause
		if (key == KEY_SELECT && touchEnabled_)
		{
			paused_ = !paused_;

			Input* input = GetSubsystem<Input>();
			if (screenJoystickSettingsIndex_ == M_MAX_UNSIGNED)
			{
				// Lazy initialization
				ResourceCache* cache = GetSubsystem<ResourceCache>();
				screenJoystickSettingsIndex_ = input->AddScreenJoystick(cache->GetResource<XMLFile>("UI/ScreenJoystickSettings_Samples.xml"), cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
			}
			else
				input->SetScreenJoystickVisible(screenJoystickSettingsIndex_, paused_);
		}
	}
}

void App::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
	// Move the camera by touch, if the camera node is initialized by descendant sample class
	if (touchEnabled_ && cameraNode_)
	{
		Input* input = GetSubsystem<Input>();
		for (unsigned i = 0; i < input->GetNumTouches(); ++i)
		{
			TouchState* state = input->GetTouch(i);
			if (!state->touchedElement_)    // Touch on empty space
			{
				if (state->delta_.x_ || state->delta_.y_)
				{
					Camera* camera = cameraNode_->GetComponent<Camera>();
					if (!camera)
						return;

					Graphics* graphics = GetSubsystem<Graphics>();
					yaw_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.x_;
					pitch_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.y_;

					// Construct new orientation for the camera scene node from yaw and pitch; roll is fixed to zero
					cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
				}
				else
				{
					// Move the cursor to the touch position
					Cursor* cursor = GetSubsystem<UI>()->GetCursor();
					if (cursor && cursor->IsVisible())
						cursor->SetPosition(state->position_);
				}
			}
		}
	}
}

void App::HandleMouseModeRequest(StringHash eventType, VariantMap& eventData)
{
	Console* console = GetSubsystem<Console>();
	if (console && console->IsVisible())
		return;
	Input* input = GetSubsystem<Input>();
	if (useMouseMode_ == MM_ABSOLUTE)
		input->SetMouseVisible(false);
	else if (useMouseMode_ == MM_FREE)
		input->SetMouseVisible(true);
	input->SetMouseMode(useMouseMode_);
}

void App::HandleMouseModeChange(StringHash eventType, VariantMap& eventData)
{
	Input* input = GetSubsystem<Input>();
	bool mouseLocked = eventData[MouseModeChanged::P_MOUSELOCKED].GetBool();
	input->SetMouseVisible(!mouseLocked);
}

void App::HandleTouchBegin(StringHash eventType, VariantMap& eventData)
{
	// On some platforms like Windows the presence of touch input can only be detected dynamically
	InitTouchInput();
	UnsubscribeFromEvent("TouchBegin");
}