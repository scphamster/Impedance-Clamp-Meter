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

#include "clamp_meter_concepts.hpp"

class MenuModelIndex {
  public:
    using Column = int;
    using Row    = int;

    MenuModelIndex() = default;
    MenuModelIndex(std::pair<Column, Row> new_position)
      : position{ new_position }
    { }

    [[nodiscard]] Column GetColumn() const noexcept;
    [[nodiscard]] Row    GetRow() const noexcept;
    void                 SetColumn(Column new_column) noexcept;
    void                 SetRow(Row new_row) noexcept;

  private:
    std::pair<Column, Row> position{ 0, 0 };
};

class MenuModelPageItemData {
  public:
    using NameT         = std::string;
    using IntegerType   = int;
    using FloatType     = float;
    using StringType    = std::string;
    using UniversalType = std::variant<IntegerType, FloatType, StringType>;

    enum class StoredDataType {
        Integer = 0,
        Float,
        String
    };

    MenuModelPageItemData(const NameT &new_name, const UniversalType &new_value);

    void SetValue(const auto &new_value) noexcept;

    [[nodiscard]] std::string GetName() const noexcept;
    template<typename RetType>
    [[nodiscard]] RetType GetValue() const noexcept;
    [[nodiscard]] int     GetStoredDataType() const noexcept;

  private:
    StoredDataType storedDataType;
    NameT          name;
    UniversalType  value;
};

template<typename RetType>
RetType
MenuModelPageItemData::GetValue() const noexcept
{
    return std::get<RetType>(value);
}

class MenuModelPageItem {
  public:
    using ChildIndex       = int;
    using EditorCursorPosT = int;

    [[nodiscard]] std::shared_ptr<MenuModelPageItemData> GetData() const noexcept;
    [[nodiscard]] bool                                   HasChild(const MenuModelIndex &at_position) const noexcept;
    [[nodiscard]] std::shared_ptr<MenuModelPageItem>     GetChild(const MenuModelIndex &at_position) const noexcept;
    [[nodiscard]] std::vector<std::shared_ptr<MenuModelPageItem>> GetChildren() const noexcept;
    [[nodiscard]] MenuModelIndex                                  GetPosition() const noexcept;
    [[nodiscard]] MenuModelIndex                                  GetSelection() const noexcept;
    [[nodiscard]] bool                                            SomeItemIsSelected() const noexcept;
    [[nodiscard]] EditorCursorPosT                                GetEdittingCursorPosition() const noexcept;
    [[nodiscard]] bool                                            EdittingCursorIsActive() const noexcept;
    [[nodiscard]] MenuModelPageItem                              *GetParent() const noexcept;

    void SetData(std::shared_ptr<MenuModelPageItemData> new_data) noexcept;
    void InsertChild(std::shared_ptr<MenuModelPageItem> child, const MenuModelIndex &at_position) noexcept;
    void InsertChild(std::shared_ptr<MenuModelPageItem> child) noexcept;
    void SetPosition(const MenuModelIndex &new_position) noexcept;
    void SetRow(MenuModelIndex::Row new_row) noexcept;
    void SetColumn(MenuModelIndex::Column new_column) noexcept;
    void SetSelectionPosition(const MenuModelIndex &selected_item_position) noexcept;
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
