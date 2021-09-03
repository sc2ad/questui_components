#include "shared/components/settings/IncrementSetting.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "TMPro/TextMeshPro.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

using namespace QuestUI;
using namespace QuestUI_Components;

Component* QuestUI_Components::IncrementSetting::render(UnityEngine::Transform *parentTransform) {
    CallbackWrapper callback = constructWrapperCallback(parentTransform);

    bool hasMax = data.max.has_value();
    bool hasMin = data.min.has_value();

    float min = data.min.value_or(0);
    float max = data.max.value_or(0);

    int decimals = data.decimals;
    float increment = data.increment;

    if (incrementInitData) {
        uiIncrement = BeatSaberUI::CreateIncrementSetting(parentTransform, std::string(data.text), decimals, increment, getValue(), hasMin, hasMax, min, max,incrementInitData->anchoredPosition, callback);
    } else {
        uiIncrement = BeatSaberUI::CreateIncrementSetting(parentTransform, std::string(data.text), decimals, increment, getValue(), callback);
    }

    transform = uiIncrement->get_transform();

    // From QuestUI
    uiText = transform->GetComponentInChildren<TMPro::TextMeshProUGUI*>();


    markAsRendered();
    update();

    return this;
}

void QuestUI_Components::IncrementSetting::update() {
    if (!rendered)
        throw std::runtime_error("Increment setting component has not rendered!");

    BaseSetting::update();
    uiIncrement->set_enabled(data.enabled);

    // TODO: How?
    // uiIncrement->set_interactable(data.interactable);

    uiIncrement->Decimals = data.decimals;
    uiIncrement->Increment = data.increment;

    uiIncrement->MaxValue = data.max.value_or(0);
    uiIncrement->MinValue = data.min.value_or(0);

    uiIncrement->HasMax = (bool) data.max;
    uiIncrement->HasMin = (bool) data.min;

    uiIncrement->CurrentValue = getValue();
}

