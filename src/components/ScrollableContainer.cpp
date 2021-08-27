#include "components/ScrollableContainer.hpp"
#include "questui/shared/BeatSaberUI.hpp"

#include "main.hpp"

using namespace QuestUI;

UnityEngine::Transform* QuestUI_Components::ScrollableContainer::render(UnityEngine::Transform *parentTransform) {
    UnityEngine::GameObject* scrollableContainer = BeatSaberUI::CreateScrollableSettingsContainer(parentTransform);

    transform = scrollableContainer->get_transform();

    rendered = true;
    for (auto& comp : renderChildren)
        renderComponent(comp, const_cast<UnityEngine::Transform*>(transform));

    return transform;
}

void QuestUI_Components::ScrollableContainer::renderComponentInContainer(QuestUI_Components::ComponentWrapper &comp) {
    if (rendered) {
        renderComponent(comp, const_cast<UnityEngine::Transform*>(transform));
    }
}