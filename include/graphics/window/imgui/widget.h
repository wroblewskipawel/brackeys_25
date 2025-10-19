#pragma once

#include <imgui.h>

#include "collections/unique_list.h"

template <typename...>
class WidgetListBuilder;

template <typename... Widgets>
class WidgetList;

template <typename T>
concept IsWidget = requires(WidgetList<T> widgetList, float dTime) {
    { widgetList.draw() } -> std::same_as<void>;
    { widgetList.update(dTime) } -> std::same_as<void>;
};

template <typename... Widgets>
class WidgetList {
   public:
    void draw() noexcept { (drawWidget<Widgets>(), ...); }

    void update(float dTime) noexcept { (updateWidget<Widgets>(dTime), ...); }

   private:
    friend class WidgetListBuilder<Widgets...>;

    WidgetList(UniqueTypeList<Widgets...>&& widgets)
        : widgets(std::move(widgets)) {}

    template <typename Widget>
    void drawWidget() noexcept {
        widgets.get<Widget>().draw();
    }

    template <typename Widget>
    void updateWidget(float dTime) noexcept {
        widgets.get<Widget>().update(dTime);
    }

    UniqueTypeList<Widgets...> widgets;
};

template <typename Type>
struct IsWidgetListT : std::false_type {};

template <typename... Widgets>
struct IsWidgetListT<WidgetList<Widgets...>> : std::true_type {};

template <typename... Widgets>
inline constexpr bool IsWidgetListV = IsWidgetListT<Widgets...>::value;

template <typename Type>
concept WidgetListType = IsWidgetListV<std::remove_cvref_t<Type>>;

template <typename... Widgets>
class WidgetListBuilder {
   public:
    auto build() { return WidgetList<Widgets...>{std::move(widgets)}; }

    template <IsWidget Widget>
    auto append(Widget&& widget) {
        return WidgetListBuilder<Widget, Widgets...>(
            UniqueTypeList{std::forward<Widget>(widget), std::move(widgets)});
    }

   private:
    template <typename...>
    friend class WidgetListBuilder;

    WidgetListBuilder(UniqueTypeList<Widgets...>&& widgets) noexcept
        : widgets(std::move(widgets)) {}

    UniqueTypeList<Widgets...> widgets;
};

template <>
class WidgetListBuilder<> {
   public:
    template <IsWidget Widget>
    auto append(Widget&& widget) {
        return WidgetListBuilder<Widget>(
            UniqueTypeList<Widget>{std::forward<Widget>(widget)});
    }
};

class FpsDisplay {
   public:
    FpsDisplay(float smoothing) noexcept
        : smoothing(smoothing), smoothedFPS{0.0} {}

   private:
    template <typename...>
    friend class WidgetList;

    void draw() noexcept {
        ImGui::Begin("IDK");
        ImGui::Text("FPS: %.1f", smoothedFPS);
        ImGui::End();
    }

    void update(float dTime) noexcept {
        float currentFPS = 1.0f / dTime;
        smoothedFPS = smoothing * smoothedFPS + (1.0f - smoothing) * currentFPS;
    };
    float smoothedFPS;
    float smoothing;
};
