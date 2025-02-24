#pragma once

#include "UnityEngine/Vector2.hpp"

#include "shared/context.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include <string>
#include <array>

namespace QUC {
    namespace detail {
        template<size_t sz>
        struct DropdownSetting {
            static_assert(renderable<DropdownSetting>);
            using OnCallback = std::function<void(DropdownSetting*, std::string, UnityEngine::Transform*)>;
            std::string text;
            OnCallback callback;
            bool enabled;
            bool interactable;
            std::string value;
            std::array<std::string, sz> values;
            template<class F>
            DropdownSetting(std::string_view txt, std::string_view current, F&& callable, bool enabled_ = true, bool interact = true, std::array<std::string, sz> v = std::array<std::string, 0>())
                : text(txt), callback(callable), enabled(enabled_), interactable(interact), value(current), values(v) {}
            
            auto render(RenderContext& ctx) {
                // TODO: Cache this properly
                auto parent = &ctx.parentTransform;
                auto setting = QuestUI::BeatSaberUI::CreateStringSetting(parent, text, value, anchoredPosition, keyboardPositionOffset, [this, parent](std::string_view val) {
                    callback(this, std::string(val), parent);
                });
                auto txt = setting->placeholderText->GetComponent<TMPro::TextMeshProUGUI*>();
                CRASH_UNLESS(txt);
                txt->set_text(il2cpp_utils::newcsstr(text));
                setting->set_enabled(enabled);
                setting->set_interactable(interactable);
                setting->SetText(il2cpp_utils::newcsstr(value));
            }
        };
    }
}

namespace QuestUI_Components {

    struct MutableDropdownSettingsData : public MutableSettingsData<std::string> {
        std::vector<std::string> values;
    };

    class DropdownSetting : public BaseSetting<std::string, DropdownSetting, MutableDropdownSettingsData> {
    public:


        explicit DropdownSetting(std::string_view text, std::string_view currentValue,
                                 std::vector<std::string> const& values,
                                 OnCallback callback = nullptr)
                : BaseSetting(std::string(text), std::string(currentValue), std::move(callback))
        {
            data.values = values;
        }

    protected:
        void update() override;
        Component* render(UnityEngine::Transform *parentTransform) override;

        // render time
        HMUI::SimpleTextDropdown* uiDropdown = nullptr;
    };

// TODO: Test if it works
#pragma region ConfigEnum
#if defined(AddConfigValue) || __has_include("config-utils/shared/config-utils.hpp")
    template<typename EnumType>
    using EnumToStrType = std::unordered_map<EnumType, std::string>;

    template<typename EnumType>
    using StrToEnumType = std::unordered_map<std::string, EnumType>;

    template<typename EnumType>
    struct EnumToStr;

    template<typename EnumType>
    struct StrToEnum;

    template<typename EnumType>
    struct EnumStrValues;



    template<typename EnumType>
    EnumToStrType<EnumType> createFromKeysAndValues(std::initializer_list<int> keysList, std::initializer_list<std::string> valuesList) {
        std::vector<int> keys(keysList);
        std::vector<std::string> values(valuesList);
        EnumToStrType<EnumType> map(keys.size());
        for (int i =0; i < keys.size(); i++) {
            map.emplace((EnumType) keys[i], values[i]);
        }

        return map;
    }


    template<typename EnumType>
    StrToEnumType<EnumType> createFromKeysAndValues(std::initializer_list<std::string> keysList, std::initializer_list<int> valuesList) {
        std::vector<std::string> keys(keysList);
        std::vector<int> values(valuesList);
        StrToEnumType<EnumType> map(keys.size());
        for (int i =0; i < keys.size(); i++) {
            map.emplace(keys[i], (EnumType) values[i]);
        }

        return map;
    }

#define STR_LIST(...) __VA_ARGS__

#define DROPDOWN_CREATE_ENUM_CLASS(EnumName, strlist, ...) \
enum struct EnumName {                            \
    __VA_ARGS__                                   \
};                                                \
namespace QuestUI_Components {             \
enum FakeEnum__##EnumName {                       \
    __VA_ARGS__                                   \
};                                                         \
template<> struct ::QuestUI_Components::EnumToStr<EnumName> {                      \
    inline static const EnumToStrType<EnumName> map = createFromKeysAndValues<EnumName>({__VA_ARGS__}, {strlist}); \
    static EnumToStrType<EnumName> get() {                                       \
        return map;                                          \
    }                                              \
};                                                \
template<> struct ::QuestUI_Components::StrToEnum<EnumName> {                      \
    inline static const QuestUI_Components::StrToEnumType<EnumName> map = createFromKeysAndValues<EnumName>({strlist}, {__VA_ARGS__}); \
    static StrToEnumType<EnumName> get() {                                       \
        return map;                                          \
    }                                              \
};                                                \
template<> struct ::QuestUI_Components::EnumStrValues<EnumName> {                      \
    inline static const std::vector<std::string> values = std::vector<std::string>({strlist}); \
    static std::vector<std::string> get() {                                       \
        return values;                                          \
    }                                              \
};                                                \
} // end namespace EnumNamespace__##EnumName

    // c++ inheritance is a pain
    template<typename EnumType, typename EnumConfigValue = int, bool CrashOnBoundsExit = false>
    requires (std::is_enum<EnumType>::value)
    class ConfigUtilsEnumDropdownSetting : public DropdownSetting {

    public:
        static_assert(&EnumToStr<EnumType>::get, "Please create a type specialization for EnumToStr");
        static_assert(&StrToEnum<EnumType>::get, "Please create a type specialization for StrToEnum");
        static_assert(&EnumStrValues<EnumType>::get, "Please create a type specialization for EnumStrValues");

        template<typename... TArgs>
        explicit
        ConfigUtilsEnumDropdownSetting(ConfigUtils::ConfigValue<EnumConfigValue> &configValue, TArgs &&... args)
                : configValue(configValue), DropdownSetting(configValue.GetName(), "", EnumStrValues<EnumType>::values, args...) {
            this->setValueOfData(this->data, this->getValue());
        }

    protected:
        // reference capture should be safe here
        ConfigUtils::ConfigValue<int>& configValue;

        Component *render(UnityEngine::Transform *parentTransform) override {
            DropdownSetting::render(parentTransform);

            if (!configValue.GetHoverHint().empty()) {
                QuestUI::BeatSaberUI::AddHoverHint(this->getTransform()->get_gameObject(), configValue.GetHoverHint());
            }

            return this;

        };


        std::string getValue() override {
            EnumToStrType<EnumType> map = EnumToStr<EnumType>::map;
            if constexpr (CrashOnBoundsExit) {
                return map[(EnumType) this->configValue];
            } else {
                auto it = map.find((EnumType) this->configValue.GetValue());

                if (it == map.end()) {
                    if (map.size() == 0) return "";
                    else return map.cbegin()->second;
                }

                return it->second;
            }
        };

        void internalSetValue(const std::string &val) override {
            StrToEnumType<EnumType> map = StrToEnum<EnumType>::map;
            if constexpr (CrashOnBoundsExit) {
                EnumType newValue = map[val];
                this->configValue.SetValue((int) newValue);
            } else {
                auto it = map.find(val);

                if (it == map.end()) {
                    if (map.size() == 0) this->configValue.SetValue((int) 0);
                    else this->configValue.SetValue((int) map.cbegin()->second);
                }

                this->configValue.SetValue((int) it->second);
            }
        }
    };
#endif
#pragma endregion
}