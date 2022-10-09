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
        Integer,
        Float,
        String
    };

    [[nodiscard]] std::string GetName() const noexcept;

  private:
    std::string    name;
    StoredDataType storedDataType;
    UniversalType  value;
};

class MenuModelPageItem {
  public:
    using ColumnVector    = std::vector<std::shared_ptr<MenuModelPageItem>>;
    using ItemsTableT     = std::vector<ColumnVector>;
    using EdittingCursorT = int;

    [[nodiscard]] std::shared_ptr<MenuModelPageItemData> GetData() const noexcept;
    [[nodiscard]] bool                                   HasChild(const MenuModelIndex &at_position) const noexcept;
    [[nodiscard]] std::shared_ptr<MenuModelPageItem>     GetChild(const MenuModelIndex &at_position) const noexcept;
    [[nodiscard]] MenuModelIndex                         GetPosition() const noexcept;
    [[nodiscard]] MenuModelIndex                         GetSelection() const noexcept;
    [[nodiscard]] bool                                   SomeItemIsSelected() const noexcept;
    [[nodiscard]] EdittingCursorT                        GetEdittingCursorPosition() const noexcept;
    [[nodiscard]] bool                                   EdittingCursorIsActive() const noexcept;

    void SetData(std::shared_ptr<MenuModelPageItemData> new_data) noexcept;
    void InsertChild(std::shared_ptr<MenuModelPageItem> child, const MenuModelIndex &at_position);
    void SetPosition(const MenuModelIndex &new_position) noexcept;
    void SetRow(MenuModelIndex::Row new_row) noexcept;
    void SetColumn(MenuModelIndex::Column new_column) noexcept;
    void SetSelectionPosition(const MenuModelIndex &selected_item_position) noexcept;
    void SetEddittingCursorPosition(EdittingCursorT new_position) noexcept;
    void ActivateEdittingCursor(bool if_activate) noexcept;

  private:
    MenuModelIndex  residesAtIndex;
    MenuModelIndex  childSelectionIndex;
    bool            isSomeChildSelected{ false };
    bool            editCursorActive{ false };
    EdittingCursorT edittingCursorPosition{ 0 };
    std::shared_ptr<MenuModelPageItemData> data;
    ItemsTableT                            childItems;
};
