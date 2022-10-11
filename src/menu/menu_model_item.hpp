#pragma once
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <memory>
#include <variant>
#include <vector>
#include <map>
#include <string>
#include <utility>

#include "universal_type.hpp"
#include "clamp_meter_concepts.hpp"
//
// class MenuModelIndex {
//  public:
//    using Column = int;
//    using Row    = int;
//
//    MenuModelIndex() = default;
//    MenuModelIndex(std::pair<Column, Row> new_position)
//      : position{ new_position }
//    { }
//
//    [[nodiscard]] Column GetColumn() const noexcept;
//    [[nodiscard]] Row    GetRow() const noexcept;
//    void                 SetColumn(Column new_column) noexcept;
//    void                 SetIndex(Row new_row) noexcept;
//
//  private:
//    std::pair<Column, Row> position{ 0, 0 };
//};

using MenuModelIndex = int;

class MenuModelPageItemData {
  public:
    using NameT       = std::string;
    using IntegerType = int;
    using FloatType   = float;
    using StringType  = std::string;

    MenuModelPageItemData(const NameT &new_name, const auto new_value);
    MenuModelPageItemData()
      : MenuModelPageItemData{ NameT{}, UniversalType{} }
    { }

    void SetName(const NameT &new_name) { name = new_name; }
    void SetValue(auto new_value) noexcept {value = std::move(new_value);}
    void SetValue(const std::string &new_value) noexcept {value = new_value;}

    [[nodiscard]] NameT         GetName() const noexcept;
    [[nodiscard]] UniversalType GetValue() const noexcept;

    bool operator==(const auto &rhs)
    {
        if (name == rhs.name and value == rhs.value)
            return true;
        else
            return false;
    }

  private:
    NameT         name;
    UniversalType value;
};

MenuModelPageItemData::MenuModelPageItemData(const MenuModelPageItemData::NameT &new_name, const auto new_value)
  : name{ new_name }
  , value{ std::move(new_value) }
{ }

class MenuModelPageItem {
  public:
    using ChildIndex       = int;
    using EditorCursorPosT = int;

    [[nodiscard]] std::shared_ptr<MenuModelPageItemData>          GetData() const noexcept;
    [[nodiscard]] bool                                            HasChild(MenuModelIndex at_position) const noexcept;
    [[nodiscard]] std::shared_ptr<MenuModelPageItem>              GetChild(MenuModelIndex at_position) const noexcept;
    [[nodiscard]] std::vector<std::shared_ptr<MenuModelPageItem>> GetChildren() const noexcept;
    [[nodiscard]] MenuModelIndex                                  GetPosition() const noexcept;
    [[nodiscard]] MenuModelIndex                                  GetSelection() const noexcept;
    [[nodiscard]] bool                                            SomeItemIsSelected() const noexcept;
    [[nodiscard]] EditorCursorPosT                                GetEdittingCursorPosition() const noexcept;
    [[nodiscard]] bool                                            EdittingCursorIsActive() const noexcept;
    [[nodiscard]] MenuModelPageItem                              *GetParent() const noexcept;

    void SetData(std::shared_ptr<MenuModelPageItemData> new_data) noexcept;
    void InsertChild(std::shared_ptr<MenuModelPageItem> child, MenuModelIndex at_position) noexcept;
    void InsertChild(std::shared_ptr<MenuModelPageItem> child) noexcept;
    //    void SetPosition(MenuModelIndex new_position) noexcept;
    void SetIndex(MenuModelIndex idx) noexcept;
    //    void SetColumn(MenuModelIndex::Column new_column) noexcept;
    void SetSelectionPosition(MenuModelIndex selected_item_position) noexcept;
    void SetEddittingCursorPosition(EditorCursorPosT new_position) noexcept;
    void ActivateEdittingCursor(bool if_activate) noexcept;
    void SetParent(MenuModelPageItem *new_parent) noexcept;

  private:
    MenuModelIndex                                  residesAtIndex;
    MenuModelIndex                                  childSelectionIndex;
    bool                                            isSomeChildSelected{ false };
    bool                                            editCursorActive{ false };
    EditorCursorPosT                                edittingCursorPosition{ 0 };
    std::shared_ptr<MenuModelPageItemData>          data;
    std::vector<std::shared_ptr<MenuModelPageItem>> childItems{};
    MenuModelPageItem                              *parent{ nullptr };
};
