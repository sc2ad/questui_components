#include "main.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "shared/components/Text.hpp"
#include "shared/components/ScrollableContainer.hpp"
#include "shared/components/HoverHint.hpp"
#include "shared/components/ViewComponent.hpp"
#include "shared/components/Button.hpp"
#include "shared/components/Modal.hpp"
#include "shared/components/layouts/VerticalLayoutGroup.hpp"
#include "shared/components/layouts/HorizontalLayoutGroup.hpp"
#include "shared/components/layouts/ModifierContainer.hpp"
#include "shared/components/settings/ToggleSetting.hpp"
#include "shared/components/settings/StringSetting.hpp"
#include "shared/components/settings/IncrementSetting.hpp"
#include "shared/components/settings/DropdownSetting.hpp"
#include "shared/components/misc/RainbowText.hpp"

// Custom components
#include "TestComponent.hpp"
#include "TacoImage.hpp"

using namespace QuestUI;
using namespace QuestUI_Components;
using namespace QuestUI_Components::Loggerr;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}


// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
}

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    getLogger().info("DidActivate: %p, %d, %d, %d", self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (firstActivation) {
        // keep these pointers alive so the lambdas can capture them. These would usually be instance fields in a ViewCoordinator
        static ScrollableContainer* container;
        static Text* newText;
        static int count = 0;

        // We can use new because ComponentWrapper takes ownership of the pointer
        // A component will then only be freed if the parent is freed
        // We then make sure the parent never dies by never freeing
        // A better way of doing this is tying the highest parent to a ViewCoordinator or anything else


#pragma region Loading
        // Adds a "Loading" text that updates while the UI "loads"
        static ViewComponent* loadingView = nullptr;
        if (!container) {
            loadingView = new ViewComponent{self->get_transform(), {
                    new ScrollableContainer({
                        (new Text("Loading"))->with([](Text* text) {
                            std::string textStr = text->getData().text;
                            std::thread([textStr, text]{
                                int periodCount = 0;
                                bool once = true;

                                while (loadingView || once) {
                                    getLogger().debug("Loading check! %i", periodCount);
                                    once = false;
                                    std::this_thread::sleep_for(std::chrono::milliseconds(200));

                                    if (!loadingView) break;

                                    periodCount++;
                                    if (periodCount > 4) periodCount = 1;

                                    text->mutateData([&textStr, &periodCount](MutableTextData data) {
                                        data.text = textStr + std::string(periodCount, '.');

                                        return data;
                                    });

                                    QuestUI::MainThreadScheduler::Schedule([] {
                                        if (loadingView) {
                                            // Updates the entire hierarchy
                                            loadingView->render();
                                        }
                                    });
                                }
                            }).detach();
                        })
                    })
            }};
        }
        if (loadingView)
            loadingView->render();
#pragma endregion
        // async UI!

#pragma region FullyLoadedUI

        // Image has to be loaded on main thread
        // When passing this to a container, it will take ownership, so we don't need to call delete
        auto* tacoImage = new TacoImage(Image::InitialImageData{
                .sizeDelta = {128, 128}
        });

// CLion why
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"

        std::thread([self, tacoImage]{
            // Simulate slow UI
            std::this_thread::sleep_for(std::chrono::seconds(2));

            auto* view = new ViewComponent{self->get_transform(), {

                    container = new ScrollableContainer {{
                        new HoverHint("hint", new Text("hi!")),
                        (new Text("this is cool! Pink Cute!"))->with([](Text* text){
                            text->mutateData([](MutableTextData data) {
                                data.color = UnityEngine::Color(255.0f / 255.0f, 61.0f / 255.0f, 171.0f / 255.0f, 1.0f);
                                return data;
                            });
                            // we don't update here because it hasn't rendered, this is called before rendering
                        }),

                        // we can create components using lambdas too
                        {[]{
                            Modal* modal = (new Modal({}, nullptr))->with([](Modal* modal){
                                auto* horizontalWrapper = new HorizontalLayoutGroup({
                                    new VerticalLayoutGroup({
                                        new Text("Look at me!"),
                                        new Button("Close!", [modal](Button* button, UnityEngine::Transform* parentTransform){
                                            modal->dismiss();
                                        })
                                    })
                                });

                                // we need the modal pointer, so use the lambda
                                modal->addToHierarchy(horizontalWrapper);
                            });



                            return new Button("More info!", [modal](Button* button, UnityEngine::Transform* parentTransform) {
                                // Add to `container` on click because `container` at modal construction is null
                                if (!modal->isRendered()) {
                                    container->addToHierarchy(modal);
                                }

                                modal->show();
                            });
                        }},
                        // Custom component
                        new TestComponent("pink cute eris cute"),
                        new RainbowText("Rainbow!"),

                        // Image is loaded on main thread
                        new HorizontalLayoutGroup{{
                         new VerticalLayoutGroup{{
                             tacoImage,
                         }}
                        }},


                        // Toggles
                        new ToggleSetting("Toggle false", false),
                        new ToggleSetting("Toggle true", true, [](ToggleSetting* set, bool val, UnityEngine::Transform*) {
                            set->mutateData([&val](MutableToggleSettingsData data){
                                data.text = "Toggle " + std::string(val ? "true" : "false");
                                return data;
                            });
                            set->doUpdate();
                        }),
                        new StringSetting("Text setting", "The current val!", [](StringSetting*, const std::string& input, UnityEngine::Transform*){
                            getLogger().debug("Input! %s", input.c_str());
                        }),
                        new QuestUI_Components::IncrementSetting("Increment!", 5.0f, 2, 0.05f, [](QuestUI_Components::IncrementSetting* set, float input, UnityEngine::Transform*){
                            getLogger().debug("Increment value! %f", input);
                            set->mutateData([&input](MutableIncrementSettingsData data){
                                data.text = "Increment value: " + std::to_string(input);
                                return data;
                            });
                            set->doUpdate();
                        }),
                        new DropdownSetting("Dropdowns are cool!", "some val", {"value1", "value2", "some val", "value3"}, [](DropdownSetting* set, const std::string& selected, UnityEngine::Transform*){
                            getLogger().debug("Dropdowns are cool %s", selected.c_str());
                            set->mutateData([&selected](MutableDropdownSettingsData data) {
                               data.text = "Dropdowns are coeaweol!" + selected;
                               data.values.emplace_back(std::to_string(count));
                               return data;
                            });
                            set->doUpdate();
                        }),

                        new HoverHint("hintee", new Text("hello from other world!")),
                        new HoverHint("another hintee", new Text("this is cooler!!")),
                        new Button("Click me!", [](Button* button, UnityEngine::Transform* parentTransform) {
                            if (!newText) {
                                newText = new Text("New text!");
                                container->addToHierarchy(newText);
                            } else {
                                count++;
                                newText->mutateData([](MutableTextData data){
                                    data.text = "someOtherText" + std::to_string(count);
                                    return data;
                                });
                                newText->doUpdate();
                            }

                            // Update button text
                            button->mutateData([](MutableButtonData data){
                                data.text = "Clicked: " + std::to_string(count);
                                return data;
                            });
                            button->doUpdate();
                        })
                    }}

            }};

            QuestUI::MainThreadScheduler::Schedule([view]{
                if (loadingView) {
                    loadingView->destroyAll();
                    delete loadingView;
                    loadingView = nullptr;
                }
                view->render();

                // Multiple renders should simply just update, not crash or duplicate.
                view->render();
                view->render();
            });
        }).detach();
#pragma clang diagnostic pop

#pragma endregion
    }
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    getLogger().info("Installing hooks...");
    // Install our hooks (none defined yet)
    getLogger().info("Installed all hooks!");

    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
    getLogger().info("Successfully installed Settings UI!");
}